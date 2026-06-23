/*
 * png_decode.c - Decode PNG files to compact raw pixel formats
 *
 * Uses stb_image for PNG decoding, then analyzes the pixel data
 * to choose the most compact representation:
 *   - 1-bit B&W if only 2 unique alpha values and grayscale
 *   - Grayscale+Alpha if all pixels are gray
 *   - Palette indexed if <= 256 unique RGBA colors
 *   - RGBA8888 as fallback
 */

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO  /* We'll load files ourselves for better control */
#include "stb_image.h"

#include "png_decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *zx0em_pixfmt_name(int pixfmt) {
    switch (pixfmt) {
        case ZX0EM_PIXFMT_GRAY_ALPHA: return "GRAY_ALPHA";
        case ZX0EM_PIXFMT_PALETTE:    return "PALETTE";
        case ZX0EM_PIXFMT_RGBA:       return "RGBA";
        case ZX0EM_PIXFMT_1BIT:       return "1BIT";
        case ZX0EM_PIXFMT_NONE:       return "NONE";
        default:                      return "UNKNOWN";
    }
}

int zx0em_pixfmt_bpp(int pixfmt) {
    switch (pixfmt) {
        case ZX0EM_PIXFMT_GRAY_ALPHA: return 2;
        case ZX0EM_PIXFMT_PALETTE:    return 1;
        case ZX0EM_PIXFMT_RGBA:       return 4;
        case ZX0EM_PIXFMT_1BIT:       return 0; /* sub-byte */
        default:                      return 0;
    }
}

/* Check if all pixels are grayscale (R == G == B) */
static int is_grayscale(const uint8_t *rgba, int w, int h) {
    int n = w * h;
    for (int i = 0; i < n; i++) {
        int idx = i * 4;
        if (rgba[idx] != rgba[idx + 1] || rgba[idx] != rgba[idx + 2])
            return 0;
    }
    return 1;
}

/* Check if image is pure 1-bit (only 2 distinct gray+alpha combos) */
static int is_1bit(const uint8_t *rgba, int w, int h) {
    uint16_t seen[2] = {0, 0};
    int count = 0;
    int n = w * h;

    for (int i = 0; i < n; i++) {
        int idx = i * 4;
        /* Must be grayscale */
        if (rgba[idx] != rgba[idx + 1] || rgba[idx] != rgba[idx + 2])
            return 0;
        uint16_t val = (uint16_t)((rgba[idx] << 8) | rgba[idx + 3]);
        int found = 0;
        for (int j = 0; j < count; j++) {
            if (seen[j] == val) { found = 1; break; }
        }
        if (!found) {
            if (count >= 2) return 0;
            seen[count++] = val;
        }
    }
    return 1;
}

/* Sort function for palette colors */
static int color_cmp(const void *a, const void *b) {
    uint32_t ca = *(const uint32_t *)a;
    uint32_t cb = *(const uint32_t *)b;
    if (ca < cb) return -1;
    if (ca > cb) return 1;
    return 0;
}

/* Count unique RGBA colors (up to max_colors+1) and sort them */
static int count_unique_colors(const uint8_t *rgba, int w, int h,
                                uint32_t *palette_out, int max_colors) {
    int count = 0;
    int n = w * h;

    for (int i = 0; i < n; i++) {
        int idx = i * 4;
        uint32_t color = ((uint32_t)rgba[idx] << 24) |
                         ((uint32_t)rgba[idx + 1] << 16) |
                         ((uint32_t)rgba[idx + 2] << 8) |
                         (uint32_t)rgba[idx + 3];
        int found = 0;
        for (int j = 0; j < count; j++) {
            if (palette_out[j] == color) { found = 1; break; }
        }
        if (!found) {
            if (count >= max_colors) return max_colors + 1; /* overflow */
            palette_out[count++] = color;
        }
    }
    
    /* Sort the palette to guarantee stable indices across identical/similar images */
    if (count > 0 && count <= max_colors) {
        qsort(palette_out, (size_t)count, sizeof(uint32_t), color_cmp);
    }
    
    return count;
}

/* Convert RGBA to 1-bit packed */
static uint8_t *convert_1bit(const uint8_t *rgba, int w, int h, size_t *out_size) {
    int n = w * h;
    /* Determine which gray value is "on" (non-transparent/white) */
    /* We pack: 1 = foreground pixel, 0 = background/transparent */
    /* Plus a separate alpha bitmask */
    size_t row_bytes = (size_t)((w + 7) / 8);
    size_t pixel_bytes = row_bytes * (size_t)h;
    /* Two bitmasks: pixel values + alpha */
    *out_size = pixel_bytes * 2;
    uint8_t *out = (uint8_t *)calloc(1, *out_size);
    if (!out) return NULL;

    uint8_t *val_plane = out;
    uint8_t *alpha_plane = out + pixel_bytes;

    /* Determine threshold: which value means "foreground" */
    /* Use the first non-transparent pixel as foreground reference */
    uint8_t fg_gray = 0;
    int found_fg = 0;
    for (int i = 0; i < n && !found_fg; i++) {
        if (rgba[i * 4 + 3] > 127) {
            fg_gray = rgba[i * 4];
            found_fg = 1;
        }
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int i = y * w + x;
            int byte_idx = y * (int)row_bytes + x / 8;
            int bit_pos = 7 - (x % 8);

            if (rgba[i * 4 + 3] > 127) {
                alpha_plane[byte_idx] |= (uint8_t)(1 << bit_pos);
                if (rgba[i * 4] == fg_gray)
                    val_plane[byte_idx] |= (uint8_t)(1 << bit_pos);
            }
        }
    }
    return out;
}

