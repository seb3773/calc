/*
 * zx0em_pack.c - Main CLI tool for zx0em blob packer
 *
 * Usage: zx0em_pack <assets_dir> <output_dir> [options]
 *
 * Options:
 *   --verbose        Show detailed analysis
 *   --stats          Show compression statistics
 *   --no-delta       Disable delta encoding (future)
 *   --no-xor         Disable XOR inter-images (future)
 *   --no-bpe         Disable BPE for texts (future)
 *   --no-strip-ttf   Disable TTF stripping (future)
 *   --brute-local N  Brute-force window size (future)
 */

#include "zx0em_blob.h"
#include "png_decode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static void print_usage(const char *prog) {
    fprintf(stderr,
        "zx0em_pack - Optimized asset blob packer for ZX0 by seb3773\n\n"
        "Usage: %s <assets_dir> <output_dir> [options]\n\n"
        "Options:\n"
        "  --verbose        Show detailed analysis\n"
        "  --stats          Show compression statistics table\n"
        "  --optimal        Use optimal Saukas ZX0 compressor (slower, slightly better ratio)\n"
        "  --version        Show version and author information\n"
        "  --help           Show this help\n\n"
        "Output files:\n"
        "  zx0em_blob_data.h   - Compressed blob constants\n"
        "  zx0em_blob_data.c   - Compressed blob array\n"
        "  zx0em_index.h       - Asset index (enum + entry table)\n"
        "  zx0em_runtime.h     - Standalone runtime API\n",
        prog);
}

static void print_stats(zx0em_ctx_t *ctx) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════╗\n");
    printf("║                     zx0em Compression Report                       ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════╣\n");
    printf("║                                                                    ║\n");

    /* Count by type */
    int n_img = 0, n_fnt = 0, n_txt = 0, n_bin = 0;
    size_t s_img = 0, s_fnt = 0, s_txt = 0, s_bin = 0;
    for (int i = 0; i < ctx->asset_count; i++) {
        zx0em_asset_t *a = &ctx->assets[i];
        size_t sz = a->data_size;
        if (a->pixfmt == ZX0EM_PIXFMT_PALETTE && a->palette_count > 0)
            sz += 1 + (size_t)(a->palette_count) * 4;
        switch (a->asset_type) {
            case ZX0EM_TYPE_IMAGE:
                n_img++; s_img += sz; break;
            case ZX0EM_TYPE_FONT:
            case ZX0EM_TYPE_FONT_STRIPPED:
                n_fnt++; s_fnt += sz; break;
            case ZX0EM_TYPE_TEXT_RAW:
            case ZX0EM_TYPE_TEXT_BPE:
                n_txt++; s_txt += sz; break;
            default:
                n_bin++; s_bin += sz; break;
        }
    }

    printf("║  Assets: %-4d total                                                ║\n",
           ctx->asset_count);
    printf("║    Images: %-4d (%7zu bytes raw)                                 ║\n",
           n_img, s_img);
    printf("║    Fonts:  %-4d (%7zu bytes)                                      ║\n",
           n_fnt, s_fnt);
    printf("║    Texts:  %-4d (%7zu bytes)                                      ║\n",
           n_txt, s_txt);
    if (n_bin > 0)
        printf("║    Binary: %-4d (%7zu bytes)                                      ║\n",
               n_bin, s_bin);
    printf("║                                                                    ║\n");
    printf("║  Uncompressed blob:  %8zu bytes                                 ║\n",
           ctx->blob_size);
    printf("║  ZX0 compressed:     %8zu bytes                                 ║\n",
           ctx->compressed_size);

    double ratio = (double)ctx->compressed_size / (double)ctx->blob_size * 100.0;
    printf("║  Ratio:              %7.1f%%                                      ║\n",
           ratio);
    printf("║                                                                    ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════╝\n");

    /* Detailed per-asset table */
    printf("\nDetailed asset list:\n");
    printf("┌────┬──────────────────────────┬──────────┬───────┬──────────┬──────────┐\n");
    printf("│ ## │ Name                     │ Type     │ Fmt   │ Dims     │ Raw Size │\n");
    printf("├────┼──────────────────────────┼──────────┼───────┼──────────┼──────────┤\n");

    for (int i = 0; i < ctx->asset_count; i++) {
        zx0em_asset_t *a = &ctx->assets[i];
        const char *tname = "BIN";
        switch (a->asset_type) {
            case ZX0EM_TYPE_IMAGE:
            case ZX0EM_TYPE_FONT:        tname = "FONT"; break;
            case ZX0EM_TYPE_TEXT_RAW:
            case ZX0EM_TYPE_TEXT_BPE:    tname = "TEXT"; break;
        }

        char dims[16] = "       ";
        if (a->width > 0)
            snprintf(dims, sizeof(dims), "%3dx%-3d", a->width, a->height);

        size_t sz = a->data_size;
        if (a->pixfmt == ZX0EM_PIXFMT_PALETTE && a->palette_count > 0)
            sz += 1 + (size_t)(a->palette_count) * 4;

        printf("│ %2d │ %-24.24s │ %-8s │ %-5s │ %-8s │ %7zu  │\n",
               i, a->name, tname,
               a->width > 0 ? zx0em_pixfmt_name(a->pixfmt) : "---",
               dims, sz);
    }
    printf("└────┴──────────────────────────┴──────────┴───────┴──────────┴──────────┘\n");
}

