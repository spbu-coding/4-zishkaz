#include <stdio.h>
#include <stdlib.h>

#define HEADERS_OFFSET 54
#define MAX_UNEQUAL_COORDINATES 100

struct metadata_t {

    unsigned long file_size;
    unsigned long pixelar_offset;
    unsigned long bitmap_width;
    long bitmap_height;
    unsigned char bits_per_pixel;
    unsigned long image_size;
    unsigned char bitmap_header_remains[16];
};

struct BMP {

    unsigned long palette[256];
    unsigned long *pixel_array;
};

unsigned long hash(unsigned char r, unsigned char g, unsigned char b) {

    return r + 256*g + 65280*b;
}

int read_INT(const unsigned char *buf, unsigned long *offset) {

    *offset +=4;
    if (buf[*offset - 1] > 15) {

        long x = buf[*offset - 1] << 24 | buf[*offset - 2] << 16 | buf[*offset - 3] << 8 | buf[*offset - 4];
        x--;
        return -~x;
    } else return buf[*offset - 1] << 24 | buf[*offset - 2] << 16 | buf[*offset - 3] << 8 | buf[*offset - 4];
}

unsigned short read_USHORT(const unsigned char *buf, unsigned long *offset) {

    *offset += 2;
    return buf[*offset - 1] << 8 | buf[*offset - 2];
}

unsigned int read_UINT(const unsigned char *buf, unsigned long *offset) {

    *offset +=4;
    return buf[*offset - 1] << 24 | buf[*offset - 2] << 16 | buf[*offset - 3] << 8 | buf[*offset - 4];
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
    if (meta_first.bits_per_pixel == 8) {

        unsigned char *buf = malloc(1024);
        fread(buf, 1024, 1, first);
        for (unsigned int i = 4; i < 1025; i += 4) {

            first_bmp.palette[i / 4 - 1] = hash(buf[i - 2], buf[i - 3], buf[i - 4]);
        }
        free(buf);
        buf = malloc(meta_first.image_size);
        fread(buf, meta_first.image_size, 1, first);
        first_bmp.pixel_array = malloc(meta_first.image_size * sizeof(long));
        for (unsigned long i = 0; i < meta_first.image_size; i++) {

            first_bmp.pixel_array[i] = first_bmp.palette[buf[i]];
        }
        free(buf);
    } else {

        unsigned long row_size = (meta_first.bits_per_pixel * meta_first.bitmap_width + 31) / 32 * 4;
        first_bmp.pixel_array = malloc(abs(meta_first.bitmap_height) * meta_first.bitmap_width * sizeof(long));
        unsigned char *buf = malloc(meta_first.image_size);
        fread(buf, meta_first.image_size, 1, first);
        unsigned long counter = 0, global_i = 0;
        for (unsigned long j = 0; j < abs(meta_first.bitmap_height); j++) {

            for (unsigned long i = 0; i < meta_first.bitmap_width; i++) {

                first_bmp.pixel_array[counter] = hash(buf[global_i + i*3 + 2], buf[global_i + i*3 + 1], buf[global_i + i*3]);
                counter++;
            }
            global_i += row_size;
        }
        free(buf);
    }
    if (meta_second.bits_per_pixel == 8) {

        unsigned char *buf = malloc(1024);
        fread(buf, 1024, 1, second);
        for (unsigned int i = 4; i < 1025; i += 4) {

            second_bmp.palette[i / 4 - 1] = hash(buf[i - 2], buf[i - 3], buf[i - 4]);
        }
        free(buf);
        buf = malloc(meta_second.image_size);
        fread(buf, meta_second.image_size, 1, second);
        second_bmp.pixel_array = malloc(meta_second.image_size * sizeof(long));
        for (unsigned long i = 0; i < meta_second.image_size; i++) {

            second_bmp.pixel_array[i] = second_bmp.palette[buf[i]];
        }
        free(buf);
    } else {

        unsigned long row_size = (meta_second.bits_per_pixel * meta_second.bitmap_width + 31) / 32 * 4;
        second_bmp.pixel_array = malloc(row_size * abs(meta_second.bitmap_height) * sizeof(long));
        unsigned char *buf = malloc(meta_second.image_size);
        fread(buf, meta_second.image_size, 1, second);
        unsigned long counter = 0, global_i = 0;
        for (unsigned long j = 0; j < abs(meta_second.bitmap_height); j++) {

            for (unsigned long i = 0; i < meta_second.bitmap_width; i++) {

                second_bmp.pixel_array[counter] = hash(buf[global_i + i*3 + 2], buf[global_i + i*3 + 1], buf[global_i + i*3]);
                counter++;
            }
            global_i += row_size;
        }
        free(buf);
    }
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
