/*
 * blob_builder.c - Scan assets directory, decode PNGs, build raw blob
 *
 * Phase 2: Naive concat of raw pixels + non-image assets
 * Later phases add: sorting, delta, XOR, BPE, TTF stripping
 */

#define _GNU_SOURCE /* for strcasecmp */
#include "zx0em_blob.h"
#include "png_decode.h"
#include "zx0/zx0_compress_wrapper.h"
#include "libsalvador.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static int str_ends_with(const char *s, const char *suffix) {
    size_t slen = strlen(s);
    size_t sufflen = strlen(suffix);
    if (sufflen > slen) return 0;
    return strcasecmp(s + slen - sufflen, suffix) == 0;
}

/* Convert filename to C macro name */
static void macroize(const char *name, char *out, size_t out_sz) {
    size_t j = 0;
    for (size_t i = 0; name[i] && j < out_sz - 1; i++) {
        char c = name[i];
        if (c == '.' || c == '-' || c == ' ') c = '_';
        else c = (char)tolower((unsigned char)c);
        if (!isalnum((unsigned char)c) && c != '_') continue;
        out[j++] = c;
    }
    out[j] = '\0';
    /* Remove trailing underscores */
    while (j > 0 && out[j - 1] == '_') out[--j] = '\0';
}

/* Compare function for sorting assets by name within type groups */
static int asset_cmp(const void *a, const void *b) {
    const zx0em_asset_t *aa = (const zx0em_asset_t *)a;
    const zx0em_asset_t *bb = (const zx0em_asset_t *)b;
    /* Group by asset type first */
    if (aa->asset_type != bb->asset_type)
        return (int)aa->asset_type - (int)bb->asset_type;
    /* Then by name */
    return strcmp(aa->name, bb->name);
}

/* ------------------------------------------------------------------ */
/* API                                                                 */
/* ------------------------------------------------------------------ */

void zx0em_init_ctx(zx0em_ctx_t *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->use_salvador = 1; /* Default to fast Salvador compression */
}

static int load_raw_file(const char *path, uint8_t **data, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return -1; }
    *data = (uint8_t *)malloc((size_t)sz);
    if (!*data) { fclose(f); return -1; }
    if (fread(*data, 1, (size_t)sz, f) != (size_t)sz) {
        free(*data); *data = NULL;
        fclose(f);
        return -1;
    }
    fclose(f);
    *size = (size_t)sz;
    return 0;
}

/* Recursively scan directory for assets */
static int scan_dir_recursive(zx0em_ctx_t *ctx, const char *dir) {
    DIR *d = opendir(dir);
    if (!d) {
        fprintf(stderr, "zx0em: cannot open directory '%s'\n", dir);
        return -1;
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue; /* skip hidden files */

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            /* Recurse into subdirectories */
            if (scan_dir_recursive(ctx, path) != 0) {
                closedir(d);
                return -1;
            }
            continue;
        }

        if (!S_ISREG(st.st_mode)) continue;
        if (ctx->asset_count >= ZX0EM_MAX_ASSETS) {
            fprintf(stderr, "zx0em: too many assets (max %d)\n", ZX0EM_MAX_ASSETS);
            closedir(d);
            return -1;
        }

        zx0em_asset_t *asset = &ctx->assets[ctx->asset_count];
        memset(asset, 0, sizeof(*asset));
        snprintf(asset->name, sizeof(asset->name), "%s", ent->d_name);
        macroize(ent->d_name, asset->macro, sizeof(asset->macro));

        if (str_ends_with(ent->d_name, ".png")) {
            /* Decode PNG to raw pixels */
            zx0em_raw_image_t img;
            if (zx0em_decode_png(path, &img) != 0) {
                fprintf(stderr, "zx0em: skipping '%s' (decode failed)\n", path);
                continue;
            }
            asset->asset_type = ZX0EM_TYPE_IMAGE;
            asset->pixfmt = img.pixfmt;
            asset->width = img.width;
            asset->height = img.height;
            asset->data = img.pixels; /* take ownership */
            asset->data_size = img.pixels_size;
            asset->palette_count = img.palette_count;
            if (img.palette_count > 0) {
                memcpy(asset->palette_rgba, img.palette_rgba,
                       (size_t)(img.palette_count) * sizeof(uint32_t));
            }

            if (ctx->verbose) {
                printf("  [IMG] %-25s %4dx%-4d %-10s %zu bytes (was %ld PNG)\n",
                       ent->d_name, img.width, img.height,
                       zx0em_pixfmt_name(img.pixfmt),
                       img.pixels_size, (long)st.st_size);
            }
        } else if (str_ends_with(ent->d_name, ".ttf")) {
            asset->asset_type = ZX0EM_TYPE_FONT;
            asset->pixfmt = ZX0EM_PIXFMT_NONE;
            if (load_raw_file(path, &asset->data, &asset->data_size) != 0) {
                fprintf(stderr, "zx0em: skipping '%s' (read failed)\n", path);
                continue;
            }
            if (ctx->verbose)
                printf("  [FNT] %-25s %zu bytes\n", ent->d_name, asset->data_size);
        } else if (str_ends_with(ent->d_name, ".txt") ||
                   str_ends_with(ent->d_name, ".conf")) {
            asset->asset_type = ZX0EM_TYPE_TEXT_RAW;
            asset->pixfmt = ZX0EM_PIXFMT_NONE;
            if (load_raw_file(path, &asset->data, &asset->data_size) != 0) {
                fprintf(stderr, "zx0em: skipping '%s' (read failed)\n", path);
                continue;
            }
            if (ctx->verbose)
                printf("  [TXT] %-25s %zu bytes\n", ent->d_name, asset->data_size);
        } else {
            /* Generic binary */
            asset->asset_type = ZX0EM_TYPE_BINARY;
            asset->pixfmt = ZX0EM_PIXFMT_NONE;
            if (load_raw_file(path, &asset->data, &asset->data_size) != 0) {
                fprintf(stderr, "zx0em: skipping '%s' (read failed)\n", path);
                continue;
            }
            if (ctx->verbose)
                printf("  [BIN] %-25s %zu bytes\n", ent->d_name, asset->data_size);
        }

        ctx->asset_count++;
    }

    closedir(d);
    return 0;
}

