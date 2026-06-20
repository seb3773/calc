#ifndef CALC_TRANSLATION_H
#define CALC_TRANSLATION_H

#include <tqstring.h>
#include <tdeconfig.h>
#include <cstdlib>
#include <cstring>
#include <tqmap.h>
#include <tqstringlist.h>
#include "embedded_txt.h"
#include "tqtembeddedimages.h"

enum CalcLang {
    LangEnglish = 0,
    LangFrench,
    LangGerman,
    LangSpanish,
    LangRussian,
    LangHindi,
    LangCount
};

class Translation {
private:
    static CalcLang s_lang;
    static bool s_initialized;
    static TQMap<TQString, TQStringList> s_map;

public:
    static void init() {
        if (s_initialized) return;
        s_initialized = true;

        // Initialize the embedded images (decompresses them)
        tqt_embimg_init();

        s_map.clear();
        if (translations_txt && translations_txt_len > 0) {
            TQString content = TQString::fromUtf8((const char*)translations_txt, (int)translations_txt_len);
            TQStringList lines = TQStringList::split('\n', content);
            for (TQStringList::Iterator it = lines.begin(); it != lines.end(); ++it) {
                TQString line = (*it).stripWhiteSpace();
                if (line.isEmpty()) continue;
                
                TQStringList parts = TQStringList::split('|', line, true);
                if (parts.count() >= 7) {
                    TQString key = parts[0];
                    TQStringList langs;
                    for (int i = 1; i <= 6; ++i) {
                        langs.append(parts[i]);
                    }
                    s_map.insert(key, langs);
                }
            }
        }

        TDEConfig *config = new TDEConfig("calcrc");
        config->setGroup("Preferences");
        TQString lang = config->readEntry("Language", "");
        delete config;

        if (lang.isEmpty()) {
            // Auto-detect from system locale
            const char *env = getenv("LC_ALL");
            if (!env || !env[0]) env = getenv("LC_MESSAGES");
            if (!env || !env[0]) env = getenv("LANG");
            if (env && env[0]) {
                if (strncmp(env, "fr", 2) == 0) s_lang = LangFrench;
                else if (strncmp(env, "de", 2) == 0) s_lang = LangGerman;
                else if (strncmp(env, "es", 2) == 0) s_lang = LangSpanish;
                else if (strncmp(env, "ru", 2) == 0) s_lang = LangRussian;
                else if (strncmp(env, "hi", 2) == 0) s_lang = LangHindi;
                else s_lang = LangEnglish;
            } else {
                s_lang = LangEnglish;
            }
        } else {
            if (lang == "fr") s_lang = LangFrench;
            else if (lang == "german") s_lang = LangGerman;
            else if (lang == "spanish") s_lang = LangSpanish;
            else if (lang == "russian") s_lang = LangRussian;
            else if (lang == "indi") s_lang = LangHindi;
            else s_lang = LangEnglish;
        }
    }

    static void setLang(CalcLang lang) {
        s_lang = lang;
        s_initialized = true;
    }

    static CalcLang lang() {
        if (!s_initialized) init();
        return s_lang;
    }

    static void reload() {
        s_initialized = false;
        init();
    }

    static TQString tr(const char *key) {
        if (!s_initialized) init();
        TQString k = TQString::fromUtf8(key);
        TQMap<TQString, TQStringList>::ConstIterator it = s_map.find(k);
        if (it != s_map.end()) {
            const TQStringList &langs = it.data();
            if ((int)s_lang < (int)langs.count()) {
                TQString txt = langs[s_lang];
                if (!txt.isEmpty()) return txt;
            }
            if ((int)LangEnglish < (int)langs.count()) {
                return langs[LangEnglish];
            }
        }
        return k;
    }
};

// Static member initialization
inline CalcLang Translation::s_lang = LangEnglish;
inline bool Translation::s_initialized = false;
inline TQMap<TQString, TQStringList> Translation::s_map;

// Convenience function
inline TQString tr_str(const char *key) {
    return Translation::tr(key);
}

#endif /* CALC_TRANSLATION_H */
