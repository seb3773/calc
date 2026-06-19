#include "tqtlcdwidget.h"

#include <ntqpainter.h>
#include <ntqevent.h>

#include <stdlib.h>

TQtLcdWidget::TQtLcdWidget(TQWidget* parent, const char* name)
    : TQWidget(parent, name),
      m_display(0),
      m_row(2),
      m_column(16),
      m_displaySizeW(0),
      m_displaySizeH(0),
      m_displayCharBuffer(0),
      m_cursorPosX(0),
      m_cursorPosY(0),
      m_colorBg1(TQColor(21, 31, 255)),
      m_colorBg2(TQColor(19, 10, 233)),
      m_colorPixel(TQColor(230, 230, 245)),
      m_hasBorder(false),
      m_borderColor(0, 0, 0) {
    setBackgroundMode(PaletteBackground);
    setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Preferred);

    home();
    calculateDisplaySize();
    copyCharRomToRam();

    // Define custom LCD characters for mathematical symbols
    const uint8_t charSqrt[8] = { 0x07, 0x04, 0x04, 0x04, 0x05, 0x0E, 0x08, 0x00 };
    const uint8_t charCube[8] = { 0x0E, 0x02, 0x04, 0x02, 0x0E, 0x00, 0x00, 0x00 };
    const uint8_t charY[8]    = { 0x0A, 0x0A, 0x0E, 0x02, 0x0C, 0x00, 0x00, 0x00 };
    const uint8_t charMul[8]  = { 0x00, 0x09, 0x09, 0x06, 0x09, 0x09, 0x00, 0x00 };
    const uint8_t charDiv[8]  = { 0x00, 0x04, 0x00, 0x0E, 0x00, 0x04, 0x00, 0x00 };

    setUserChar(1, charSqrt);
    setUserChar(2, charCube);
    setUserChar(3, charY);
    setUserChar(4, charMul);
    setUserChar(5, charDiv);

    refreshDisplay();
}

TQtLcdWidget::~TQtLcdWidget() {
    delete m_display;
    m_display = 0;

    delete[] m_displayCharBuffer;
    m_displayCharBuffer = 0;
}

TQString TQtLcdWidget::getPaddedText(int cols) const {
    TQString str;
    int len = m_row * m_column;
    if (m_displayCharBuffer) {
        for (int idx = 0; idx < len; ++idx) {
            str.append(TQChar(m_displayCharBuffer[idx]));
        }
    }
    
    // Strip leading spaces
    int first_non_space = 0;
    while (first_non_space < str.length() && str[first_non_space] == ' ') {
        first_non_space++;
    }
    TQString unpadded = str.mid(first_non_space);
    
    // Right justify to the new cols count
    return unpadded.rightJustify(cols, ' ', true);
}

void TQtLcdWidget::resizeEvent(TQResizeEvent* e) {
    TQWidget::resizeEvent(e);
    
    // Calculate new column count based on height and width
    int pw = 1;
    int g = 0;
    for (int test_pw = 1; test_pw <= 64; ++test_pw) {
        int test_g = (test_pw >= 4) ? 1 : 0;
        int char_h = 8 * test_pw + 7 * test_g;
        int space_y = test_pw;
        int grid_h = m_row * char_h + (m_row - 1) * space_y;
        
        if (grid_h + 4 <= height()) {
            pw = test_pw;
            g = test_g;
        } else {
            break;
        }
    }
    
    int char_w = 5 * pw + 4 * g;
    int space_x = pw;
    int cols = (width() - 10 + space_x) / (char_w + space_x);
    if (cols < 1) cols = 1;
    
    if (cols != m_column) {
        TQString unpadded;
        TQString str;
        int len = m_row * m_column;
        if (m_displayCharBuffer) {
            for (int idx = 0; idx < len; ++idx) {
                str.append(TQChar(m_displayCharBuffer[idx]));
            }
            int first_non_space = 0;
            while (first_non_space < str.length() && str[first_non_space] == ' ') {
                first_non_space++;
            }
            unpadded = str.mid(first_non_space);
        }
        
        m_column = cols;
        
        delete[] m_displayCharBuffer;
        m_displayCharBuffer = new uint8_t[m_row * m_column];
        for (int idx = 0; idx < m_row * m_column; ++idx) {
            m_displayCharBuffer[idx] = ' ';
        }
        
        if (!unpadded.isEmpty()) {
            TQString padded = unpadded.rightJustify(m_column, ' ', true);
            for (int idx = 0; idx < m_column && idx < padded.length(); ++idx) {
                m_displayCharBuffer[idx] = (uint8_t)padded.at(idx).latin1();
            }
        }
        
        updateGeometry();
    }
}

