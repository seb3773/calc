#ifndef TQTDATEPERIODPICKER_YEARVIEW_H
#define TQTDATEPERIODPICKER_YEARVIEW_H

#include <ntqwidget.h>
#include <ntqdatetime.h>

class TQtDatePeriodPickerYearView : public TQWidget {
    TQ_OBJECT
public:
    TQtDatePeriodPickerYearView(TQWidget* parent = 0);

signals:
    void monthClicked(int month);

public slots:
    void setDate(const TQDate& date);
    void setMinimumDate(const TQDate& date);
    void setMaximumDate(const TQDate& date);

protected:
    void paintEvent(TQPaintEvent* ev);
    void mousePressEvent(TQMouseEvent* ev);
    void mouseMoveEvent(TQMouseEvent* ev);
    void leaveEvent(TQEvent* ev);

private:
    int monthAt_(int x, int y) const;
    int monthEnabled_(int month) const;

private:
    TQDate m_current;
    TQDate m_min;
    TQDate m_max;

    int m_cellW;
    int m_cellH;
    int m_cols;
    int m_rows;
    int m_hoveredMonth;
};

#endif
