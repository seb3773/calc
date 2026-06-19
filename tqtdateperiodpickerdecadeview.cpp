#include "tqtdateperiodpickerdecadeview.h"
#include "tqtdateperiodpicker_common.h"

#include <ntqpainter.h>
#include <ntqevent.h>

TQtDatePeriodPickerDecadeView::TQtDatePeriodPickerDecadeView(TQWidget* parent)
    : TQWidget(parent),
      m_current(TQDate::currentDate()),
      m_min(),
      m_max(),
      m_cellW(1),
      m_cellH(1),
      m_cols(4),
      m_rows(3),
      m_hoveredYear(0)
{
    setBackgroundMode(PaletteBase);
    setMouseTracking(true);
}

void TQtDatePeriodPickerDecadeView::setDate(const TQDate& date)
{
    if (date.isValid()) m_current = date;
    update();
}

void TQtDatePeriodPickerDecadeView::setMinimumDate(const TQDate& date)
{
    m_min = date;
    update();
}

void TQtDatePeriodPickerDecadeView::setMaximumDate(const TQDate& date)
{
    m_max = date;
    update();
}

int TQtDatePeriodPickerDecadeView::yearEnabled_(int year) const
{
    if (m_min.isValid() && year < m_min.year()) return 0;
    if (m_max.isValid() && year > m_max.year()) return 0;
    return 1;
}

void TQtDatePeriodPickerDecadeView::paintEvent(TQPaintEvent*)
{
    TQColor bg, fg;
    dppGetPanelColors(bg, fg);
    int r_inv = (fg.red() + 3 * bg.red()) / 4;
    int g_inv = (fg.green() + 3 * bg.green()) / 4;
    int b_inv = (fg.blue() + 3 * bg.blue()) / 4;
    TQColor invFg(r_inv, g_inv, b_inv);
    int r_hl = (2 * fg.red() + 3 * bg.red()) / 5;
    int g_hl = (2 * fg.green() + 3 * bg.green()) / 5;
    int b_hl = (2 * fg.blue() + 3 * bg.blue()) / 5;
    TQColor hlBg(r_hl, g_hl, b_hl);

    TQPainter p(this);
    p.fillRect(rect(), bg);

    const int w = width();
    const int h = height();

    m_cellW = (w > 0) ? (w / m_cols) : 1;
    m_cellH = (h > 0) ? (h / m_rows) : 1;
    if (m_cellW < 10) m_cellW = 10;
    if (m_cellH < 10) m_cellH = 10;

    TQFont f = font();
    p.setFont(f);

    const int baseYear = m_current.year();

    for (int i = 0; i < 12; ++i) {
        const int year = baseYear + (i - 5);
        const int col = i % m_cols;
        const int row = i / m_cols;

        const int x0 = col * m_cellW;
        const int y0 = row * m_cellH;
        const TQRect r(x0, y0, m_cellW, m_cellH);

        const int enabled = yearEnabled_(year);

        if (enabled && (year == baseYear || year == m_hoveredYear)) {
            p.fillRect(r, hlBg);
        }

        p.setPen(enabled ? fg : invFg);
        p.drawText(r, AlignCenter, TQString::number(year));
    }
}

int TQtDatePeriodPickerDecadeView::yearAt_(int x, int y) const
{
    if (x < 0 || y < 0) return 0;
    const int col = x / m_cellW;
    const int row = y / m_cellH;
    if ((unsigned int)col >= (unsigned int)m_cols) return 0;
    if ((unsigned int)row >= (unsigned int)m_rows) return 0;
    const int idx = row * m_cols + col;
    if ((unsigned int)idx >= 12u) return 0;
    return m_current.year() + (idx - 5);
}

void TQtDatePeriodPickerDecadeView::mousePressEvent(TQMouseEvent* ev)
{
    if (!ev) return;
    const int y = yearAt_(ev->x(), ev->y());
    if (!y) return;
    if (!yearEnabled_(y)) return;
    emit yearClicked(y);
}

void TQtDatePeriodPickerDecadeView::mouseMoveEvent(TQMouseEvent* ev)
{
    if (!ev) return;
    int y = yearAt_(ev->x(), ev->y());
    if (!yearEnabled_(y)) y = 0;

    if (y != m_hoveredYear) {
        m_hoveredYear = y;
        update();
    }
}

void TQtDatePeriodPickerDecadeView::leaveEvent(TQEvent*)
{
    if (m_hoveredYear != 0) {
        m_hoveredYear = 0;
        update();
    }
}

#include "tqtdateperiodpickerdecadeview.moc"
