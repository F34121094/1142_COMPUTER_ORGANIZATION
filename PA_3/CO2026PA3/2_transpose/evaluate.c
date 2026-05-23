#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <getopt.h>
#include <sys/wait.h>
#include <stdint.h>
#include "errenum.h"

uint64_t hits = 0;
uint64_t misses = 0;
uint64_t evictions = 0;

int naive_stats(int N, int tc){
    int flag, status;
    unsigned int len;
    unsigned long long int marker_start, marker_end, a_start, a_end, b_start, b_end, addr;
    char buf[1000], cmd[255];
    char filename[128];

    FILE* full_trace_fp;  
    FILE* part_trace_fp; 

    sprintf(cmd, "valgrind --tool=lackey --trace-mem=yes --log-fd=1 -v ./trans-driver -N %d -n -c %d > full.trace", N, tc);
    status = WEXITSTATUS(system(cmd));
    if(status != 0) return status;

    FILE* marker_fp = fopen(".marker", "r");
    assert(marker_fp);
    fscanf(marker_fp, "%llx %llx %llx %llx %llx %llx", &marker_start, &marker_end, &a_start, &a_end, &b_start, &b_end);
    fclose(marker_fp);

    full_trace_fp = fopen("full.trace", "r");
    assert(full_trace_fp);


    sprintf(filename, "naive_trans%d.trace", N);
    part_trace_fp = fopen(filename, "w");
    assert(part_trace_fp);

    flag = 0;
    while (fgets(buf, 1000, full_trace_fp) != NULL) {

        if (buf[0]==' ' && buf[2]==' ' &&
            (buf[1]=='S' || buf[1]=='M' || buf[1]=='L' )) {
            sscanf(buf+3, "%llx,%u", &addr, &len);
    
            if (addr == marker_start) {
                flag = 1;
                continue;
            }

            if (addr == marker_end) {
                flag = 0;
                fclose(part_trace_fp);
                break;
            }

            if (flag && ((addr >= a_start && addr <= a_end) || (addr >= b_start && addr <= b_end))) {
                fputs(buf, part_trace_fp);
            }
        }
    }
    fclose(full_trace_fp);

    sprintf(cmd, "./csim_cpp -S 16 -W 2 -B 32 -v -t naive_trans%d.trace > naive_hitf%d", N, N);
    status = WEXITSTATUS(system(cmd));
    if(status != 0) return status;

    FILE* in_fp = fopen(".csim_results","r");
    assert(in_fp);
    fscanf(in_fp, "%lu %lu %lu", &hits, &misses, &evictions);
    fclose(in_fp);

    return NO_BUG;
}

int evaluate(int N, int tc) {
    int flag, status;
    unsigned int len;
    unsigned long long int marker_start, marker_end, a_start, a_end, b_start, b_end, addr;
    char buf[1000], cmd[255];
    char filename[128];

    FILE* full_trace_fp;  
    FILE* part_trace_fp; 

    sprintf(cmd, "valgrind --tool=lackey --trace-mem=yes --log-fd=1 -v ./trans-driver -N %d -c %d > full.trace", N, tc);
    status = WEXITSTATUS(system(cmd));
    if(status != 0) return status;

    FILE* marker_fp = fopen(".marker", "r");
    assert(marker_fp);
    fscanf(marker_fp, "%llx %llx %llx %llx %llx %llx", &marker_start, &marker_end, &a_start, &a_end, &b_start, &b_end);
    fclose(marker_fp);

    full_trace_fp = fopen("full.trace", "r");
    assert(full_trace_fp);


    sprintf(filename, "trans%d.trace", N);
    part_trace_fp = fopen(filename, "w");
    assert(part_trace_fp);

    flag = 0;
    while (fgets(buf, 1000, full_trace_fp) != NULL) {

        if (buf[0]==' ' && buf[2]==' ' &&
            (buf[1]=='S' || buf[1]=='M' || buf[1]=='L' )) {
            sscanf(buf+3, "%llx,%u", &addr, &len);
    
            if (addr == marker_start) {
                flag = 1;
                continue;
            }

            if (addr == marker_end) {
                flag = 0;
                fclose(part_trace_fp);
                break;
            }

            if (flag && ((addr >= a_start && addr <= a_end) || (addr >= b_start && addr <= b_end))) {
                fputs(buf, part_trace_fp);
            }
        }
    }
    fclose(full_trace_fp);

    sprintf(cmd, "./csim_cpp -S 16 -W 2 -B 32 -v -t trans%d.trace > hitf%d", N, N);
    status = WEXITSTATUS(system(cmd));
    if(status != 0) return status;

    FILE* in_fp = fopen(".csim_results","r");
    assert(in_fp);
    fscanf(in_fp, "%lu %lu %lu", &hits, &misses, &evictions);
    fclose(in_fp);

    return NO_BUG;
}

int main(int argc, char *argv[]) {

    int n=0, naive=0;
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
            exit(1);
        }
    }

    int status = 0;
    if(naive) status = naive_stats(n, tc);
    else status = evaluate(n, tc);


    if(status) {
        printf("Transpose failed.\n");
        printf("Check your implemetation, arguments, status=%d.\n", status);
    }

    return status;
}