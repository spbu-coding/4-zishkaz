#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdbmp.h"
#include "negative.h"

#define HEADERS_OFFSET 54
#define LIBRARY_ERROR_CODE -3
#define COMMON_ERROR_CODE -1
#define FILE_STRUCTURE_ERROR_CODE -2
#define INFO_HEADER_SIZE 40
//Улучшить сообщения об ошибках

struct metadata_t {

    unsigned long file_size;
    unsigned long pixelar_offset;
    unsigned long bitmap_width;
    long bitmap_height;
    unsigned char bits_per_pixel;
    unsigned long image_size;
    unsigned char bitmap_header_remains[16];
};

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

void write_INT(unsigned char *buf, unsigned long *offset, long x) {

    unsigned long y;
    if (x < 0) y = ~abs(x) + 1; else y = x;
    buf[*offset] = (y & 0x000000ff);
    buf[*offset + 1] = (y & 0x0000ff00) >> 8;
    buf[*offset + 2] = (y & 0x00ff0000) >> 16;
    buf[*offset + 3] = (y & 0xff000000) >> 24;
    *offset += 4;
}

void write_USHORT(unsigned char *buf, unsigned long *offset, unsigned short x) {

    buf[*offset] = (x & 0x000000ff);
    buf[*offset + 1] = (x & 0x0000ff00) >> 8;
    *offset += 2;
}

void write_UINT(unsigned char *buf, unsigned long *offset, unsigned int x) {

    buf[*offset] = (x & 0x000000ff);
    buf[*offset + 1] = (x & 0x0000ff00) >> 8;
    buf[*offset + 2] = (x & 0x00ff0000) >> 16;
    buf[*offset + 3] = (x & 0xff000000) >> 24;
    *offset += 4;
}

void write_metadata(struct metadata_t *data, FILE *file) {

    unsigned char *buf = malloc(HEADERS_OFFSET - 16);
    unsigned long offset = 0;
    write_USHORT(buf, &offset, 0x4D42);
    write_UINT(buf, &offset, data->file_size);
    write_UINT(buf, &offset, 0x0);
    write_UINT(buf, &offset, data->pixelar_offset);
    write_UINT(buf, &offset, 0x28);
    write_UINT(buf, &offset, data->bitmap_width);
    write_INT(buf, &offset, data->bitmap_height);
    write_USHORT(buf, &offset, 0x1);
    write_USHORT(buf, &offset, data->bits_per_pixel);
    write_UINT(buf, &offset, 0x0);
    write_UINT(buf, &offset, data->image_size);
    fwrite(buf, HEADERS_OFFSET - 16, 1, file);
    fwrite(data->bitmap_header_remains, 16, 1, file);
    free(buf);
}

int bit8_handler(FILE *input, FILE *output, struct metadata_t *data) {

    unsigned char byte4[4];
    for(unsigned int i = 0; i < 256; i++) {

        fread(byte4, 4, 1, input);
        byte4[0] = ~byte4[0];
        byte4[1] = ~byte4[1];
        byte4[2] = ~byte4[2];
        if (byte4[3]) {

            fprintf(stderr,"%s %u", "Invalid color table value with index", i);
            return FILE_STRUCTURE_ERROR_CODE;
        }
        fwrite(byte4, 4, 1, output);
    }
    unsigned char byte[1];
    for(unsigned long i = 0; i < data->image_size; i++) {

        fread(byte, 1, 1, input);
        fwrite(byte, 1, 1, output);
    }
    return 0;
}

void bit24_handler(FILE *input, FILE *output, struct metadata_t *data) {

    unsigned long row_size = (data->bits_per_pixel * data->bitmap_width + 31) / 32 * 4;
    printf("%s %ld\n", "Row size:", row_size);
    unsigned long count;
    unsigned char byte3[3];
    for(unsigned long i = 0; i < abs(data->bitmap_height); i++) {

        count = row_size;
        while(count > 2) {

            fread(byte3, 3, 1, input);
            byte3[0] = ~byte3[0];
            byte3[1] = ~byte3[1];
            byte3[2] = ~byte3[2];
            fwrite(byte3, 3, 1, output);
            count -= 3;
        }
        fread(byte3, count, 1, input);
        fwrite(byte3, count, 1, output);
    }
}

