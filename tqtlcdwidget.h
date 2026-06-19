#ifndef TQT_LCD_WIDGET_H
#define TQT_LCD_WIDGET_H

#include <ntqwidget.h>
#include <ntqimage.h>
#include <ntqcolor.h>
#include <ntqstring.h>

#include <stdint.h>

#define TQT_LCD_CHAR_RAM_SIZE        256
#define TQT_LCD_CGRAM_STORAGE_CHARS  16
#define TQT_LCD_ROM_FONT_CHARS       (TQT_LCD_CHAR_RAM_SIZE - TQT_LCD_CGRAM_STORAGE_CHARS)

#define TQT_LCD_CHAR_W 5
#define TQT_LCD_CHAR_H 8

#define TQT_LCD_BORDER_SIZE 10
#define TQT_LCD_PIXEL_SIZE_W 4
#define TQT_LCD_PIXEL_SIZE_H 4
#define TQT_LCD_PIXEL_SPACE_X 1
#define TQT_LCD_PIXEL_SPACE_Y 1

#define TQT_LCD_CHAR_PIXEL_SIZE_W (TQT_LCD_PIXEL_SIZE_W + TQT_LCD_PIXEL_SPACE_X) * TQT_LCD_CHAR_W
#define TQT_LCD_CHAR_PIXEL_SIZE_H (TQT_LCD_PIXEL_SIZE_H + TQT_LCD_PIXEL_SPACE_Y) * TQT_LCD_CHAR_H
#define TQT_LCD_CHAR_SPACE_X (TQT_LCD_PIXEL_SIZE_W + TQT_LCD_PIXEL_SPACE_X) * 1
#define TQT_LCD_CHAR_SPACE_Y (TQT_LCD_PIXEL_SIZE_H + TQT_LCD_PIXEL_SPACE_Y) * 1

class TQtLcdWidget : public TQWidget {
    TQ_OBJECT
public:
    TQtLcdWidget(TQWidget* parent = 0, const char* name = 0);
    ~TQtLcdWidget();

    void refreshDisplay();

    int currentColumn() const;
    int currentRow() const;

    void setColumn(int column);
    void setRow(int row);

    void setColorBackground1(const TQColor& color);
    void setColorBackground2(const TQColor& color);
    void setColorPixel(const TQColor& color);

    TQColor colorBackground1() const;
    TQColor colorBackground2() const;
    TQColor colorPixel() const;

    uint8_t* displayCharBuffer();
    int displayCharBufferLength() const;

    void clear();
    void home();
    void setCursor(uint8_t column, uint8_t row);
    void data(uint8_t data);
    void string(const TQString& text);
    void setUserChar(uint8_t char_nr, const uint8_t* pixel_buffer);

    void setBorder(bool has_border, const TQColor& color = TQColor());

    bool saveImage(const TQString& filename);

    TQSize minimumSizeHint() const;
    TQSize sizeHint() const;

protected:
    void paintEvent(TQPaintEvent* event);
    void resizeEvent(TQResizeEvent* event);

private:
    void calculateDisplaySize();
    void drawChar(int x, int y, uint8_t c);
    void copyCharRomToRam();
    TQString getPaddedText(int cols) const;

private:
    static const uint8_t fontA00[TQT_LCD_ROM_FONT_CHARS][TQT_LCD_CHAR_W];

    TQImage* m_display;

    int m_row;
    int m_column;

    int m_displaySizeW;
    int m_displaySizeH;

    uint8_t m_charRam[TQT_LCD_CHAR_RAM_SIZE][TQT_LCD_CHAR_W];
    uint8_t* m_displayCharBuffer;

    uint16_t m_cursorPosX;
    uint16_t m_cursorPosY;

    TQColor m_colorBg1;
    TQColor m_colorBg2;
    TQColor m_colorPixel;
    bool m_hasBorder;
    TQColor m_borderColor;
};

#endif
