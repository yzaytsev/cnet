#include <net/state_enc.h>

/***
 * 8 bits, 4 is always set and 4 is always unset.
 * It provides 70 states (8! / (4! * 4!)) which is enough to encode 6 bits
 * and some commands if needed.
 **/
static uint8_t num_to_state[] = {
    15,  
};

