#include <stdint.h>
#include <getopt.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "errenum.h"

static int32_t A[256][256] __attribute__((aligned(4096)));
static int32_t B[256][256] __attribute__((aligned(4096)));

volatile char MARKER_START, MARKER_END;

/** 
 * Transpose functions:
 * B = A^T 
 */
extern void trans(int N, int32_t A[N][N], int32_t B[N][N]);
void trans_naive(int N, int32_t A[N][N], int32_t B[N][N]);

int read_matrix(int N, int32_t mat[N][N], int tc);
int write_matrix(int N, int32_t mat[N][N], int tc, int ans);

int main(int argc, char* argv[]) {
    int n=0, status=NO_BUG, naive=0;
    int c;
    int tc;
    while( (c=getopt(argc,argv,"nN:c:")) != -1){
        switch(c){
        case 'c':
            tc = atoi(optarg);
            break;
        case 'n':
            naive = 1;
            break;
        case 'N':
            n = atoi(optarg);
            break;
        default:
            return UNKNOWN_ARGS;
        }
    }

    FILE* marker_fp = fopen(".marker","w");
    if (marker_fp == NULL) {
        perror("fopen");
        return OPEN_FAIL;
    }
    fprintf(marker_fp, "%llx %llx %llx %llx %llx %llx", 
            (unsigned long long int) &MARKER_START,
            (unsigned long long int) &MARKER_END,
            (unsigned long long int) &A[0][0],
            (unsigned long long int) &A[n-1][n-1],
            (unsigned long long int) &B[0][0],
            (unsigned long long int) &B[n-1][n-1]);
    fclose(marker_fp);

    status = read_matrix(n, A, tc);
    if(status != NO_BUG) return status;

    MARKER_START = 33;
    if(naive) trans_naive(n, A, B);
    else trans(n, A, B);
    MARKER_END   = 34;

    status = write_matrix(n, B, tc, naive);

    return status;
}

void trans_naive(int N, int32_t A[N][N], int32_t B[N][N]) {
    int i, j, t0;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            t0 = A[i][j];
            B[j][i] = t0;
        }
    }
}

int read_matrix(int N, int32_t mat[N][N], int tc) {
    char filename[100];
    snprintf(filename, sizeof(filename), "input/%d_%d.in", N, tc);

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        return OPEN_FAIL;
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (fscanf(fp, "%d", &mat[i][j]) != 1) {
                fprintf(stderr, "Failed to read mat[%d][%d]\n", i, j);
                fclose(fp);
                return READ_FAIL;
            }
        }
    }
    fclose(fp);
    return NO_BUG;
}

int write_matrix(int N, int32_t mat[N][N], int tc, int ans) {
    char filename[100];
    if(ans)
        snprintf(filename, sizeof(filename), "answer/%d_%d.ans", N, tc);
    else
        snprintf(filename, sizeof(filename), "output/%d_%d.out", N, tc);

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("fopen");
        return OPEN_FAIL;
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fprintf(fp, "%d", mat[i][j]);
            if (j != N - 1) {
                fprintf(fp, " ");
            }
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    return NO_BUG;
}