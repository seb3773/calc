/*
 * text_bpe.c - Byte Pair Encoding for Text Assets
 */

#include "zx0em_blob.h"
#include "zx0/zx0_compress_wrapper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKENS 256

/* Count frequencies of pairs */
static void count_pairs(const uint8_t *data, size_t size, int pair_counts[256][256]) {
    if (size < 2) return;
    for (size_t i = 0; i < size - 1; i++) {
        pair_counts[data[i]][data[i+1]]++;
    }
}

/* Replace a pair with a token */
static void replace_pair(uint8_t *data, size_t *size, uint8_t c1, uint8_t c2, uint8_t token) {
    size_t in = 0, out = 0;
    while (in < *size) {
        if (in < *size - 1 && data[in] == c1 && data[in+1] == c2) {
            data[out++] = token;
            in += 2;
        } else {
            data[out++] = data[in++];
        }
    }
    *size = out;
}

int zx0em_apply_bpe(zx0em_asset_t *a) {
    /* Find unused bytes */
    int used[256] = {0};
    for (size_t i = 0; i < a->data_size; i++) {
        used[a->data[i]] = 1;
    }
    
    uint8_t free_tokens[256];
    int num_free = 0;
    for (int i = 0; i < 256; i++) {
        if (!used[i]) {
            free_tokens[num_free++] = (uint8_t)i;
        }
    }
    
    if (num_free == 0 || a->data_size < 10) return 0;
    
    uint8_t dict_token[256];
    uint8_t dict_c1[256];
    uint8_t dict_c2[256];
    int dict_count = 0;
    
    uint8_t *temp_data = malloc(a->data_size);
    if (!temp_data) return 0;
    memcpy(temp_data, a->data, a->data_size);
    size_t temp_size = a->data_size;
    
    while (dict_count < num_free) {
        int pair_counts[256][256] = {{0}};
        count_pairs(temp_data, temp_size, pair_counts);
        
        int max_count = 0;
        int best_c1 = -1, best_c2 = -1;
        
        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < 256; j++) {
                if (pair_counts[i][j] > max_count) {
                    max_count = pair_counts[i][j];
                    best_c1 = i;
                    best_c2 = j;
                }
            }
        }
        
        /* Stop if the pair occurs rarely (replacing saves 1 byte per occurrence,
         * but dictionary costs 3 bytes + overhead, so need at least 4 occurrences) */
        if (max_count < 4) break;
        
        uint8_t token = free_tokens[dict_count];
        replace_pair(temp_data, &temp_size, (uint8_t)best_c1, (uint8_t)best_c2, token);
        
        dict_token[dict_count] = token;
        dict_c1[dict_count] = (uint8_t)best_c1;
        dict_c2[dict_count] = (uint8_t)best_c2;
        dict_count++;
    }
    
    if (dict_count == 0) {
        free(temp_data);
        return 0;
    }
    
    /* Build final block:
     * 1 byte: dict_count
     * dict_count * 3 bytes: token, c1, c2
     * temp_size bytes: encoded data
     */
    size_t final_size = 1 + dict_count * 3 + temp_size;
    
    /* Trial compression */
    unsigned char *out_raw = NULL;
    size_t zx0_raw = 0;
    zx0_compress_c(a->data, a->data_size, &out_raw, &zx0_raw);
    
    uint8_t *final_data = malloc(final_size);
    final_data[0] = (uint8_t)dict_count;
    size_t pos = 1;
    for (int i = 0; i < dict_count; i++) {
        final_data[pos++] = dict_token[i];
        final_data[pos++] = dict_c1[i];
        final_data[pos++] = dict_c2[i];
    }
    memcpy(&final_data[pos], temp_data, temp_size);
    
    unsigned char *out_bpe = NULL;
    size_t zx0_bpe = 0;
    zx0_compress_c(final_data, final_size, &out_bpe, &zx0_bpe);
    
    int used_bpe = 0;
    if (zx0_bpe < zx0_raw) {
        /* BPE improved compression! */
        free(a->data);
        a->data = final_data;
        a->data_size = final_size;
        a->asset_type = ZX0EM_TYPE_TEXT_BPE;
        used_bpe = 1;
    } else {
        free(final_data);
    }
    
    free(temp_data);
    free(out_raw);
    free(out_bpe);
    
    return used_bpe;
}
