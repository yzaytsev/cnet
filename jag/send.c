#include <stdio.h>
#include <stdlib.h>
#include "send.h"
#include "common.h"
#include "../cache/evict.h"
#include "../cnet.h"

void jag_send(cjag_config_t* config, jag_callback_t cb) {
    int got = 0;
    const int detection_threshold = 300;

    volatile uint8_t** addrs;
    if(!config->addr) {
        addrs = malloc(config->cache_ways * config->channels * sizeof(void*));
        for (int i = 0; i < config->channels; i++) {
            for (int j = 0; j < config->cache_ways; j++) {
                addrs[i * config->cache_ways + j] = ((uint8_t**)(config->cache_sets))[(i + config->set_offset) * config->cache_ways + j];
            }
        }
        config->addr = (void**)addrs;
    } else {
        addrs = (volatile uint8_t**)config->addr;
    }

    for (int i = 0; i < config->channels; i++) //
    {
        while (1) {
            for (int j = 0; j < config->jag_send_count; ++j) {
                evict_set(addrs + (i * config->cache_ways), config->cache_kill_count);
            }
            uint32_t misses = jag_check_set(addrs + (i * config->cache_ways), detection_threshold, config->jag_send_count * 2, config);
            if (misses >= detection_threshold) {
                got++;
                if(cb) {
                    cb(config, got);
                }
                break;
            }
        }
    }
}

#if 0
void jag_send_bits(cjag_config_t *config, const uint8_t *mask, int len) {
    const int detection_threshold = 300;

    volatile uint8_t** addrs = (volatile uint8_t**)config->addr;

    for (int k = 0; k < 250; k++) {
        for (int i = 0; i < config->channels; i++) //
        {
            //printf("sending %d over channel %d...\n", (int)mask[i], i);
            if (!mask[i] || i >= len)
                continue;

            for (int j = 0; j < config->jag_send_count; ++j) {
                evict_set(addrs + (i * config->cache_ways), config->cache_kill_count);
            }
            jag_check_set(addrs + (i * config->cache_ways), detection_threshold, config->jag_send_count * 2, config);
        }
    }
}

#else

/*
static uint32_t bits_to_int(const uint8_t *bits, int len) {
    uint32_t x = 0;
    for (int i = 0; i < len; i++) {
        x <<= 1;
        x |= bits[i];
        printf("bits[%d]=%d, x=%d\n", i, bits[i], x);
    }
    return x;
}

void jag_send_bits(cjag_config_t *config, const uint8_t *mask, int len) {
    const int detection_threshold = 300;

    volatile uint8_t** addrs = (volatile uint8_t**)config->addr;

    int num1 = bits_to_int(mask, len / 2);
    int num2 = bits_to_int(mask + len / 2, len / 2);
    int indexes[2] = { num1, num2 };
    printf("sending 1 on channels [ ");
    for (int i = 0; i < (sizeof(indexes) / sizeof(indexes[0])); i++) {
        printf("%d ", indexes[i]);
    }
    printf("]\n");
    //printf("sending 1 on channels %d and %d...\n", num1, num2);

    for (int i = 0; i < (sizeof(indexes) / sizeof(indexes[0])); i++) {
        while (1) {
            for (int j = 0; j < config->jag_send_count; ++j) {
                evict_set(addrs + (indexes[i] * config->cache_ways), config->cache_kill_count);
            }
            uint32_t misses = jag_check_set(addrs + (indexes[i] * config->cache_ways), detection_threshold, config->jag_send_count * 2, config);
            if (misses >= detection_threshold) {
                break;
            }
        }
    }

}
*/


void jag_send_code(cjag_config_t *config, cncode_t code) {
    const int detection_threshold = 300;

    volatile uint8_t** addrs = (volatile uint8_t**)config->addr;

    //int num1 = bits_to_int(mask, len / 2);
    //int num2 = bits_to_int(mask + len / 2, len / 2);
    /*
    int indexes[2] = { num1, num2 };
    printf("sending 1 on channels [ ");
    for (int i = 0; i < (sizeof(indexes) / sizeof(indexes[0])); i++) {
        printf("%d ", indexes[i]);
    }
    printf("]\n");
    */
    //printf("sending 1 on channels %d and %d...\n", num1, num2);

    //printf("sending code %x...\n", (unsigned)code);
    putchar('.');
    fflush(stdout);
    //for (int i = 0; i < (sizeof(indexes) / sizeof(indexes[0])); i++) {
    unsigned ucode = (unsigned)code;
    for (int i = 0; i < sizeof(code) * 8; i++, ucode >>= 1) {
        if (!(ucode & 0x01u)) {
            continue;
        }

        while (1) {
            for (int j = 0; j < config->jag_send_count; ++j) {
                evict_set(addrs + (i * config->cache_ways), config->cache_kill_count);
            }
            uint32_t misses = jag_check_set(addrs + (i * config->cache_ways), detection_threshold, config->jag_send_count * 2, config);
            if (misses >= detection_threshold) {
                break;
            }
        }
    }
}

#endif
