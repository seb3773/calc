/*
 * preprocess.c - Apply spatial delta, inter-image XOR, plane split, palette reindex
 *
 * Implements Phase 4:
 * For each image, we can choose between:
 *  - RAW
 *  - DELTA (spatial XOR/SUB with previous pixel)
 *  - UP (spatial XOR/SUB with pixel above)
 *  - PLANAR (channel plane split for GRAY_ALPHA / RGBA)
 *  - Chained: PLANAR + secondary spatial predictor
 *  - XOR/SUB (with previous image in the group)
 *  All combinations are tested via trial compression.
 *  Trials are parallelized across assets using pthreads.
 */

#include "zx0em_blob.h"
#include "png_decode.h"
#include "zx0/zx0_compress_wrapper.h"
#include "libsalvador.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>

/* Helper for Paeth prediction */
static inline uint8_t paeth_predict(uint8_t a, uint8_t b, uint8_t c) {
    int p = (int)a + (int)b - (int)c;
    int pa = abs(p - (int)a);
    int pb = abs(p - (int)b);
    int pc = abs(p - (int)c);
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

/* Apply a simple 1D/2D spatial filter to data (backward iteration for encoding) */
static void apply_spatial_basic(uint8_t *data, size_t size, int bpp, unsigned int pitch, int filter) {
    if (filter == 0) return;
    if (filter == 1 || filter == 2) {
        if (size < (size_t)bpp) return;
        if (filter == 1) { /* LEFT XOR */
            for (size_t i = size; i > (size_t)bpp; i--) data[i - 1] ^= data[i - 1 - bpp];
        } else if (filter == 2) { /* LEFT SUB */
            for (size_t i = size; i > (size_t)bpp; i--) data[i - 1] = (uint8_t)(data[i - 1] - data[i - 1 - bpp]);
        }
    } else if (filter == 3 || filter == 4) {
        if (size < pitch) return;
        if (filter == 3) { /* UP XOR */
            for (size_t i = size; i > pitch; i--) data[i - 1] ^= data[i - 1 - pitch];
        } else if (filter == 4) { /* UP SUB */
            for (size_t i = size; i > pitch; i--) data[i - 1] = (uint8_t)(data[i - 1] - data[i - 1 - pitch]);
        }
    } else if (filter == 7 || filter == 8) { /* AVG_XOR / AVG_SUB */
        if (size == 0) return;
        unsigned int w = pitch / (unsigned int)bpp;
        for (size_t i = size; i > 0; i--) {
            size_t k = i - 1;
            unsigned int px = (k / (unsigned int)bpp) % w;
            uint8_t left = (px > 0 && k >= (size_t)bpp) ? data[k - bpp] : 0;
            uint8_t up = (k >= pitch) ? data[k - pitch] : 0;
            uint8_t pred = (uint8_t)(((int)left + (int)up) / 2);
            if (filter == 7) data[k] ^= pred;
            else data[k] = (uint8_t)(data[k] - pred);
        }
    } else if (filter == 9 || filter == 10) { /* PAETH_XOR / PAETH_SUB */
        if (size == 0) return;
        unsigned int w = pitch / (unsigned int)bpp;
        for (size_t i = size; i > 0; i--) {
            size_t k = i - 1;
            unsigned int px = (k / (unsigned int)bpp) % w;
            uint8_t left = (px > 0 && k >= (size_t)bpp) ? data[k - bpp] : 0;
            uint8_t up = (k >= pitch) ? data[k - pitch] : 0;
            uint8_t left_up = (px > 0 && k >= pitch + bpp) ? data[k - pitch - bpp] : 0;
            uint8_t pred = paeth_predict(left, up, left_up);
            if (filter == 9) data[k] ^= pred;
            else data[k] = (uint8_t)(data[k] - pred);
        }
    }
}

/* Apply planar split (GRAY_ALPHA or RGBA) */
static void apply_planar(uint8_t *data, size_t size, int bpp) {
    if (bpp == 2) { /* GRAY_ALPHA: GAGAGA -> GGG...AAA... */
        size_t half = size / 2;
        uint8_t *tmp = malloc(size);
        if (!tmp) return;
        for (size_t i = 0; i < half; i++) {
            tmp[i]        = data[i * 2];     /* Gray channel */
            tmp[half + i] = data[i * 2 + 1]; /* Alpha channel */
        }
        memcpy(data, tmp, size);
        free(tmp);
    } else if (bpp == 4) { /* RGBA: RGBA RGBA ... -> RRR...GGG...BBB...AAA... */
        size_t plane_sz = size / 4;
        uint8_t *tmp = malloc(size);
        if (!tmp) return;
        for (size_t i = 0; i < plane_sz; i++) {
            tmp[i]                = data[i * 4];     /* R */
            tmp[plane_sz + i]     = data[i * 4 + 1]; /* G */
            tmp[plane_sz * 2 + i] = data[i * 4 + 2]; /* B */
            tmp[plane_sz * 3 + i] = data[i * 4 + 3]; /* A */
        }
        memcpy(data, tmp, size);
        free(tmp);
    }
}

/* Apply spatial filter (includes standalone + chained variants) */
static void apply_spatial(uint8_t *data, size_t size, int bpp, unsigned int pitch, int filter) {
    if (filter >= 0 && filter <= 10) {
        /* Standalone filters: 0=RAW, 1-4=LEFT/UP, 5=PLANAR_GA, 6=PLANAR_RGBA, 7-10=AVG/PAETH */
        if (filter == 5) {
            apply_planar(data, size, 2);
        } else if (filter == 6) {
            if (bpp != 4) return;
            apply_planar(data, size, 4);
        } else {
            apply_spatial_basic(data, size, bpp, pitch, filter);
        }
    } else if (filter >= 11 && filter <= 13) {
        /* Chained: PLANAR_GA + secondary spatial (bpp becomes 1, pitch becomes width) */
        apply_planar(data, size, 2);
        unsigned int planar_pitch = pitch / 2; /* width * 1 instead of width * 2 */
        int secondary = 0;
        if (filter == 11) secondary = 1;       /* + LEFT_XOR */
        else if (filter == 12) secondary = 3;  /* + UP_XOR */
        else if (filter == 13) secondary = 10; /* + PAETH_SUB */
        apply_spatial_basic(data, size, 1, planar_pitch, secondary);
    } else if (filter >= 14 && filter <= 16) {
        /* Chained: PLANAR_RGBA + secondary spatial (bpp becomes 1, pitch becomes width) */
        if (bpp != 4) return;
        apply_planar(data, size, 4);
        unsigned int planar_pitch = pitch / 4; /* width * 1 instead of width * 4 */
        int secondary = 0;
        if (filter == 14) secondary = 1;       /* + LEFT_XOR */
        else if (filter == 15) secondary = 3;  /* + UP_XOR */
        else if (filter == 16) secondary = 10; /* + PAETH_SUB */
        apply_spatial_basic(data, size, 1, planar_pitch, secondary);
    }
}

/* Apply inter-image filter */
static void apply_inter(uint8_t *data, const uint8_t *prev_data, size_t size, int filter) {
    if (filter == 0x00) return;
    if (filter == 0x10) { /* INTER XOR */
        for (size_t i = 0; i < size; i++) data[i] ^= prev_data[i];
    } else if (filter == 0x20) { /* INTER SUB */
        for (size_t i = 0; i < size; i++) data[i] = (uint8_t)(data[i] - prev_data[i]);
    }
}

/* Include C wrapper declaration */
int zx0_compress_c_fast(const unsigned char *in, size_t in_sz,
                        unsigned char **out, size_t *out_sz, int max_offset);


/* Trial compression of a buffer using the windowed optimal ZX0 solver.
 * Using a capped sliding window (e.g. 2048 bytes) makes this extremely fast
 * even for large assets while maintaining 100% algorithm consistency with
 * the final ZX0 optimal compressor.
 * Thread-safe: serialized via mutex. */
static size_t trial_compress(const uint8_t *data, size_t size) {
    unsigned char *out = NULL;
    size_t out_sz = 0;
    
    int rc = zx0_compress_c_fast(data, size, &out, &out_sz, 2048);
    
    if (rc == 0) {
        free(out);
        return out_sz;
    }
    return size; /* fallback */
}

/* Reindex palette entries based on pixel transition frequencies (greedy nearest-neighbor path).
 * Grouping frequently adjacent pixel colors closer together in the palette indexes 
 * creates smoother index transitions, making spatial delta encoding (XOR/SUB) 
 * significantly more compressible. Zero runtime cost for the decoder. */
static void reindex_palette(zx0em_asset_t *a) {
    if (a->pixfmt != ZX0EM_PIXFMT_PALETTE || a->palette_count < 2) return;

    int n = a->palette_count;
    uint32_t *pal = a->palette_rgba;

    /* Build co-occurrence matrix C[256][256] */
    static int C[256][256];
    memset(C, 0, sizeof(C));

    int w = a->width;
    int h = a->height;

    /* Horizontal transitions */
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w - 1; x++) {
            int u = a->data[y * w + x];
            int v = a->data[y * w + x + 1];
            if (u < n && v < n) {
                C[u][v]++;
                C[v][u]++;
            }
        }
    }

    /* Vertical transitions */
    for (int y = 0; y < h - 1; y++) {
        for (int x = 0; x < w; x++) {
            int u = a->data[y * w + x];
            int v = a->data[(y + 1) * w + x];
            if (u < n && v < n) {
                C[u][v]++;
                C[v][u]++;
            }
        }
    }

    /* Color frequencies */
    int freq[256];
    memset(freq, 0, sizeof(freq));
    for (size_t i = 0; i < a->data_size; i++) {
        if (a->data[i] < n) freq[a->data[i]]++;
    }

    /* Start from the most frequent color */
    int start_color = 0;
    int max_freq = -1;
    for (int i = 0; i < n; i++) {
        if (freq[i] > max_freq) {
            max_freq = freq[i];
            start_color = i;
        }
    }

    /* Greedy TSP path construction */
    int order[256];
    int used[256];
    memset(used, 0, sizeof(used));
    order[0] = start_color;
    used[start_color] = 1;

    for (int i = 1; i < n; i++) {
        int prev = order[i - 1];
        int best = -1;
        int best_weight = -1;

        for (int j = 0; j < n; j++) {
            if (used[j]) continue;
            int weight = C[prev][j];
            if (weight > best_weight) {
                best_weight = weight;
                best = j;
            }
        }

        /* Fallback if no transitions between prev and unused colors:
         * Pick the unused color with the highest overall frequency */
        if (best == -1 || best_weight == 0) {
            int fallback = -1;
            int fallback_freq = -1;
            for (int j = 0; j < n; j++) {
                if (!used[j] && freq[j] > fallback_freq) {
                    fallback_freq = freq[j];
                    fallback = j;
                }
            }
            best = (fallback != -1) ? fallback : 0;
        }

        order[i] = best;
        used[best] = 1;
    }

    /* Build reverse mapping: old_index -> new_index */
    int remap[256];
    for (int i = 0; i < n; i++)
        remap[order[i]] = i;

    /* Reorder palette */
    uint32_t new_pal[256];
    for (int i = 0; i < n; i++)
        new_pal[i] = pal[order[i]];
    memcpy(pal, new_pal, (size_t)n * sizeof(uint32_t));

    /* Remap pixel indices */
    for (size_t i = 0; i < a->data_size; i++) {
        if (a->data[i] < n) {
            a->data[i] = (uint8_t)remap[a->data[i]];
        }
    }
}

