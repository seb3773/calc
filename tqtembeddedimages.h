#ifndef TQT_EMBEDDED_IMAGES_H
#define TQT_EMBEDDED_IMAGES_H

#include <ntqglobal.h>

class TQPixmap;

extern unsigned char tqt_embimg_buf_[];

int tqt_embimg_init();

int tqt_embimg_count();

const unsigned char* tqt_embimg_data(int id, unsigned int* size);

#endif
