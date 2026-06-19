#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Compressor declarations from CAD wrapper headers
int lz4_compress_c(const unsigned char *in, size_t in_sz, unsigned char **out, size_t *out_sz);
int zx0_compress_c(const unsigned char *in, size_t in_sz, unsigned char **out, size_t *out_sz);

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <lz4|zx0> <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    const char *mode = argv[1];
    const char *in_path = argv[2];
    const char *out_path = argv[3];

    int use_zx0 = 0;
    if (strcmp(mode, "zx0") == 0) {
        use_zx0 = 1;
    } else if (strcmp(mode, "lz4") != 0) {
        fprintf(stderr, "Error: Unknown mode '%s'. Must be 'lz4' or 'zx0'.\n", mode);
        return 1;
    }

    // Read input file
    FILE *f_in = fopen(in_path, "rb");
    if (!f_in) {
        perror("Error opening input file");
        return 1;
    }

    fseek(f_in, 0, SEEK_END);
    long file_sz = ftell(f_in);
    fseek(f_in, 0, SEEK_SET);

    if (file_sz <= 0) {
        fprintf(stderr, "Error: Input file is empty or invalid size.\n");
        fclose(f_in);
        return 1;
    }

    size_t in_sz = (size_t)file_sz;
    unsigned char *in_buf = malloc(in_sz);
    if (!in_buf) {
        perror("Allocation error for input buffer");
        fclose(f_in);
        return 1;
    }

    if (fread(in_buf, 1, in_sz, f_in) != in_sz) {
        perror("Error reading input file");
        free(in_buf);
        fclose(f_in);
        return 1;
    }
    fclose(f_in);

    unsigned char *out_buf = NULL;
    size_t out_sz = 0;
    int ret = 0;

    if (use_zx0) {
        ret = zx0_compress_c(in_buf, in_sz, &out_buf, &out_sz);
    } else {
        ret = lz4_compress_c(in_buf, in_sz, &out_buf, &out_sz);
    }

    free(in_buf);

    if (ret != 0 || !out_buf || out_sz == 0) {
        fprintf(stderr, "Compression failed!\n");
        if (out_buf) free(out_buf);
        return 1;
    }

    // Write output file
    FILE *f_out = fopen(out_path, "wb");
    if (!f_out) {
        perror("Error opening output file");
        free(out_buf);
        return 1;
    }

    if (fwrite(out_buf, 1, out_sz, f_out) != out_sz) {
        perror("Error writing output file");
        fclose(f_out);
        free(out_buf);
        return 1;
    }

    fclose(f_out);
    free(out_buf);

    return 0;
}