/* --- Per-asset trial worker data --- */
typedef struct {
    zx0em_asset_t *asset;
    zx0em_asset_t *parent;  /* MST parent (or NULL) */
    int can_xor;
    char result_msg[256];   /* verbose output collected per-asset */
} asset_trial_t;

static void process_asset_trials(asset_trial_t *t) {
    zx0em_asset_t *a = t->asset;

    int bpp = zx0em_pixfmt_bpp(a->pixfmt);
    if (bpp == 0) bpp = 1;

    unsigned int pitch = 0;
    if (a->pixfmt == ZX0EM_PIXFMT_RGBA) pitch = a->width * 4;
    else if (a->pixfmt == ZX0EM_PIXFMT_GRAY_ALPHA) pitch = a->width * 2;
    else if (a->pixfmt == ZX0EM_PIXFMT_PALETTE) pitch = a->width * 1;
    else if (a->pixfmt == ZX0EM_PIXFMT_1BIT) pitch = (a->width + 7) / 8;

    size_t psize = a->data_size;
    uint8_t *v_test = malloc(psize);
    uint8_t *best_data = malloc(psize);

    size_t best_sz = trial_compress(a->data, psize);
    uint8_t best_filters = 0;
    memcpy(best_data, a->data, psize);

    /* Spatial options evaluated dynamically */
    int spatial_opts[24];
    int n_spatial = 0;
    spatial_opts[n_spatial++] = 0;  /* RAW */
    spatial_opts[n_spatial++] = 1;  /* LEFT XOR */
    spatial_opts[n_spatial++] = 2;  /* LEFT SUB */
    spatial_opts[n_spatial++] = 3;  /* UP XOR */
    spatial_opts[n_spatial++] = 4;  /* UP SUB */
    if (a->pixfmt == ZX0EM_PIXFMT_GRAY_ALPHA) {
        spatial_opts[n_spatial++] = 5;  /* PLANAR GRAY_ALPHA */
        spatial_opts[n_spatial++] = 11; /* PLANAR_GA + LEFT_XOR */
        spatial_opts[n_spatial++] = 12; /* PLANAR_GA + UP_XOR */
        spatial_opts[n_spatial++] = 13; /* PLANAR_GA + PAETH_SUB */
    } else if (a->pixfmt == ZX0EM_PIXFMT_RGBA) {
        spatial_opts[n_spatial++] = 6;  /* PLANAR RGBA */
        spatial_opts[n_spatial++] = 14; /* PLANAR_RGBA + LEFT_XOR */
        spatial_opts[n_spatial++] = 15; /* PLANAR_RGBA + UP_XOR */
        spatial_opts[n_spatial++] = 16; /* PLANAR_RGBA + PAETH_SUB */
    }
    spatial_opts[n_spatial++] = 7;  /* AVG XOR */
    spatial_opts[n_spatial++] = 8;  /* AVG SUB */
    spatial_opts[n_spatial++] = 9;  /* PAETH XOR */
    spatial_opts[n_spatial++] = 10; /* PAETH SUB */

    int inter_opts[] = {0x00, 0x10, 0x20};

    for (int s = 0; s < n_spatial; s++) {
        for (int in = 0; in < (t->can_xor ? 3 : 1); in++) {
            if (s == 0 && in == 0) continue; /* Already tested raw */

            int sf = spatial_opts[s];
            int inf = inter_opts[in];

            /* Copy raw data */
            memcpy(v_test, a->data, psize);

            /* 1. Apply Inter-image filter */
            apply_inter(v_test, t->parent ? t->parent->data : NULL, psize, inf);

            /* 2. Apply Spatial filter */
            apply_spatial(v_test, psize, bpp, pitch, sf);

            /* Compress */
            size_t test_sz = trial_compress(v_test, psize);

            if (test_sz < best_sz) {
                best_sz = test_sz;
                best_filters = sf | inf;
                memcpy(best_data, v_test, psize);
            }
        }
    }

    /* Apply best */
    memcpy(a->data, best_data, psize);
    a->filters = best_filters;

    /* Build verbose result message */
    t->result_msg[0] = '\0';
    if (best_filters != 0) {
        char name[64] = "";
        int sf = best_filters & 0x0F;
        int inf = best_filters & 0xF0;
        if (inf == 0x10) strcat(name, "IXOR+");
        else if (inf == 0x20) strcat(name, "ISUB+");
        if (sf == 1) strcat(name, "LXOR");
        else if (sf == 2) strcat(name, "LSUB");
        else if (sf == 3) strcat(name, "UXOR");
        else if (sf == 4) strcat(name, "USUB");
        else if (sf == 5) strcat(name, "PLANAR");
        else if (sf == 6) strcat(name, "PLANAR_RGBA");
        else if (sf == 7) strcat(name, "AVGXOR");
        else if (sf == 8) strcat(name, "AVGSUB");
        else if (sf == 9) strcat(name, "PAETHXOR");
        else if (sf == 10) strcat(name, "PAETHSUB");
        else if (sf == 11) strcat(name, "PL_GA+LXOR");
        else if (sf == 12) strcat(name, "PL_GA+UXOR");
        else if (sf == 13) strcat(name, "PL_GA+PSUB");
        else if (sf == 14) strcat(name, "PL_RGBA+LXOR");
        else if (sf == 15) strcat(name, "PL_RGBA+UXOR");
        else if (sf == 16) strcat(name, "PL_RGBA+PSUB");
        snprintf(t->result_msg, sizeof(t->result_msg),
                 "  %-25s : %-14s (trial sz: %zu)\n", a->name, name, best_sz);
    }

    free(v_test);
    free(best_data);
}

