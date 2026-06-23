#!/usr/bin/env python3
import os
import re
import struct
import sys
import zlib

def _macroize(name: str) -> str:
    base = name
    if base.lower().endswith('.png'):
        base = base[:-4] + '_png'
    elif base.lower().endswith('.ttf'):
        base = base[:-4] + '_ttf'
    elif base.lower().endswith('.txt'):
        base = base[:-4] + '_txt'
    s = base.lower()
    s = re.sub(r'[^A-Za-z0-9]+', '_', s)
    s = re.sub(r'_+', '_', s)
    s = s.strip('_')
    return s

import subprocess
import tempfile

def strip_ttf(data: bytes) -> bytes:
    if len(data) < 12:
        return data
    
    version, num_tables, search_range, entry_selector, range_shift = struct.unpack('>IHHHH', data[0:12])
    if version != 0x00010000 and version != 0x4f54544f:
        return data
    
    kept_tables = []
    essential_tags = {
        b'glyf', b'head', b'hhea', b'hmtx', b'loca', b'maxp', b'name',
        b'post', b'cmap', b'OS/2', b'cvt ', b'fpgm', b'prep'
    }
    
    for i in range(num_tables):
        entry_offset = 12 + i * 16
        if entry_offset + 16 > len(data):
            break
        tag, checksum, offset, length = struct.unpack('>4sIII', data[entry_offset:entry_offset+16])
        if offset + length > len(data):
            continue
        if tag in essential_tags:
            table_data = data[offset:offset+length]
            kept_tables.append((tag, table_data))
            
    if len(kept_tables) == num_tables:
        return data
    
    kept_count = len(kept_tables)
    kept_tables.sort(key=lambda x: x[0])
    
    max_pow2 = 1
    log2 = 0
    while (max_pow2 * 2) <= kept_count:
        max_pow2 *= 2
        log2 += 1
    search_range = max_pow2 * 16
    entry_selector = log2
    range_shift = kept_count * 16 - search_range
    
    current_offset = 12 + kept_count * 16
    header = struct.pack('>IHHHH', version, kept_count, search_range, entry_selector, range_shift)
    
    dir_entries = []
    table_blocks = []
    head_table_offset = -1
    
    for tag, table_data in kept_tables:
        length = len(table_data)
        padded_length = (length + 3) & ~3
        
        if tag == b'head':
            if len(table_data) >= 12:
                table_data = bytearray(table_data)
                table_data[8:12] = b'\x00\x00\x00\x00'
                table_data = bytes(table_data)
                head_table_offset = current_offset
        
        checksum = 0
        padded_data = table_data + b'\x00' * (padded_length - length)
        for idx in range(0, padded_length, 4):
            word, = struct.unpack('>I', padded_data[idx:idx+4])
            checksum = (checksum + word) & 0xffffffff
            
        dir_entries.append(struct.pack('>4sIII', tag, checksum, current_offset, length))
        table_blocks.append(padded_data)
        current_offset += padded_length

    new_ttf = bytearray(header)
    for entry in dir_entries:
        new_ttf.extend(entry)
    for block in table_blocks:
        new_ttf.extend(block)
        
    if head_table_offset != -1:
        padded_new_ttf = bytes(new_ttf)
        total_len = len(padded_new_ttf)
        padded_len = (total_len + 3) & ~3
        padded_new_ttf += b'\x00' * (padded_len - total_len)
        
        file_sum = 0
        for idx in range(0, padded_len, 4):
            word, = struct.unpack('>I', padded_new_ttf[idx:idx+4])
            file_sum = (file_sum + word) & 0xffffffff
        
        checksum_adjustment = (0xB1B02BA2 - file_sum) & 0xffffffff
        struct.pack_into('>I', new_ttf, head_table_offset + 8, checksum_adjustment)
        
    return bytes(new_ttf)

