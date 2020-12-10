#ifndef TASK4_TASK4_H
#define TASK4_TASK4_H

unsigned short read_USHORT(unsigned char *, unsigned long *);
unsigned long read_UINT(unsigned char *, unsigned long *);
long read_INT(unsigned char *, unsigned long *);

struct metadata_t {

    unsigned long file_size;
    unsigned long pixelar_offset;
    unsigned long bitmap_width;
    long bitmap_height;
    unsigned char bits_per_pixel;
    unsigned long image_size;
    unsigned char bitmap_header_remains[16];
};



#endif //TASK4_TASK4_H
