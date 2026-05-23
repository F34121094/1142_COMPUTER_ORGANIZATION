#include <stdio.h>
#include <stdint.h>

#include "input/weights.h"
#include "input/data.h"


/* ── dimensions (defined in weights.h) ──────────────────────────────────────
 *   INPUT_DIM  = 784
 *   HIDDEN_DIM = 128
 *   OUTPUT_DIM = 10
 *   NUM_TEST   = 300  (defined in data.h)
 * ─────────────────────────────────────────────────────────────────────────── */

#define read_csr(reg)                                                          \
  ({                                                                           \
    uint64_t __tmp;                                                            \
    asm volatile("csrr %0, " #reg : "=r"(__tmp));                              \
    __tmp;                                                                     \
  })
#define rdcycle() read_csr(cycle)

/* ============================================================
 * TODO: implement this function with your optimized version
 *
 * Computes C = A * B
 *   A : M x K  (row-major)
 *   B : K x N  (row-major)
 *   C : M x N  (row-major)
 * ============================================================ */
extern void matmul(float *A, float *B, float *C, int M, int K, int N);

/* ── fixed helpers (do not modify) ──────────────────────────────────────────*/

/* Add bias vector b[N] to every row of x[M][N] */
static void add_bias(float *x, float *b, int M, int N) {
    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            x[i*N + j] += b[j];
}

/* ReLU applied element-wise */
static void relu(float *x, int n) {
    for (int i = 0; i < n; i++)
        if (x[i] < 0.0f) x[i] = 0.0f;
}

/* ── forward pass ────────────────────────────────────────────────────────────
 *
 *  input  : NUM_TEST x INPUT_DIM
 *  hidden : NUM_TEST x HIDDEN_DIM   (intermediate buffer)
 *  output : NUM_TEST x OUTPUT_DIM
 *
 *  Layer 1:  hidden = ReLU( input  * W1 + b1 )
 *  Layer 2:  output =       hidden * W2 + b2
 * ─────────────────────────────────────────────────────────────────────────── */
static void forward(float *input, float *hidden, float *output) {
    /* Layer 1 */
    matmul(input, W1, hidden, NUM_TEST, INPUT_DIM, HIDDEN_DIM);
    add_bias(hidden, b1, NUM_TEST, HIDDEN_DIM);
    relu(hidden, NUM_TEST * HIDDEN_DIM);

    /* Layer 2 */
    matmul(hidden, W2, output, NUM_TEST, HIDDEN_DIM, OUTPUT_DIM);
    add_bias(output, b2, NUM_TEST, OUTPUT_DIM);

}

int main(int argc, char *argv[]) {
    static float hidden[HIDDEN_DIM*NUM_TEST]__attribute__((aligned(4096)));
    static float output[OUTPUT_DIM*NUM_TEST]__attribute__((aligned(4096)));

    uint64_t start = rdcycle();
    forward(test_images, hidden, output);
    uint64_t end = rdcycle();
    printf("inst. cycles: %lu\n", end - start);

    if (argc == 2) {
        FILE *f = fopen(argv[1], "wb");
        if (!f) return 1;
        fwrite(output, sizeof(float), OUTPUT_DIM*NUM_TEST, f);
        fclose(f);
    }
    
    return 0;
}