int zx0em_scan_assets(zx0em_ctx_t *ctx, const char *dir) {
    if (ctx->verbose)
        printf("Scanning assets in '%s'...\n", dir);

    if (scan_dir_recursive(ctx, dir) != 0)
        return -1;

    /* Sort: images first, then fonts, then text, then binary */
    qsort(ctx->assets, (size_t)ctx->asset_count, sizeof(zx0em_asset_t), asset_cmp);

    if (ctx->verbose)
        printf("Found %d assets.\n\n", ctx->asset_count);

    return 0;
}

int zx0em_build_blob(zx0em_ctx_t *ctx) {
    /* Calculate total size */
    size_t total = 0;
    for (int i = 0; i < ctx->asset_count; i++) {
        zx0em_asset_t *a = &ctx->assets[i];
        /* For palette images, include palette data before pixel data */
        size_t entry_size = a->data_size;
        if (a->pixfmt == ZX0EM_PIXFMT_PALETTE && a->palette_count > 0) {
            /* Store palette as: count(1 byte) + RGBA8888 entries */
            entry_size += 1 + (size_t)(a->palette_count) * 4;
        }
        a->blob_offset = (uint32_t)total;
        total += entry_size;
    }

    /* Allocate and fill blob */
    ctx->blob = (uint8_t *)malloc(total);
    if (!ctx->blob) return -1;
    ctx->blob_size = total;

    size_t pos = 0;
    for (int i = 0; i < ctx->asset_count; i++) {
        zx0em_asset_t *a = &ctx->assets[i];
        a->blob_offset = (uint32_t)pos;

        /* For palette images, write palette first */
        if (a->pixfmt == ZX0EM_PIXFMT_PALETTE && a->palette_count > 0) {
            ctx->blob[pos++] = a->palette_count;
            for (int j = 0; j < a->palette_count; j++) {
                uint32_t c = a->palette_rgba[j];
                ctx->blob[pos++] = (uint8_t)(c >> 24);
                ctx->blob[pos++] = (uint8_t)(c >> 16);
                ctx->blob[pos++] = (uint8_t)(c >> 8);
                ctx->blob[pos++] = (uint8_t)(c);
            }
        }

        memcpy(ctx->blob + pos, a->data, a->data_size);
        pos += a->data_size;
    }

    return 0;
}

int zx0em_compress_blob(zx0em_ctx_t *ctx) {
    if (!ctx->blob || ctx->blob_size == 0) return -1;

    unsigned char *out = NULL;
    size_t out_sz = 0;

    if (ctx->use_salvador) {
        if (ctx->verbose)
            printf("Compressing %zu bytes with Salvador...\n", ctx->blob_size);
        
        size_t max_out = salvador_get_max_compressed_size(ctx->blob_size);
        out = malloc(max_out);
        if (!out) return -1;

        out_sz = salvador_compress(ctx->blob, out, ctx->blob_size, max_out, FLG_IS_INVERTED, 0, 0, NULL, NULL);
        if (out_sz == (size_t)-1) {
            fprintf(stderr, "zx0em: Salvador compression failed\n");
            free(out);
            return -1;
        }
        
        unsigned char *out_shrink = realloc(out, out_sz);
        if (out_shrink) out = out_shrink;
    } else {
        if (ctx->verbose)
            printf("Compressing %zu bytes with ZX0...\n", ctx->blob_size);

        int ret = zx0_compress_c(ctx->blob, ctx->blob_size, &out, &out_sz);
        if (ret != 0) {
            fprintf(stderr, "zx0em: ZX0 compression failed\n");
            return -1;
        }
    }

    ctx->compressed = out;
    ctx->compressed_size = out_sz;

    if (ctx->verbose) {
        double ratio = (double)out_sz / (double)ctx->blob_size * 100.0;
        printf("Compressed: %zu -> %zu bytes (%.1f%%)\n",
               ctx->blob_size, out_sz, ratio);
    }

    return 0;
}

void zx0em_free_ctx(zx0em_ctx_t *ctx) {
    for (int i = 0; i < ctx->asset_count; i++) {
        free(ctx->assets[i].data);
        ctx->assets[i].data = NULL;
    }
    free(ctx->blob);
    ctx->blob = NULL;
    free(ctx->compressed);
    ctx->compressed = NULL;
}
