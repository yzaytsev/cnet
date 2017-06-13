#ifndef CODEMAP_H
#   define CODEMAP_H 

#include <stdint.h>

#define PRODUCE_CNCODE_TYPE_NX(bitness) uint ## bitness ## _t
#define PRODUCE_CNCODE_TYPE(bitness) PRODUCE_CNCODE_TYPE_NX(bitness)
#define CNCODE_TYPE PRODUCE_CNCODE_TYPE(CNCODE_BITS)

//typedef uint8_t cncode_t;
//typedef uint32_t cncode_t;
typedef CNCODE_TYPE cncode_t;

//#undef PRODUCE_CNCODE_TYPE_NX
//#undef PRODUCE_CNCODE_TYPE
//#undef CNCODE_TYPE

//enum {
//    NO_SUCH_CODE  =  -1,
//    NO_SUCH_INDEX = 255
//}
const cncode_t NO_SUCH_INDEX;
const int      NO_SUCH_CODE;

const int CMD_EXTENSION;
const int CMD_PKT_BEGIN;
const int CMD_FIRST_DATA;
const int CMD_LAST_DATA;
const int CMD_PKT_END;
const int CMD_FIN;

/* These constants have to be changed if code bitness in codemap.c is changed! */
/* There are 70 codes, 64 is used for transfering 6-bit data, remaining 6 codes are used for commands */
#if 0
enum Command {
    CMD_NOP1       =  0, /* reserve 00001111 and 11110000 for the sake of robustness */
    CMD_NOP2       = 69,
    CMD_EXTENSION  =  1, /* not used for now */
    CMD_PKT_BEGIN  =  2, /* start packet transfer */
    CMD_FIRST_DATA =  3, /* the first data code */
    CMD_LAST_DATA  = 66, /* the last data code */
    CMD_PKT_END    = 67, /* end packet transfer */
    CMD_FIN        = 68  /* finish communication */
};
#endif

const int DATA_BITS_PER_CODE;
const int PACKET_SIZE_FIELD_LEN;

/* These constants have to be changed if code bitness in codemap.c is changed! */
enum {
//    DATA_BITS_PER_CODE = 6,            /* data bits transfered per single send */
    CODE_SET_BITS             = ENC_SET_BITS,  /* count of bits required to be set in each code */
    MAX_PACKET_SIZE_FIELD_LEN = 10,      /* the goal is to store 2^10 bytes (1KB), so even for 1 bit per cncode_t this should be enough */
};

cncode_t index_to_code(int index);
int      code_to_index(cncode_t code);

#endif /* #ifndef CODEMAP_H */

