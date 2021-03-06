#include <stdio.h>
#include <net/encoder_13_7.h>

size_t encoded_size(size_t data_size) {
    return ((data_size + 1) * 8 / DATA_BITS_PER_CODE + 1);
}

size_t decoded_size(size_t enc_size) {
    return ((enc_size + 1) * DATA_BITS_PER_CODE / 8 + 1);
}

int encode(cncode_t *dst, const uint8_t *src, size_t size, int bitsize) {
    //int cpos = 0; /* pos in code */
    //for (size_t i = 0; i < size; i++) {
    int bit_limit = (bitsize >= 0) ? bitsize : (int)size * 8;
    size_t idx = 0;
    for (int dbit = 0; dbit < bit_limit; dbit += 6) {
        uint8_t x  = 0;
        int bit_ofs     = dbit % 8;
        size_t byte_pos = (size_t)dbit / 8;
        switch (bit_ofs) {
            case 0: {
                x = (unsigned)src[byte_pos] & 0x3fu;
                //printf("dbit=%d, ofs=%d: src[%d] = %x, x = %x\n", dbit, (int)bit_ofs, (int)byte_pos, (unsigned)src[byte_pos], x);
            } break;
            case 2: {
                x = ((unsigned)src[byte_pos] & 0xfcu) >> 2;
                //printf("dbit=%d, ofs=%d: src[%d] = %x, x = %x\n", dbit, (int)bit_ofs, (int)byte_pos, (unsigned)src[byte_pos], x);
            } break;
            case 4: {
                x = (((unsigned)src[byte_pos] & 0xf0u) >> 4);
                if (byte_pos + 1 < size) {
                    unsigned x1 = ((unsigned)src[byte_pos + 1] & 0x03u) << 4;
                    x |= x1;
                }
                //printf("dbit=%d, ofs=%d: src[%d] = %x %x, x = %x\n", dbit, (int)bit_ofs, (int)byte_pos, (unsigned)src[byte_pos], (unsigned)src[byte_pos + 1], x);
            } break;
            case 6: {
                x = (((unsigned)src[byte_pos] & 0xc0u) >> 6);
                //printf("ofs=%d: src[%d] = %x, x = %x\n", (int)bit_ofs, (int)byte_pos, (unsigned)src[byte_pos], x);
                if (byte_pos + 1 < size) {
                    unsigned x1 = ((unsigned)src[byte_pos + 1] & 0x0fu) << 2;
                    x |= x1;
                }
                //printf("dbit=%d, ofs=%d: src[%d] = %x %x, x = %x\n", dbit, (int)bit_ofs, (int)byte_pos, (unsigned)src[byte_pos], (unsigned)src[byte_pos + 1], x);
            } break;
            default: {
                printf("ERROR: incorrect bit offset %d while encoding!\n", bit_ofs);
                return -1;
            } break;
        }

        dst[idx++] = index_to_code(x + CMD_FIRST_DATA);
    }
    return (int)idx;
}

int decode(uint8_t *dst,  const cncode_t *src, size_t size) {
    //int dbit = 0;
    //int byte_pos = 0;
    int dbit = 0;
    for (size_t i = 0; i < size; i++, dbit += 6) {
        int res = code_to_index(src[i]);
        if (res == NO_SUCH_CODE || res < CMD_FIRST_DATA || res > CMD_LAST_DATA) {
            printf("cannot decode code %x (decoded to %d)!\n", (unsigned)src[i], res);
            break;
        }
        res -= CMD_FIRST_DATA;

        int bit_ofs     = dbit % 8;
        size_t byte_pos = (size_t)dbit / 8;
        unsigned x = (unsigned)res;

        //uint8_t x = 0;
        switch (bit_ofs) {
            case 0: {
                //x = (unsigned)src[byte_pos] & 0x3fu;
                dst[byte_pos] = x;
            } break;
            case 2: {
                //x = ((unsigned)src[byte_pos] & 0xfcu) >> 2;
                dst[byte_pos] |= (x << 2);
            } break;
            case 4: {
                dst[byte_pos] |= (x << 4);
                dst[byte_pos + 1] = x >> 4;
                //byte_pos++;
            } break;
            case 6: {
                dst[byte_pos] |= (x << 6);
                dst[byte_pos + 1] = x >> 2;
                //byte_pos++;
            } break;
            default: {
                printf("ERROR: incorrect bit offset %d while decoding!\n", bit_ofs);
                return -1;
            } break;
        }

        //dbit += 8;
    }
    //return dbit / 8 + !!(dbit % 8);
    /**
     * It's just (dbit / 8) and not (dbit / 8 + !!(dbit % 8)) because
     * at the encoding time few extra bits are added to match division by 6.
     * E.g. if there were 2 bytes (16 bits) they are encoded to 3 bytes (18 bits)
     * and at the decoding time we need only 16 bits of these 18 bits.
     */
    return dbit / 8;
}

