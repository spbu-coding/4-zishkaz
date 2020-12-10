#include <stdio.h>
#include <stdlib.h>
#include "task4.h"

#define HEADERS_OFFSET 54
#define MAX_UNEQUAL_COORDINATES 100

struct BMP {

    unsigned long palette[256];
    unsigned long *pixel_array;
};

unsigned long hash(unsigned char r, unsigned char g, unsigned char b) {

    return r + 256*g + 65280*b;
}

void bit8_handler(struct BMP *bmp, struct metadata_t *meta, FILE *file) {

    unsigned char *buf = malloc(1024);
    fread(buf, 1024, 1, file);
    for (unsigned int i = 4; i < 1025; i += 4) {

        bmp->palette[i / 4 - 1] = hash(buf[i - 2], buf[i - 3], buf[i - 4]);
    }
    free(buf);
    buf = malloc(meta->image_size);
    fread(buf, meta->image_size, 1, file);
    bmp->pixel_array = malloc(meta->image_size * sizeof(long));
    for (unsigned long i = 0; i < meta->image_size; i++) {

        bmp->pixel_array[i] = bmp->palette[buf[i]];
    }
    free(buf);
}

void bit24_handler(struct BMP *bmp, struct metadata_t *meta, FILE *file) {

    unsigned long row_size = (meta->bits_per_pixel * meta->bitmap_width + 31) / 32 * 4;
    bmp->pixel_array = malloc(abs(meta->bitmap_height) * meta->bitmap_width * sizeof(long));
    unsigned char *buf = malloc(meta->image_size);
    fread(buf, meta->image_size, 1, file);
    unsigned long counter = 0, global_i = 0;
    for (unsigned long j = 0; j < abs(meta->bitmap_height); j++) {

        for (unsigned long i = 0; i < meta->bitmap_width; i++) {

            bmp->pixel_array[counter] = hash(buf[global_i + i*3 + 2], buf[global_i + i*3 + 1], buf[global_i + i*3]);
            counter++;
        }
        global_i += row_size;
    }
    free(buf);
}

int main(int argc, char **argv) {

    if (argc != 3) {

        fprintf(stderr, "Invalid arguments!");
        return -1;
    }
    FILE *first, *second;
    unsigned char *buf_first = malloc(HEADERS_OFFSET - 16), *buf_second = malloc(HEADERS_OFFSET - 16);
    first = fopen(argv[1], "rb");
    second = fopen(argv[2], "rb");
    if (first == NULL || second == NULL) {

        fprintf(stderr, "Couldn't open on of the files!");
        return -1;
    }
    struct metadata_t meta_first;
    struct metadata_t meta_second;
    unsigned long offset_first = 0, offset_second = 0;
    fread(buf_first, HEADERS_OFFSET - 16, 1, first);
    fread(buf_second, HEADERS_OFFSET - 16, 1, second);
    if (read_USHORT(buf_first, &offset_first) != 0x4D42 || read_USHORT(buf_second, &offset_second) != 0x4D42) {

        fprintf(stderr, "One of the files is not BMP type!");
        return -1;
    }
    read_UINT(buf_first, &offset_first); read_UINT(buf_second, &offset_second);
    if (read_UINT(buf_first, &offset_first) != 0 || read_UINT(buf_second, &offset_second) != 0) {

        fprintf(stderr, "Some file has reserved field with a value not zero!");
        return -1;
    }
    meta_first.pixelar_offset = read_UINT(buf_first, &offset_first); meta_second.pixelar_offset = read_UINT(buf_second, &offset_second);
    if (!(read_UINT(buf_first, &offset_first) == 40 && read_UINT(buf_second, &offset_second) == 40)) {

        fprintf(stderr, "Invalid header size!");
        return -1;
    }
    meta_first.bitmap_width = read_UINT(buf_first, &offset_first); meta_second.bitmap_width = read_UINT(buf_second, &offset_second);
    if (meta_first.bitmap_width != meta_second.bitmap_width) {

        fprintf(stderr, "Image width is different!");
        return -1;
    }
    meta_first.bitmap_height = read_INT(buf_first, &offset_first); meta_second.bitmap_height = read_INT(buf_second, &offset_second);
    if (abs(meta_first.bitmap_height) != abs(meta_second.bitmap_height)) {

        fprintf(stderr, "Image height is different!");
        return -1;
    }
    if (!(read_USHORT(buf_first, &offset_first) == 1 && read_USHORT(buf_second, &offset_second) == 1)) {

        fprintf(stderr, "One of the files has incorrect number of color planes!");
        return -1;
    }
    meta_first.bits_per_pixel = read_USHORT(buf_first, &offset_first); meta_second.bits_per_pixel = read_USHORT(buf_second, &offset_second);
    if (!(meta_first.bits_per_pixel == 8 || meta_first.bits_per_pixel == 24) || !(meta_second.bits_per_pixel == 8 || meta_second.bits_per_pixel == 24)) {

        fprintf(stderr, "Image bits per pixel values are incomparable!");
        return -1;
    }
    if (read_UINT(buf_first, &offset_first) || read_UINT(buf_second, &offset_second)) {

        fprintf(stderr, "One of the files has not null compression field!");
        return -1;
    }
    meta_first.image_size = read_UINT(buf_first, &offset_first); meta_second.image_size = read_UINT(buf_second, &offset_second);
    if (meta_first.bits_per_pixel == 8) {

        meta_first.image_size = abs(meta_first.bitmap_height) * meta_first.bitmap_width;
        meta_second.image_size = abs(meta_second.bitmap_height) * meta_second.bitmap_width;
    } else {

        unsigned long row_size = (meta_first.bits_per_pixel * meta_first.bitmap_width + 31) / 32 * 4;
        meta_first.image_size = abs(meta_first.bitmap_height) * row_size;
        meta_second.image_size = abs(meta_second.bitmap_height) * row_size;
    }
    if (meta_first.image_size != meta_second.image_size) {

        fprintf(stderr, "Sizes of raw bitmap data are unequal!");
        return -1;
    }
    fread(meta_first.bitmap_header_remains, 16, 1, first);
    fread(meta_second.bitmap_header_remains, 16, 1, second);
    free(buf_first); free(buf_second);
    struct BMP first_bmp, second_bmp;
    if (meta_first.bits_per_pixel == 8) bit8_handler(&first_bmp, &meta_first, first);
    else bit24_handler(&first_bmp, &meta_first, first);
    if (meta_second.bits_per_pixel == 8) bit8_handler(&second_bmp, &meta_second, second);
    else bit24_handler(&second_bmp, &meta_second, second);
    unsigned char error = 0;
    unsigned long all_pixels = abs(meta_first.bitmap_height) * meta_first.bitmap_width;
    for (unsigned long y = 0; y < abs(meta_first.bitmap_height); y++) {

        for (unsigned long x = 0; x < meta_first.bitmap_width; x++) {

            long a, b;
            if (meta_first.bitmap_height < 0) a = all_pixels - (y + 1) * meta_first.bitmap_width + x; else a = y * meta_first.bitmap_width + x;
            if (meta_second.bitmap_height < 0) b = all_pixels - (y + 1) * meta_second.bitmap_width + x; else b = y * meta_second.bitmap_width + x;
            if (first_bmp.pixel_array[a] != second_bmp.pixel_array[b]) {

                printf("%lu %lu\n", x, y);
                error++;
                if (error == MAX_UNEQUAL_COORDINATES + 1) return -1;
            }
        }
    }
    if (!error) printf("Pictures are equal!\n"); else return -1;
    return 0;
}
