#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <tqdialog.h>
#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqpushbutton.h>
#include <tqlabel.h>
#include <tqscrollview.h>
#include <kcolorbutton.h>
#include <tqgroupbox.h>
#include <tqlayout.h>
#include <tqwidgetstack.h>

class TDEFontRequester;

class SettingsDialog : public TQDialog
{
    TQ_OBJECT

public:
    SettingsDialog(bool startInAboutMode = false, TQWidget *parent = 0, const char *name = 0);
    virtual ~SettingsDialog();

private slots:
    void slotClassicToggled(bool checked);
    void slotClassicDarkToggled(bool checked);
    void slotCustomToggled(bool checked);
    void slotThemeToggled(bool checked);
    void slotThemeChanged(const TQString &themeName);
    void slotDisplayTypeChanged(int index);
    void slotSaveAsThemeClicked();
    void slotDeleteThemeClicked();
    void slotAboutClicked();
    void slotCloseClicked();
    void slotAboutCloseClicked();
    void slotLanguageChanged(int index);
    void slotIconModeChanged(int index);
    void slotKeyFontTypeChanged(int index);

private:
    TQWidgetStack *stack;
    TQWidget *prefsWidget;
    TQWidget *aboutWidget;
    bool m_startedInAboutMode;
    void loadSettings();
    void saveSettings();
    void updateSubElementsEnabledState();
    void populateThemesCombo();
    void loadThemeColors(const TQString &themeName);
    void retranslateUi();

    // Appearance Checkboxes (Mutually Exclusive)
    TQCheckBox *chkClassic;
    TQCheckBox *chkClassicDark;
    TQCheckBox *chkCustom;
    TQCheckBox *chkTheme;

    // Theme Selector
    TQComboBox *comboTheme;

    // Custom Appearance Sub-elements
    TQWidget *customSubContainer;
    KColorButton *colorBg;
    KColorButton *colorFg;
    KColorButton *colorSmallFg;
    KColorButton *colorNumKeyFg;
    KColorButton *colorNumKeyBg;
    KColorButton *colorOpKeyFg;
    KColorButton *colorOpKeyBg;
    KColorButton *colorClearKeyFg;
    KColorButton *colorClearKeyBg;
    KColorButton *colorMemKeyFg;
    KColorButton *colorMemKeyBg;
    KColorButton *colorEqualBg;
    KColorButton *colorEqualFg;
    TQComboBox *comboDisplayType;
    TQLabel *lblDisplayFontSel;
    TDEFontRequester *fontDisplay;
    TQComboBox *comboKeyFontType;
    TQLabel *lblKeyFont;
    TQLabel *lblKeyFontSel;
    TDEFontRequester *fontKey;
    KColorButton *colorDisplayFg;
    KColorButton *colorDisplayBg;
    KColorButton *colorPanelBg;
    KColorButton *colorMenuBg;
    TQCheckBox *chkNoDeco;
    TQCheckBox *chkStandardMenuBar;
    TQComboBox *comboIconMode;
    KColorButton *btnIconColor;
    TQLabel *lblIcons;
    TQCheckBox *chkKeysBorders;
    KColorButton *colorKeysBorders;
    TQCheckBox *chkWindowBorder;
    KColorButton *colorWindowBorder;
    TQCheckBox *chkDisplayBorder;
    KColorButton *colorDisplayBorder;
    TQCheckBox *chkBoldIcons;
    TQPushButton *btnSaveAsTheme;
    TQPushButton *btnDeleteTheme;

    // Language Selector
    TQComboBox *comboLanguage;
    TQLabel *lblAppearance;
    TQLabel *lblLanguage;

    // Action buttons
    TQPushButton *btnAbout;
    TQPushButton *btnClose;
    TQPushButton *btnAboutClose;
};

#endif // SETTINGS_DIALOG_H
