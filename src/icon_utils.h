#ifndef CALC_ICON_UTILS_H
#define CALC_ICON_UTILS_H

#include <tqpixmap.h>
#include <tqimage.h>
#include "embedded_icons.h"

#include <tdeconfig.h>
#include <tdeglobal.h>
#include <map>
#include "zx0em_runtime.h"

class IconUtils {
private:
    struct CacheKey {
        const unsigned char* data;
        unsigned int len;
        int w;
        int h;
        int icon_mode;
        unsigned int color_rgb;
        bool bold;
        
        bool operator<(const CacheKey& o) const {
            if (data != o.data) return data < o.data;
            if (len != o.len) return len < o.len;
            if (w != o.w) return w < o.w;
            if (h != o.h) return h < o.h;
            if (icon_mode != o.icon_mode) return icon_mode < o.icon_mode;
            if (color_rgb != o.color_rgb) return color_rgb < o.color_rgb;
            return bold < o.bold;
        }
    };
    
    struct ConfigCache {
        bool has_values;
        int icon_mode;
        TQColor icon_color;
        bool bold;
        ConfigCache() : has_values(false), icon_mode(0), icon_color(0, 0, 0), bold(false) {}
    };

    static ConfigCache& configCache() {
        static ConfigCache instance;
        return instance;
    }

    static std::map<CacheKey, TQPixmap>& cache() {
        static std::map<CacheKey, TQPixmap> instance;
        return instance;
    }

    static const zx0em_entry_t* findEntryByData(const unsigned char* data) {
        if (!data) return nullptr;
        for (int i = 0; i < ZX0EM_ENTRY_COUNT; i++) {
            const zx0em_entry_t *e = &zx0em_entries[i];
            if (zx0em_buf_ + e->offset == data) {
                return e;
            }
        }
        return nullptr;
    }

public:
    static void getIconSettings(const unsigned char* data, unsigned int len, int &icon_mode, TQColor &icon_color, bool &bold) {
        // Excluded icons: about_calc.png, calc.png, config_display.png, config_lang.png
        if (data == about_calc_png || data == calc_png || 
            data == config_display_png || data == config_lang_png ||
            len == 7929 || len == 225 || len == 507 || len == 635) {
            icon_mode = 0;
            icon_color = TQColor(255, 255, 255);
            bold = false;
            return;
        }
        
        ConfigCache& cc = configCache();
        if (cc.has_values) {
            icon_mode = cc.icon_mode;
            icon_color = cc.icon_color;
            bold = cc.bold && (data == divide_png || data == equal_png || data == invert_png ||
                               data == multiply_png || data == nfact_png || data == percent_png ||
                               data == pi_png || data == plusminus_png || data == plus_png ||
                               data == square_png || data == squareroot_png || data == tenexpx_png ||
                               data == xexpy_png);
            return;
        }
        
        // Read configuration
        TDEConfig *config = new TDEConfig("calcrc");
        config->reparseConfiguration();
        config->setGroup("Preferences");
        
        int temp_icon_mode = 0;
        TQColor temp_icon_color = TQColor(255, 255, 255);
        bool temp_bold = false;

        int mode = config->readNumEntry("AppearanceMode", 2);

        if (mode == 0) { // Classic Dark
            temp_icon_mode = 1;
            temp_bold = false;
        } else if (mode == 2) { // Classic
            temp_icon_mode = 0;
            temp_bold = false;
        } else if (mode == 3) { // Theme
            TQString selectedTheme = config->readEntry("SelectedTheme", "Midnight Blue");
            if (selectedTheme == "Midnight Blue" || selectedTheme == "Forest Green" || 
                selectedTheme == "internal_classic_dark" || selectedTheme == "Classic Dark" ||
                selectedTheme == "orange style" || selectedTheme == "Orange Style" ||
                selectedTheme == "Computo") {
                temp_icon_mode = 1;
                temp_icon_color = TQColor(255, 255, 255);
                temp_bold = false;
            } else if (selectedTheme == "Classic Gray") {
                temp_icon_mode = 0;
                temp_icon_color = TQColor(255, 255, 255);
                temp_bold = true;
            } else if (selectedTheme == "Default" || selectedTheme == "internal_classic" || selectedTheme == "Classic") {
                temp_icon_mode = 0;
                temp_icon_color = TQColor(255, 255, 255);
                temp_bold = false;
            } else if (selectedTheme == "Coloured") {
                temp_icon_mode = 2;
                temp_icon_color = TQColor(255, 252, 62);
                temp_bold = true;
            } else {
                TDEConfig *themeConfig = new TDEConfig("calcrc");
                themeConfig->reparseConfiguration();
                themeConfig->setGroup("Theme_" + selectedTheme);
                temp_icon_mode = themeConfig->readNumEntry("IconMode", themeConfig->readBoolEntry("InvertIcons", false) ? 1 : 0);
                TQColor defIconColor(255, 255, 255);
                temp_icon_color = themeConfig->readColorEntry("IconColor", &defIconColor);
                temp_bold = themeConfig->readBoolEntry("BoldIcons", false);
                delete themeConfig;
            }
        } else { // Custom
            temp_icon_mode = config->readNumEntry("CustomIconMode", config->readBoolEntry("CustomInvertIcons", false) ? 1 : 0);
            TQColor defIconColor(255, 255, 255);
            temp_icon_color = config->readColorEntry("CustomIconColor", &defIconColor);
            temp_bold = config->readBoolEntry("CustomBoldIcons", false);
        }

        cc.icon_mode = temp_icon_mode;
        cc.icon_color = temp_icon_color;
        cc.bold = temp_bold;
        cc.has_values = true;

        icon_mode = temp_icon_mode;
        icon_color = temp_icon_color;
        bold = temp_bold && (data == divide_png || data == equal_png || data == invert_png ||
                             data == multiply_png || data == nfact_png || data == percent_png ||
                             data == pi_png || data == plusminus_png || data == plus_png ||
                             data == square_png || data == squareroot_png || data == tenexpx_png ||
                             data == xexpy_png);

        delete config;
    }

