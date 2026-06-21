#include "settings_dialog.h"
#include "translation.h"
#include <tdefontrequester.h>
#include <tqobjectlist.h>
#include "icon_utils.h"
#include "embedded_icons.h"
#include <kdialog.h>
#include <tdeglobal.h>
#include <tdeconfig.h>
#include <tdelocale.h>
#include <kinputdialog.h>
#include <tdemessagebox.h>
#include <tqtooltip.h>
#include <tqpainter.h>
#include <tqframe.h>

static int findComboText(TQComboBox *combo, const TQString &text) {
    for (int i = 0; i < combo->count(); ++i) {
        if (combo->text(i) == text) {
            return i;
        }
    }
    return -1;
}

static const TQColor defaultBg(230, 230, 230);
static const TQColor defaultFg(0, 0, 0);
static const TQColor defaultSmallFg(0, 0, 0);
static const TQColor defaultNumKeyFg(0, 0, 0);
static const TQColor defaultNumKeyBg(255, 255, 255);
static const TQColor defaultOtherKeyFg(0, 0, 0);
static const TQColor defaultOtherKeyBg(230, 230, 230);
static const TQColor defaultEqualBg(0, 103, 192);
static const TQColor defaultEqualFg(255, 255, 255);
static const TQColor defaultKeysBorders(0, 0, 0);
static const TQColor defaultWindowBorder(0, 0, 0);
static const TQColor defaultDisplayBorder(0, 0, 0);
static const TQColor defaultDisplayFg(0, 0, 0);
static const TQColor defaultDisplayBg(189, 255, 180);
static const TQColor defaultPanelBg(230, 230, 230);
static const TQColor defaultMenuBg(230, 230, 230);

