#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include "receive.h"
#include "common.h"
#include "../cache/evict.h"
#include "../cnet.h"

void jag_receive(void **ret_addrs, size_t* recv_sets, cjag_config_t *config, jag_callback_t cb) {
    uint8_t received_set_count = 0;
    uint16_t *received_sets = calloc(32 * config->cache_slices, sizeof(uint16_t));
    const int detection_threshold = 300;
    volatile uint8_t **all_addrs = (volatile uint8_t**)config->cache_sets;

    while (received_set_count != config->channels) {
        for (int i = 0; i < 32 * config->cache_slices; i++) {
            uint32_t misses = jag_check_set(all_addrs + i * config->cache_ways, detection_threshold, config->jag_send_count * 2, config);

            if (misses >= detection_threshold) {
                //printf("i = %d, misses = %d\n", i, misses);
                for (int j = 0; j < config->jag_recv_count; ++j) {
                    evict_set(all_addrs + i * config->cache_ways, config->cache_ways);
                }

                if (!received_sets[i]) {
                    for (int j = 0; j < config->cache_ways; ++j) {
                        ret_addrs[(received_set_count) * config->cache_ways + j] = (void*)all_addrs[i * config->cache_ways + j];
                    }

                    received_set_count++;
                    if (cb) {
                        if(recv_sets) {
                            recv_sets[received_set_count - 1] = i;
                        }
                        cb(config, received_set_count);
                    }
                }
                received_sets[i]++;

                //jam the last set some more because there is no feedback
                if (received_set_count == config->channels) {
                    for (int j = 0; j < (int)(config->jag_recv_count * 1.5); ++j) {
                        evict_set(all_addrs + i * config->cache_ways, config->cache_kill_count);
                    }
                    break;
                }

            }
        }
    }
    free(received_sets);
}

void jag_receive_code(cncode_t *code, const size_t *sets_map, cjag_config_t *config) {
    const int detection_threshold = 300;
    //const int detection_threshold = 200;
    volatile uint8_t **all_addrs = (volatile uint8_t**)config->cache_sets;
    int received_set_count = 0;
    //memset(dst_buf, 0, config->packet_size);

    //const int max_set_bits = CODE_SET_BITS;
    unsigned x = 0;
    *code = 0;

    while (received_set_count < CODE_SET_BITS) {
        for (int i = 0; i < config->channels; i++) {
            int index = sets_map[i];
            uint32_t misses = jag_check_set(all_addrs + index * config->cache_ways, detection_threshold, config->jag_send_count * 2, config);

            if (misses >= detection_threshold) {
                //printf("channel: %d -> %d, misses: %d\n", i, index, misses);
                for (int j = 0; j < config->jag_recv_count; ++j) {
                    evict_set(all_addrs + index * config->cache_ways, config->cache_ways);
                }
                //dst_buf[received_set_count] = i;
                if (x & ((unsigned)1 << i))
                    continue;

                x |= ((unsigned)1 << i);
                received_set_count++;

                //jam the last set some more because there is no feedback
                if (received_set_count == CODE_SET_BITS) {
                    for (int j = 0; j < (int)(config->jag_recv_count * 1.5); ++j) {
                        evict_set(all_addrs + index * config->cache_ways, config->cache_kill_count);
                    }
                    break;
                }
            }
        }
    }

    *code = x;
    //printf("received code %x.\n", x);
    putchar('.');
    fflush(stdout);
    /*
    printf("dst_buf = [ ");
    for (int i = 0; i < received_set_count; i++) {
        printf("%d ", dst_buf[i]);
    }
    printf("]\n");
    */
}