void TQtLcdWidget::paintEvent(TQPaintEvent*) {
    TQPainter p(this);
    
    // Fill background with the outer background color
    p.fillRect(rect(), m_colorBg1);
    
    // Find the largest pixel size 'pw' and gap 'g' that fits the height
    int pw = 1;
    int g = 0;
    for (int test_pw = 1; test_pw <= 64; ++test_pw) {
        int test_g = (test_pw >= 4) ? 1 : 0;
        int char_h = 8 * test_pw + 7 * test_g;
        int space_y = test_pw;
        int grid_h = m_row * char_h + (m_row - 1) * space_y;
        
        if (grid_h <= height()) {
            pw = test_pw;
            g = test_g;
        } else {
            break;
        }
    }
    
    int char_w = 5 * pw + 4 * g;
    int char_h = 8 * pw + 7 * g;
    int space_x = pw;
    int space_y = pw;
    
    int grid_w = m_column * char_w + (m_column - 1) * space_x;
    int grid_h = m_row * char_h + (m_row - 1) * space_y;
    
    // Right-align the grid, leaving a small margin (e.g. 2 pixels) from the right edge!
    int start_x = width() - grid_w - 2;
    
    int start_y = (height() - grid_h) / 2;
    
    int i = 0;
    for (int y = 0; y < m_row; ++y) {
        for (int x = 0; x < m_column; ++x) {
            uint8_t c = m_displayCharBuffer ? m_displayCharBuffer[i++] : ' ';
            int char_start_x = start_x + x * (char_w + space_x);
            int char_start_y = start_y + y * (char_h + space_y);
            
            int draw_w = (pw > 1 && g == 0) ? (pw - 1) : pw;
            for (int cx = 0; cx < 5; ++cx) {
                for (int ry = 0; ry < 8; ++ry) {
                    bool active = (m_charRam[c][cx] >> (7 - ry)) & 1;
                    p.fillRect(char_start_x + cx * (pw + g),
                               char_start_y + ry * (pw + g),
                               draw_w, draw_w,
                               active ? m_colorPixel : m_colorBg2);
                }
            }
        }
    }
    if (m_hasBorder) {
        TQColor bgCol = m_colorBg1;
        TQColor c1 = m_borderColor;
        TQColor c2((c1.red() + bgCol.red()) / 2,
                   (c1.green() + bgCol.green()) / 2,
                   (c1.blue() + bgCol.blue()) / 2);
        p.setPen(c1);
        p.drawRect(0, 0, width(), height());
        p.setPen(c2);
        p.drawRect(1, 1, width() - 2, height() - 2);
    }
}

void TQtLcdWidget::refreshDisplay() {
    update();
}

int TQtLcdWidget::currentColumn() const { return m_column; }

int TQtLcdWidget::currentRow() const { return m_row; }

void TQtLcdWidget::setColumn(int column) {
    if (column < 1) column = 1;
    m_column = column;
    calculateDisplaySize();
    refreshDisplay();
}

void TQtLcdWidget::setRow(int row) {
    if (row < 1) row = 1;
    m_row = row;
    calculateDisplaySize();
    refreshDisplay();
}

void TQtLcdWidget::setColorBackground1(const TQColor& color) {
    m_colorBg1 = color;
    refreshDisplay();
}

void TQtLcdWidget::setColorBackground2(const TQColor& color) {
    m_colorBg2 = color;
    refreshDisplay();
}

void TQtLcdWidget::setColorPixel(const TQColor& color) {
    m_colorPixel = color;
    refreshDisplay();
}

TQColor TQtLcdWidget::colorBackground1() const { return m_colorBg1; }

TQColor TQtLcdWidget::colorBackground2() const { return m_colorBg2; }

TQColor TQtLcdWidget::colorPixel() const { return m_colorPixel; }

uint8_t* TQtLcdWidget::displayCharBuffer() { return m_displayCharBuffer; }

int TQtLcdWidget::displayCharBufferLength() const { return m_row * m_column; }

