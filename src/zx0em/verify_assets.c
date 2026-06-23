#include "zx0em_runtime.h"
#include "png_decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper to convert palette image to RGBA8888 from raw bytes */
static void convert_palette_bytes_to_rgba(int w, int h, const uint8_t *src_pixels, const uint8_t *palette_bytes, uint8_t palette_count, uint8_t *dst_rgba) {
    for (int i = 0; i < w * h; i++) {
        uint8_t idx = src_pixels[i];
        if (idx >= palette_count) idx = 0;
        dst_rgba[i * 4]     = palette_bytes[idx * 4];     /* R */
        dst_rgba[i * 4 + 1] = palette_bytes[idx * 4 + 1]; /* G */
        dst_rgba[i * 4 + 2] = palette_bytes[idx * 4 + 2]; /* B */
        dst_rgba[i * 4 + 3] = palette_bytes[idx * 4 + 3]; /* A */
    }
}

/* Helper to convert palette image to RGBA8888 from uint32_t RGBA8888 array */
static void convert_palette_rgba_to_rgba(int w, int h, const uint8_t *src_pixels, const uint32_t *palette_rgba, uint8_t palette_count, uint8_t *dst_rgba) {
    for (int i = 0; i < w * h; i++) {
        uint8_t idx = src_pixels[i];
        if (idx >= palette_count) idx = 0;
        uint32_t color = palette_rgba[idx];
        dst_rgba[i * 4]     = (uint8_t)(color >> 24);
        dst_rgba[i * 4 + 1] = (uint8_t)(color >> 16);
        dst_rgba[i * 4 + 2] = (uint8_t)(color >> 8);
        dst_rgba[i * 4 + 3] = (uint8_t)color;
    }
}

/* Helper to convert 16-bit palette RGBA4444 to RGBA8888 (just in case) */
static void convert_palette16_to_rgba(int w, int h, const uint8_t *src_pixels, const uint16_t *palette, uint8_t palette_count, uint8_t *dst_rgba) {
    for (int i = 0; i < w * h; i++) {
        uint8_t idx = src_pixels[i];
        if (idx >= palette_count) idx = 0;
        uint16_t c = palette[idx];
        uint8_t r4 = (uint8_t)((c >> 12) & 0xF);
        uint8_t g4 = (uint8_t)((c >> 8) & 0xF);
        uint8_t b4 = (uint8_t)((c >> 4) & 0xF);
        uint8_t a4 = (uint8_t)(c & 0xF);
        dst_rgba[i * 4]     = (uint8_t)((r4 << 4) | r4);
        dst_rgba[i * 4 + 1] = (uint8_t)((g4 << 4) | g4);
        dst_rgba[i * 4 + 2] = (uint8_t)((b4 << 4) | b4);
        dst_rgba[i * 4 + 3] = (uint8_t)((a4 << 4) | a4);
    }
}

static const char *g_base_dir = NULL;

