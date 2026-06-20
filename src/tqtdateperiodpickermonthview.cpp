#include "tqtdateperiodpickermonthview.h"
#include "tqtdateperiodpicker_common.h"

#include <ntqpainter.h>
#include <ntqfontmetrics.h>
#include <ntqevent.h>

static inline int dppClampI_(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static inline TQDate dppMinDateFallback_()
{
    return TQDate(1752, 9, 14);
}

TQtDatePeriodPickerMonthView::TQtDatePeriodPickerMonthView(TQWidget* parent)
    : TQWidget(parent),
      m_minDate(),
      m_maxDate(),
      m_selected(),
      m_periodShown(0),
      m_periodBegin(),
      m_periodEnd(),
      m_firstDayMonday(1),
      m_year(0),
      m_month(0),
      m_gridOrigin(),
      m_cellW(32),
      m_cellH(32),
      m_headerH(26),
      m_normalBg(245, 245, 245),
      m_normalFg(0, 0, 0),
      m_invalidFg(190, 190, 190),
      m_highlightBg(0xd3, 0xdd, 0xe5),
      m_highlightFg(0, 0, 0),
      m_hoveredRow(-1),
      m_hoveredCol(-1)
{
    setBackgroundMode(PaletteBase);
    setMouseTracking(true);

    const TQDate cd = TQDate::currentDate();
    m_year = cd.year();
    m_month = cd.month();
    setCurrentPage_(m_year, m_month);
}

TQSize TQtDatePeriodPickerMonthView::minimumSizeHint() const
{
    return TQSize(1, 1);
}

TQSize TQtDatePeriodPickerMonthView::sizeHint() const
{
    return TQSize(100, 100);
}

TQDate TQtDatePeriodPickerMonthView::selectedDate() const
{
    return m_selected;
}

void TQtDatePeriodPickerMonthView::setMinimumDate(const TQDate& d)
{
    m_minDate = d;
    update();
}

void TQtDatePeriodPickerMonthView::setCurrentPage(int year, int month)
{
    setCurrentPage_(year, month);
    update();
}

void TQtDatePeriodPickerMonthView::setMaximumDate(const TQDate& d)
{
    m_maxDate = d;
    update();
}

void TQtDatePeriodPickerMonthView::setFirstDayOfWeek(int monday1)
{
    m_firstDayMonday = monday1 ? 1 : 0;
    rebuildGridOrigin_();
    update();
}

void TQtDatePeriodPickerMonthView::setCurrentPage_(int year, int month)
{
    if (month < 1) month = 1;
    if (month > 12) month = 12;

    if (m_year == year && m_month == month) {
        rebuildGridOrigin_();
        return;
    }

    m_year = year;
    m_month = month;
    rebuildGridOrigin_();

    emit currentPageChanged(m_year, m_month);
}

void TQtDatePeriodPickerMonthView::rebuildGridOrigin_()
{
    const TQDate first(m_year, m_month, 1);

    const int desiredDow = m_firstDayMonday ? 1 : 7;

    int delta = first.dayOfWeek() - desiredDow;
    if (delta < 0) delta += 7;

    m_gridOrigin = first.addDays(-delta);
}

TQDate TQtDatePeriodPickerMonthView::cellDate_(int row, int col) const
{
    return m_gridOrigin.addDays(row * 7 + col);
}

int TQtDatePeriodPickerMonthView::hitRow_(int y) const
{
    y -= m_headerH;
    if (y < 0) return -1;
    return y / m_cellH;
}

int TQtDatePeriodPickerMonthView::hitCol_(int x) const
{
    if (x < 0) return -1;
    return x / m_cellW;
}

void TQtDatePeriodPickerMonthView::setDate(const TQDate& d)
{
    m_periodShown = 0;
    m_periodBegin = TQDate();
    m_periodEnd = TQDate();

    if (d.isValid()) {
        m_selected = d;
        setCurrentPage_(d.year(), d.month());
    }

    update();
}

void TQtDatePeriodPickerMonthView::setPeriod(const TQDate& begin, const TQDate& end, int showsBegin)
{
    m_periodShown = (begin.isValid() && end.isValid()) ? 1 : 0;
    m_periodBegin = begin;
    m_periodEnd = end;

    const TQDate shown = showsBegin ? begin : end;
    if (shown.isValid()) {
        m_selected = shown;
        setCurrentPage_(shown.year(), shown.month());
    }

    update();
}

void TQtDatePeriodPickerMonthView::paintEvent(TQPaintEvent*)
{
    TQColor bg, fg;
    dppGetPanelColors(bg, fg);
    m_normalBg = bg;
    m_normalFg = fg;
    int r_inv = (fg.red() + 3 * bg.red()) / 4;
    int g_inv = (fg.green() + 3 * bg.green()) / 4;
    int b_inv = (fg.blue() + 3 * bg.blue()) / 4;
    m_invalidFg = TQColor(r_inv, g_inv, b_inv);
    int r_hl = (2 * fg.red() + 3 * bg.red()) / 5;
    int g_hl = (2 * fg.green() + 3 * bg.green()) / 5;
    int b_hl = (2 * fg.blue() + 3 * bg.blue()) / 5;
    m_highlightBg = TQColor(r_hl, g_hl, b_hl);
    m_highlightFg = fg;

    TQPainter p(this);
    p.fillRect(rect(), bg);

    TQFont f = font();
    p.setFont(f);

    const TQFontMetrics fm(p.fontMetrics());
    m_headerH = fm.height() + 8;

    const int w = width();
    const int h = height();

    int cellW = w / 7;
    if (cellW < 18) cellW = 18;

    int cellH = (h - m_headerH) / 6;
    if (cellH < 18) cellH = 18;

    m_cellW = cellW;
    m_cellH = cellH;

    static const char* kMon[7] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
    static const char* kSun[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

    const char* const* names = m_firstDayMonday ? kMon : kSun;

    int i;
    for (i = 0; i < 7; ++i) {
        const int x0 = i * cellW;
        const int tw = fm.width(names[i]);
        const int tx = x0 + (cellW - tw) / 2;
        const int ty = (m_headerH + fm.ascent()) / 2;

        p.setPen(m_normalFg);
        p.drawText(tx, ty, names[i]);
    }

    const TQDate minv = m_minDate.isValid() ? m_minDate : dppMinDateFallback_();
    const TQDate maxv = m_maxDate.isValid() ? m_maxDate : TQDate(7999, 12, 31);

    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 7; ++col) {
            const TQDate d = cellDate_(row, col);

            const int isInvalid = (d < minv) || (d > maxv) || (d.month() != m_month) || (d.year() != m_year);

            int isHighlighted = (d == m_selected);
            if (m_periodShown && m_periodBegin.isValid() && m_periodEnd.isValid()) {
                if (d >= m_periodBegin && d <= m_periodEnd) isHighlighted = 1;
            }

            const int isHovered = (row == m_hoveredRow && col == m_hoveredCol);

            const int x0 = col * cellW;
            const int y0 = m_headerH + row * cellH;

            const TQColor bg = (isHighlighted || isHovered) ? m_highlightBg : m_normalBg;
            p.fillRect(x0, y0, cellW, cellH, bg);

            if (isInvalid) p.setPen(m_invalidFg);
            else if (isHighlighted || isHovered) p.setPen(m_highlightFg);
            else p.setPen(m_normalFg);

            TQString ds = TQString::number(d.day());
            const int tw = fm.width(ds);
            const int tx = x0 + (cellW - tw) / 2;
            const int ty = y0 + (cellH + fm.ascent() - fm.descent()) / 2;
            p.drawText(tx, ty, ds);
        }
    }
}