int mine_handler(char **argv) {

    char *input_filename = argv[2];
    char *output_filename = argv[3];
    FILE *input, *output;
    input = fopen(input_filename, "rb");
    if (input == NULL)  {

        fprintf(stderr, "Couldn't open input file!");
        return COMMON_ERROR_CODE;
    }
    struct metadata_t data;

    unsigned char *buf = malloc(HEADERS_OFFSET - 16);
    unsigned long offset_counter = 0;
    fread(buf, HEADERS_OFFSET - 16, 1, input);
    unsigned short file_type = read_USHORT(buf, &offset_counter);
    if (file_type != 0x4D42) {

        fprintf(stderr, "Invalid file type!");
        return COMMON_ERROR_CODE;
    }
    printf("%s %hX\n", "File type:", file_type);

    data.file_size = read_UINT(buf, &offset_counter);
    printf("%s %ld\n", "File size:", data.file_size);

    if (read_UINT(buf, &offset_counter) != 0) {

        fprintf(stderr, "Reserved field must be zero!");
        return FILE_STRUCTURE_ERROR_CODE;
    }

    data.pixelar_offset = read_UINT(buf, &offset_counter);
    if(data.pixelar_offset < HEADERS_OFFSET) {

        fprintf(stderr, "Invalid offset of bitmap image data!");
        return FILE_STRUCTURE_ERROR_CODE;
    }
    printf("%s %lX\n", "Pixel array offset:", data.pixelar_offset);

    unsigned int temp = read_UINT(buf, &offset_counter);
    if (temp != INFO_HEADER_SIZE) {

        fprintf(stderr, "%s %d%s %u", "Invalid header size! Need", INFO_HEADER_SIZE, ", found", temp);
        return FILE_STRUCTURE_ERROR_CODE;
    }
    printf("%s %d\n", "Size of header:", temp);

    data.bitmap_width = read_UINT(buf, &offset_counter);
    printf("%s %ld\n", "Image width:", data.bitmap_width);

    data.bitmap_height = read_INT(buf, &offset_counter);
    printf("%s %ld\n", "Image height:", data.bitmap_height);

    if (read_USHORT(buf, &offset_counter) != 1) {

        fprintf(stderr, "Invalid number of color planes!");
        return FILE_STRUCTURE_ERROR_CODE;
    }

    data.bits_per_pixel = read_USHORT(buf, &offset_counter);
    if (!(data.bits_per_pixel == 8 || data.bits_per_pixel == 24)) {

        fprintf(stderr, "Invalid bits per pixel count!");
        return FILE_STRUCTURE_ERROR_CODE;
    }
    printf("%s %d\n", "Bits per pixel:", data.bits_per_pixel);

    if (read_UINT(buf, &offset_counter)) {

        fprintf(stderr, "Compression field must be null!");
        return FILE_STRUCTURE_ERROR_CODE;
    }

    data.image_size = read_UINT(buf, &offset_counter);
    if (data.image_size == 0) {
        if (data.bits_per_pixel == 8) data.image_size = data.bitmap_width * abs(data.bitmap_height); else
        data.image_size = data.bitmap_width * abs(data.bitmap_height) * 4;
    }
    if (data.image_size > data.file_size - data.pixelar_offset) {

        fprintf(stderr, "Invalid image size!");
        return FILE_STRUCTURE_ERROR_CODE;
    }
    printf("%s %ld\n", "Image size:", data.image_size);
    free(buf);

    fread(data.bitmap_header_remains, 16, 1, input);

    output = fopen(output_filename, "wb");
    if (output == NULL) {

        fprintf(stderr, "Couldn't create output file!");
        return COMMON_ERROR_CODE;
    }
    write_metadata(&data, output);
    switch(data.bits_per_pixel) {

        int error_value;
        case 8:

            error_value = bit8_handler(input, output, &data);
            if (error_value) return error_value;
            break;

        case 24:

            bit24_handler(input, output, &data);
            break;
    }
    fclose(input);
    fclose(output);
    return 0;
}

int theirs_handler(char **argv) {

    BMP *bmp_input = BMP_ReadFile(argv[2]);
    if(BMP_LAST_ERROR_CODE != BMP_OK) return -3;
    UCHAR r, g, b;
    switch(bmp_input->Header.BitsPerPixel) {

        case 8:

            for(unsigned int i = 0; i < 256; i++) {

                BMP_GetPaletteColor(bmp_input, i, &r, &g, &b);
                if(BMP_LAST_ERROR_CODE != BMP_OK) return -3;
                BMP_SetPaletteColor(bmp_input, i, ~r, ~g, ~b);
            }
            BMP_WriteFile(bmp_input, argv[3]);
            BMP_Free(bmp_input);
            if(BMP_LAST_ERROR_CODE != BMP_OK) return -3;
            return 0;

        case 24:

            BMP_Free(bmp_input);
            return convert(argv[2], argv[3]);
    }
    BMP_Free(bmp_input);
    return -3;
}


int main(int argc, char **argv) {

    if (argc != 4) {

        fprintf(stderr, "Invalid arguments!");
        return COMMON_ERROR_CODE;
    }
    if(strcmp(".bmp", strchr(argv[3], '.')) != 0) {

        fprintf(stderr, "Output file is not bmp-type!");
        return COMMON_ERROR_CODE;
    }
    if (strcmp(argv[1], "--mine") == 0) {

        mine_handler(argv);
    } else if (strcmp(argv[1], "--theirs") == 0) {

        if (theirs_handler(argv)) {

            fprintf(stderr, "Error with alien library!\n");
            return LIBRARY_ERROR_CODE;
        }
    } else {

        fprintf(stderr, "Invalid arguments!");
        return COMMON_ERROR_CODE;
    }
    return 0;
}
