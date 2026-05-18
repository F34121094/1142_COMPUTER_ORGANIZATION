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

#include <riscv_vector.h>
#include <mel_spectrogram.h>
extern double sinf(float angle);
extern double cosf(float angle);
/* RVV hint: each butterfly stage is data-parallel across independent pairs. */
void fft(float *__restrict real, float *__restrict imag, size_t n) {
    for(size_t i = 0 ;i < n ; i++){ //位元反轉 i 的迴圈
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
    for(size_t s = 2 ; s<=n ; s <<= 1){ //第一層迴圈是 s = 2 開始的
        size_t half = s >> 1;           //half = s >> 1
        size_t num_groups = n/s;        //num_groups = n / s

        float angle = -2.0 * 3.1415926 / s;     //angle = -2.0 * 3.1415926 / s
        float w_r_step = cosf(angle);           //w_r_step = cosf(angle)
        float w_i_step = sinf(angle);           //w_i_step = sinf(angle)
        float w_r = 1.0;
        float w_i = 0.0;
    
        ptrdiff_t ptr_diff = s * sizeof(float);

        for(size_t k = 0 ; k < half ; k++){     //第二層的for是k 限制是 < half
            for(size_t groups = num_groups,j=0,vl ; groups > 0 ; groups -= vl , j += s * vl){
                //1.定義 vl
                vl = __riscv_vsetvl_e32m8(groups);
                //2.取大家的記憶體位置
                float* u_r = k + j + real; 
                float* u_i = k + j + imag; 
                float* d_r = k + j + real + half; 
                float* d_i = k + j + imag + half;
                //3.load大家的值
                vfloat32m8_t ur = __riscv_vlse32_v_f32m8(u_r, ptr_diff,vl);
                vfloat32m8_t ui = __riscv_vlse32_v_f32m8(u_i, ptr_diff,vl);
                vfloat32m8_t dr = __riscv_vlse32_v_f32m8(d_r, ptr_diff,vl);
                vfloat32m8_t di = __riscv_vlse32_v_f32m8(d_i, ptr_diff,vl);
                //4.將下翅膀轉成對應的位置
                vfloat32m8_t tr = __riscv_vfsub_vv_f32m8(__riscv_vfmul_vf_f32m8(dr,w_r,vl),__riscv_vfmul_vf_f32m8(di,w_i,vl),vl);
                vfloat32m8_t ti = __riscv_vfadd_vv_f32m8(__riscv_vfmul_vf_f32m8(dr,w_i,vl),__riscv_vfmul_vf_f32m8(di,w_r,vl),vl);
                //5.store回去
                __riscv_vsse32_v_f32m8(u_r,ptr_diff,__riscv_vfadd_vv_f32m8(ur,tr,vl),vl);
                __riscv_vsse32_v_f32m8(u_i,ptr_diff,__riscv_vfadd_vv_f32m8(ui,ti,vl),vl);
                __riscv_vsse32_v_f32m8(d_r,ptr_diff,__riscv_vfsub_vv_f32m8(ur,tr,vl),vl);
                __riscv_vsse32_v_f32m8(d_i,ptr_diff,__riscv_vfsub_vv_f32m8(ui,ti,vl),vl);
            }
            //更新指針
            float new_w_r = w_r * w_r_step - w_i * w_i_step;
            w_i = w_r * w_i_step + w_i * w_r_step;
            w_r = new_w_r;
        }
    }
}

/* RVV hint: vlse32 with stride=8 bytes extracts all re (or im) values in one pass. */
void power_spectrum(const float *__restrict stft_data, size_t num_frames,float *__restrict output) {
    size_t n = num_frames * 257;
    for(size_t vl ; n > 0 ; n -= vl , stft_data += 2*vl , output += vl){
        //1.定義vl
        vl = __riscv_vsetvl_e32m8(n);
        //2.load值
        vfloat32m8_t re = __riscv_vlse32_v_f32m8(stft_data, 8, vl);
        vfloat32m8_t im = __riscv_vlse32_v_f32m8(stft_data+1, 8, vl);
        //3.平方相加
        vfloat32m8_t temp = __riscv_vfmul_vv_f32m8(re,re,vl);
        vfloat32m8_t res = __riscv_vfmacc_vv_f32m8(temp,im,im,vl);
        //4.直接存在output
        __riscv_vse32_v_f32m8(output,res,vl);
    }
}

/* RVV hint: vfmul + vfredusum computes one dot product per (frame, mel) pair. */
void mel_filter_bank(const float *__restrict power,
                     const float *__restrict mel_bank, size_t num_frames,
                     size_t n_mels, size_t n_freq_bins,
                     float *__restrict output) {
                        
                        size_t vlmax = __riscv_vsetvlmax_e32m8();
                        vfloat32m1_t v_zero = __riscv_vfmv_v_f_f32m1(0.0f,1);
                        
                        for(size_t i = 0 ; i < num_frames ; i++){
                            const float* p = power + (i * n_freq_bins);
                            for(size_t j = 0 ; j < n_mels ; j++){
                                const float* p_ptr = p;
                                const float* m_ptr = mel_bank + (j * n_freq_bins);

                                vfloat32m8_t v_acc = __riscv_vfmv_v_f_f32m8(0.0f, vlmax);
                        
                                for(size_t left_bit = n_freq_bins ,vl; left_bit > 0 ; left_bit -= vl, p_ptr += vl , m_ptr += vl){
                                    //定義 vl
                                    vl = __riscv_vsetvl_e32m8(left_bit);
                                    //load值
                                    vfloat32m8_t v_p = __riscv_vle32_v_f32m8(p_ptr,vl);
                                    vfloat32m8_t v_m = __riscv_vle32_v_f32m8(m_ptr,vl);
                                    //相乘累加
                                    v_acc = __riscv_vfmacc_vv_f32m8(v_acc , v_p, v_m,vl);
                                }
                                
                                vfloat32m1_t v_res = __riscv_vfredusum_vs_f32m8_f32m1(v_acc, v_zero , vlmax);
                                output[i * n_mels + j] = __riscv_vfmv_f_s_f32m1_f32(v_res);
                            }
                        }
                    }

