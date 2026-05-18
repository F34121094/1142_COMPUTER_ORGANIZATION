/**
 * @file main.c
 * @brief Student implementation of mel spectrogram core functions.
 *
 * Implement the three functions below. The remaining pipeline functions
 * (hann_window, stft, melspectrogram) are provided in utils.c.
 *
 * Python source of truth: scripts/mel_spectrogram.py
 * Include RVV intrinsics via: #include <riscv_vector.h>
 */
#include <mel_spectrogram.h>
#include <riscv_vector.h>
extern double cosf(float angle);
extern double sinf(float angle);
/* RVV hint: each butterfly stage is data-parallel across independent pairs. */
void fft(float *__restrict real, float *__restrict imag, size_t n) { 
    for(size_t i = 0 ; i < n ; i ++){
        size_t temp_i = i;
        size_t j = 0;
        for(size_t bit = 0 ; (1 << bit) < n ; bit++){
            j = (j << 1) | (temp_i & 1);
            temp_i >>= 1;  
        }
        if(i < j){
            float t_r = real[i];
            real[i] = real[j];
            real[j] = t_r;
            
            float t_i = imag[i];
            imag[i] = imag[j];
            imag[j] = t_i;
        }
    }
    for(size_t s = 2 ; s <= n ; s <<= 1){
        size_t half = s >> 1;
        size_t num_groups = n/s;

        float angle = -2.0 * 3.1415926 / s;
        float w_r_step = cosf(angle);
        float w_i_step = sinf(angle);
        float w_r = 1.0;
        float w_i = 0.0;

        ptrdiff_t ptr_diff = sizeof(float) * s;
        for(size_t k = 0 ; k < half ; k++){
            for(size_t groups = num_groups,j = 0,vl; groups > 0; groups -= vl , j += s*vl){
                vl = __riscv_vsetvl_e32m8(groups);
                float* ur = real + j + k;
                float* ui = imag + j + k;
                float* dr = real + j + k + half;
                float* di = imag + j + k + half;

                vfloat32m8_t u_r = __riscv_vlse32_v_f32m8(ur,ptr_diff,vl);
                vfloat32m8_t u_i = __riscv_vlse32_v_f32m8(ui,ptr_diff,vl);
                vfloat32m8_t d_r = __riscv_vlse32_v_f32m8(dr,ptr_diff,vl);
                vfloat32m8_t d_i = __riscv_vlse32_v_f32m8(di,ptr_diff,vl);

                vfloat32m8_t t_r = __riscv_vfsub_vv_f32m8(__riscv_vfmul_vf_f32m8(d_r,w_r,vl),__riscv_vfmul_vf_f32m8(d_i,w_i,vl),vl);
                vfloat32m8_t t_i = __riscv_vfadd_vv_f32m8(__riscv_vfmul_vf_f32m8(d_r,w_i,vl),__riscv_vfmul_vf_f32m8(d_i,w_r,vl),vl);

                __riscv_vsse32_v_f32m8(ur, ptr_diff, __riscv_vfadd_vv_f32m8(u_r,t_r,vl),vl);
                __riscv_vsse32_v_f32m8(ui, ptr_diff, __riscv_vfadd_vv_f32m8(u_i,t_i,vl),vl);
                __riscv_vsse32_v_f32m8(dr, ptr_diff, __riscv_vfsub_vv_f32m8(u_r,t_r,vl),vl);
                __riscv_vsse32_v_f32m8(di, ptr_diff, __riscv_vfsub_vv_f32m8(u_i,t_i,vl),vl);

            }
            float new_w_r = w_r_step * w_r - w_i_step * w_i;
            w_i = w_r_step * w_i + w_i_step * w_r;
            w_r = new_w_r;
        }
    }
}

/* RVV hint: vlse32 with stride=8 bytes extracts all re (or im) values in one pass. */
void power_spectrum(const float *__restrict stft_data, size_t num_frames,float *__restrict output) {
        size_t n = num_frames * 257;
        for(size_t vl ; n > 0 ; n-=vl, stft_data += 2*vl, output += vl){
            vl = __riscv_vsetvl_e32m8(n);
            vfloat32m8_t real = __riscv_vlse32_v_f32m8(stft_data, 8, vl);   
            vfloat32m8_t imag = __riscv_vlse32_v_f32m8(stft_data+1, 8, vl);   
        
            vfloat32m8_t temp = __riscv_vfmul_vv_f32m8(real,real,vl);
            vfloat32m8_t result = __riscv_vfmacc_vv_f32m8(temp,imag,imag,vl);
        
            __riscv_vse32_v_f32m8(output, result, vl);
        }
 }

/* RVV hint: vfmul + vfredusum computes one dot product per (frame, mel) pair. */
void mel_filter_bank(const float *__restrict power,
                     const float *__restrict mel_bank, size_t num_frames,
                     size_t n_mels, size_t n_freq_bins,
                     float *__restrict output) { 
    size_t vlmax = __riscv_vsetvlmax_e32m8();
    vfloat32m1_t v_zero = __riscv_vfmv_v_f_f32m1(0.0f,1);
    for(size_t i = 0 ;i < num_frames ; i++){
        const float *p = power + (i * n_freq_bins);
        for(size_t j = 0; j < n_mels ; j++){
            const float *p_ptr = p;
            const float *m_ptr = mel_bank + (j * n_freq_bins);
            
            vfloat32m8_t v_acc = __riscv_vfmv_v_f_f32m8(0.0f, vlmax);
            for(size_t left_bit = n_freq_bins,vl ;left_bit > 0 ; left_bit -= vl, p_ptr += vl, m_ptr += vl){
                vl = __riscv_vsetvl_e32m8(left_bit);
                
                vfloat32m8_t p_v = __riscv_vle32_v_f32m8(p_ptr,vl);
                vfloat32m8_t m_v = __riscv_vle32_v_f32m8(m_ptr,vl);
            
                v_acc = __riscv_vfmacc_vv_f32m8(v_acc, p_v, m_v,vl);
            }

            vfloat32m1_t v_res = __riscv_vfredusum_vs_f32m8_f32m1(v_acc, v_zero, vlmax);
            output[i * n_mels + j] = __riscv_vfmv_f_s_f32m1_f32(v_res);
        }
    }
}