SettingsDialog::SettingsDialog(bool startInAboutMode, TQWidget *parent, const char *name)
    : TQDialog(parent, name, true), m_startedInAboutMode(startInAboutMode)
{
    if (m_startedInAboutMode) {
        setMinimumSize(765 / 2, 750 / 2);
        resize(765 / 2, 750 / 2);
    } else {
        setMinimumSize(765, 750);
        resize(765, 750);
    }
    setCaption(m_startedInAboutMode ? "Calculator - About" : "Calculator - settings");

    TQVBoxLayout *dialogLay = new TQVBoxLayout(this);
    stack = new TQWidgetStack(this);
    dialogLay->addWidget(stack);

    // Preferences Widget
    if (!m_startedInAboutMode) {
        prefsWidget = new TQWidget(stack);
        TQVBoxLayout *mainLay = new TQVBoxLayout(prefsWidget, 15, 10);

        // --- APPEARANCE SECTION ---
    TQFrame *appHeaderFrame = new TQFrame(prefsWidget);
    appHeaderFrame->setPaletteBackgroundColor(TQColor(255, 255, 255));
    appHeaderFrame->setFrameStyle(TQFrame::NoFrame);
    TQHBoxLayout *appTitleLay = new TQHBoxLayout(appHeaderFrame, 4, 8);

    TQLabel *iconAppearance = new TQLabel(appHeaderFrame);
    iconAppearance->setPixmap(IconUtils::load(config_display_png, config_display_png_len, 48, 32));
    appTitleLay->addWidget(iconAppearance);

    lblAppearance = new TQLabel("Appearance", appHeaderFrame);
    TQFont fontApp = lblAppearance->font();
    fontApp.setBold(true);
    fontApp.setPixelSize(17);
    lblAppearance->setFont(fontApp);
    appTitleLay->addWidget(lblAppearance);
    appTitleLay->addStretch(1);

    mainLay->addWidget(appHeaderFrame);

    mainLay->addSpacing(3); // Small margin under header

    TQFont boldCheckFont = font();
    boldCheckFont.setBold(true);



    // Theme Layout (horizontal)
    TQHBoxLayout *themeLay = new TQHBoxLayout();
    mainLay->addLayout(themeLay);
    chkTheme = new TQCheckBox("Theme :", prefsWidget);
    chkTheme->setFont(boldCheckFont);
    themeLay->addWidget(chkTheme);
    connect(chkTheme, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotThemeToggled(bool)));

    comboTheme = new TQComboBox(prefsWidget);
    themeLay->addWidget(comboTheme);
    connect(comboTheme, TQ_SIGNAL(activated(int)), TQ_SLOT(slotThemeChanged(int)));

    btnDeleteTheme = new TQPushButton(prefsWidget);
    btnDeleteTheme->setPixmap(IconUtils::load(trash_png, trash_png_len, 20, 20));
    btnDeleteTheme->setFixedWidth(28);
    btnDeleteTheme->setFixedHeight(28);
    btnDeleteTheme->setFlat(true);
    TQToolTip::add(btnDeleteTheme, "Delete selected custom theme");
    themeLay->addWidget(btnDeleteTheme);
    connect(btnDeleteTheme, TQ_SIGNAL(clicked()), TQ_SLOT(slotDeleteThemeClicked()));

    mainLay->addSpacing(4);

    chkCustom = new TQCheckBox("Custom", prefsWidget);
    chkCustom->setFont(boldCheckFont);
    mainLay->addWidget(chkCustom);
    connect(chkCustom, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotCustomToggled(bool)));
    mainLay->addSpacing(4);

    // Custom Appearance Container (holding labels & pickers)
    customSubContainer = new TQWidget(prefsWidget);
    mainLay->addWidget(customSubContainer);
    TQGridLayout *grid = new TQGridLayout(customSubContainer, 16, 4, 0, 4);
    grid->setColSpacing(1, 100); // Spacing between the columns

    // Column 1 (Left): Window background to Equal button background
    grid->addWidget(new TQLabel("Window background:", customSubContainer, "Window background:"), 0, 0);
    colorBg = new KColorButton(customSubContainer);
    colorBg->setFixedWidth(50);
    grid->addWidget(colorBg, 0, 1);

    grid->addWidget(new TQLabel("Main texts:", customSubContainer, "Main texts:"), 1, 0);
    colorFg = new KColorButton(customSubContainer);
    colorFg->setFixedWidth(50);
    grid->addWidget(colorFg, 1, 1);

    grid->addWidget(new TQLabel("Keys (numbers) foreground:", customSubContainer, "Keys (numbers) foreground:"), 2, 0);
    colorNumKeyFg = new KColorButton(customSubContainer);
    colorNumKeyFg->setFixedWidth(50);
    grid->addWidget(colorNumKeyFg, 2, 1);

    grid->addWidget(new TQLabel("Keys (numbers) background:", customSubContainer, "Keys (numbers) background:"), 3, 0);
    colorNumKeyBg = new KColorButton(customSubContainer);
    colorNumKeyBg->setFixedWidth(50);
    grid->addWidget(colorNumKeyBg, 3, 1);

    grid->addWidget(new TQLabel("Keys (+ - / *) foreground:", customSubContainer, "Keys (+ - / *) foreground:"), 4, 0);
    colorOpKeyFg = new KColorButton(customSubContainer);
    colorOpKeyFg->setFixedWidth(50);
    colorOpKeyFg->setColor(defaultOtherKeyFg);
    grid->addWidget(colorOpKeyFg, 4, 1);

    grid->addWidget(new TQLabel("Keys (+ - / *) background:", customSubContainer, "Keys (+ - / *) background:"), 5, 0);
    colorOpKeyBg = new KColorButton(customSubContainer);
    colorOpKeyBg->setFixedWidth(50);
    colorOpKeyBg->setColor(defaultOtherKeyBg);
    grid->addWidget(colorOpKeyBg, 5, 1);

    grid->addWidget(new TQLabel("Keys (CE, C, BCKSP) foreground:", customSubContainer, "Keys (CE, C, BCKSP) foreground:"), 6, 0);
    colorClearKeyFg = new KColorButton(customSubContainer);
    colorClearKeyFg->setFixedWidth(50);
    colorClearKeyFg->setColor(defaultOtherKeyFg);
    grid->addWidget(colorClearKeyFg, 6, 1);

    grid->addWidget(new TQLabel("Keys (CE, C, BCKSP) background:", customSubContainer, "Keys (CE, C, BCKSP) background:"), 7, 0);
    colorClearKeyBg = new KColorButton(customSubContainer);
    colorClearKeyBg->setFixedWidth(50);
    colorClearKeyBg->setColor(defaultOtherKeyBg);
    grid->addWidget(colorClearKeyBg, 7, 1);

    grid->addWidget(new TQLabel("Keys (Memory & specials) foreground:", customSubContainer, "Keys (Memory & specials) foreground:"), 8, 0);
    colorMemKeyFg = new KColorButton(customSubContainer);
    colorMemKeyFg->setFixedWidth(50);
    colorMemKeyFg->setColor(defaultOtherKeyFg);
    grid->addWidget(colorMemKeyFg, 8, 1);

    grid->addWidget(new TQLabel("Keys (Memory & specials) background:", customSubContainer, "Keys (Memory & specials) background:"), 9, 0);
    colorMemKeyBg = new KColorButton(customSubContainer);
    colorMemKeyBg->setFixedWidth(50);
    colorMemKeyBg->setColor(defaultOtherKeyBg);
    grid->addWidget(colorMemKeyBg, 9, 1);

    grid->addWidget(new TQLabel("Key Equal foreground:", customSubContainer, "Key Equal foreground:"), 10, 0);
    colorEqualFg = new KColorButton(customSubContainer);
    colorEqualFg->setFixedWidth(50);
    grid->addWidget(colorEqualFg, 10, 1);

    grid->addWidget(new TQLabel("Key Equal background:", customSubContainer, "Key Equal background:"), 11, 0);
    colorEqualBg = new KColorButton(customSubContainer);
    colorEqualBg->setFixedWidth(50);
    grid->addWidget(colorEqualBg, 11, 1);

    grid->addWidget(new TQLabel("Panels background color:", customSubContainer, "Panels background color:"), 12, 0);
    colorPanelBg = new KColorButton(customSubContainer);
    colorPanelBg->setFixedWidth(50);
    grid->addWidget(colorPanelBg, 12, 1);

    grid->addWidget(new TQLabel("Menu(s) background color:", customSubContainer, "Menu(s) background color:"), 13, 0);
    colorMenuBg = new KColorButton(customSubContainer);
    colorMenuBg->setFixedWidth(50);
    grid->addWidget(colorMenuBg, 13, 1);

    // Column 2 (Right): Display font, font chooser, display foreground, display background, display expression, checkboxes
    grid->addWidget(new TQLabel("Display font:", customSubContainer, "Display font:"), 0, 2);
    comboDisplayType = new TQComboBox(customSubContainer);
    comboDisplayType->insertItem("Default");
    comboDisplayType->insertItem("System");
    comboDisplayType->insertItem("Custom");
    comboDisplayType->insertItem("LCD");
    comboDisplayType->insertItem("Calculator");
    comboDisplayType->insertItem("Computo");
    comboDisplayType->insertItem("DigitalCounter7");
    comboDisplayType->insertItem("PocketCalculator");
    comboDisplayType->insertItem("Casio");
    comboDisplayType->setFixedWidth(135);
    grid->addWidget(comboDisplayType, 0, 3);
    connect(comboDisplayType, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotDisplayTypeChanged(int)));

    lblDisplayFontSel = new TQLabel("Font:", customSubContainer, "Font:");
    grid->addWidget(lblDisplayFontSel, 1, 2);
    fontDisplay = new TDEFontRequester(customSubContainer);
    grid->addWidget(fontDisplay, 1, 3);

    grid->addWidget(new TQLabel("Display foreground:", customSubContainer, "Display foreground:"), 2, 2);
    colorDisplayFg = new KColorButton(customSubContainer);
    colorDisplayFg->setFixedWidth(50);
    grid->addWidget(colorDisplayFg, 2, 3);

    grid->addWidget(new TQLabel("Display background:", customSubContainer, "Display background:"), 3, 2);
    colorDisplayBg = new KColorButton(customSubContainer);
    colorDisplayBg->setFixedWidth(50);
    grid->addWidget(colorDisplayBg, 3, 3);

    grid->addWidget(new TQLabel("Display expression:", customSubContainer, "Display expression:"), 4, 2);
    colorSmallFg = new KColorButton(customSubContainer);
    colorSmallFg->setFixedWidth(50);
    grid->addWidget(colorSmallFg, 4, 3);

    lblKeyFont = new TQLabel("Key fonts:", customSubContainer, "Key fonts:");
    grid->addWidget(lblKeyFont, 5, 2);
    comboKeyFontType = new TQComboBox(customSubContainer);
    comboKeyFontType->insertItem("Default");
    comboKeyFontType->insertItem("System");
    comboKeyFontType->insertItem("Custom");
    comboKeyFontType->setFixedWidth(135);
    grid->addWidget(comboKeyFontType, 5, 3);
    connect(comboKeyFontType, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotKeyFontTypeChanged(int)));

    lblKeyFontSel = new TQLabel("Font:", customSubContainer, "Font:");
    grid->addWidget(lblKeyFontSel, 6, 2);
    fontKey = new TDEFontRequester(customSubContainer);
    grid->addWidget(fontKey, 6, 3);

    chkNoDeco = new TQCheckBox("No window decorations", customSubContainer);
    grid->addMultiCellWidget(chkNoDeco, 7, 7, 2, 3);

    chkStandardMenuBar = new TQCheckBox("Standard menu bar", customSubContainer);
    grid->addMultiCellWidget(chkStandardMenuBar, 8, 8, 2, 3);

    lblIcons = new TQLabel("Icons:", customSubContainer, "Icons:");
    grid->addWidget(lblIcons, 9, 2);

    TQHBoxLayout *iconsLay = new TQHBoxLayout();
    comboIconMode = new TQComboBox(customSubContainer);
    comboIconMode->insertItem("Default");
    comboIconMode->insertItem("Invert");
    comboIconMode->insertItem("Color");
    comboIconMode->setFixedWidth(100);
    iconsLay->addWidget(comboIconMode);

    btnIconColor = new KColorButton(customSubContainer);
    btnIconColor->setFixedWidth(40);
    iconsLay->addWidget(btnIconColor);

    grid->addLayout(iconsLay, 9, 3);
    connect(comboIconMode, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotIconModeChanged(int)));

    chkKeysBorders = new TQCheckBox("Keys borders", customSubContainer);
    grid->addWidget(chkKeysBorders, 10, 2);
    colorKeysBorders = new KColorButton(customSubContainer);
    colorKeysBorders->setFixedWidth(50);
    grid->addWidget(colorKeysBorders, 10, 3);
    connect(chkKeysBorders, TQ_SIGNAL(toggled(bool)), colorKeysBorders, TQ_SLOT(setEnabled(bool)));

    chkWindowBorder = new TQCheckBox("Window border", customSubContainer);
    grid->addWidget(chkWindowBorder, 11, 2);
    colorWindowBorder = new KColorButton(customSubContainer);
    colorWindowBorder->setFixedWidth(50);
    grid->addWidget(colorWindowBorder, 11, 3);
    connect(chkWindowBorder, TQ_SIGNAL(toggled(bool)), colorWindowBorder, TQ_SLOT(setEnabled(bool)));

    chkDisplayBorder = new TQCheckBox("Display border", customSubContainer);
    grid->addWidget(chkDisplayBorder, 12, 2);
    colorDisplayBorder = new KColorButton(customSubContainer);
    colorDisplayBorder->setFixedWidth(50);
    grid->addWidget(colorDisplayBorder, 12, 3);
    connect(chkDisplayBorder, TQ_SIGNAL(toggled(bool)), colorDisplayBorder, TQ_SLOT(setEnabled(bool)));

    chkBoldIcons = new TQCheckBox("Bold icons", customSubContainer);
    grid->addMultiCellWidget(chkBoldIcons, 13, 13, 2, 3);

    // Save as theme and Apply buttons row
    TQWidget *buttonRowWidget = new TQWidget(prefsWidget);
    TQHBoxLayout *themeButtonsLay = new TQHBoxLayout(buttonRowWidget, 0, 6);
    themeButtonsLay->addStretch();

    btnApply = new TQPushButton("Apply", buttonRowWidget);
    connect(btnApply, TQ_SIGNAL(clicked()), TQ_SLOT(slotApplyClicked()));
    themeButtonsLay->addWidget(btnApply);

    btnSaveAsTheme = new TQPushButton("Save as theme", buttonRowWidget);
    connect(btnSaveAsTheme, TQ_SIGNAL(clicked()), TQ_SLOT(slotSaveAsThemeClicked()));
    themeButtonsLay->addWidget(btnSaveAsTheme);

    mainLay->addWidget(buttonRowWidget);

    mainLay->addSpacing(8);

    // --- SEPARATOR LINE ---
    TQFrame *sepLine = new TQFrame(prefsWidget);
    sepLine->setFrameShape(TQFrame::HLine);
    sepLine->setFrameShadow(TQFrame::Sunken);
    mainLay->addWidget(sepLine);
    mainLay->addSpacing(8);

    // --- LANGUAGE SECTION ---
    TQFrame *langHeaderFrame = new TQFrame(prefsWidget);
    langHeaderFrame->setPaletteBackgroundColor(TQColor(255, 255, 255));
    langHeaderFrame->setFrameStyle(TQFrame::NoFrame);
    TQHBoxLayout *langTitleLay = new TQHBoxLayout(langHeaderFrame, 4, 8);

    TQLabel *iconLanguage = new TQLabel(langHeaderFrame);
    iconLanguage->setPixmap(IconUtils::load(config_lang_png, config_lang_png_len, 48, 32));
    langTitleLay->addWidget(iconLanguage);

    lblLanguage = new TQLabel("Language", langHeaderFrame);
    TQFont fontLang = lblLanguage->font();
    fontLang.setBold(true);
    fontLang.setPixelSize(17);
    lblLanguage->setFont(fontLang);
    langTitleLay->addWidget(lblLanguage);
    langTitleLay->addStretch(1);

    mainLay->addWidget(langHeaderFrame);

    mainLay->addSpacing(3); // Small margin under header

    comboLanguage = new TQComboBox(prefsWidget);
    comboLanguage->insertItem("English");
    comboLanguage->insertItem("French");
    comboLanguage->insertItem("German");
    comboLanguage->insertItem("Spanish");
    comboLanguage->insertItem("Russian");
    comboLanguage->insertItem("Hindi");
    mainLay->addWidget(comboLanguage);
    connect(comboLanguage, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotLanguageChanged(int)));

    // Spacer to push everything to top
    mainLay->addStretch(1);

    // --- BOTTOM BUTTONS ---
    TQHBoxLayout *btnLay = new TQHBoxLayout(mainLay);
    btnAbout = new TQPushButton("About", prefsWidget);
    btnLay->addWidget(btnAbout);
    connect(btnAbout, TQ_SIGNAL(clicked()), TQ_SLOT(slotAboutClicked()));

    btnLay->addStretch(1);

    btnClose = new TQPushButton("Close", prefsWidget);
    btnLay->addWidget(btnClose);
        connect(btnClose, TQ_SIGNAL(clicked()), TQ_SLOT(slotCloseClicked()));

        stack->addWidget(prefsWidget, 0);
    }

    // --- ABOUT WIDGET ---
    aboutWidget = new TQWidget(stack);
    aboutWidget->setPaletteBackgroundColor(TQColor(255, 255, 255));
    TQVBoxLayout *aboutLay = new TQVBoxLayout(aboutWidget, 20, 10);
    aboutLay->addStretch(1);

    TQLabel *iconAbout = new TQLabel(aboutWidget);
    iconAbout->setPixmap(IconUtils::load(about_calc_png, about_calc_png_len, 256, 256));
    iconAbout->setAlignment(AlignHCenter);
    aboutLay->addWidget(iconAbout);

    aboutLay->addSpacing(10);

    TQHBoxLayout *centerLay = new TQHBoxLayout();
    aboutLay->addLayout(centerLay);
    centerLay->addStretch(1);
    
    TQVBoxLayout *textLay = new TQVBoxLayout();
    centerLay->addLayout(textLay);
    centerLay->addStretch(1);

    TQLabel *lblTitle = new TQLabel("CALC", aboutWidget);
    TQFont fTitle = lblTitle->font();
    fTitle.setBold(true);
    fTitle.setPixelSize(48);
    lblTitle->setFont(fTitle);
    lblTitle->setAlignment(AlignHCenter);
    textLay->addWidget(lblTitle);

    textLay->addSpacing(5);

    TQLabel *lblSubtitle = new TQLabel("A customizable calculator for Trinity DE", aboutWidget, "A customizable calculator for Trinity DE");
    TQFont fSub = lblSubtitle->font();
    fSub.setItalic(true);
    fSub.setPixelSize(20);
    lblSubtitle->setFont(fSub);
    lblSubtitle->setAlignment(AlignLeft);
    textLay->addWidget(lblSubtitle);

    textLay->addSpacing(5);

    TQLabel *lblAuthor = new TQLabel("by seb3773 - https://github.com/seb3773", aboutWidget);
    TQFont fAuthor = lblAuthor->font();
    fAuthor.setPixelSize(14);
    lblAuthor->setFont(fAuthor);
    lblAuthor->setAlignment(AlignLeft);
    textLay->addWidget(lblAuthor);

    aboutLay->addStretch(1);

    TQHBoxLayout *aboutBtnLay = new TQHBoxLayout(aboutLay);
    aboutBtnLay->addStretch(1);
    btnAboutClose = new TQPushButton("Close", aboutWidget);
    aboutBtnLay->addWidget(btnAboutClose);
    connect(btnAboutClose, TQ_SIGNAL(clicked()), TQ_SLOT(slotAboutCloseClicked()));

    stack->addWidget(aboutWidget, 1);

    if (m_startedInAboutMode) {
        stack->raiseWidget(1);
        retranslateUi();
    } else {
        stack->raiseWidget(0);
        // Load initial settings
        loadSettings();
    }
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::slotApplyClicked()
{
    saveSettings();
    emit settingsApplied();
    m_initMode = chkCustom->isChecked() ? 1 : 
                (comboTheme->currentText() == "Classic" ? 2 : 
                 (comboTheme->currentText() == "Classic Dark" ? 0 : 3));
    m_initTheme = comboTheme->currentText();
    updateApplyButtonState();
}