void TQtLcdWidget::clear() {
    if (!m_displayCharBuffer) return;
    const int n = m_row * m_column;
    for (int i = 0; i < n; ++i) m_displayCharBuffer[i] = ' ';
    home();
    refreshDisplay();
}

void TQtLcdWidget::home() {
    m_cursorPosX = 0;
    m_cursorPosY = 0;
}

void TQtLcdWidget::setCursor(uint8_t column, uint8_t row) {
    if (row == 0) return;
    m_cursorPosX = column;
    m_cursorPosY = (uint16_t)(row - 1);
}

void TQtLcdWidget::data(uint8_t data) {
    if (!m_displayCharBuffer) return;
    const int idx = (int)m_cursorPosY * m_column + (int)m_cursorPosX;
    if (idx < 0 || idx >= m_row * m_column) return;

    m_displayCharBuffer[idx] = data;
    refreshDisplay();
}

void TQtLcdWidget::string(const TQString& text) {
    if (!m_displayCharBuffer) return;

    const int n = text.length();
    for (int i = 0; i < n; ++i) {
        const int idx = (int)m_cursorPosY * m_column + (int)m_cursorPosX;
        if (idx >= 0 && idx < m_row * m_column) {
            const TQChar c = text.at(i);
            uint8_t byteVal = 0;
            if (c.unicode() == 0x221A) {        // √
                byteVal = 1;
            } else if (c.unicode() == 0x00B3) { // ³
                byteVal = 2;
            } else if (c.unicode() == 0x02B8) { // ʸ
                byteVal = 3;
            } else if (c.unicode() == 0x00D7) { // ×
                byteVal = 4;
            } else if (c.unicode() == 0x00F7) { // ÷
                byteVal = 5;
            } else {
                byteVal = (uint8_t)c.latin1();
            }
            m_displayCharBuffer[idx] = byteVal;
        }

        ++m_cursorPosX;
        if (m_cursorPosX == (uint16_t)m_column) {
            m_cursorPosX = 0;
            ++m_cursorPosY;
            if (m_cursorPosY == (uint16_t)m_row) m_cursorPosY = 0;
        }
    }

    refreshDisplay();
}

void TQtLcdWidget::setUserChar(uint8_t char_nr, const uint8_t* pixel_buffer) {
    if (!pixel_buffer) return;

    char_nr &= 0x0f;
    for (int i = 0; i < TQT_LCD_CHAR_W; ++i) {
        uint8_t byte = 0;
        for (int j = 0; j < 8; ++j) {
            byte |= (uint8_t)(((pixel_buffer[j] >> i) & 1) << (7 - j));
        }
        m_charRam[char_nr][4 - i] = byte;
    }
}

bool TQtLcdWidget::saveImage(const TQString& filename) {
    (void)filename;
    return false;
}

TQSize TQtLcdWidget::minimumSizeHint() const {
    return TQSize(16 * 6 + 10, m_row * 10 + 6);
}

TQSize TQtLcdWidget::sizeHint() const {
    return TQSize(24 * 18 + 20, m_row * 30 + 10);
}

void TQtLcdWidget::calculateDisplaySize() {
    delete[] m_displayCharBuffer;
    m_displayCharBuffer = 0;

    const int n = m_column * m_row;
    if (n > 0) {
        m_displayCharBuffer = new uint8_t[n];
        for (int i = 0; i < n; ++i) m_displayCharBuffer[i] = ' ';
    }
    updateGeometry();
}

void TQtLcdWidget::drawChar(int x, int y, uint8_t c) {
    (void)x;
    (void)y;
    (void)c;
}

void TQtLcdWidget::copyCharRomToRam() {
    for (int i = 0; i < TQT_LCD_ROM_FONT_CHARS; ++i) {
        for (int j = 0; j < TQT_LCD_CHAR_W; ++j) {
            m_charRam[i + TQT_LCD_CGRAM_STORAGE_CHARS][j] = fontA00[i][j];
        }
    }
}

const uint8_t TQtLcdWidget::fontA00[TQT_LCD_ROM_FONT_CHARS][TQT_LCD_CHAR_W] = {
#include "tqtlcdwidget_font_a00.inl"
};

void TQtLcdWidget::setBorder(bool has_border, const TQColor& color) {
    m_hasBorder = has_border;
    m_borderColor = color;
    update();
}

#include "tqtlcdwidget.moc"