static int find_file(const char *name, char *out_path, size_t out_path_len) {
    if (g_base_dir) {
        // Try direct first
        snprintf(out_path, out_path_len, "%s/%s", g_base_dir, name);
        FILE *f = fopen(out_path, "rb");
        if (f) {
            fclose(f);
            return 1;
        }
        // Try subdirectories
        const char *subdirs[] = { "icons", "fonts", "translations" };
        for (int i = 0; i < 3; i++) {
            snprintf(out_path, out_path_len, "%s/%s/%s", g_base_dir, subdirs[i], name);
            f = fopen(out_path, "rb");
            if (f) {
                fclose(f);
                return 1;
            }
        }
        return 0;
    }

    const char *dirs[] = {
        "mixed_assets2",
        "mixed_assets2/icons",
        "mixed_assets2/fonts",
        "mixed_assets2/translations",
        "mixed_assets1"
    };
    for (int i = 0; i < 5; i++) {
        snprintf(out_path, out_path_len, "%s/%s", dirs[i], name);
        FILE *f = fopen(out_path, "rb");
        if (f) {
            fclose(f);
            return 1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        g_base_dir = argv[1];
    }
    printf("Initializing zx0em runtime...\n");
    if (!zx0em_init()) {
        fprintf(stderr, "ERROR: Runtime initialization failed!\n");
        return 1;
    }
    printf("Runtime initialized successfully with %d assets.\n", ZX0EM_ENTRY_COUNT);

    int failed = 0;
    for (int id = 0; id < ZX0EM_ENTRY_COUNT; id++) {
        const zx0em_entry_t *e = &zx0em_entries[id];
        char filepath[256];
        if (!find_file(e->name, filepath, sizeof(filepath))) {
            fprintf(stderr, "\nFAILED: Could not locate original asset '%s'\n", e->name);
            failed++;
            continue;
        }

        printf("[%2d] Verifying '%s' (type: %d)... ", id, e->name, e->type);

        if (e->type == ZX0EM_TYPE_IMAGE) {
            zx0em_raw_image_t orig_img;
            if (zx0em_decode_png(filepath, &orig_img) != 0) {
                fprintf(stderr, "FAILED (could not decode original PNG %s)\n", filepath);
                failed++;
                continue;
            }

            int w, h, pixfmt;
            const unsigned char *runtime_data = zx0em_get_image(id, &w, &h, &pixfmt);
            if (!runtime_data) {
                fprintf(stderr, "FAILED (zx0em_get_image returned NULL)\n");
                zx0em_free_image(&orig_img);
                failed++;
                continue;
            }

            if (w != orig_img.width || h != orig_img.height) {
                fprintf(stderr, "FAILED (dimension mismatch: original %dx%d, runtime %dx%d)\n",
                        orig_img.width, orig_img.height, w, h);
                zx0em_free_image(&orig_img);
                failed++;
                continue;
            }

            if (pixfmt != orig_img.pixfmt) {
                fprintf(stderr, "FAILED (pixfmt mismatch: original %d, runtime %d)\n",
                        orig_img.pixfmt, pixfmt);
                zx0em_free_image(&orig_img);
                failed++;
                continue;
            }

            if (pixfmt == ZX0EM_PIXFMT_PALETTE) {
                /* For palette images, we must compare the resolved RGBA8888 pixels 
                 * because color reindexing permutes both palette entries and indices. */
                uint8_t runtime_pal_count = runtime_data[0];
                const uint8_t *runtime_pal_rgba = runtime_data + 1;
                const uint8_t *runtime_pixels = runtime_data + 1 + (size_t)runtime_pal_count * 4;

                size_t num_pixels = (size_t)w * h;
                uint8_t *orig_rgba = malloc(num_pixels * 4);
                uint8_t *runtime_rgba = malloc(num_pixels * 4);

                if (!orig_rgba || !runtime_rgba) {
                    fprintf(stderr, "FAILED (out of memory during RGBA conversion)\n");
                    free(orig_rgba);
                    free(runtime_rgba);
                    zx0em_free_image(&orig_img);
                    failed++;
                    continue;
                }

                convert_palette_rgba_to_rgba(w, h, orig_img.pixels, orig_img.palette_rgba, orig_img.palette_count, orig_rgba);
                convert_palette_bytes_to_rgba(w, h, runtime_pixels, runtime_pal_rgba, runtime_pal_count, runtime_rgba);

                int pixel_mismatch = 0;
                for (size_t i = 0; i < num_pixels; i++) {
                    if (orig_rgba[i * 4]     != runtime_rgba[i * 4] ||
                        orig_rgba[i * 4 + 1] != runtime_rgba[i * 4 + 1] ||
                        orig_rgba[i * 4 + 2] != runtime_rgba[i * 4 + 2] ||
                        orig_rgba[i * 4 + 3] != runtime_rgba[i * 4 + 3]) {
                        pixel_mismatch = 1;
                        break;
                    }
                }

                free(orig_rgba);
                free(runtime_rgba);

                if (pixel_mismatch) {
                    fprintf(stderr, "FAILED (palette color/pixel resolved RGBA8888 mismatch!)\n");
                    failed++;
                } else {
                    printf("OK (resolved RGBA8888 matched perfectly)\n");
                }

            } else {
                /* For RGBA, GRAY_ALPHA, 1BIT, the raw pixels must match byte-for-byte */
                size_t runtime_pixels_size = e->size;
                if (runtime_pixels_size != orig_img.pixels_size) {
                    fprintf(stderr, "FAILED (pixel data size mismatch: original %zu, runtime %zu)\n",
                            orig_img.pixels_size, runtime_pixels_size);
                    zx0em_free_image(&orig_img);
                    failed++;
                    continue;
                }

                if (memcmp(runtime_data, orig_img.pixels, runtime_pixels_size) != 0) {
                    fprintf(stderr, "FAILED (pixel data mismatch!)\n");
                    failed++;
                } else {
                    printf("OK (pixels byte-matched perfectly)\n");
                }
            }

            zx0em_free_image(&orig_img);

        } else if (e->type == ZX0EM_TYPE_TEXT_RAW || e->type == ZX0EM_TYPE_TEXT_BPE) {
            /* Compare text file */
            FILE *f = fopen(filepath, "rb");
            if (!f) {
                fprintf(stderr, "FAILED (could not open original text %s)\n", filepath);
                failed++;
                continue;
            }
            fseek(f, 0, SEEK_END);
            long file_size = ftell(f);
            fseek(f, 0, SEEK_SET);

            char *orig_text = malloc((size_t)file_size + 1);
            size_t bytes_read = fread(orig_text, 1, (size_t)file_size, f);
            orig_text[bytes_read] = '\0';
            fclose(f);

            /* Decompress text using runtime helper */
            int runtime_max_len = 1024 * 1024;
            char *runtime_text = malloc((size_t)runtime_max_len);
            int runtime_len = zx0em_decode_bpe(id, runtime_text, runtime_max_len);

            if (runtime_len < 0) {
                fprintf(stderr, "FAILED (zx0em_decode_bpe failed)\n");
                failed++;
            } else if (strcmp(orig_text, runtime_text) != 0) {
                fprintf(stderr, "FAILED (text content mismatch!)\n");
                int diff_idx = 0;
                while (orig_text[diff_idx] && runtime_text[diff_idx] && orig_text[diff_idx] == runtime_text[diff_idx]) {
                    diff_idx++;
                }
                fprintf(stderr, "First diff at char %d: orig='%c'(0x%02X) vs runtime='%c'(0x%02X)\n",
                        diff_idx, orig_text[diff_idx], orig_text[diff_idx], runtime_text[diff_idx], runtime_text[diff_idx]);
                fprintf(stderr, "Original len: %ld (read: %zu), Runtime len: %d\n", file_size, bytes_read, runtime_len);
                failed++;
            } else {
                printf("OK (text content matched perfectly, decompressed size: %d)\n", runtime_len);
            }

            free(orig_text);
            free(runtime_text);

        } else {
            /* Fonts / Binary assets: Since TTF stripping is lossy, we don't compare stripped fonts 
             * directly against original TTFs. For unstripped binary/other assets we check sizes and data. */
            if (e->type == ZX0EM_TYPE_FONT) {
                printf("OK (font, skipped content matching because TTF stripping is lossy)\n");
            } else {
                FILE *f = fopen(filepath, "rb");
                if (!f) {
                    fprintf(stderr, "FAILED (could not open original file %s)\n", filepath);
                    failed++;
                    continue;
                }
                fseek(f, 0, SEEK_END);
                long file_size = ftell(f);
                fseek(f, 0, SEEK_SET);

                unsigned char *orig_data = malloc((size_t)file_size);
                fread(orig_data, 1, (size_t)file_size, f);
                fclose(f);

                unsigned int runtime_size = 0;
                const unsigned char *runtime_data = zx0em_get(id, &runtime_size);

                if (runtime_size != (unsigned int)file_size) {
                    fprintf(stderr, "FAILED (size mismatch: original %ld, runtime %u)\n", file_size, runtime_size);
                    failed++;
                } else if (memcmp(runtime_data, orig_data, runtime_size) != 0) {
                    fprintf(stderr, "FAILED (binary data mismatch!)\n");
                    failed++;
                } else {
                    printf("OK (binary data matched perfectly)\n");
                }
                free(orig_data);
            }
        }
    }

    if (failed > 0) {
        printf("\nVerification FAILED: %d errors encountered.\n", failed);
        return 1;
    }

    printf("\nVerification SUCCESS: All assets decompressed and validated perfectly!\n");
    return 0;
}
