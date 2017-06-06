#ifndef TRANSFER_H
#   define TRANSFER_H 

#include <stdint.h>
#include "../cnet.h"

int send_packet(cjag_config_t *config, const uint8_t *data, size_t size);
int receive_packet(cjag_config_t *config, const size_t *sets_map, uint8_t *buffer, size_t size);

#endif /* #ifndef TRANSFER_H */