void SettingsDialog::slotCustomToggled(bool checked)
{
    if (checked) {
        chkTheme->blockSignals(true);
        chkTheme->setChecked(false);
        chkTheme->blockSignals(false);

        updateSubElementsEnabledState();
    } else {
        if (!chkTheme->isChecked()) {
            chkCustom->blockSignals(true);
            chkCustom->setChecked(true);
            chkCustom->blockSignals(false);
        }
    }
    updateApplyButtonState();
}

void SettingsDialog::slotThemeToggled(bool checked)
{
    if (checked) {
        chkCustom->blockSignals(true);
        chkCustom->setChecked(false);
        chkCustom->blockSignals(false);

        updateSubElementsEnabledState();
        loadThemeColors(comboTheme->currentText());
    } else {
        if (!chkCustom->isChecked()) {
            chkTheme->blockSignals(true);
            chkTheme->setChecked(true);
            chkTheme->blockSignals(false);
        }
    }
    updateApplyButtonState();
}

void SettingsDialog::slotThemeChanged(int index)
{
    (void)index;
    TQString themeName = comboTheme->currentText();
    if (chkTheme->isChecked()) {
        loadThemeColors(themeName);
    }
    updateSubElementsEnabledState();
    updateApplyButtonState();
}

void SettingsDialog::slotDisplayTypeChanged(int index)
{
    (void)index;
    updateSubElementsEnabledState();
}

void SettingsDialog::slotKeyFontTypeChanged(int index)
{
    (void)index;
    updateSubElementsEnabledState();
}

void SettingsDialog::slotIconModeChanged(int index)
{
    (void)index;
    updateSubElementsEnabledState();
}

static void setCheckboxTextColor(TQCheckBox* chk, const TQColor& activeColor, const TQColor& disabledColor)
{
    TQPalette pal = chk->palette();
    TQColor c = chk->isChecked() ? activeColor : disabledColor;
    pal.setColor(TQPalette::Active, TQColorGroup::Foreground, c);
    pal.setColor(TQPalette::Inactive, TQColorGroup::Foreground, c);
    pal.setColor(TQPalette::Active, TQColorGroup::ButtonText, c);
    pal.setColor(TQPalette::Inactive, TQColorGroup::ButtonText, c);
    chk->setPalette(pal);
}

void SettingsDialog::updateSubElementsEnabledState()
{
    bool customActive = chkCustom->isChecked();
    bool themeActive = chkTheme->isChecked();

    customSubContainer->setEnabled(customActive);
    comboTheme->setEnabled(themeActive);
    btnSaveAsTheme->setEnabled(customActive);

    TQString selectedTheme = comboTheme->currentText();
    bool isCustomTheme = (selectedTheme != "Default" && selectedTheme != "Midnight Blue" &&
                          selectedTheme != "Forest Green" && selectedTheme != "Classic Gray" &&
                          selectedTheme != "Coloured" && selectedTheme != "orange style" &&
                          selectedTheme != "Computo" && selectedTheme != "computo_new" &&
                          selectedTheme != "Classic" && selectedTheme != "Classic Dark" &&
                          !selectedTheme.isEmpty());
    btnDeleteTheme->setEnabled(themeActive && isCustomTheme);

    colorKeysBorders->setEnabled(customActive && chkKeysBorders->isChecked());
    colorWindowBorder->setEnabled(customActive && chkWindowBorder->isChecked());
    colorDisplayBorder->setEnabled(customActive && chkDisplayBorder->isChecked());
    fontDisplay->setEnabled(customActive && comboDisplayType->currentItem() == 2);
    lblDisplayFontSel->setEnabled(customActive && comboDisplayType->currentItem() == 2);

    fontKey->setEnabled(customActive && comboKeyFontType->currentItem() == 2);
    lblKeyFontSel->setEnabled(customActive && comboKeyFontType->currentItem() == 2);
    lblKeyFont->setEnabled(customActive);
    comboKeyFontType->setEnabled(customActive);

    lblIcons->setEnabled(customActive);
    comboIconMode->setEnabled(customActive);
    btnIconColor->setEnabled(customActive && comboIconMode->currentItem() == 2);

    TQPalette activePal = palette();
    TQColor activeColor = activePal.color(TQPalette::Active, TQColorGroup::Foreground);
    TQColor disabledColor = activePal.color(TQPalette::Disabled, TQColorGroup::Foreground);
    if (!disabledColor.isValid() || disabledColor == activeColor) {
        disabledColor = TQColor(128, 128, 128);
    }

    setCheckboxTextColor(chkTheme, activeColor, disabledColor);
    setCheckboxTextColor(chkCustom, activeColor, disabledColor);
}