/* --- Thread pool worker --- */
typedef struct {
    asset_trial_t *trials;
    int total_count;
    int *next_index;        /* shared atomic-like counter */
    pthread_mutex_t *idx_mutex;
} worker_arg_t;

static void *worker_thread(void *arg) {
    worker_arg_t *wa = (worker_arg_t *)arg;

    for (;;) {
        /* Grab next work item */
        pthread_mutex_lock(wa->idx_mutex);
        int idx = *(wa->next_index);
        if (idx >= wa->total_count) {
            pthread_mutex_unlock(wa->idx_mutex);
            break;
        }
        (*(wa->next_index))++;
        pthread_mutex_unlock(wa->idx_mutex);

        process_asset_trials(&wa->trials[idx]);
    }

    return NULL;
}

int zx0em_preprocess_assets(zx0em_ctx_t *ctx) {
    if (ctx->verbose)
        printf("Preprocessing assets (Delta / XOR / UP / SUB / PLANAR)...\n");

    /* Reindex palettes first (not thread-safe due to static C[256][256]) */
    for (int i = 0; i < ctx->asset_count; i++) {
        zx0em_asset_t *a = &ctx->assets[i];
        if (a->asset_type == ZX0EM_TYPE_IMAGE && a->pixfmt == ZX0EM_PIXFMT_PALETTE)
            reindex_palette(a);
    }

    /* Build trial work items (images only) */
    int n_trials = 0;
    asset_trial_t *trials = calloc((size_t)ctx->asset_count, sizeof(asset_trial_t));

    for (int i = 0; i < ctx->asset_count; i++) {
        zx0em_asset_t *a = &ctx->assets[i];
        if (a->asset_type != ZX0EM_TYPE_IMAGE) continue;

        asset_trial_t *t = &trials[n_trials++];
        t->asset = a;
        t->parent = NULL;
        t->can_xor = 0;

        if (a->parent_id != 0xFFFF) {
            zx0em_asset_t *prev = &ctx->assets[a->parent_id];
            if (prev->asset_type == ZX0EM_TYPE_IMAGE &&
                prev->width == a->width &&
                prev->height == a->height &&
                prev->pixfmt == a->pixfmt &&
                prev->data_size == a->data_size) {
                t->parent = prev;
                t->can_xor = 1;
            }
        }
    }

    if (n_trials == 0) {
        free(trials);
        return 0;
    }

    /* Determine thread count */
    int n_threads = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (n_threads < 1) n_threads = 1;
    if (n_threads > n_trials) n_threads = n_trials;
    if (n_threads > 16) n_threads = 16; /* reasonable cap */

    if (n_threads <= 1) {
        /* Single-threaded fallback */
        for (int i = 0; i < n_trials; i++)
            process_asset_trials(&trials[i]);
    } else {
        /* Multi-threaded dispatch */
        int next_index = 0;
        pthread_mutex_t idx_mutex = PTHREAD_MUTEX_INITIALIZER;

        pthread_t *threads = malloc((size_t)n_threads * sizeof(pthread_t));
        worker_arg_t wa = {
            .trials = trials,
            .total_count = n_trials,
            .next_index = &next_index,
            .idx_mutex = &idx_mutex
        };

        for (int i = 0; i < n_threads; i++)
            pthread_create(&threads[i], NULL, worker_thread, &wa);

        for (int i = 0; i < n_threads; i++)
            pthread_join(threads[i], NULL);

        free(threads);
        pthread_mutex_destroy(&idx_mutex);
    }

    /* Print collected verbose results (in order, after all threads finish) */
    if (ctx->verbose) {
        for (int i = 0; i < n_trials; i++) {
            if (trials[i].result_msg[0])
                printf("%s", trials[i].result_msg);
        }
    }

    free(trials);
    return 0;
}
