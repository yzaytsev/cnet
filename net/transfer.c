#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <net/transfer.h>
#include <net/codemap.h>
#include <net/encoder.h>
#include <jag/send.h>
#include <jag/receive.h>

enum { PACKET_SIZE_FIELD_LEN = 2 };

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

    /* 12 bits for packet size */
    int enc_data_bytes = encode(packet + idx + PACKET_SIZE_FIELD_LEN, data, size, -1);
    //printf("enc_data_bytes = %d.\n", enc_data_bytes);
    if (enc_data_bytes < 0) {
        goto finalize;
    }
    packet[idx] = packet[idx + 1] = 0;
    uint8_t raw_size[PACKET_SIZE_FIELD_LEN];
    raw_size[0] = (unsigned)enc_data_bytes & 0xffu;
    raw_size[1] = ((unsigned)enc_data_bytes >> 8) & 0x0fu;
    int enc_size_bytes = encode(packet + idx, raw_size, PACKET_SIZE_FIELD_LEN, PACKET_SIZE_FIELD_LEN * DATA_BITS_PER_CODE);
    //printf("enc_size_bytes = %d.\n", enc_data_bytes);
    if (enc_size_bytes < 0) {
        goto finalize;
    }
    idx += (PACKET_SIZE_FIELD_LEN + enc_data_bytes);
    packet[idx++] = index_to_code(CMD_PKT_END);

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
    size_t max_size = (1 << 12) + 10; /* 12 bits for packet size length + some headers */
    cncode_t *packet = (cncode_t*)calloc(max_size, 1);

    printf("Receiving: ");
    fflush(stdout);

    enum RcvStage stage = EXP_BEGIN;
    unsigned data_size  = 0;
    cncode_t data_size_codes[PACKET_SIZE_FIELD_LEN];

    int idx = 0;
    int final_bytes = 0;
    //for (idx = 0; idx < (int)max_size; idx++) {
    while ((stage != EXP_FINALIZE) && (idx < (int)max_size)) {
        cncode_t code;
        jag_receive_code(&code, sets_map, config);
        watchdog_reset(config->watchdog);
        int res = code_to_index(code);
        if (res == NO_SUCH_CODE) {
            printf("got unknown code %x, receive aborted.\n", (unsigned)code);
            goto finalize;
        }
        switch (stage) {
            case EXP_BEGIN: {
                if (res != CMD_PKT_BEGIN) {
                    printf("Expected packet begin marker, got %d (index %d). Receiving packet aborted.\n", (int)code, (int)res);
                    goto finalize;
                }
                stage = EXP_SIZE1;
            } break;
            case EXP_SIZE1: {
                data_size_codes[0] = code;
                data_size_codes[1] = 0;
                stage = EXP_SIZE2;
            } break;
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
            case EXP_DATA: {
                if (res < CMD_FIRST_DATA || res > CMD_LAST_DATA) {
                    printf("Got incorrect data code %d (index %d). Receiving packet aborted.\n", (int)code, (int)res);
                    goto finalize;
                }

                /* it is guaranteed on the prev. stage that there is at least 1 byte of data */
                packet[idx++] = code;
                //buffer[idx++] = res - CMD_FIRST_DATA;
                if ((unsigned)idx >= data_size) {
                    final_bytes = decode(buffer, packet, data_size);
                    if (final_bytes < 0) {
                        printf("Decoding data failed. Receiving packet aborted.\n");
                        goto finalize;
                    }
                    stage = EXP_END;
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