void SettingsDialog::populateThemesCombo()
{
    comboTheme->clear();
    comboTheme->insertItem("Classic");
    comboTheme->insertItem("Classic Dark");
    comboTheme->insertItem("Midnight Blue");
    comboTheme->insertItem("Forest Green");
    comboTheme->insertItem("Classic Gray");
    comboTheme->insertItem("Coloured");
    comboTheme->insertItem("orange style");
    comboTheme->insertItem("Computo");

    TDEConfig *config = new TDEConfig("calcrc");
    TQStringList groups = config->groupList();
    for (TQStringList::Iterator it = groups.begin(); it != groups.end(); ++it) {
        if ((*it).startsWith("Theme_")) {
            TQString themeName = (*it).mid(6);
            if (themeName != "Default" && themeName != "Midnight Blue" &&
                themeName != "Forest Green" && themeName != "Classic Gray" &&
                themeName != "Coloured" && themeName != "orange style" &&
                themeName != "Computo" && themeName != "computo_new" &&
                themeName != "internal_classic" && themeName != "internal_classic_dark" &&
                themeName != "Classic" && themeName != "Classic Dark") {
                comboTheme->insertItem(themeName);
            }
        }
    }
    delete config;
}

void SettingsDialog::loadThemeColors(const TQString &themeName)
{
    TQString fixGroupName = "";
    if (themeName == "Classic Gray") {
        fixGroupName = "Theme_FIX_Classic Gray";
    } else if (themeName == "orange style" || themeName == "Orange Style") {
        fixGroupName = "Theme_FIX_Orange Style";
    } else if (themeName == "Computo") {
        fixGroupName = "Theme_FIX_Computo";
    }

    if (!fixGroupName.isEmpty()) {
        TDEConfig *config = new TDEConfig("calcrc");
        config->reparseConfiguration();
        if (config->hasGroup(fixGroupName)) {
            config->setGroup(fixGroupName);
            colorBg->setColor(config->readColorEntry("BgColor", &defaultBg));
            colorFg->setColor(config->readColorEntry("FgColor", &defaultFg));
            colorSmallFg->setColor(config->readColorEntry("SmallFgColor", &defaultSmallFg));
            colorNumKeyFg->setColor(config->readColorEntry("NumKeyFgColor", &defaultNumKeyFg));
            colorNumKeyBg->setColor(config->readColorEntry("NumKeyBgColor", &defaultNumKeyBg));
            colorOpKeyFg->setColor(config->readColorEntry("OpKeyFgColor", &defaultOtherKeyFg));
            colorOpKeyBg->setColor(config->readColorEntry("OpKeyBgColor", &defaultOtherKeyBg));
            colorClearKeyFg->setColor(config->readColorEntry("ClearKeyFgColor", &defaultOtherKeyFg));
            colorClearKeyBg->setColor(config->readColorEntry("ClearKeyBgColor", &defaultOtherKeyBg));
            colorMemKeyFg->setColor(config->readColorEntry("MemKeyFgColor", &defaultOtherKeyFg));
            colorMemKeyBg->setColor(config->readColorEntry("MemKeyBgColor", &defaultOtherKeyBg));
            colorEqualBg->setColor(config->readColorEntry("EqualBgColor", &defaultEqualBg));
            colorEqualFg->setColor(config->readColorEntry("EqualFgColor", &defaultEqualFg));
            int displayType = config->readNumEntry("DisplayType", 0);
            if (displayType == 2 && !config->hasKey("DisplayFont")) {
                displayType = 3;
            }
            comboDisplayType->setCurrentItem(displayType);
            TQFont defaultF = TQFont("Courier New", 12);
            fontDisplay->setFont(config->readFontEntry("DisplayFont", &defaultF));

            int keyFontType = config->readNumEntry("KeyFontType", 0);
            comboKeyFontType->setCurrentItem(keyFontType);
            TQFont defaultKF = font();
            fontKey->setFont(config->readFontEntry("KeyFont", &defaultKF));
            colorDisplayFg->setColor(config->readColorEntry("DisplayFgColor", &defaultDisplayFg));
            colorDisplayBg->setColor(config->readColorEntry("DisplayBgColor", &defaultDisplayBg));
            TQColor defaultMenuBgThemeVal = config->readColorEntry("PanelBgColor", &defaultPanelBg);
            colorPanelBg->setColor(defaultMenuBgThemeVal);
            colorMenuBg->setColor(config->readColorEntry("MenuBgColor", &defaultMenuBgThemeVal));
            chkNoDeco->setChecked(config->readBoolEntry("NoDeco", false));
            chkStandardMenuBar->setChecked(config->readBoolEntry("StandardMenuBar", false));
            comboIconMode->setCurrentItem(config->readNumEntry("IconMode", config->readBoolEntry("InvertIcons", false) ? 1 : 0));
            TQColor defIconColor(255, 255, 255);
            btnIconColor->setColor(config->readColorEntry("IconColor", &defIconColor));
            chkKeysBorders->setChecked(config->readBoolEntry("KeysBorders", false));
            colorKeysBorders->setColor(config->readColorEntry("KeysBordersColor", &defaultKeysBorders));
            chkWindowBorder->setChecked(config->readBoolEntry("WindowBorder", false));
            colorWindowBorder->setColor(config->readColorEntry("WindowBorderColor", &defaultWindowBorder));
            chkDisplayBorder->setChecked(config->readBoolEntry("DisplayBorder", false));
            colorDisplayBorder->setColor(config->readColorEntry("DisplayBorderColor", &defaultDisplayBorder));
            chkBoldIcons->setChecked(config->readBoolEntry("BoldIcons", false));
            delete config;
            return;
        }
        delete config;
    }

    if (themeName == "Classic Dark" || themeName == "Dark Mode" || themeName == "internal_classic_dark") {
        colorBg->setColor(TQColor(0, 0, 0));
        colorFg->setColor(TQColor(255, 255, 255));
        colorSmallFg->setColor(TQColor(222, 222, 222));
        colorNumKeyFg->setColor(TQColor(255, 255, 255));
        colorNumKeyBg->setColor(TQColor(0, 0, 0));
        colorOpKeyFg->setColor(TQColor(255, 255, 255));
        colorOpKeyBg->setColor(TQColor(0, 0, 0));
        colorClearKeyFg->setColor(TQColor(255, 255, 255));
        colorClearKeyBg->setColor(TQColor(0, 0, 0));
        colorMemKeyFg->setColor(TQColor(255, 255, 255));
        colorMemKeyBg->setColor(TQColor(0, 0, 0));
        colorEqualBg->setColor(TQColor(0, 103, 192));
        colorEqualFg->setColor(TQColor(255, 255, 255));
        comboDisplayType->setCurrentItem(0);
        fontDisplay->setFont(font());
        comboKeyFontType->setCurrentItem(0);
        fontKey->setFont(font());
        colorDisplayFg->setColor(TQColor(255, 255, 255));
        colorDisplayBg->setColor(TQColor(0, 0, 0));
        colorPanelBg->setColor(TQColor(62, 62, 62));
        colorMenuBg->setColor(TQColor(44, 44, 44));
        chkNoDeco->setChecked(false);
        chkStandardMenuBar->setChecked(false);
        comboIconMode->setCurrentItem(1);
        btnIconColor->setColor(TQColor(255, 255, 255));
        chkKeysBorders->setChecked(false);
        colorKeysBorders->setColor(TQColor(0, 0, 0));
        chkWindowBorder->setChecked(false);
        colorWindowBorder->setColor(TQColor(0, 0, 0));
        chkDisplayBorder->setChecked(false);
        colorDisplayBorder->setColor(TQColor(0, 0, 0));
        chkBoldIcons->setChecked(false);
    } else if (themeName == "Classic" || themeName == "Default" || themeName == "internal_classic") {
        colorBg->setColor(TQColor(242, 242, 242));
        colorFg->setColor(TQColor(0, 0, 0));
        colorSmallFg->setColor(TQColor(71, 71, 71));
        colorNumKeyFg->setColor(TQColor(0, 0, 0));
        colorNumKeyBg->setColor(TQColor(255, 255, 255));
        colorOpKeyFg->setColor(TQColor(0, 0, 0));
        colorOpKeyBg->setColor(TQColor(242, 242, 242));
        colorClearKeyFg->setColor(TQColor(0, 0, 0));
        colorClearKeyBg->setColor(TQColor(242, 242, 242));
        colorMemKeyFg->setColor(TQColor(0, 0, 0));
        colorMemKeyBg->setColor(TQColor(242, 242, 242));
        colorEqualBg->setColor(TQColor(0, 103, 192));
        colorEqualFg->setColor(TQColor(255, 255, 255));
        comboDisplayType->setCurrentItem(0);
        fontDisplay->setFont(font());
        comboKeyFontType->setCurrentItem(0);
        fontKey->setFont(font());
        colorDisplayFg->setColor(TQColor(0, 0, 0));
        colorDisplayBg->setColor(TQColor(242, 242, 242));
        colorPanelBg->setColor(TQColor(231, 231, 231));
        colorMenuBg->setColor(TQColor(230, 230, 230));
        chkNoDeco->setChecked(false);
        chkStandardMenuBar->setChecked(false);
        comboIconMode->setCurrentItem(0);
        btnIconColor->setColor(TQColor(255, 255, 255));
        chkKeysBorders->setChecked(false);
        colorKeysBorders->setColor(TQColor(0, 0, 0));
        chkWindowBorder->setChecked(false);
        colorWindowBorder->setColor(TQColor(0, 0, 0));
        chkDisplayBorder->setChecked(false);
        colorDisplayBorder->setColor(TQColor(0, 0, 0));
        chkBoldIcons->setChecked(false);
    } else if (themeName == "Midnight Blue") {
        colorBg->setColor(TQColor(26, 37, 54));
        colorFg->setColor(TQColor(255, 255, 255));
        colorSmallFg->setColor(TQColor(160, 170, 181));
        colorNumKeyFg->setColor(TQColor(255, 255, 255));
        colorNumKeyBg->setColor(TQColor(46, 61, 82));
        colorOpKeyFg->setColor(TQColor(255, 255, 255));
        colorOpKeyBg->setColor(TQColor(35, 47, 66));
        colorClearKeyFg->setColor(TQColor(255, 255, 255));
        colorClearKeyBg->setColor(TQColor(35, 47, 66));
        colorMemKeyFg->setColor(TQColor(255, 255, 255));
        colorMemKeyBg->setColor(TQColor(35, 47, 66));
        colorEqualBg->setColor(TQColor(74, 144, 226));
        colorEqualFg->setColor(TQColor(255, 255, 255));
        comboDisplayType->setCurrentItem(3);
        fontDisplay->setFont(TQFont("Segoe Calc"));
        comboKeyFontType->setCurrentItem(0);
        fontKey->setFont(TQFont("Segoe Calc"));
        colorDisplayFg->setColor(TQColor(0, 255, 204));
        colorDisplayBg->setColor(TQColor(13, 21, 32));
        colorPanelBg->setColor(TQColor(46, 61, 82));
        colorMenuBg->setColor(TQColor(46, 61, 82));
        chkNoDeco->setChecked(false);
        chkStandardMenuBar->setChecked(false);
        comboIconMode->setCurrentItem(1);
        btnIconColor->setColor(TQColor(255, 255, 255));
        chkKeysBorders->setChecked(false);
        colorKeysBorders->setColor(TQColor(0, 0, 0));
        chkWindowBorder->setChecked(false);
        colorWindowBorder->setColor(TQColor(0, 0, 0));
        chkDisplayBorder->setChecked(false);
        colorDisplayBorder->setColor(TQColor(0, 0, 0));
        chkBoldIcons->setChecked(false);
    } else if (themeName == "Forest Green") {
        colorBg->setColor(TQColor(28, 46, 36));
        colorFg->setColor(TQColor(230, 242, 235));
        colorSmallFg->setColor(TQColor(141, 163, 151));
        colorNumKeyFg->setColor(TQColor(230, 242, 235));
        colorNumKeyBg->setColor(TQColor(42, 66, 53));
        colorOpKeyFg->setColor(TQColor(230, 242, 235));
        colorOpKeyBg->setColor(TQColor(33, 54, 42));
        colorClearKeyFg->setColor(TQColor(230, 242, 235));
        colorClearKeyBg->setColor(TQColor(33, 54, 42));
        colorMemKeyFg->setColor(TQColor(230, 242, 235));
        colorMemKeyBg->setColor(TQColor(33, 54, 42));
        colorEqualBg->setColor(TQColor(46, 139, 87));
        colorEqualFg->setColor(TQColor(255, 255, 255));
        comboDisplayType->setCurrentItem(7);
        fontDisplay->setFont(TQFont("Pocket Calculator"));
        comboKeyFontType->setCurrentItem(0);
        fontKey->setFont(TQFont("Segoe Calc", 12));
        colorDisplayFg->setColor(TQColor(57, 255, 20));
        colorDisplayBg->setColor(TQColor(11, 19, 14));
        colorPanelBg->setColor(TQColor(42, 66, 53));
        colorMenuBg->setColor(TQColor(42, 66, 53));
        chkNoDeco->setChecked(false);
        chkStandardMenuBar->setChecked(false);
        comboIconMode->setCurrentItem(1);
        btnIconColor->setColor(TQColor(255, 255, 255));
        chkKeysBorders->setChecked(false);
        colorKeysBorders->setColor(TQColor(0, 0, 0));
        chkWindowBorder->setChecked(false);
        colorWindowBorder->setColor(TQColor(0, 0, 0));
        chkDisplayBorder->setChecked(false);
        colorDisplayBorder->setColor(TQColor(0, 0, 0));
        chkBoldIcons->setChecked(false);
    } else if (themeName == "Classic Gray") {
        colorBg->setColor(TQColor(204, 204, 204));
        colorFg->setColor(TQColor(0, 0, 0));
        colorSmallFg->setColor(TQColor(203, 203, 203));
        colorNumKeyFg->setColor(TQColor(0, 0, 0));
        colorNumKeyBg->setColor(TQColor(221, 221, 221));
        colorOpKeyFg->setColor(TQColor(0, 0, 0));
        colorOpKeyBg->setColor(TQColor(192, 192, 192));
        colorClearKeyFg->setColor(TQColor(0, 0, 0));
        colorClearKeyBg->setColor(TQColor(192, 192, 192));
        colorMemKeyFg->setColor(TQColor(0, 0, 0));
        colorMemKeyBg->setColor(TQColor(192, 192, 192));
        colorEqualBg->setColor(TQColor(128, 128, 128));
        colorEqualFg->setColor(TQColor(255, 255, 255));
        comboDisplayType->setCurrentItem(8);
        fontDisplay->setFont(TQFont("ClassWiz Math CW"));
        comboKeyFontType->setCurrentItem(0);
        fontKey->setFont(TQFont("Segoe Calc", 10));
        colorDisplayFg->setColor(TQColor(242, 242, 242));
        colorDisplayBg->setColor(TQColor(126, 126, 126));
        colorPanelBg->setColor(TQColor(133, 133, 133));
        colorMenuBg->setColor(TQColor(221, 221, 221));
        chkNoDeco->setChecked(false);
        chkStandardMenuBar->setChecked(false);
        comboIconMode->setCurrentItem(0);
        btnIconColor->setColor(TQColor(255, 255, 255));
        chkKeysBorders->setChecked(false);
        colorKeysBorders->setColor(TQColor(0, 0, 0));
        chkWindowBorder->setChecked(false);
        colorWindowBorder->setColor(TQColor(0, 0, 0));
        chkDisplayBorder->setChecked(false);
        colorDisplayBorder->setColor(TQColor(0, 0, 0));
        chkBoldIcons->setChecked(true);
    } else if (themeName == "Coloured") {
        colorBg->setColor(TQColor(93, 115, 215));
        colorFg->setColor(TQColor(255, 255, 255));
        colorSmallFg->setColor(TQColor(130, 228, 255));
        colorNumKeyFg->setColor(TQColor(255, 255, 255));
        colorNumKeyBg->setColor(TQColor(19, 121, 16));
        colorOpKeyFg->setColor(TQColor(255, 255, 255));
        colorOpKeyBg->setColor(TQColor(95, 160, 51));
        colorClearKeyFg->setColor(TQColor(255, 255, 255));
        colorClearKeyBg->setColor(TQColor(176, 32, 34));
        colorMemKeyFg->setColor(TQColor(255, 255, 255));
        colorMemKeyBg->setColor(TQColor(26, 56, 115));
        colorEqualBg->setColor(TQColor(210, 188, 45));
        colorEqualFg->setColor(TQColor(255, 255, 255));
        comboDisplayType->setCurrentItem(6);
        fontDisplay->setFont(TQFont("Digital Counter 7"));
        comboKeyFontType->setCurrentItem(0);
        fontKey->setFont(TQFont("Segoe Calc", 12));
        colorDisplayFg->setColor(TQColor(255, 255, 255));
        colorDisplayBg->setColor(TQColor(11, 19, 14));
        colorPanelBg->setColor(TQColor(87, 108, 201));
        colorMenuBg->setColor(TQColor(36, 77, 110));
        chkNoDeco->setChecked(false);
        chkStandardMenuBar->setChecked(false);
        comboIconMode->setCurrentItem(2);
        btnIconColor->setColor(TQColor(255, 252, 62));
        chkKeysBorders->setChecked(true);
        colorKeysBorders->setColor(TQColor(84, 104, 194));
        chkWindowBorder->setChecked(true);
        colorWindowBorder->setColor(TQColor(96, 119, 222));
        chkDisplayBorder->setChecked(true);
        colorDisplayBorder->setColor(TQColor(46, 46, 46));
        chkBoldIcons->setChecked(true);
    } else if (themeName == "orange style") {
        colorBg->setColor(TQColor(181, 111, 25));
        colorFg->setColor(TQColor(230, 242, 235));
        colorSmallFg->setColor(TQColor(201, 119, 18));
        colorNumKeyFg->setColor(TQColor(0, 0, 0));
        colorNumKeyBg->setColor(TQColor(249, 152, 34));
        colorOpKeyFg->setColor(TQColor(0, 0, 0));
        colorOpKeyBg->setColor(TQColor(224, 137, 31));
        colorClearKeyFg->setColor(TQColor(0, 0, 0));
        colorClearKeyBg->setColor(TQColor(165, 101, 23));
        colorMemKeyFg->setColor(TQColor(255, 255, 255));
        colorMemKeyBg->setColor(TQColor(176, 75, 17));
        colorEqualBg->setColor(TQColor(156, 59, 24));
        colorEqualFg->setColor(TQColor(255, 255, 255));
        comboDisplayType->setCurrentItem(6);
        fontDisplay->setFont(TQFont("Digital Counter 7"));
        comboKeyFontType->setCurrentItem(0);
        fontKey->setFont(TQFont("Segoe Calc", 12));
        colorDisplayFg->setColor(TQColor(253, 139, 63));
        colorDisplayBg->setColor(TQColor(44, 22, 16));
        colorPanelBg->setColor(TQColor(110, 52, 41));
        colorMenuBg->setColor(TQColor(181, 111, 25));
        chkNoDeco->setChecked(true);
        chkStandardMenuBar->setChecked(false);
        comboIconMode->setCurrentItem(1);
        btnIconColor->setColor(TQColor(255, 255, 255));
        chkKeysBorders->setChecked(false);
        colorKeysBorders->setColor(TQColor(0, 0, 0));
        chkWindowBorder->setChecked(true);
        colorWindowBorder->setColor(TQColor(124, 79, 0));
        chkDisplayBorder->setChecked(false);
        colorDisplayBorder->setColor(TQColor(0, 0, 0));
        chkBoldIcons->setChecked(false);
    } else if (themeName == "Computo") {
        colorBg->setColor(TQColor(28, 46, 36));
        colorFg->setColor(TQColor(230, 242, 235));
        colorSmallFg->setColor(TQColor(30, 137, 11));
        colorNumKeyFg->setColor(TQColor(230, 242, 235));
        colorNumKeyBg->setColor(TQColor(42, 66, 53));
        colorOpKeyFg->setColor(TQColor(230, 242, 235));
        colorOpKeyBg->setColor(TQColor(33, 54, 42));
        colorClearKeyFg->setColor(TQColor(230, 242, 235));
        colorClearKeyBg->setColor(TQColor(33, 54, 42));
        colorMemKeyFg->setColor(TQColor(230, 242, 235));
        colorMemKeyBg->setColor(TQColor(33, 54, 42));
        colorEqualBg->setColor(TQColor(46, 139, 87));
        colorEqualFg->setColor(TQColor(255, 255, 255));
        comboDisplayType->setCurrentItem(5);
        fontDisplay->setFont(TQFont("Computo Monospace"));
        comboKeyFontType->setCurrentItem(0);
        fontKey->setFont(TQFont("Segoe UI", 12));
        colorDisplayFg->setColor(TQColor(57, 255, 20));
        colorDisplayBg->setColor(TQColor(11, 19, 14));
        colorPanelBg->setColor(TQColor(42, 66, 53));
        colorMenuBg->setColor(TQColor(42, 66, 53));
        chkNoDeco->setChecked(true);
        chkStandardMenuBar->setChecked(false);
        comboIconMode->setCurrentItem(1);
        btnIconColor->setColor(TQColor(255, 255, 255));
        chkKeysBorders->setChecked(false);
        colorKeysBorders->setColor(TQColor(0, 0, 0));
        chkWindowBorder->setChecked(false);
        colorWindowBorder->setColor(TQColor(0, 0, 0));
        chkDisplayBorder->setChecked(false);
        colorDisplayBorder->setColor(TQColor(0, 0, 0));
        chkBoldIcons->setChecked(false);
    } else {
        TDEConfig *config = new TDEConfig("calcrc");
        config->setGroup("Theme_" + themeName);
        colorBg->setColor(config->readColorEntry("BgColor", &defaultBg));
        colorFg->setColor(config->readColorEntry("FgColor", &defaultFg));
        colorSmallFg->setColor(config->readColorEntry("SmallFgColor", &defaultSmallFg));
        colorNumKeyFg->setColor(config->readColorEntry("NumKeyFgColor", &defaultNumKeyFg));
        colorNumKeyBg->setColor(config->readColorEntry("NumKeyBgColor", &defaultNumKeyBg));
        colorOpKeyFg->setColor(config->readColorEntry("OpKeyFgColor", &defaultOtherKeyFg));
        colorOpKeyBg->setColor(config->readColorEntry("OpKeyBgColor", &defaultOtherKeyBg));
        colorClearKeyFg->setColor(config->readColorEntry("ClearKeyFgColor", &defaultOtherKeyFg));
        colorClearKeyBg->setColor(config->readColorEntry("ClearKeyBgColor", &defaultOtherKeyBg));
        colorMemKeyFg->setColor(config->readColorEntry("MemKeyFgColor", &defaultOtherKeyFg));
        colorMemKeyBg->setColor(config->readColorEntry("MemKeyBgColor", &defaultOtherKeyBg));
        colorEqualBg->setColor(config->readColorEntry("EqualBgColor", &defaultEqualBg));
        colorEqualFg->setColor(config->readColorEntry("EqualFgColor", &defaultEqualFg));
        int displayType = config->readNumEntry("DisplayType", 0);
        if (displayType == 2 && !config->hasKey("DisplayFont")) {
            displayType = 3; // LCD compatibility
        }
        comboDisplayType->setCurrentItem(displayType);
        TQFont defaultF = TQFont("Courier New", 12);
        fontDisplay->setFont(config->readFontEntry("DisplayFont", &defaultF));

        int keyFontType = config->readNumEntry("KeyFontType", 0);
        comboKeyFontType->setCurrentItem(keyFontType);
        TQFont defaultKF = font();
        fontKey->setFont(config->readFontEntry("KeyFont", &defaultKF));
        colorDisplayFg->setColor(config->readColorEntry("DisplayFgColor", &defaultDisplayFg));
        colorDisplayBg->setColor(config->readColorEntry("DisplayBgColor", &defaultDisplayBg));
        TQColor defaultMenuBgThemeVal = config->readColorEntry("PanelBgColor", &defaultPanelBg);
        colorPanelBg->setColor(defaultMenuBgThemeVal);
        colorMenuBg->setColor(config->readColorEntry("MenuBgColor", &defaultMenuBgThemeVal));
        chkNoDeco->setChecked(config->readBoolEntry("NoDeco", false));
        chkStandardMenuBar->setChecked(config->readBoolEntry("StandardMenuBar", false));
        comboIconMode->setCurrentItem(config->readNumEntry("IconMode", config->readBoolEntry("InvertIcons", false) ? 1 : 0));
        TQColor defIconColor(255, 255, 255);
        btnIconColor->setColor(config->readColorEntry("IconColor", &defIconColor));
        chkKeysBorders->setChecked(config->readBoolEntry("KeysBorders", false));
        colorKeysBorders->setColor(config->readColorEntry("KeysBordersColor", &defaultKeysBorders));
        chkWindowBorder->setChecked(config->readBoolEntry("WindowBorder", false));
        colorWindowBorder->setColor(config->readColorEntry("WindowBorderColor", &defaultWindowBorder));
        chkDisplayBorder->setChecked(config->readBoolEntry("DisplayBorder", false));
        colorDisplayBorder->setColor(config->readColorEntry("DisplayBorderColor", &defaultDisplayBorder));
        chkBoldIcons->setChecked(config->readBoolEntry("BoldIcons", false));
        delete config;
    }
}