int main(int argc, char **argv) {
    const char *assets_dir = NULL;
    const char *output_dir = NULL;
    int verbose = 0;
    int stats = 0;
    zx0em_ctx_t ctx;
    zx0em_init_ctx(&ctx);

    /* Parse args */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0) { verbose = 1; }
        else if (strcmp(argv[i], "--stats") == 0) { stats = 1; }
        else if (strcmp(argv[i], "--salvador") == 0) { ctx.use_salvador = 1; }
        else if (strcmp(argv[i], "--optimal") == 0) { ctx.use_salvador = 0; }
        else if (strcmp(argv[i], "--naive") == 0) { ctx.naive_mode = 1; }
        else if (strcmp(argv[i], "--version") == 0) {
            printf("zx0em_pack v5.0.0 by seb3773\n");
            return 0;
        }
        else if (strcmp(argv[i], "--help") == 0) { print_usage(argv[0]); return 0; }
        else if (!assets_dir) { assets_dir = argv[i]; }
        else if (!output_dir) { output_dir = argv[i]; }
    }

    if (!assets_dir || !output_dir) {
        print_usage(argv[0]);
        return 1;
    }

    /* Create output directory */
    mkdir(output_dir, 0755);

    /* Initialize context */
    ctx.verbose = verbose;

    /* Phase 1: Scan & decode */
    printf("zx0em_pack: scanning '%s'...\n", assets_dir);
    if (zx0em_scan_assets(&ctx, assets_dir) != 0) {
        fprintf(stderr, "zx0em: scan failed\n");
        return 1;
    }

    if (ctx.asset_count == 0) {
        fprintf(stderr, "zx0em: no assets found in '%s'\n", assets_dir);
        return 1;
    }

    if (!ctx.naive_mode) {
        /* Sort using MST before building blob */
        printf("Sorting assets (MST)...\n");
        zx0em_sort_assets_mst(&ctx);

        /* Phase 4: Preprocess (Delta / XOR) and BPE */
        zx0em_preprocess_assets(&ctx);
        for (int i = 0; i < ctx.asset_count; i++) {
            if (ctx.assets[i].asset_type == ZX0EM_TYPE_TEXT_RAW) {
                zx0em_apply_bpe(&ctx.assets[i]);
            } else if (ctx.assets[i].asset_type == ZX0EM_TYPE_FONT) {
                zx0em_strip_ttf(&ctx.assets[i]);
            }
        }
    }

    /* Phase 2: Build blob */
    printf("Building blob (%d assets)...\n", ctx.asset_count);
    if (zx0em_build_blob(&ctx) != 0) {
        fprintf(stderr, "zx0em: blob build failed\n");
        zx0em_free_ctx(&ctx);
        return 1;
    }

    /* Compress with ZX0 */
    clock_t t0 = clock();
    printf("Compressing with ZX0...\n");
    if (zx0em_compress_blob(&ctx) != 0) {
        zx0em_free_ctx(&ctx);
        return 1;
    }
    clock_t t1 = clock();
    double compress_time = (double)(t1 - t0) / CLOCKS_PER_SEC;

    /* Emit output files */
    if (zx0em_emit_header(&ctx, output_dir) != 0) {
        zx0em_free_ctx(&ctx);
        return 1;
    }

    /* Report */
    double ratio = (double)ctx.compressed_size / (double)ctx.blob_size * 100.0;
    printf("\nDone! %d assets, %zu → %zu bytes (%.1f%%), %.1fs\n",
           ctx.asset_count, ctx.blob_size, ctx.compressed_size,
           ratio, compress_time);
    printf("Output: %s/\n", output_dir);

    if (stats)
        print_stats(&ctx);

    zx0em_free_ctx(&ctx);
    return 0;
}
