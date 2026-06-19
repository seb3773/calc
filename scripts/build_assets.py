#!/usr/bin/env python3
import os
import re
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