/* Convert RGBA to GrayAlpha (2 bytes/pixel) */
static uint8_t *convert_gray_alpha(const uint8_t *rgba, int w, int h, size_t *out_size) {
    int n = w * h;
    *out_size = (size_t)(n * 2);
    uint8_t *out = (uint8_t *)malloc(*out_size);
    if (!out) return NULL;

    for (int i = 0; i < n; i++) {
        out[i * 2]     = rgba[i * 4];     /* gray */
        out[i * 2 + 1] = rgba[i * 4 + 3]; /* alpha */
    }
    return out;
}

/* Convert RGBA to palette indexed (1 byte/pixel) */
static uint8_t *convert_palette(const uint8_t *rgba, int w, int h,
                                 const uint32_t *palette, int palette_count,
                                 size_t *out_size) {
    int n = w * h;
    *out_size = (size_t)n;
    uint8_t *out = (uint8_t *)malloc(*out_size);
    if (!out) return NULL;

    for (int i = 0; i < n; i++) {
        int idx = i * 4;
        uint32_t color = ((uint32_t)rgba[idx] << 24) |
                         ((uint32_t)rgba[idx + 1] << 16) |
                         ((uint32_t)rgba[idx + 2] << 8) |
                         (uint32_t)rgba[idx + 3];
        /* Linear search is fine for <= 256 entries */
        for (int j = 0; j < palette_count; j++) {
            if (palette[j] == color) {
                out[i] = (uint8_t)j;
                break;
            }
        }
    }
    return out;
}

int zx0em_decode_png(const char *path, zx0em_raw_image_t *img) {
    memset(img, 0, sizeof(*img));

    /* Read file into memory */
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "zx0em: cannot open '%s'\n", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (file_size <= 0) {
        fclose(f);
        return -1;
    }

    uint8_t *file_data = (uint8_t *)malloc((size_t)file_size);
    if (!file_data) { fclose(f); return -1; }
    if (fread(file_data, 1, (size_t)file_size, f) != (size_t)file_size) {
        free(file_data);
        fclose(f);
        return -1;
    }
    fclose(f);

    /* Decode PNG to RGBA */
    int w, h, channels;
    uint8_t *rgba = stbi_load_from_memory(file_data, (int)file_size,
                                           &w, &h, &channels, 4);
    free(file_data);

    if (!rgba) {
        fprintf(stderr, "zx0em: failed to decode PNG '%s': %s\n",
                path, stbi_failure_reason());
        return -1;
    }

    img->width = (uint16_t)w;
    img->height = (uint16_t)h;

    /* Analyze and choose most compact format */

    /* Try 1-bit first */
    if (is_1bit(rgba, w, h)) {
        img->pixfmt = ZX0EM_PIXFMT_1BIT;
        img->pixels = convert_1bit(rgba, w, h, &img->pixels_size);
        stbi_image_free(rgba);
        return img->pixels ? 0 : -1;
    }

    /* Try grayscale+alpha */
    if (is_grayscale(rgba, w, h)) {
        img->pixfmt = ZX0EM_PIXFMT_GRAY_ALPHA;
        img->pixels = convert_gray_alpha(rgba, w, h, &img->pixels_size);
        stbi_image_free(rgba);
        return img->pixels ? 0 : -1;
    }

    /* Try palette (up to 256 colors) */
    uint32_t palette[256];
    int ncolors = count_unique_colors(rgba, w, h, palette, 256);
    if (ncolors <= 256) {
        img->pixfmt = ZX0EM_PIXFMT_PALETTE;
        img->palette_count = (uint8_t)ncolors;
        memcpy(img->palette_rgba, palette, (size_t)(ncolors * 4));
        /* Convert RGBA8888 to RGBA4444 for compact palette storage */
        for (int i = 0; i < ncolors; i++) {
            uint32_t c = palette[i];
            uint8_t r4 = (uint8_t)((c >> 28) & 0xF);
            uint8_t g4 = (uint8_t)((c >> 20) & 0xF);
            uint8_t b4 = (uint8_t)((c >> 12) & 0xF);
            uint8_t a4 = (uint8_t)((c >> 4) & 0xF);
            img->palette[i] = (uint16_t)((r4 << 12) | (g4 << 8) | (b4 << 4) | a4);
        }
        img->pixels = convert_palette(rgba, w, h, palette, ncolors,
                                       &img->pixels_size);
        stbi_image_free(rgba);
        return img->pixels ? 0 : -1;
    }

    /* Fallback: RGBA */
    img->pixfmt = ZX0EM_PIXFMT_RGBA;
    img->pixels_size = (size_t)(w * h * 4);
    img->pixels = (uint8_t *)malloc(img->pixels_size);
    if (!img->pixels) {
        stbi_image_free(rgba);
        return -1;
    }
    memcpy(img->pixels, rgba, img->pixels_size);
    stbi_image_free(rgba);
    return 0;
}

void zx0em_free_image(zx0em_raw_image_t *img) {
    if (img->pixels) {
        free(img->pixels);
        img->pixels = NULL;
    }
}
