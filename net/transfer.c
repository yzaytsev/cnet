#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <net/transfer.h>
#include <net/codemap.h>
#include <net/encoder.h>
#include <jag/send.h>
#include <jag/receive.h>

//enum { PACKET_SIZE_FIELD_LEN = 2 };

int send_packet(cjag_config_t *config, const uint8_t *data, size_t size) {
    int failed = 1;
    cncode_t *packet = (cncode_t*)calloc(encoded_size(size) + 10, sizeof(cncode_t));
    if (!packet) {
        goto finalize;
    }

    printf("Sending: ");
    fflush(stdout);
    // create command
    // convert data to codes
    int idx = 0;
    packet[idx++] = index_to_code(CMD_PKT_BEGIN);
#if 0
    packet[idx++] = index_to_code(CMD_PKT_BEGIN2);
    packet[idx++] = index_to_code(CMD_EXTENSION);
    printf("packet[0] = %02x, packet[1] = %02x\n", (unsigned)packet[0], (unsigned)packet[1]);
#endif

    /* 12 bits for packet size */
    int enc_data_chunks_count = encode(packet + idx + PACKET_SIZE_FIELD_LEN, data, size, -1);
    //printf("enc_data_chunks_count = %d.\n", enc_data_chunks_count);
    if (enc_data_chunks_count < 0 || enc_data_chunks_count > (1 << 12)) {
        goto finalize;
    }

    for (int i = 0; i < PACKET_SIZE_FIELD_LEN; i++) {
        packet[idx + i] = 0;
    }
    //packet[idx] = packet[idx + 1] = 0;
    uint8_t raw_size[MAX_PACKET_SIZE_FIELD_LEN];
    //for (int i = 0; i < PACKET_SIZE_FIELD_LEN; i++) {
    //    raw_size[i] = (unsigned)enc_data_chunks_count & 0xffu;
    //raw_size[0] = (unsigned)enc_data_chunks_count & 0xffu;
    //raw_size[1] = ((unsigned)enc_data_chunks_count >> 8) & 0x0fu;
    // up to 12 bits for packet data size
    raw_size[0] = (unsigned)size & 0xffu;
    raw_size[1] = ((unsigned)size >> 8) & 0x0fu;
    //}
    int enc_size_chunks_count = encode(packet + idx, raw_size, PACKET_SIZE_FIELD_LEN, PACKET_SIZE_FIELD_LEN * DATA_BITS_PER_CODE);
    printf("enc_size_chunks_count = %d, enc_data_chunks_count = %d, size = %d.\n", enc_size_chunks_count, enc_data_chunks_count, (int)size);
    if (enc_size_chunks_count < 0) {
        goto finalize;
    }
    idx += (PACKET_SIZE_FIELD_LEN + enc_data_chunks_count);
    packet[idx++] = index_to_code(CMD_PKT_END);

    //for (int i = 0; i < idx; i++) {
    //    printf("packet[%d] = %u\n", i, (unsigned)packet[i]);
    //}
    // call jag_send_bits (it is to be renamed to jag_send_code)
    for (int i = 0; i < idx; i++) {
        jag_send_code(config, packet[i]);
        watchdog_reset(config->watchdog);
    }
    failed = 0;

finalize:
    if (packet) {
        free(packet);
    }
    printf("\n");
    if (failed) {
        return -1;
    }
    return idx;
}

enum RcvStage { EXP_BEGIN, EXP_SIZE1, EXP_SIZE2, EXP_DATA, EXP_END, EXP_FINALIZE };