void SettingsDialog::slotSaveAsThemeClicked()
{
    bool ok;
    TQString name = KInputDialog::text(
        TQString("Save Theme"),
        TQString("Enter a name for the new theme:"),
        "",
        &ok,
        this
    );
    if (ok && !name.isEmpty()) {
        TDEConfig *config = new TDEConfig("calcrc");
        config->setGroup("Theme_" + name);
        config->writeEntry("BgColor", colorBg->color());
        config->writeEntry("FgColor", colorFg->color());
        config->writeEntry("SmallFgColor", colorSmallFg->color());
        config->writeEntry("NumKeyFgColor", colorNumKeyFg->color());
        config->writeEntry("NumKeyBgColor", colorNumKeyBg->color());
        config->writeEntry("OpKeyFgColor", colorOpKeyFg->color());
        config->writeEntry("OpKeyBgColor", colorOpKeyBg->color());
        config->writeEntry("ClearKeyFgColor", colorClearKeyFg->color());
        config->writeEntry("ClearKeyBgColor", colorClearKeyBg->color());
        config->writeEntry("MemKeyFgColor", colorMemKeyFg->color());
        config->writeEntry("MemKeyBgColor", colorMemKeyBg->color());
        config->writeEntry("EqualBgColor", colorEqualBg->color());
        config->writeEntry("EqualFgColor", colorEqualFg->color());
        config->writeEntry("DisplayType", comboDisplayType->currentItem());
        config->writeEntry("KeyFontType", comboKeyFontType->currentItem());
        config->writeEntry("KeyFont", fontKey->font());
        config->writeEntry("DisplayFgColor", colorDisplayFg->color());
        config->writeEntry("DisplayBgColor", colorDisplayBg->color());
        config->writeEntry("PanelBgColor", colorPanelBg->color());
        config->writeEntry("MenuBgColor", colorMenuBg->color());
        config->writeEntry("NoDeco", chkNoDeco->isChecked());
        config->writeEntry("StandardMenuBar", chkStandardMenuBar->isChecked());
        config->writeEntry("IconMode", comboIconMode->currentItem());
        config->writeEntry("IconColor", btnIconColor->color());
        config->writeEntry("KeysBorders", chkKeysBorders->isChecked());
        config->writeEntry("KeysBordersColor", colorKeysBorders->color());
        config->writeEntry("WindowBorder", chkWindowBorder->isChecked());
        config->writeEntry("WindowBorderColor", colorWindowBorder->color());
        config->writeEntry("DisplayBorder", chkDisplayBorder->isChecked());
        config->writeEntry("DisplayBorderColor", colorDisplayBorder->color());
        config->writeEntry("BoldIcons", chkBoldIcons->isChecked());
        config->sync();
        delete config;

        populateThemesCombo();
        int idx = findComboText(comboTheme, name);
        if (idx >= 0) {
            comboTheme->setCurrentItem(idx);
        }
    }
}

