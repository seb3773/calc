#!/usr/bin/env python3
import os
import re
import sys
import subprocess

def main() -> int:
    if len(sys.argv) != 5:
        sys.stderr.write('usage: build_assets.py <assets_dir> <out_dir> <bin_compress_path> <lz4|zx0>\n')
        return 2

    assets_dir = sys.argv[1]
    out_dir = sys.argv[2]
    bin_compress = sys.argv[3]
    mode = sys.argv[4]

    os.makedirs(out_dir, exist_ok=True)

    # 1. Run the zx0em_pack compiler
    pack_args = [bin_compress, assets_dir, out_dir, '--stats']
    if mode == 'lz4':
        pack_args.append('--salvador')
    else:
        pack_args.append('--optimal')

    print(f"Running zx0em_pack: {' '.join(pack_args)}")
    res = subprocess.run(pack_args, capture_output=True)
    if res.returncode != 0:
        sys.stderr.write(f"zx0em_pack failed: {res.stderr.decode('utf-8')}\n")
        return 1

    # 2. Patch zx0em_runtime.h to convert static buffers to extern globals
    zx0em_runtime_h = os.path.join(out_dir, 'zx0em_runtime.h')
    with open(zx0em_runtime_h, 'r', encoding='utf-8') as f:
        run_content = f.read()

    run_content = run_content.replace(
        'static unsigned char zx0em_buf_[ZX0EM_UNCOMPRESSED_SIZE];',
        'extern unsigned char zx0em_buf_[ZX0EM_UNCOMPRESSED_SIZE];'
    )
    run_content = run_content.replace(
        'static int zx0em_inited_ = 0;',
        'extern int zx0em_inited_;'
    )
    run_content = run_content.replace(
        'static unsigned char _zx0em_planar_scratch[ZX0EM_MAX_PLANAR_SCRATCH_SIZE];',
        'extern unsigned char _zx0em_planar_scratch[ZX0EM_MAX_PLANAR_SCRATCH_SIZE];'
    )

    with open(zx0em_runtime_h, 'w', encoding='utf-8') as f:
        f.write(run_content)

    # 3. Read the generated zx0em_index.h to map macro names and sizes
    zx0em_idx_h = os.path.join(out_dir, 'zx0em_index.h')
    with open(zx0em_idx_h, 'r', encoding='utf-8') as f:
        idx_content = f.read()

    offsets = re.findall(r'#define\s+ZX0EM_(\w+)_OFFSET\s+(\d+)U', idx_content)
    sizes = re.findall(r'#define\s+ZX0EM_(\w+)_SIZE\s+(\d+)U', idx_content)

    offset_map = {name: int(off) for name, off in offsets}

    out_icons_h = os.path.join(out_dir, 'embedded_icons.h')
    out_fonts_h = os.path.join(out_dir, 'embedded_fonts.h')
    out_txt_h = os.path.join(out_dir, 'embedded_txt.h')
    out_images_data_h = os.path.join(out_dir, 'embedded_images_data.h')
    out_images_data_cpp = os.path.join(out_dir, 'embedded_images_data.cpp')
    out_compat_cpp = os.path.join(out_dir, 'embedded_assets_compat.cpp')

    # 4. Generate backwards-compatible embedded_icons.h
    with open(out_icons_h, 'w', encoding='utf-8') as out:
        out.write('// Auto-generated file. Do not edit.\n')
        out.write('#pragma once\n\n')
        out.write('#include "zx0em_runtime.h"\n\n')
        for name in offset_map.keys():
            if name.endswith('_png'):
                out.write('#define %s (zx0em_get_image(ZX0EM_%s, NULL, NULL, NULL))\n' % (name, name))
                out.write('#define %s_len ZX0EM_%s_SIZE\n\n' % (name, name))

    # 5. Generate backwards-compatible embedded_fonts.h
    with open(out_fonts_h, 'w', encoding='utf-8') as out:
        out.write('// Auto-generated file. Do not edit.\n')
        out.write('#pragma once\n\n')
        out.write('#include "zx0em_runtime.h"\n\n')
        for name in offset_map.keys():
            if name.endswith('_ttf'):
                out.write('#define %s (zx0em_get(ZX0EM_%s, NULL))\n' % (name, name))
                out.write('#define %s_len ZX0EM_%s_SIZE\n\n' % (name, name))

    # 6. Generate backwards-compatible embedded_txt.h
    with open(out_txt_h, 'w', encoding='utf-8') as out:
        out.write('// Auto-generated file. Do not edit.\n')
        out.write('#pragma once\n\n')
        out.write('#include "zx0em_runtime.h"\n\n')
        for name in offset_map.keys():
            if name.endswith('_txt'):
                out.write('#define %s (zx0em_get_text(ZX0EM_%s, NULL))\n' % (name, name))
                out.write('#define %s_len ZX0EM_%s_SIZE\n\n' % (name, name))

    # 7. Generate embedded_images_data.h
    with open(out_images_data_h, 'w', encoding='utf-8') as out:
        out.write('// Auto-generated file. Do not edit.\n')
        out.write('#pragma once\n\n')
        out.write('#include "zx0em_blob_data.h"\n')
        out.write('#include "zx0em_index.h"\n')

    # 8. Generate embedded_images_data.cpp
    with open(out_images_data_cpp, 'w', encoding='utf-8') as out:
        out.write('// Auto-generated file. Do not edit.\n')
        out.write('#include "embedded_images_data.h"\n')
        out.write('#include "zx0em_blob_data.c"\n')

    # 9. Generate embedded_assets_compat.cpp defining zx0em globals
    with open(out_compat_cpp, 'w', encoding='utf-8') as out:
        out.write('// Auto-generated file. Do not edit.\n')
        out.write('#include "tqtembeddedimages.h"\n')
        out.write('#include "zx0em_runtime.h"\n\n')
        out.write('// Define zx0em globals with external linkage\n')
        out.write('unsigned char zx0em_buf_[ZX0EM_UNCOMPRESSED_SIZE];\n')
        out.write('int zx0em_inited_ = 0;\n')
        out.write('#if ZX0EM_MAX_PLANAR_SCRATCH_SIZE > 0\n')
        out.write('unsigned char _zx0em_planar_scratch[ZX0EM_MAX_PLANAR_SCRATCH_SIZE];\n')
        out.write('#endif\n\n')
        out.write('// Satisfy the linker extern definition of tqt_embimg_buf_\n')
        out.write('unsigned char tqt_embimg_buf_[1] = {0};\n\n')
        out.write('int tqt_embimg_init_compat() {\n')
        out.write('    return 1;\n')
        out.write('}\n')

    return 0

if __name__ == '__main__':
    raise SystemExit(main())
