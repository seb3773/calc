#include "tqtembeddedimages.h"
#include "zx0em_runtime.h"

int tqt_embimg_init()
{
    return zx0em_init();
}

int tqt_embimg_count()
{
    return ZX0EM_ENTRY_COUNT;
}

const unsigned char* tqt_embimg_data(int id, unsigned int* size)
{
    return zx0em_get(id, size);
}