void TQtDatePeriodPickerMonthView::mousePressEvent(TQMouseEvent* ev)
{
    if (!ev) return;

    const int col = hitCol_(ev->x());
    const int row = hitRow_(ev->y());

    if ((unsigned int)col >= 7u) return;
    if ((unsigned int)row >= 6u) return;

    const int rows = 6;
    const int cols = 7;

    const TQDate d = cellDate_(row, col);

    const TQDate minv = m_minDate.isValid() ? m_minDate : dppMinDateFallback_();
    const TQDate maxv = m_maxDate.isValid() ? m_maxDate : TQDate(7999, 12, 31);

    if (d < minv || d > maxv) return;

    m_selected = d;
    setCurrentPage_(d.year(), d.month());
    update();

    emit dateClicked(d);
}

void TQtDatePeriodPickerMonthView::mouseMoveEvent(TQMouseEvent* ev)
{
    if (!ev) return;

    int col = hitCol_(ev->x());
    int row = hitRow_(ev->y());

    if (col < 0 || col >= 7 || row < 0 || row >= 6) {
        col = -1;
        row = -1;
    } else {
        const TQDate d = cellDate_(row, col);
        const TQDate minv = m_minDate.isValid() ? m_minDate : dppMinDateFallback_();
        const TQDate maxv = m_maxDate.isValid() ? m_maxDate : TQDate(7999, 12, 31);
        if (d < minv || d > maxv || d.month() != m_month || d.year() != m_year) {
            col = -1;
            row = -1;
        }
    }

    if (col != m_hoveredCol || row != m_hoveredRow) {
        m_hoveredCol = col;
        m_hoveredRow = row;
        update();
    }
}

void TQtDatePeriodPickerMonthView::leaveEvent(TQEvent*)
{
    if (m_hoveredCol != -1 || m_hoveredRow != -1) {
        m_hoveredCol = -1;
        m_hoveredRow = -1;
        update();
    }
}

#include "tqtdateperiodpickermonthview.moc"