void SettingsDialog::slotDeleteThemeClicked()
{
    TQString selectedTheme = comboTheme->currentText();
    if (selectedTheme.isEmpty() ||
        selectedTheme == "Default" ||
        selectedTheme == "Midnight Blue" ||
        selectedTheme == "Forest Green" ||
        selectedTheme == "Classic Gray" ||
        selectedTheme == "Coloured" ||
        selectedTheme == "orange style" ||
        selectedTheme == "Computo" ||
        selectedTheme == "computo_new" ||
        selectedTheme == "Classic" ||
        selectedTheme == "Classic Dark") {
        return;
    }

    int result = KMessageBox::warningContinueCancel(
        this,
        TQString("Are you sure you want to delete the theme '%1'?").arg(selectedTheme),
        TQString("Delete Theme")
    );
    if (result != KMessageBox::Continue) {
        return;
    }

    TDEConfig *config = new TDEConfig("calcrc");
    config->deleteGroup("Theme_" + selectedTheme);
    config->sync();
    delete config;

    populateThemesCombo();
    comboTheme->setCurrentItem(0);
    loadThemeColors(comboTheme->currentText());
    updateSubElementsEnabledState();
}

void SettingsDialog::slotAboutClicked()
{
    stack->raiseWidget(1);
    setCaption("Calculator - About");
}