    static void clearCache() {
        cache().clear();
        configCache().has_values = false;
        TDEConfig *config = new TDEConfig("calcrc");
        config->reparseConfiguration();
        delete config;
    }

    static TQImage loadImageRaw(const unsigned char* data, unsigned int len) {
        TQImage img;
        const zx0em_entry_t *e = findEntryByData(data);
        if (e) {
            int w = e->width;
            int h = e->height;
            int pixfmt = e->pixfmt;
            const unsigned char *pixels = data;
            
            if (pixfmt == ZX0EM_PIXFMT_RGBA) {
                img = TQImage(w, h, 32);
                img.setAlphaBuffer(true);
                for (int y = 0; y < h; ++y) {
                    TQRgb *dest = (TQRgb*)img.scanLine(y);
                    const unsigned char *src = pixels + y * w * 4;
                    for (int x = 0; x < w; ++x) {
                        dest[x] = tqRgba(src[x*4], src[x*4+1], src[x*4+2], src[x*4+3]);
                    }
                }
            } else if (pixfmt == ZX0EM_PIXFMT_GRAY_ALPHA) {
                img = TQImage(w, h, 32);
                img.setAlphaBuffer(true);
                for (int y = 0; y < h; ++y) {
                    TQRgb *dest = (TQRgb*)img.scanLine(y);
                    const unsigned char *src = pixels + y * w * 2;
                    for (int x = 0; x < w; ++x) {
                        unsigned char g = src[x*2];
                        unsigned char a = src[x*2+1];
                        dest[x] = tqRgba(g, g, g, a);
                    }
                }
            } else if (pixfmt == ZX0EM_PIXFMT_PALETTE) {
                img = TQImage(w, h, 32);
                img.setAlphaBuffer(true);
                const unsigned char *pal_data = zx0em_buf_ + e->offset;
                int pal_count = pal_data[0];
                const unsigned char *colors = pal_data + 1;
                const unsigned char *indices = data + 1 + pal_count * 4;
                for (int y = 0; y < h; ++y) {
                    TQRgb *dest = (TQRgb*)img.scanLine(y);
                    const unsigned char *src = indices + y * w;
                    for (int x = 0; x < w; ++x) {
                        int idx = src[x];
                        int r = colors[idx*4];
                        int g = colors[idx*4+1];
                        int b = colors[idx*4+2];
                        int a = colors[idx*4+3];
                        dest[x] = tqRgba(r, g, b, a);
                    }
                }
            } else if (pixfmt == ZX0EM_PIXFMT_1BIT) {
                img = TQImage(w, h, 32);
                img.setAlphaBuffer(true);
                const unsigned char *bits = data;
                unsigned int pitch = (w + 7) / 8;
                const unsigned char *alpha = data + pitch * h;
                for (int y = 0; y < h; ++y) {
                    TQRgb *dest = (TQRgb*)img.scanLine(y);
                    const unsigned char *src_val = bits + y * pitch;
                    const unsigned char *src_alpha = alpha + y * pitch;
                    for (int x = 0; x < w; ++x) {
                        int bit_val = (src_val[x / 8] >> (7 - (x % 8))) & 1;
                        int bit_alpha = (src_alpha[x / 8] >> (7 - (x % 8))) & 1;
                        if (bit_alpha) {
                            dest[x] = bit_val ? tqRgba(255, 255, 255, 255) : tqRgba(0, 0, 0, 255);
                        } else {
                            dest[x] = tqRgba(0, 0, 0, 0); // Transparent
                        }
                    }
                }
            }
        } else {
            img.loadFromData(data, len, "PNG");
        }
        if (!img.hasAlphaBuffer()) img.setAlphaBuffer(true);
        if (img.depth() != 32) img = img.convertDepth(32);
        return img;
    }

