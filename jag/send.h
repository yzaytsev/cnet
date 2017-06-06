#ifndef BLACKHAT_SEND_H
#define BLACKHAT_SEND_H

#include <stdint.h>
#include "../cnet.h"
#include "common.h"
#include <net/codemap.h>

void jag_send(cjag_config_t *config, jag_callback_t cb);
//void jag_send_bits(cjag_config_t *config, const uint8_t *bitmask, int len);
void jag_send_code(cjag_config_t *config, cncode_t code);

#endif //BLACKHAT_SEND_H
