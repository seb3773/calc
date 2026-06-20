#ifndef TQTDATEPERIODPICKER_DECADEVIEW_H
#define TQTDATEPERIODPICKER_DECADEVIEW_H

#include <ntqwidget.h>
#include <ntqdatetime.h>

class TQtDatePeriodPickerDecadeView : public TQWidget {
    TQ_OBJECT
public:
    TQtDatePeriodPickerDecadeView(TQWidget* parent = 0);

signals:
    void yearClicked(int year);

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
    int yearAt_(int x, int y) const;
    int yearEnabled_(int year) const;

private:
    TQDate m_current;
    TQDate m_min;
    TQDate m_max;

    int m_cellW;
    int m_cellH;
    int m_cols;
    int m_rows;
    int m_hoveredYear;
};

#endif