    static TQPixmap load(const unsigned char* data, unsigned int len, int w = -1, int h = -1, const TQColor &custom_color = TQColor()) {
        int icon_mode = 0;
        TQColor icon_color;
        bool bold = false;
        
        if (custom_color.isValid()) {
            icon_mode = 2; // Color mode
            icon_color = custom_color;
            int dummy_mode;
            TQColor dummy_color;
            getIconSettings(data, len, dummy_mode, dummy_color, bold);
        } else {
            getIconSettings(data, len, icon_mode, icon_color, bold);
        }

        CacheKey k = { data, len, w, h, icon_mode, icon_color.rgb(), bold };
        
        std::map<CacheKey, TQPixmap>& c = cache();
        if (c.find(k) != c.end()) {
            return c[k];
        }

        TQImage img = loadImageRaw(data, len);
        
        if (icon_mode == 1) { // Invert
            if (img.depth() != 32) img = img.convertDepth(32);
            if (!img.hasAlphaBuffer()) img.setAlphaBuffer(true);
            
            for (int y = 0; y < img.height(); ++y) {
                TQRgb *line = (TQRgb*)img.scanLine(y);
                for (int x = 0; x < img.width(); ++x) {
                    int alpha = tqAlpha(line[x]);
                    int r = 255 - tqRed(line[x]);
                    int g = 255 - tqGreen(line[x]);
                    int b = 255 - tqBlue(line[x]);
                    line[x] = tqRgba(r, g, b, alpha);
                }
            }
        } else if (icon_mode == 2) { // Color
            if (img.depth() != 32) img = img.convertDepth(32);
            if (!img.hasAlphaBuffer()) img.setAlphaBuffer(true);
            
            int r = icon_color.red();
            int g = icon_color.green();
            int b = icon_color.blue();
            
            for (int y = 0; y < img.height(); ++y) {
                TQRgb *line = (TQRgb*)img.scanLine(y);
                for (int x = 0; x < img.width(); ++x) {
                    int alpha = tqAlpha(line[x]);
                    line[x] = tqRgba(r, g, b, alpha);
                }
            }
        }

        if (w > 0 && h > 0 && (img.width() != w || img.height() != h)) {
            img = img.smoothScale(w, h);
        }

        if (bold) {
            if (img.depth() != 32) img = img.convertDepth(32);
            if (!img.hasAlphaBuffer()) img.setAlphaBuffer(true);
            
            TQImage boldImg(img.width(), img.height(), 32);
            boldImg.setAlphaBuffer(true);
            
            for (int y = 0; y < img.height(); ++y) {
                TQRgb *destLine = (TQRgb*)boldImg.scanLine(y);
                TQRgb *srcLine = (TQRgb*)img.scanLine(y);
                for (int x = 0; x < img.width(); ++x) {
                    TQRgb pix_orig = srcLine[x];
                    TQRgb pix_shifted = (x > 0) ? srcLine[x-1] : 0;
                    
                    double a_top = tqAlpha(pix_orig);
                    double r_top = tqRed(pix_orig);
                    double g_top = tqGreen(pix_orig);
                    double b_top = tqBlue(pix_orig);
                    
                    double a_bottom = tqAlpha(pix_shifted);
                    double r_bottom = tqRed(pix_shifted);
                    double g_bottom = tqGreen(pix_shifted);
                    double b_bottom = tqBlue(pix_shifted);
                    
                    double k = (255.0 - a_top) / 255.0;
                    double a_out_d = a_top + a_bottom * k;
                    int a_out = static_cast<int>(a_out_d + 0.5);
                    if (a_out > 255) a_out = 255;
                    
                    int r_out = 0, g_out = 0, b_out = 0;
                    if (a_out_d > 0.0) {
                        r_out = static_cast<int>((r_top * a_top + r_bottom * a_bottom * k) / a_out_d + 0.5);
                        g_out = static_cast<int>((g_top * a_top + g_bottom * a_bottom * k) / a_out_d + 0.5);
                        b_out = static_cast<int>((b_top * a_top + b_bottom * a_bottom * k) / a_out_d + 0.5);
                    }
                    if (r_out > 255) r_out = 255;
                    if (g_out > 255) g_out = 255;
                    if (b_out > 255) b_out = 255;
                    
                    destLine[x] = tqRgba(r_out, g_out, b_out, a_out);
                }
            }
            img = boldImg;
        }
        
        TQPixmap pm(img);
        c[k] = pm;
        return pm;
    }
};

#endif /* CALC_ICON_UTILS_H */