void SettingsDialog::slotAboutCloseClicked()
{
    if (m_startedInAboutMode) {
        accept();
    } else {
        stack->raiseWidget(0);
        setCaption("Calculator - settings");
    }
}

void SettingsDialog::slotCloseClicked()
{
    saveSettings();
    accept();
}

void SettingsDialog::loadSettings()
{
    TDEConfig *config = new TDEConfig("calcrc");
    config->setGroup("Preferences");

    int mode = config->readNumEntry("AppearanceMode", 2); // default to Theme
    TQString selectedTheme = config->readEntry("SelectedTheme", "Midnight Blue");

    colorBg->setColor(config->readColorEntry("CustomBgColor", &defaultBg));
    colorFg->setColor(config->readColorEntry("CustomFgColor", &defaultFg));
    colorSmallFg->setColor(config->readColorEntry("CustomSmallFgColor", &defaultSmallFg));
    colorNumKeyFg->setColor(config->readColorEntry("CustomNumKeyFgColor", &defaultNumKeyFg));
    colorNumKeyBg->setColor(config->readColorEntry("CustomNumKeyBgColor", &defaultNumKeyBg));
    colorOpKeyFg->setColor(config->readColorEntry("CustomOpKeyFgColor", &defaultOtherKeyFg));
    colorOpKeyBg->setColor(config->readColorEntry("CustomOpKeyBgColor", &defaultOtherKeyBg));
    colorClearKeyFg->setColor(config->readColorEntry("CustomClearKeyFgColor", &defaultOtherKeyFg));
    colorClearKeyBg->setColor(config->readColorEntry("CustomClearKeyBgColor", &defaultOtherKeyBg));
    colorMemKeyFg->setColor(config->readColorEntry("CustomMemKeyFgColor", &defaultOtherKeyFg));
    colorMemKeyBg->setColor(config->readColorEntry("CustomMemKeyBgColor", &defaultOtherKeyBg));
    colorEqualBg->setColor(config->readColorEntry("CustomEqualBgColor", &defaultEqualBg));
    colorEqualFg->setColor(config->readColorEntry("CustomEqualFgColor", &defaultEqualFg));
    int displayType = config->readNumEntry("CustomDisplayType", 0);
    if (displayType == 2 && !config->hasKey("CustomDisplayFont")) {
        displayType = 3; // LCD compatibility
    }
    comboDisplayType->setCurrentItem(displayType);
    TQFont defaultF = TQFont("Courier New", 12);
    fontDisplay->setFont(config->readFontEntry("CustomDisplayFont", &defaultF));

    int keyFontType = config->readNumEntry("CustomKeyFontType", 0);
    comboKeyFontType->setCurrentItem(keyFontType);
    TQFont defaultKF = font();
    fontKey->setFont(config->readFontEntry("CustomKeyFont", &defaultKF));
    colorDisplayFg->setColor(config->readColorEntry("CustomDisplayFgColor", &defaultDisplayFg));
    colorDisplayBg->setColor(config->readColorEntry("CustomDisplayBgColor", &defaultDisplayBg));
    TQColor defaultMenuBgCustomVal = config->readColorEntry("CustomPanelBgColor", &defaultPanelBg);
    colorPanelBg->setColor(defaultMenuBgCustomVal);
    colorMenuBg->setColor(config->readColorEntry("CustomMenuBgColor", &defaultMenuBgCustomVal));
    chkNoDeco->setChecked(config->readBoolEntry("CustomNoDeco", false));
    chkStandardMenuBar->setChecked(config->readBoolEntry("CustomStandardMenuBar", false));
    comboIconMode->setCurrentItem(config->readNumEntry("CustomIconMode", config->readBoolEntry("CustomInvertIcons", false) ? 1 : 0));
    TQColor defIconColor(255, 255, 255);
    btnIconColor->setColor(config->readColorEntry("CustomIconColor", &defIconColor));
    chkKeysBorders->setChecked(config->readBoolEntry("CustomKeysBorders", false));
    colorKeysBorders->setColor(config->readColorEntry("CustomKeysBordersColor", &defaultKeysBorders));
    chkWindowBorder->setChecked(config->readBoolEntry("CustomWindowBorder", false));
    colorWindowBorder->setColor(config->readColorEntry("CustomWindowBorderColor", &defaultWindowBorder));
    chkDisplayBorder->setChecked(config->readBoolEntry("CustomDisplayBorder", false));
    colorDisplayBorder->setColor(config->readColorEntry("CustomDisplayBorderColor", &defaultDisplayBorder));
    chkBoldIcons->setChecked(config->readBoolEntry("CustomBoldIcons", false));

    TQString lang = config->readEntry("Language", "");
    int langIdx = 0;
    if (lang.isEmpty()) {
        CalcLang cur = Translation::lang();
        if (cur == LangFrench) langIdx = 1;
        else if (cur == LangGerman) langIdx = 2;
        else if (cur == LangSpanish) langIdx = 3;
        else if (cur == LangRussian) langIdx = 4;
        else if (cur == LangHindi) langIdx = 5;
        else langIdx = 0;
    } else {
        if (lang == "fr") langIdx = 1;
        else if (lang == "german") langIdx = 2;
        else if (lang == "spanish") langIdx = 3;
        else if (lang == "russian") langIdx = 4;
        else if (lang == "indi") langIdx = 5;
    }
    comboLanguage->setCurrentItem(langIdx);

    delete config;

    populateThemesCombo();

    if (mode == 0) {
        selectedTheme = "Classic Dark";
    } else if (mode == 2) {
        selectedTheme = "Classic";
    }

    int themeIdx = findComboText(comboTheme, selectedTheme);
    if (themeIdx >= 0) {
        comboTheme->setCurrentItem(themeIdx);
    } else {
        comboTheme->setCurrentItem(0);
    }

    chkCustom->setChecked(mode == 1);
    chkTheme->setChecked(mode != 1);

    updateSubElementsEnabledState();

    if (mode != 1) {
        loadThemeColors(comboTheme->currentText());
    }

    m_initMode = mode;
    m_initTheme = selectedTheme;
    updateApplyButtonState();

    slotLanguageChanged(langIdx);
}

