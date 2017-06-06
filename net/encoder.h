#ifndef ENCODER_H
#   define ENCODER_H 

#include <stdint.h>
#include <stddef.h>
#include <net/codemap.h>

//typedef uint8_t cncode_t;

size_t encoded_size(size_t data_size);
size_t decoded_size(size_t enc_size);
int encode(cncode_t *dst, const uint8_t *src, size_t size, int bitsize);
int decode(uint8_t *dst,  const cncode_t *src, size_t size);

#endif /* #ifndef ENCODER_H */

