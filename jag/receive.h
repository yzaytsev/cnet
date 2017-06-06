#ifndef BLACKHAT_RECEIVE_H
#define BLACKHAT_RECEIVE_H
#include "../cnet.h"
#include "common.h"
#include <stdint.h>
#include <net/codemap.h>

void jag_receive(void **ret_addrs, size_t* recv_sets, cjag_config_t* config, jag_callback_t cb);
//void jag_receive_bits(uint8_t *dst_buf, const size_t *sets_map, cjag_config_t *config);
void jag_receive_code(cncode_t *code, const size_t *sets_map, cjag_config_t *config);

#endif //BLACKHAT_RECEIVE_H