void SettingsDialog::saveSettings()
{
    TDEConfig *config = new TDEConfig("calcrc");
    config->setGroup("Preferences");

    int mode = 2; // Default to Classic
    if (chkCustom->isChecked()) {
        mode = 1;
    } else if (chkTheme->isChecked()) {
        TQString currentTheme = comboTheme->currentText();
        if (currentTheme == "Classic") {
            mode = 2;
        } else if (currentTheme == "Classic Dark") {
            mode = 0;
        } else {
            mode = 3;
        }
    }

    config->writeEntry("AppearanceMode", mode);
    config->writeEntry("SelectedTheme", comboTheme->currentText());

    if (mode == 1) {
        config->writeEntry("CustomBgColor", colorBg->color());
        config->writeEntry("CustomFgColor", colorFg->color());
        config->writeEntry("CustomSmallFgColor", colorSmallFg->color());
        config->writeEntry("CustomNumKeyFgColor", colorNumKeyFg->color());
        config->writeEntry("CustomNumKeyBgColor", colorNumKeyBg->color());
        config->writeEntry("CustomOpKeyFgColor", colorOpKeyFg->color());
        config->writeEntry("CustomOpKeyBgColor", colorOpKeyBg->color());
        config->writeEntry("CustomClearKeyFgColor", colorClearKeyFg->color());
        config->writeEntry("CustomClearKeyBgColor", colorClearKeyBg->color());
        config->writeEntry("CustomMemKeyFgColor", colorMemKeyFg->color());
        config->writeEntry("CustomMemKeyBgColor", colorMemKeyBg->color());
        config->writeEntry("CustomEqualBgColor", colorEqualBg->color());
        config->writeEntry("CustomEqualFgColor", colorEqualFg->color());
        config->writeEntry("CustomDisplayType", comboDisplayType->currentItem());
        config->writeEntry("CustomDisplayFont", fontDisplay->font());
        config->writeEntry("CustomKeyFontType", comboKeyFontType->currentItem());
        config->writeEntry("CustomKeyFont", fontKey->font());
        config->writeEntry("CustomDisplayFgColor", colorDisplayFg->color());
        config->writeEntry("CustomDisplayBgColor", colorDisplayBg->color());
        config->writeEntry("CustomPanelBgColor", colorPanelBg->color());
        config->writeEntry("CustomMenuBgColor", colorMenuBg->color());
        config->writeEntry("CustomNoDeco", chkNoDeco->isChecked());
        config->writeEntry("CustomStandardMenuBar", chkStandardMenuBar->isChecked());
        config->writeEntry("CustomIconMode", comboIconMode->currentItem());
        config->writeEntry("CustomIconColor", btnIconColor->color());
        config->writeEntry("CustomKeysBorders", chkKeysBorders->isChecked());
        config->writeEntry("CustomKeysBordersColor", colorKeysBorders->color());
        config->writeEntry("CustomWindowBorder", chkWindowBorder->isChecked());
        config->writeEntry("CustomWindowBorderColor", colorWindowBorder->color());
        config->writeEntry("CustomDisplayBorder", chkDisplayBorder->isChecked());
        config->writeEntry("CustomDisplayBorderColor", colorDisplayBorder->color());
        config->writeEntry("CustomBoldIcons", chkBoldIcons->isChecked());
    }

    int langIdx = comboLanguage->currentItem();
    TQString lang = "eng";
    if (langIdx == 1) lang = "fr";
    else if (langIdx == 2) lang = "german";
    else if (langIdx == 3) lang = "spanish";
    else if (langIdx == 4) lang = "russian";
    else if (langIdx == 5) lang = "indi";
    config->writeEntry("Language", lang);

    config->sync();
    delete config;
}

void SettingsDialog::slotLanguageChanged(int index)
{
    CalcLang lang = LangEnglish;
    if (index == 1) lang = LangFrench;
    else if (index == 2) lang = LangGerman;
    else if (index == 3) lang = LangSpanish;
    else if (index == 4) lang = LangRussian;
    else if (index == 5) lang = LangHindi;

    Translation::setLang(lang);
    retranslateUi();
}

void SettingsDialog::retranslateUi()
{
    // 1. Caption (title)
    if (m_startedInAboutMode) {
        setCaption(tr_str("Calculator - About"));
    } else {
        setCaption(tr_str("Calculator - settings"));
    }

    // 2. Section Headers
    if (lblAppearance) lblAppearance->setText(tr_str("Appearance"));
    if (lblLanguage) lblLanguage->setText(tr_str("Language"));

    // 3. Mutually exclusive appearance checkboxes
    chkCustom->setText(tr_str("Custom"));
    chkTheme->setText(tr_str("Theme :"));

    // 4. Custom mode appearance labels (find and translate dynamically)
    TQObjectList *lblList = queryList("TQLabel");
    if (lblList) {
        TQObjectListIt it(*lblList);
        TQObject *obj;
        while ((obj = it.current()) != 0) {
            ++it;
            TQLabel *lbl = static_cast<TQLabel*>(obj);
            TQString n = lbl->name();
            if (!n.isEmpty() && n != "unnamed" && n != "qt_heading") {
                lbl->setText(tr_str(n.latin1()));
            }
        }
        delete lblList;
    }

    comboKeyFontType->blockSignals(true);
    int curKeyF = comboKeyFontType->currentItem();
    comboKeyFontType->clear();
    comboKeyFontType->insertItem(tr_str("Default"));
    comboKeyFontType->insertItem(tr_str("System"));
    comboKeyFontType->insertItem(tr_str("Custom"));
    comboKeyFontType->setCurrentItem(curKeyF);
    comboKeyFontType->blockSignals(false);

     comboDisplayType->blockSignals(true);
     int curDisp = comboDisplayType->currentItem();
     comboDisplayType->clear();
     comboDisplayType->insertItem(tr_str("Default"));
     comboDisplayType->insertItem(tr_str("System"));
     comboDisplayType->insertItem(tr_str("Custom"));
     comboDisplayType->insertItem(tr_str("LCD"));
     comboDisplayType->insertItem("Calculator");
     comboDisplayType->insertItem("Computo");
     comboDisplayType->insertItem("DigitalCounter7");
     comboDisplayType->insertItem("PocketCalculator");
     comboDisplayType->insertItem("Casio");
     comboDisplayType->setCurrentItem(curDisp);
     comboDisplayType->blockSignals(false);

    // 6. Custom checkboxes
    chkNoDeco->setText(tr_str("No window decorations"));
    chkStandardMenuBar->setText(tr_str("Standard menu bar"));
    lblIcons->setText(tr_str("Icons:"));
    
    comboIconMode->blockSignals(true);
    int curIconMode = comboIconMode->currentItem();
    comboIconMode->clear();
    comboIconMode->insertItem(tr_str("Default"));
    comboIconMode->insertItem(tr_str("Invert"));
    comboIconMode->insertItem(tr_str("Color"));
    comboIconMode->setCurrentItem(curIconMode);
    comboIconMode->blockSignals(false);
    chkKeysBorders->setText(tr_str("Keys borders"));
    chkWindowBorder->setText(tr_str("Window border"));
    chkDisplayBorder->setText(tr_str("Display border"));
    chkBoldIcons->setText(tr_str("Bold icons"));

    // 7. Buttons
    btnApply->setText(tr_str("Apply"));
    btnSaveAsTheme->setText(tr_str("Save as theme"));
    if (btnDeleteTheme) {
        TQToolTip::remove(btnDeleteTheme);
        TQToolTip::add(btnDeleteTheme, tr_str("Delete selected custom theme"));
    }
    btnAbout->setText(tr_str("About"));
    btnClose->setText(tr_str("Close"));
    if (btnAboutClose) btnAboutClose->setText(tr_str("Close"));
}

void SettingsDialog::updateApplyButtonState()
{
    bool changed = false;
    if (chkCustom->isChecked()) {
        changed = true;
    } else if (chkTheme->isChecked()) {
        TQString currentTheme = comboTheme->currentText();
        if (m_initMode == 1 || currentTheme != m_initTheme) {
            changed = true;
        }
    }
    btnApply->setEnabled(changed);
}

#include "settings_dialog.moc"
