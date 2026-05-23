/* ============================================================
 * Computes C = A * B
 *   A : M x K  (row-major)
 *   B : K x N  (row-major)
 *   C : M x N  (row-major)
 * ============================================================ */
void matmul(float *A, float *B, float *C, int M, int K, int N) {
    for (int x = 0; x < M; x++) {
        for (int y = 0; y < N; y++) {
            float sum = 0;
            for (int z = 0; z < K; z++) {
                sum += A[x * K + z] * B[z * N + y];
            }
            C[x * N + y] = sum;
        }
    }
}