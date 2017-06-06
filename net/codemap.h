#ifndef CODEMAP_H
#   define CODEMAP_H 

#include <stdint.h>

typedef uint8_t cncode_t;

enum {
    NO_SUCH_CODE  =  -1,
    NO_SUCH_INDEX = 255
};

/* These constants have to be changed if code bitness in codemap.c is changed! */
/* There are 70 codes, 64 is used for transfering 6-bit data, remaining 6 codes are used for commands */
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

/* These constants have to be changed if code bitness in codemap.c is changed! */
enum {
    DATA_BITS_PER_CODE = 6, /* data bits transfered per single send */
    CODE_SET_BITS      = 4  /* count of bits required to be set in each code */
};

cncode_t index_to_code(int index);
int      code_to_index(cncode_t code);

#endif /* #ifndef CODEMAP_H */

