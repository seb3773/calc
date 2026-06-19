#ifndef TQTDATEPERIODPICKER_MONTHVIEW_H
#define TQTDATEPERIODPICKER_MONTHVIEW_H

#include <ntqwidget.h>
#include <ntqdatetime.h>
#include <ntqstring.h>
#include <ntqcolor.h>

class TQtDatePeriodPickerMonthView : public TQWidget {
    TQ_OBJECT
public:
    TQtDatePeriodPickerMonthView(TQWidget* parent = 0);

    TQSize minimumSizeHint() const;
    TQSize sizeHint() const;

    TQDate selectedDate() const;

signals:
    void dateClicked(const TQDate& date);
    void currentPageChanged(int year, int month);

public slots:
    void setMinimumDate(const TQDate& d);
    void setMaximumDate(const TQDate& d);

    void setDate(const TQDate& d);
    void setPeriod(const TQDate& begin, const TQDate& end, int showsBegin);

    void setCurrentPage(int year, int month);

    void setFirstDayOfWeek(int monday1);

protected:
    void paintEvent(TQPaintEvent* ev);
    void mousePressEvent(TQMouseEvent* ev);
    void mouseMoveEvent(TQMouseEvent* ev);
    void leaveEvent(TQEvent* ev);

private:
    void setCurrentPage_(int year, int month);
    void rebuildGridOrigin_();

    TQDate cellDate_(int row, int col) const;
    int hitRow_(int y) const;
    int hitCol_(int x) const;

private:
    TQDate m_minDate;
    TQDate m_maxDate;

    TQDate m_selected;

    int m_periodShown;
    TQDate m_periodBegin;
    TQDate m_periodEnd;

    int m_firstDayMonday;

    int m_year;
    int m_month;

    TQDate m_gridOrigin;

    int m_cellW;
    int m_cellH;
    int m_headerH;

    TQColor m_normalBg;
    TQColor m_normalFg;
    TQColor m_invalidFg;
    TQColor m_highlightBg;
    TQColor m_highlightFg;
    int m_hoveredRow;
    int m_hoveredCol;
};

#endif
