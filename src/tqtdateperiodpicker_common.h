#ifndef TQTDATEPERIODPICKER_COMMON_H
#define TQTDATEPERIODPICKER_COMMON_H

#include <ntqdatetime.h>
#include <tdeconfig.h>
#include <tqcolor.h>

enum TQtDatePeriodPickerView {
    TQtDPPMonthView = 0,
    TQtDPPYearView = 1,
    TQtDPPDecadeView = 2
};

enum TQtDatePeriodPickerType {
    TQtDPPDayType = 0x0001,
    TQtDPPPeriodType = 0x0002
};

typedef int TQtDatePeriodPickerTypes;

static inline void dppGetPanelColors(TQColor &bg, TQColor &fg)
{
    TQColor defaultPanelBg(230, 230, 230);
    TQColor defaultFg(0, 0, 0);

    bg = defaultPanelBg;
    fg = defaultFg;

    TDEConfig config("calcrc");
    config.setGroup("Preferences");
    int mode = config.readNumEntry("AppearanceMode", 2); // default to Theme
    TQString selectedTheme = config.readEntry("SelectedTheme", "Midnight Blue");

    if (mode == 0) { // Classic Dark
        bg = TQColor(62, 62, 62);
        fg = TQColor(255, 255, 255);
    } else if (mode == 2) { // Classic
        bg = TQColor(231, 231, 231);
        fg = TQColor(0, 0, 0);
    } else if (mode == 3) { // Theme
        if (selectedTheme == "Midnight Blue") {
            bg = TQColor(46, 61, 82);
            fg = TQColor(255, 255, 255);
        } else if (selectedTheme == "Forest Green") {
            bg = TQColor(42, 66, 53);
            fg = TQColor(230, 242, 235);
        } else if (selectedTheme == "Classic Gray") {
            bg = TQColor(221, 221, 221);
            fg = TQColor(0, 0, 0);
        } else if (selectedTheme == "Coloured") {
            bg = TQColor(87, 108, 201);
            fg = TQColor(255, 255, 255);
        } else if (selectedTheme == "orange style") {
            bg = TQColor(42, 66, 53);
            fg = TQColor(230, 242, 235);
        } else if (selectedTheme == "Default" || selectedTheme == "internal_classic") {
            bg = TQColor(231, 231, 231);
            fg = TQColor(0, 0, 0);
        } else if (selectedTheme == "internal_classic_dark") {
            bg = TQColor(62, 62, 62);
            fg = TQColor(255, 255, 255);
        } else {
            TDEConfig themeConfig("calcrc");
            themeConfig.setGroup("Theme_" + selectedTheme);
            bg = themeConfig.readColorEntry("PanelBgColor", &defaultPanelBg);
            fg = themeConfig.readColorEntry("FgColor", &defaultFg);
        }
    } else { // Custom
        bg = config.readColorEntry("CustomPanelBgColor", &defaultPanelBg);
        fg = config.readColorEntry("CustomFgColor", &defaultFg);
    }
}

#endif
