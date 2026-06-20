#include "tqtembeddedimages.h"

#include <string.h>

#if defined(USE_ZX0)
#include "zx0_decompress.h"
#elif defined(USE_LZ4)
#include "codec_lz4.h"
#else
#include <zlib.h>
#endif

#include "embedded_images_data.h"

unsigned char tqt_embimg_buf_[TQT_EMBIMG_UNCOMPRESSED_SIZE];
static unsigned int tqt_embimg_inited_ = 0;

int tqt_embimg_init_compat();

int tqt_embimg_count()
{
    return (int)TQT_EMBIMG__COUNT;
}

int tqt_embimg_init()
{
    if (tqt_embimg_inited_) return 1;

#if defined(USE_ZX0)
    int decompressed = zx0_decompress_to(
        tqt_embimg_z,
        (int)tqt_embimg_compressed_size,
        tqt_embimg_buf_,
        (int)tqt_embimg_uncompressed_size
    );
    if (decompressed != (int)tqt_embimg_uncompressed_size)
        return 0;
#elif defined(USE_LZ4)
    int decompressed = lz4_decompress(
        (const char*)tqt_embimg_z,
        (char*)tqt_embimg_buf_,
        (int)tqt_embimg_compressed_size,
        (int)tqt_embimg_uncompressed_size
    );
    if (decompressed != (int)tqt_embimg_uncompressed_size)
        return 0;
#else
    unsigned long dst_len = (unsigned long)tqt_embimg_uncompressed_size;
    if (uncompress((Bytef*)tqt_embimg_buf_,
                   &dst_len,
                   (const Bytef*)tqt_embimg_z,
                   (unsigned long)tqt_embimg_compressed_size) != Z_OK)
        return 0;

    if (dst_len != (unsigned long)tqt_embimg_uncompressed_size)
        return 0;
#endif

    tqt_embimg_inited_ = 1;

    if (!tqt_embimg_init_compat())
        return 0;

    return 1;
}

const unsigned char* tqt_embimg_data(int id, unsigned int* size)
{
    if (!size) return 0;
    *size = 0;

    if (!tqt_embimg_inited_) return 0;
    if (id < 0 || id >= (int)TQT_EMBIMG__COUNT) return 0;

    const unsigned int off = tqt_embimg_offset_by_id_(id);
    const unsigned int sz = tqt_embimg_size_by_id_(id);

    if (off > tqt_embimg_uncompressed_size) return 0;
    if (sz > tqt_embimg_uncompressed_size) return 0;
    if ((off + sz) > tqt_embimg_uncompressed_size) return 0;

    *size = sz;
    return (const unsigned char*)(tqt_embimg_buf_ + off);
}