int receive_packet(cjag_config_t *config, const size_t *sets_map, uint8_t *buffer, size_t size) {
    int failed = -1;
    size_t max_size = (1 << (PACKET_SIZE_FIELD_LEN * DATA_BITS_PER_CODE)) + 10; /* 12 bits for packet size length + some headers */
    cncode_t *packet = (cncode_t*)calloc(max_size, 1);

    printf("Receiving: ");
    fflush(stdout);

    enum RcvStage stage = EXP_BEGIN;
    unsigned data_chunks_count = 0;
    unsigned data_size         = 0;
    cncode_t data_size_codes[MAX_PACKET_SIZE_FIELD_LEN];

    int idx = 0;
    int final_bytes = 0;
    //int begin_bytes = 0;
    int got_size_bytes_count = 0;
    //for (idx = 0; idx < (int)max_size; idx++) {
    while ((stage != EXP_FINALIZE) && (idx < (int)max_size)) {
        cncode_t code;
        jag_receive_code(&code, sets_map, config);
        watchdog_reset(config->watchdog);
        int res = code_to_index(code);
        //printf("got code %u (res = %d)\n", (unsigned)code, res);
        if (res == NO_SUCH_CODE) {
            printf("got unknown code %x, receive aborted.\n", (unsigned)code);
            goto finalize;
        }
        switch (stage) {
#if 0
            case EXP_BEGIN: {
                begin_bytes++;
                if ((res != CMD_PKT_BEGIN) && (res != CMD_PKT_BEGIN2) && (res != CMD_EXTENSION) && (begin_bytes >= 3)) {
                    printf("Expected packet begin marker, got %d (index %d). Receiving packet aborted.\n", (int)code, (int)res);
                    goto finalize;
                }
                if (res == CMD_EXTENSION) {
                    stage = EXP_SIZE1;
                }
            } break;
#else
            case EXP_BEGIN: {
                if (res != CMD_PKT_BEGIN) {
                    printf("Expected packet begin marker, got %d (index %d). Receiving packet aborted.\n", (int)code, (int)res);
                    goto finalize;
                }
                stage = EXP_SIZE1;
            } break;
#endif
            case EXP_SIZE1: {
                if (got_size_bytes_count < PACKET_SIZE_FIELD_LEN) {
                    data_size_codes[got_size_bytes_count] = code;
                    got_size_bytes_count++;
                }
                uint8_t data_size_fields[MAX_PACKET_SIZE_FIELD_LEN];
                memset(data_size_fields, 0, sizeof(data_size_fields));
                if (got_size_bytes_count >= PACKET_SIZE_FIELD_LEN) {
                    if (decode(data_size_fields, data_size_codes, PACKET_SIZE_FIELD_LEN) < 0) {
                        printf("Cannot decode packet size, receiving packet aborted.\n");
                        goto finalize;
                    }
                    unsigned data_size = (((unsigned)data_size_fields[1] << 8) | (unsigned)data_size_fields[0]);
                    data_chunks_count = encoded_size(data_size);
                    printf("data_size = %u, data_chunks_count = %u.\n", (unsigned)data_size, data_chunks_count);
                    stage = data_size > 0 ? EXP_DATA : EXP_END;
                }
            } break;
#if 0
            case EXP_SIZE2: {
                data_size_codes[1] = code;
                uint8_t data_size_fields[PACKET_SIZE_FIELD_LEN];
                if (decode(data_size_fields, data_size_codes, PACKET_SIZE_FIELD_LEN) < 0) {
                    printf("Cannot decode packet size, receiving packet aborted.\n");
                    goto finalize;
                }
                data_size = (((unsigned)data_size_fields[1] << 8) | (unsigned)data_size_fields[0]);
                //packet[0] = packet[1] = 0;
                stage = data_size > 0 ? EXP_DATA : EXP_END;
            } break;
#endif
            case EXP_DATA: {
                //printf("at EXP_DATA: idx = %d, data_chunks_count = %d.\n", (int)idx, (int)data_chunks_count);
                if (res < CMD_FIRST_DATA || res > CMD_LAST_DATA) {
                    //printf("Got incorrect data code %d (index %d). Receiving packet aborted.\n", (int)code, (int)res);
                    //goto finalize;
                    printf("Got incorrect data code %d (index %d). Replaced by default value.\n", (int)code, (int)res);
                    res = (((unsigned)(unsigned char)'?' << 8) | (unsigned)(unsigned char)'?');
                    code = index_to_code(CMD_FIRST_DATA + res);
                }

                /* it is guaranteed on the prev. stage that there is at least 1 byte of data */
                packet[idx++] = code;
                //buffer[idx++] = res - CMD_FIRST_DATA;
                if ((unsigned)idx >= data_chunks_count) {
                    //printf("at EXP_DATA: idx >= data_chunks_count!\n");
                    final_bytes = decode(buffer, packet, data_chunks_count);
                    if (final_bytes < 0) {
                        printf("Decoding data failed. Receiving packet aborted.\n");
                        goto finalize;
                    }
                    if (final_bytes > data_size)
                        final_bytes = data_size;
                    stage = EXP_END;
                    //printf("at EXP_DATA: switched stage to EXP_END!\n");
                }
            } break;
            case EXP_END: {
                if (res != CMD_PKT_END) {
                    printf("Expected packet end marker, got %d (index %d). Receiving packet aborted.\n", (int)code, (int)res);
                    goto finalize;
                }
                failed = 0;
                stage = EXP_FINALIZE;
            } break;
            case EXP_FINALIZE: {
                failed = 0;
                goto finalize;
            } break;
        }
    }

finalize:
    if (packet) {
        free(packet);
    }
    printf("\n");
    if (failed) {
        return -1;
    }
    //printf("Received packed data of %d bytes length.\n", final_bytes);
    return final_bytes;
}

