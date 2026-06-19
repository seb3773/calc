#include "tqtdateperiodpickeryearview.h"
#include "tqtdateperiodpicker_common.h"

#include <ntqpainter.h>
#include <ntqevent.h>

TQtDatePeriodPickerYearView::TQtDatePeriodPickerYearView(TQWidget* parent)
    : TQWidget(parent),
      m_current(TQDate::currentDate()),
      m_min(),
      m_max(),
      m_cellW(1),
      m_cellH(1),
      m_cols(4),
      m_rows(3),
      m_hoveredMonth(0)
{
    setBackgroundMode(PaletteBase);
    setMouseTracking(true);
}

void TQtDatePeriodPickerYearView::setDate(const TQDate& date)
{
    if (date.isValid()) m_current = date;
    update();
}

void TQtDatePeriodPickerYearView::setMinimumDate(const TQDate& date)
{
    m_min = date;
    update();
}

void TQtDatePeriodPickerYearView::setMaximumDate(const TQDate& date)
{
    m_max = date;
    update();
}

int TQtDatePeriodPickerYearView::monthEnabled_(int month) const
{
    if (month < 1 || month > 12) return 0;

    if (m_min.isValid()) {
        if (m_current.year() < m_min.year()) return 0;
        if (m_current.year() == m_min.year() && month < m_min.month()) return 0;
    }

    if (m_max.isValid()) {
        if (m_current.year() > m_max.year()) return 0;
        if (m_current.year() == m_max.year() && month > m_max.month()) return 0;
    }

    return 1;
}

void TQtDatePeriodPickerYearView::paintEvent(TQPaintEvent*)
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

    for (int i = 0; i < 12; ++i) {
        const int month = i + 1;
        const int col = i % m_cols;
        const int row = i / m_cols;

        const int x0 = col * m_cellW;
        const int y0 = row * m_cellH;

        const int enabled = monthEnabled_(month);

        const TQRect r(x0, y0, m_cellW, m_cellH);

        if (enabled && (month == m_current.month() || month == m_hoveredMonth)) {
            p.fillRect(r, hlBg);
        }

        p.setPen(enabled ? fg : invFg);
        p.drawText(r, AlignCenter, TQDate::shortMonthName(month));
    }
}

int TQtDatePeriodPickerYearView::monthAt_(int x, int y) const
{
    if (x < 0 || y < 0) return 0;
    const int col = x / m_cellW;
    const int row = y / m_cellH;
    if ((unsigned int)col >= (unsigned int)m_cols) return 0;
    if ((unsigned int)row >= (unsigned int)m_rows) return 0;
    const int idx = row * m_cols + col;
    if ((unsigned int)idx >= 12u) return 0;
    return idx + 1;
}

void TQtDatePeriodPickerYearView::mousePressEvent(TQMouseEvent* ev)
{
    if (!ev) return;
    const int m = monthAt_(ev->x(), ev->y());
    if (!m) return;
    if (!monthEnabled_(m)) return;
    emit monthClicked(m);
}

void TQtDatePeriodPickerYearView::mouseMoveEvent(TQMouseEvent* ev)
{
    if (!ev) return;
    int m = monthAt_(ev->x(), ev->y());
    if (!monthEnabled_(m)) m = 0;

    if (m != m_hoveredMonth) {
        m_hoveredMonth = m;
        update();
    }
}

void TQtDatePeriodPickerYearView::leaveEvent(TQEvent*)
{
    if (m_hoveredMonth != 0) {
        m_hoveredMonth = 0;
        update();
    }
}

#include "tqtdateperiodpickeryearview.moc"
