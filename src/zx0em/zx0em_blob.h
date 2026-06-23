#ifndef ZX0EM_BLOB_H
#define ZX0EM_BLOB_H

#include <stdint.h>
#include <stddef.h>

/* Asset types */
#define ZX0EM_TYPE_IMAGE        0  /* raw pixels, maybe with filters */
#define ZX0EM_TYPE_TEXT_RAW     4  /* raw text */
#define ZX0EM_TYPE_TEXT_BPE     5  /* BPE encoded text */
#define ZX0EM_TYPE_FONT         6  /* TTF font (possibly stripped) */
#define ZX0EM_TYPE_BINARY       7  /* raw binary data */
#define ZX0EM_TYPE_FONT_STRIPPED 8 /* TTF font (stripped) */

/* Filter Flags (for IMAGE types) */
#define ZX0EM_FILTER_SPATIAL_NONE      0x00
#define ZX0EM_FILTER_SPATIAL_LEFT_XOR  0x01
#define ZX0EM_FILTER_SPATIAL_LEFT_SUB  0x02
#define ZX0EM_FILTER_SPATIAL_UP_XOR    0x03
#define ZX0EM_FILTER_SPATIAL_UP_SUB    0x04
#define ZX0EM_FILTER_SPATIAL_PLANAR    0x05  /* GRAY_ALPHA plane split: GA→GG..AA.. */

#define ZX0EM_FILTER_INTER_NONE        0x00
#define ZX0EM_FILTER_INTER_XOR         0x10
#define ZX0EM_FILTER_INTER_SUB         0x20

/* Maximum assets in a single blob */
#define ZX0EM_MAX_ASSETS 512

/* An asset entry tracked during blob building */
typedef struct {
    char     name[128];       /* original filename */
    char     macro[128];      /* C macro name */
    uint8_t *data;            /* raw data (malloc'd) */
    size_t   data_size;       /* size in bytes */
    uint32_t blob_offset;     /* offset in final blob */
    uint16_t width;           /* image width (0 if non-image) */
    uint16_t height;          /* image height */
    uint8_t  asset_type;      /* ZX0EM_TYPE_* */
    uint8_t  pixfmt;          /* ZX0EM_PIXFMT_* */
    uint8_t  palette_count;   /* palette entries count */
    uint8_t  filters;         /* ZX0EM_FILTER_* combination */
    uint32_t palette_rgba[256]; /* full RGBA palette */
    int      group_id;        /* for similarity sorting */
    int      order_index;     /* final position after sorting */
    uint16_t parent_id;       /* MST parent asset index (0xFFFF if none) */
} zx0em_asset_t;

/* Blob builder context */
typedef struct {
    zx0em_asset_t assets[ZX0EM_MAX_ASSETS];
    int           asset_count;
    uint8_t      *blob;          /* concatenated raw data */
    size_t        blob_size;     /* total uncompressed size */
    uint8_t      *compressed;    /* ZX0-compressed blob */
    size_t        compressed_size;
    int           verbose;
    int           use_salvador;  /* if 1, use external Salvador instead of internal ZX0 */
    int           naive_mode;    /* if 1, skip optimizations */
} zx0em_ctx_t;

/* Initialize context */
void zx0em_init_ctx(zx0em_ctx_t *ctx);

/* Scan a directory for assets (PNG, TTF, TXT, etc.) */
int zx0em_scan_assets(zx0em_ctx_t *ctx, const char *dir);

/* Sort assets using MST (Minimum Spanning Tree) based on XOR distances */
int zx0em_sort_assets_mst(zx0em_ctx_t *ctx);

/* Apply preprocessing: spatial delta, inter-image XOR, text BPE */
int zx0em_preprocess_assets(zx0em_ctx_t *ctx);

/* Apply BPE to a text asset */
int zx0em_apply_bpe(zx0em_asset_t *a);

/* Strip unnecessary tables from a TTF font */
int zx0em_strip_ttf(zx0em_asset_t *a);

/* Build the blob (concatenate all asset data) */
int zx0em_build_blob(zx0em_ctx_t *ctx);

/* Compress the blob with ZX0 */
int zx0em_compress_blob(zx0em_ctx_t *ctx);

/* Emit output files (.h, .c) */
int zx0em_emit_header(zx0em_ctx_t *ctx, const char *out_dir);

/* Free all resources */
void zx0em_free_ctx(zx0em_ctx_t *ctx);

#endif /* ZX0EM_BLOB_H */
