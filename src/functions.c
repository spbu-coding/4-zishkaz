#include "task4.h"

unsigned short read_USHORT(unsigned char *buf, unsigned long *offset) {

    *offset += 2;
    return buf[*offset - 1] << 8 | buf[*offset - 2];
}

unsigned long read_UINT(unsigned char *buf, unsigned long *offset) {

    *offset +=4;
    return buf[*offset - 1] << 24 | buf[*offset - 2] << 16 | buf[*offset - 3] << 8 | buf[*offset - 4];
}

long read_INT(unsigned char *buf, unsigned long *offset) {

    *offset +=4;
    if (buf[*offset - 1] > 15) {

        long x = buf[*offset - 1] << 24 | buf[*offset - 2] << 16 | buf[*offset - 3] << 8 | buf[*offset - 4];
        x--;
        return -~x;
    } else return buf[*offset - 1] << 24 | buf[*offset - 2] << 16 | buf[*offset - 3] << 8 | buf[*offset - 4];
}