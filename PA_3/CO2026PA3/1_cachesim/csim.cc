#include "cachesim.h"
#include "memtracer.h"

#include <cstddef>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <bits/getopt_core.h>
#include <time.h>

typedef char filename;
size_t sets;
size_t ways;
size_t linesz;
bool verbose = false;
filename t[20];

class pa3_cache_sim_t : public cache_memtracer_t
{
 public:
  pa3_cache_sim_t(const char* config, const char* name = "PA3$")
	  : cache_memtracer_t(config, name) {}
  bool interested_in_range(uint64_t UNUSED begin, uint64_t UNUSED end, access_type type)
  {
    return type == LOAD || type == STORE;
  }
  void trace(uint64_t addr, size_t bytes, access_type type)
  {
    if (type == LOAD || type == STORE) cache->access(addr, bytes, type == STORE, verbose);
  }
  void print_cache_config() {
    printf("S:W:B=%lu:%lu:%lu\n", sets, ways, linesz);
  }
  void print_summary() {
    printf("hits:%lu misses:%lu evictions:%lu\n", cache->get_hits(), cache->get_misses(), cache->get_evictions());
    FILE* output_fp = fopen(".csim_results", "w");
    assert(output_fp);
    fprintf(output_fp, "%lu %lu %lu\n", cache->get_hits(), cache->get_misses(), cache->get_evictions());
    fclose(output_fp);
  }
};

pa3_cache_sim_t* create_cache(int argc, char** argv);
void get_trace(pa3_cache_sim_t* dc);
void show_usage();

int main(int argc, char** argv)
{
    pa3_cache_sim_t *dc = create_cache(argc, argv);
    if(dc == nullptr) return 1;
    get_trace(dc);
    dc->print_summary();
    delete dc;
    return 0;
}

pa3_cache_sim_t* create_cache(int argc, char** argv)
{
    int opt;
    while(-1 != (opt = getopt(argc, argv, "vhS:W:B:t:")))
    {
        switch(opt)
        {
            case 'h':
                show_usage();
                return nullptr;
            case 'v':
                verbose = true;
                break;
            case 'S':
                sets = atoi(optarg);
                break;
            case 'W':
                ways = atoi(optarg);
                break;
            case 'B':
                linesz = atoi(optarg);
                break;
            case 't':
                strcpy(t, optarg);
                break;
            default:
                break;
        }
    }

    char *config = new char[100];
    sprintf(config, "%lu:%lu:%lu", sets, ways, linesz);
    
    
    return new pa3_cache_sim_t(config);
}

void get_trace(pa3_cache_sim_t* dc) {
    FILE *fp = fopen(t, "r");
    if(fp == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    char op;
    uint64_t addr;
    size_t bytes;

    while(fscanf(fp, " %c %lx,%lu", &op, &addr, &bytes) == 3)
    {
        switch(op)
        {
            case 'L':
                dc->trace(addr, bytes, LOAD);
                break;
            case 'S':
                dc->trace(addr, bytes, STORE);
                break;
            case 'M':
                dc->trace(addr, bytes, LOAD);
                dc->trace(addr, bytes, STORE);
                break;
            default:
                break;
        }
    }
}

void show_usage() {
    printf("Usage: ./csim_cpp [-hv] -S <num> -W <num> -B <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -S <num>   Number of set index bits.\n");
    printf("  -W <num>   Number of lines per set.\n");
    printf("  -B <num>   Number of block offset bits.\n");
    printf("  -t <file>  Input file to be simulated.\n");
    printf("\n");
    printf("Examples:\n");
    printf("PA3>  ./csim_cpp -S 8 -W 2 -B 16 -t input/1_S8_W2_B16.in\n");
    printf("PA3>  ./csim_cpp -v -S 16 -W 2 -B 64 -t input/2_S16_W2_B64.in\n");
}