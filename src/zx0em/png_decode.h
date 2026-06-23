#ifndef ZX0EM_PNG_DECODE_H
#define ZX0EM_PNG_DECODE_H

#include <stdint.h>
#include <stddef.h>

/* Pixel formats for decoded images */
#define ZX0EM_PIXFMT_GRAY_ALPHA  0  /* 2 bytes/pixel: gray, alpha */
#define ZX0EM_PIXFMT_PALETTE     1  /* 1 byte/pixel index + palette[] */
#define ZX0EM_PIXFMT_RGBA        2  /* 4 bytes/pixel: R, G, B, A */
#define ZX0EM_PIXFMT_1BIT        3  /* 1 bit/pixel packed MSB-first, + alpha bitmask */
#define ZX0EM_PIXFMT_NONE        7  /* non-image asset */

/* Decoded raw image */
typedef struct {
    uint8_t *pixels;        /* raw pixel data (malloc'd) */
    size_t   pixels_size;   /* size of pixels[] in bytes */
    uint16_t width;
    uint16_t height;
    uint8_t  pixfmt;        /* ZX0EM_PIXFMT_* */
    uint8_t  palette_count; /* number of palette entries (0 if no palette) */
    uint16_t palette[256];  /* RGBA4444 palette (if pixfmt == PALETTE) */
                            /* Using RGBA4444: 4 bits per channel, 2 bytes/entry */
                            /* For max fidelity we store full RGBA8888 separately */
    uint32_t palette_rgba[256]; /* full RGBA8888 palette entries */
} zx0em_raw_image_t;

/*
 * Decode a PNG file to the most compact raw pixel format.
 * Returns 0 on success, -1 on failure.
 * Caller must free img->pixels when done.
 */
int zx0em_decode_png(const char *path, zx0em_raw_image_t *img);

/*
 * Free a decoded image's pixel data.
 */
void zx0em_free_image(zx0em_raw_image_t *img);

/*
 * Return human-readable name for a pixel format.
 */
const char *zx0em_pixfmt_name(int pixfmt);

/*
 * Bytes per pixel for a given format (0 for 1BIT since it's sub-byte).
 */
int zx0em_pixfmt_bpp(int pixfmt);

#endif /* ZX0EM_PNG_DECODE_H */
