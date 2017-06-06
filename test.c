#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <net/encoder.h>

void test(const uint8_t *data1, size_t size) {
    printf("before: [ ");
    for (size_t i = 0; i < size; i++) {
        printf("%x ", (unsigned)data1[i]);
    }
    printf("]\n");
    cncode_t codes[encoded_size(size)];
    int es = encode(codes, data1, size);
    if (es < 0) {
        printf("*** encoding error, encode returned %d!\n", es);
        return;
    }
    else {
        printf("%d bytes encoded to %d bytes.\n", (int)size, es);
    }
    uint8_t data2[decoded_size(es)];
    int ds = decode(data2, codes, es);
    if (ds < 0) {
        printf("*** decoding error, decode returned %d!\n", ds);
        return;
    }
    else {
        printf("%d bytes decoded to %d bytes.\n", es, ds);
    }
    printf("after:  [ ");
    for (size_t i = 0; i < size; i++) {
        printf("%x ", (unsigned)data2[i]);
    }
    printf("]\n");
    if (size != ds) {
        printf("*** length differs!\n");
    }
    else {
        int differs = memcmp(data1, data2, ds);
        printf("data1 and data2 does %s.\n", differs ? "NOT match" : "match");
    }
}

int main() {
    static const uint8_t dt1[] = { 1, 5, 200, 0, 255, 254, 60 };
    test(dt1, sizeof(dt1) / sizeof(dt1[0]));

#if 1
    srand(time(NULL));
    uint8_t dt2[15001];
    for (size_t i = 0; i < sizeof(dt2); i++) {
        dt2[i] = rand();
    }
    test(dt2, sizeof(dt2) / sizeof(dt2[0]));
#endif
    return 0;
}

