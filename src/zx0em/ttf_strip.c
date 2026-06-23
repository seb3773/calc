/*
 * ttf_strip.c - Minimal TTF parser and table stripper
 *
 * Keeps only essential tables to reduce font file size:
 * glyf, head, hhea, hmtx, loca, maxp, name, post, cmap, OS/2, cvt , fpgm, prep
 * (Added hinting tables cvt, fpgm, prep just in case)
 */

#include "zx0em_blob.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define READ32(p) (((uint32_t)(p)[0] << 24) | ((uint32_t)(p)[1] << 16) | ((uint32_t)(p)[2] << 8) | (uint32_t)(p)[3])
#define READ16(p) (((uint16_t)(p)[0] << 8) | (uint16_t)(p)[1])
#define WRITE32(p, v) do { (p)[0] = (uint8_t)((v) >> 24); (p)[1] = (uint8_t)((v) >> 16); (p)[2] = (uint8_t)((v) >> 8); (p)[3] = (uint8_t)(v); } while(0)
#define WRITE16(p, v) do { (p)[0] = (uint8_t)((v) >> 8); (p)[1] = (uint8_t)(v); } while(0)

static uint32_t calc_checksum(const uint8_t *data, uint32_t length) {
    uint32_t sum = 0;
    uint32_t nlongs = (length + 3) / 4;
    for (uint32_t i = 0; i < nlongs; i++) {
        uint32_t v = 0;
        for (int j=0; j<4; j++) {
            if (i*4+j < length) {
                v |= (uint32_t)data[i*4+j] << (24 - j*8);
            }
        }
        sum += v;
    }
    return sum;
}

static int is_kept_table(uint32_t tag) {
    const char *tags[] = {
        "glyf", "head", "hhea", "hmtx", "loca", "maxp", "name",
        "post", "cmap", "OS/2", "cvt ", "fpgm", "prep"
    };
    char tag_str[5];
    tag_str[0] = (char)(tag >> 24);
    tag_str[1] = (char)(tag >> 16);
    tag_str[2] = (char)(tag >> 8);
    tag_str[3] = (char)tag;
    tag_str[4] = 0;
    
    for (size_t i = 0; i < sizeof(tags)/sizeof(tags[0]); i++) {
        if (strcmp(tag_str, tags[i]) == 0) return 1;
    }
    return 0;
}

typedef struct {
    uint32_t tag;
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;
    uint8_t *data;
} ttf_table_t;

int zx0em_strip_ttf(zx0em_asset_t *a) {
    if (a->data_size < 12) return 0;
    
    uint32_t version = READ32(a->data);
    if (version != 0x00010000 && version != 0x4F54544F) return 0; /* Not standard TTF/OTF */
    
    uint16_t num_tables = READ16(a->data + 4);
    if (a->data_size < 12u + num_tables * 16u) return 0;
    
    ttf_table_t *tables = malloc(num_tables * sizeof(ttf_table_t));
    if (!tables) return 0;
    
    int kept_count = 0;
    size_t new_size = 12;
    
    for (int i = 0; i < num_tables; i++) {
        const uint8_t *entry = a->data + 12 + i * 16;
        uint32_t tag = READ32(entry);
        uint32_t length = READ32(entry + 12);
        uint32_t offset = READ32(entry + 8);
        
        if (offset + length > a->data_size) continue; /* Malformed */
        
        if (is_kept_table(tag)) {
            tables[kept_count].tag = tag;
            tables[kept_count].length = length;
            tables[kept_count].data = malloc(length);
            memcpy(tables[kept_count].data, a->data + offset, length);
            /* Padding to 4 bytes boundary */
            new_size += 16; /* Table directory entry */
            new_size += (length + 3) & ~3; /* Table data size padded */
            kept_count++;
        }
    }
    
    if (kept_count == num_tables) {
        /* Nothing stripped */
        for (int i = 0; i < kept_count; i++) free(tables[i].data);
        free(tables);
        return 0;
    }
    
    uint8_t *new_ttf = calloc(1, new_size);
    if (!new_ttf) {
        for (int i = 0; i < kept_count; i++) free(tables[i].data);
        free(tables);
        return 0;
    }
    
    /* Header */
    WRITE32(new_ttf, version);
    WRITE16(new_ttf + 4, kept_count);
    
    /* Calculate searchRange, entrySelector, rangeShift */
    int max_pow2 = 1;
    int log2 = 0;
    while ((max_pow2 * 2) <= kept_count) { max_pow2 *= 2; log2++; }
    uint16_t searchRange = max_pow2 * 16;
    uint16_t entrySelector = log2;
    uint16_t rangeShift = kept_count * 16 - searchRange;
    
    WRITE16(new_ttf + 6, searchRange);
    WRITE16(new_ttf + 8, entrySelector);
    WRITE16(new_ttf + 10, rangeShift);
    
    uint32_t current_offset = 12 + kept_count * 16;
    uint8_t *dir = new_ttf + 12;
    
    /* Variables to recalculate global checksum */
    int head_idx = -1;
    
    for (int i = 0; i < kept_count; i++) {
        uint32_t padded_len = (tables[i].length + 3) & ~3;
        tables[i].checksum = calc_checksum(tables[i].data, tables[i].length);
        
        WRITE32(dir, tables[i].tag);
        WRITE32(dir + 4, tables[i].checksum);
        WRITE32(dir + 8, current_offset);
        WRITE32(dir + 12, tables[i].length);
        
        if (tables[i].tag == 0x68656164) { /* 'head' */
            head_idx = i;
            /* Clear checksum adjustment before global checksum */
            if (tables[i].length >= 12) {
                WRITE32(tables[i].data + 8, 0);
            }
        }
        
        memcpy(new_ttf + current_offset, tables[i].data, tables[i].length);
        /* padding is already 0 because calloc */
        
        current_offset += padded_len;
        dir += 16;
        
        free(tables[i].data);
    }
    free(tables);
    
    if (head_idx != -1) {
        /* Recalculate whole file checksum */
        uint32_t file_sum = calc_checksum(new_ttf, new_size);
        uint32_t checkSumAdjustment = 0xB1B02BA2 - file_sum;
        /* Find head table offset */
        for (int i = 0; i < kept_count; i++) {
            uint32_t tag = READ32(new_ttf + 12 + i * 16);
            if (tag == 0x68656164) {
                uint32_t offset = READ32(new_ttf + 12 + i * 16 + 8);
                WRITE32(new_ttf + offset + 8, checkSumAdjustment);
                break;
            }
        }
    }
    
    free(a->data);
    a->data = new_ttf;
    a->data_size = new_size;
    a->asset_type = ZX0EM_TYPE_FONT;
    
    return 1;
}
