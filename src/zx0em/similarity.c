/*
 * similarity.c - MST-based asset sorting to maximize XOR/Delta redundancy
 *
 * Groups assets by type, width, height, and pixel format.
 * Within each group, constructs a complete graph where edge weights are
 * the XOR distances between images. Computes a Minimum Spanning Tree (MST)
 * using Prim's algorithm, and then performs a DFS traversal to sequence
 * the images. This maximizes LZ77 matches for ZX0.
 */

#include "zx0em_blob.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Compute the Hamming distance (popcount of XOR) between two byte arrays */
static unsigned int xor_distance(const uint8_t *a, const uint8_t *b, size_t size) {
    unsigned int dist = 0;
    for (size_t i = 0; i < size; i++) {
        uint8_t val = a[i] ^ b[i];
#ifdef __GNUC__
        dist += __builtin_popcount(val);
#else
        while (val) {
            dist += val & 1;
            val >>= 1;
        }
#endif
    }
    return dist;
}

/* Compare function for initial grouping */
static int group_cmp(const void *a, const void *b) {
    const zx0em_asset_t *aa = (const zx0em_asset_t *)a;
    const zx0em_asset_t *bb = (const zx0em_asset_t *)b;
    
    if (aa->asset_type != bb->asset_type) return (int)aa->asset_type - (int)bb->asset_type;
    if (aa->width != bb->width) return (int)aa->width - (int)bb->width;
    if (aa->height != bb->height) return (int)aa->height - (int)bb->height;
    if (aa->pixfmt != bb->pixfmt) return (int)aa->pixfmt - (int)bb->pixfmt;
    
    /* Fallback to name to ensure stability */
    return strcmp(aa->name, bb->name);
}

#define MAX_GROUP_SIZE 256

/* Prim's algorithm for MST and DFS traversal for linear ordering */
static void sort_group_mst(zx0em_asset_t *group, int count) {
    if (count <= 1) return;
    if (count > MAX_GROUP_SIZE) {
        fprintf(stderr, "zx0em: Warning: group size %d exceeds MST max %d. Skipping MST.\n", count, MAX_GROUP_SIZE);
        return;
    }

    unsigned int dist[MAX_GROUP_SIZE][MAX_GROUP_SIZE];
    for (int i = 0; i < count; i++) {
        dist[i][i] = 0;
        for (int j = i + 1; j < count; j++) {
            size_t sz = group[i].data_size < group[j].data_size ? group[i].data_size : group[j].data_size;
            unsigned int d = xor_distance(group[i].data, group[j].data, sz);
            dist[i][j] = d;
            dist[j][i] = d;
        }
    }

    int in_mst[MAX_GROUP_SIZE] = {0};
    int parent[MAX_GROUP_SIZE];
    unsigned int min_edge[MAX_GROUP_SIZE];

    for (int i = 0; i < count; i++) {
        parent[i] = -1;
        min_edge[i] = ~0U;
    }

    /* Start from node 0 */
    min_edge[0] = 0;

    for (int i = 0; i < count; i++) {
        int u = -1;
        unsigned int min_val = ~0U;
        for (int j = 0; j < count; j++) {
            if (!in_mst[j] && min_edge[j] < min_val) {
                min_val = min_edge[j];
                u = j;
            }
        }
        
        if (u == -1) break; /* Disconnected graph, shouldn't happen with complete graph */
        
        in_mst[u] = 1;
        
        for (int v = 0; v < count; v++) {
            if (!in_mst[v] && dist[u][v] < min_edge[v]) {
                parent[v] = u;
                min_edge[v] = dist[u][v];
            }
        }
    }

    /* Build adjacency list for DFS */
    int head[MAX_GROUP_SIZE];
    int next[MAX_GROUP_SIZE];
    int to[MAX_GROUP_SIZE];
    int edge_cnt = 0;
    
    for (int i = 0; i < count; i++) head[i] = -1;

    for (int i = 1; i < count; i++) {
        int u = parent[i];
        int v = i;
        if (u != -1) {
            to[edge_cnt] = v; next[edge_cnt] = head[u]; head[u] = edge_cnt++;
            to[edge_cnt] = u; next[edge_cnt] = head[v]; head[v] = edge_cnt++;
        }
    }

    /* DFS to find linear order */
    int order[MAX_GROUP_SIZE];
    int order_idx = 0;
    int visited[MAX_GROUP_SIZE] = {0};

    /* Find a good root (e.g., node with min total distance to others) */
    int root = 0;
    unsigned int min_total_dist = ~0U;
    for (int i = 0; i < count; i++) {
        unsigned int total = 0;
        for (int j = 0; j < count; j++) total += dist[i][j];
        if (total < min_total_dist) {
            min_total_dist = total;
            root = i;
        }
    }

    /* Simulate DFS with a stack */
    int stack[MAX_GROUP_SIZE];
    int top = 0;
    stack[top++] = root;
    visited[root] = 1;

    int dfs_parent[MAX_GROUP_SIZE];
    for (int i = 0; i < count; i++) dfs_parent[i] = -1;

    while (top > 0) {
        int u = stack[--top];
        order[order_idx++] = u;

        /* Push children */
        for (int e = head[u]; e != -1; e = next[e]) {
            int v = to[e];
            if (!visited[v]) {
                visited[v] = 1;
                dfs_parent[v] = u;
                stack[top++] = v;
            }
        }
    }

    /* Reorder group array and store relative parent_id */
    zx0em_asset_t temp[MAX_GROUP_SIZE];
    for (int i = 0; i < count; i++) {
        temp[i] = group[order[i]];
        
        int v = order[i];
        int u = dfs_parent[v];
        if (u == -1) {
            temp[i].parent_id = 0xFFFF;
        } else {
            int p = -1;
            for (int k = 0; k < count; k++) {
                if (order[k] == u) {
                    p = k;
                    break;
                }
            }
            temp[i].parent_id = (uint16_t)p;
        }
    }
    for (int i = 0; i < count; i++) {
        group[i] = temp[i];
    }
}

