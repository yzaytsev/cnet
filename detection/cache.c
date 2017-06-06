#include <stdio.h>
#include <stdint.h>
#include "cache.h"

#if 0
typedef struct {
    int size;
    int ways;
    int line_size;
    int sets;
    int partitions;
    int miss_threshold;
} cache_config_l3_t;
#endif

void print_cache_config(const cache_config_l3_t *config) {
    printf("cache_config_l3_t: {size=%d, ways=%d, line_size=%d, sets=%d, partitions=%d, miss_threshold=%d}\n",
        config->size, config->ways, config->line_size, config->sets, config->partitions, config->miss_threshold);
}

cache_config_l3_t get_l3_info() {
    uint32_t eax, ebx, ecx, edx;
    int level = 0;
    cache_config_l3_t config;

    do {
        asm volatile("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (4), "c" (level));
        int type = eax & 0x1f;
        if(!type) break;
        level++;
        config.line_size = (ebx & 0xfff) + 1;
        config.ways = ((ebx >> 22) & 0x3ff) + 1;
        config.sets = ecx + 1;
        config.partitions = ((ebx >> 12) & 0x3ff) + 1;
        config.size = config.line_size * config.ways * config.sets * config.partitions;
    } while(1);
    print_cache_config(&config);
    return config;
}


void show_cache_info() {
    cache_config_l3_t l3 = get_l3_info();
    printf("%d KB, %d-way L3\n", l3.size / 1024, l3.ways);
}