def main() -> int:
    if len(sys.argv) != 5:
        sys.stderr.write('usage: build_assets.py <assets_dir> <out_dir> <bin_compress_path> <lz4|zx0>\n')
        return 2

    assets_dir = sys.argv[1]
    out_dir = sys.argv[2]
    bin_compress = sys.argv[3]
    mode = sys.argv[4]

    os.makedirs(out_dir, exist_ok=True)

    files = []
    for fn in os.listdir(assets_dir):
        if fn.lower().endswith('.png') or fn.lower().endswith('.ttf') or fn.lower().endswith('.txt'):
            files.append(fn)
    files.sort()

    raw = bytearray()
    entries = []
    off = 0
    for fn in files:
        path = os.path.join(assets_dir, fn)
        with open(path, 'rb') as f:
            b = f.read()
        if fn.lower().endswith('.ttf'):
            b = strip_ttf(b)
        sz = len(b)
        entries.append((fn, _macroize(fn), off, sz))
        raw += b
        off += sz

    # Use bin_compress or zlib
    if mode == 'zlib':
        comp = zlib.compress(raw, 9)
    else:
        raw_fd, raw_path = tempfile.mkstemp()
        comp_fd, comp_path = tempfile.mkstemp()
        try:
            with os.fdopen(raw_fd, 'wb') as f:
                f.write(raw)
            os.close(comp_fd)
            
            print(f"Running: {bin_compress} {mode} {raw_path} {comp_path}")
            res = subprocess.run([bin_compress, mode, raw_path, comp_path], capture_output=True)
            if res.returncode != 0:
                sys.stderr.write(f"bin_compress failed: {res.stderr.decode('utf-8')}\n")
                return 1
                
            with open(comp_path, 'rb') as f:
                comp = f.read()
        finally:
            try:
                os.remove(raw_path)
            except OSError:
                pass
            try:
                os.remove(comp_path)
            except OSError:
                pass

    out_h = os.path.join(out_dir, 'embedded_images_data.h')
    out_cpp = os.path.join(out_dir, 'embedded_images_data.cpp')
    out_icons_h = os.path.join(out_dir, 'embedded_icons.h')
    out_fonts_h = os.path.join(out_dir, 'embedded_fonts.h')
    out_txt_h = os.path.join(out_dir, 'embedded_txt.h')
    out_compat_cpp = os.path.join(out_dir, 'embedded_assets_compat.cpp')

    # 1. Write embedded_images_data.h
    with open(out_h, 'w', encoding='utf-8') as out:
        out.write('// Auto-generated file. Do not edit.\n')
        out.write('#pragma once\n\n')
        out.write('#define TQT_EMBIMG_UNCOMPRESSED_SIZE %dU\n' % (len(raw),))
        out.write('#define TQT_EMBIMG_COMPRESSED_SIZE %dU\n\n' % (len(comp),))
        out.write('extern const unsigned int tqt_embimg_uncompressed_size;\n')
        out.write('extern const unsigned int tqt_embimg_compressed_size;\n')
        out.write('extern const unsigned char tqt_embimg_z[TQT_EMBIMG_COMPRESSED_SIZE];\n\n')
        out.write('enum TQtEmbeddedImageId {\n')
        for i, (fn, macro, off0, sz0) in enumerate(entries):
            out.write('    TQT_EMBIMG_%s = %d,\n' % (macro, i))
        out.write('    TQT_EMBIMG__COUNT = %d\n' % (len(entries),))
        out.write('};\n\n')

        for fn, macro, off0, sz0 in entries:
            out.write('#define TQT_EMBIMG_%s_OFFSET %d\n' % (macro, off0))
            out.write('#define TQT_EMBIMG_%s_SIZE %d\n\n' % (macro, sz0))

        out.write('\nstatic inline const char* tqt_embimg_name_by_id_(int id) {\n')
        out.write('    switch (id) {\n')
        for fn, macro, off0, sz0 in entries:
            out.write('    case TQT_EMBIMG_%s: return "%s";\n' % (macro, fn))
        out.write('    default: return 0;\n')
        out.write('    }\n')
        out.write('}\n')

        out.write('\nstatic inline unsigned int tqt_embimg_offset_by_id_(int id) {\n')
        out.write('    switch (id) {\n')
        for fn, macro, off0, sz0 in entries:
            out.write('    case TQT_EMBIMG_%s: return (unsigned int)TQT_EMBIMG_%s_OFFSET;\n' % (macro, macro))
        out.write('    default: return 0U;\n')
        out.write('    }\n')
        out.write('}\n')

        out.write('\nstatic inline unsigned int tqt_embimg_size_by_id_(int id) {\n')
        out.write('    switch (id) {\n')
        for fn, macro, off0, sz0 in entries:
            out.write('    case TQT_EMBIMG_%s: return (unsigned int)TQT_EMBIMG_%s_SIZE;\n' % (macro, macro))
        out.write('    default: return 0U;\n')
        out.write('    }\n')
        out.write('}\n')

    # 2. Write embedded_images_data.cpp
    with open(out_cpp, 'w', encoding='utf-8') as out:
        out.write('// Auto-generated file. Do not edit.\n')
        out.write('#include "embedded_images_data.h"\n\n')
        out.write('const unsigned int tqt_embimg_uncompressed_size = TQT_EMBIMG_UNCOMPRESSED_SIZE;\n')
        out.write('const unsigned int tqt_embimg_compressed_size = TQT_EMBIMG_COMPRESSED_SIZE;\n\n')
        out.write('const unsigned char tqt_embimg_z[TQT_EMBIMG_COMPRESSED_SIZE] = {\n')
        for i, b in enumerate(comp):
            if (i % 12) == 0:
                out.write('    ')
            out.write('0x%02x,' % (b,))
            if (i % 12) == 11:
                out.write('\n')
        if (len(comp) % 12) != 0:
            out.write('\n')
        out.write('};\n')

    # 3. Write embedded_icons.h
    with open(out_icons_h, 'w', encoding='utf-8') as out:
        out.write('// Auto-generated file. Do not edit.\n')
        out.write('#pragma once\n\n')
        out.write('#include "tqtembeddedimages.h"\n')
        out.write('#include "embedded_images_data.h"\n\n')
        for fn, macro, off0, sz0 in entries:
            if fn.lower().endswith('.png'):
                out.write('#define %s ((const unsigned char*)&tqt_embimg_buf_[TQT_EMBIMG_%s_OFFSET])\n' % (macro, macro))
                out.write('#define %s_len TQT_EMBIMG_%s_SIZE\n\n' % (macro, macro))

    # 4. Write embedded_fonts.h
    with open(out_fonts_h, 'w', encoding='utf-8') as out:
        out.write('// Auto-generated file. Do not edit.\n')
        out.write('#pragma once\n\n')
        out.write('#include "tqtembeddedimages.h"\n')
        out.write('#include "embedded_images_data.h"\n\n')
        for fn, macro, off0, sz0 in entries:
            if fn.lower().endswith('.ttf'):
                out.write('#define %s ((const unsigned char*)&tqt_embimg_buf_[TQT_EMBIMG_%s_OFFSET])\n' % (macro, macro))
                out.write('#define %s_len TQT_EMBIMG_%s_SIZE\n\n' % (macro, macro))

    # 4b. Write embedded_txt.h
    with open(out_txt_h, 'w', encoding='utf-8') as out:
        out.write('// Auto-generated file. Do not edit.\n')
        out.write('#pragma once\n\n')
        out.write('#include "tqtembeddedimages.h"\n')
        out.write('#include "embedded_images_data.h"\n\n')
        for fn, macro, off0, sz0 in entries:
            if fn.lower().endswith('.txt'):
                out.write('#define %s ((const unsigned char*)&tqt_embimg_buf_[TQT_EMBIMG_%s_OFFSET])\n' % (macro, macro))
                out.write('#define %s_len TQT_EMBIMG_%s_SIZE\n\n' % (macro, macro))

    # 5. Write embedded_assets_compat.cpp
    with open(out_compat_cpp, 'w', encoding='utf-8') as out:
        out.write('// Auto-generated file. Do not edit.\n')
        out.write('int tqt_embimg_init_compat() {\n')
        out.write('    return 1;\n')
        out.write('}\n')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