int zx0em_sort_assets_mst(zx0em_ctx_t *ctx) {
    if (ctx->asset_count <= 0) return 0;

    /* Initialize all parent_ids to 0xFFFF first */
    for (int i = 0; i < ctx->asset_count; i++) {
        ctx->assets[i].parent_id = 0xFFFF;
    }

    if (ctx->asset_count <= 1) return 0;

    /* Step 1: Sort by group_cmp to group similar assets together */
    qsort(ctx->assets, (size_t)ctx->asset_count, sizeof(zx0em_asset_t), group_cmp);

    /* Step 2: Identify groups and apply MST sorting */
    int group_start = 0;
    while (group_start < ctx->asset_count) {
        int group_end = group_start + 1;
        while (group_end < ctx->asset_count && group_cmp(&ctx->assets[group_start], &ctx->assets[group_end]) == 0) {
            group_end++;
        }
        
        int count = group_end - group_start;
        /* Only sort if it's raw images */
        if (count > 1 && ctx->assets[group_start].asset_type == ZX0EM_TYPE_IMAGE) {
            if (ctx->verbose) {
               printf("  MST sorting group: type=%d, %dx%d, pixfmt=%d, count=%d\n", 
                    ctx->assets[group_start].asset_type,
                    ctx->assets[group_start].width,
                    ctx->assets[group_start].height,
                    ctx->assets[group_start].pixfmt,
                    count);
            }
            sort_group_mst(&ctx->assets[group_start], count);

            /* Resolve absolute parent_ids by adding group_start offset */
            for (int k = 0; k < count; k++) {
                zx0em_asset_t *a = &ctx->assets[group_start + k];
                if (a->parent_id != 0xFFFF) {
                    a->parent_id += group_start;
                }
            }
        }

        group_start = group_end;
    }

    return 0;
}
