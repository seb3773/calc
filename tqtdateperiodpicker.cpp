#include "tqtdateperiodpicker.h"

#include "tqtdateperiodpickerformatter.h"
#include "tqtdateperiodpickerpopup.h"

#include <ntqapplication.h>
#include <ntqdesktopwidget.h>
#include <ntqevent.h>
#include <ntqlabel.h>
#include <ntqlayout.h>
#include <ntqobjectlist.h>
#include "calc_button.h"

#include "embedded_icons.h"
#include <tqimage.h>
#include <tqpixmap.h>

TQtDatePeriodPicker::TQtDatePeriodPicker(TQWidget* parent)
    : TQWidget(parent),
      m_editable(1),
      m_pickerType((int)TQtDPPDayType),
      m_allowed(TQtDPPDayType | TQtDPPPeriodType),
      m_timeEditable(0),
      m_dateBegin(TQDate::currentDate()),
      m_dateEnd(TQDate::currentDate()),
      m_timeBegin(TQTime(0, 0, 0)),
      m_timeEnd(TQTime(23, 59, 59)),
      m_label(0),
      m_btn(0),
      m_popup(0),
      m_formatter(0)
{
    m_formatter = new TQtDatePeriodPickerHumanReadableFormatter();

    m_label = new TQLabel(this);
    m_label->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Minimum);

    m_label->installEventFilter(this);

    m_btn = new CalcButton(this, "DatePeriodPickerButton");
    m_btn->addMode(ModeNormal, "", "Select date");
    m_btn->setButtonType(CalcButton::TypeHeader);
    m_btn->setFixedSize(32, 32);
    m_btn->setFocusPolicy(NoFocus);
    m_btn->setCustomIcon(date_calculus_png, date_calculus_png_len);

    m_popup = new TQtDatePeriodPickerPopup(this);
    m_popup->installEventFilter(this);

    connect(m_btn, SIGNAL(clicked()), this, SLOT(onShowPopup_()));

    connect(m_popup, SIGNAL(dateSelected(const TQDate&)), this, SLOT(onPopupDateSelected_(const TQDate&)));
    connect(m_popup, SIGNAL(datePeriodSelected(const TQDate&,const TQDate&)), this, SLOT(onPopupDatePeriodSelected_(const TQDate&,const TQDate&)));
    connect(m_popup, SIGNAL(timePeriodSelected(const TQTime&,const TQTime&)), this, SLOT(onPopupTimePeriodSelected_(const TQTime&,const TQTime&)));
    connect(m_popup, SIGNAL(datePickerTypeChanged(int)), this, SLOT(setDatePickerType(int)));
    connect(m_popup, SIGNAL(accepted()), this, SIGNAL(editingFinished()));

    TQHBoxLayout* lay = new TQHBoxLayout(this);
    lay->setMargin(0);
    lay->setSpacing(10);
    lay->addWidget(m_label, 0);
    lay->addWidget(m_btn, 0);
    lay->addStretch(1);

    setDate(m_dateBegin);
}

void TQtDatePeriodPicker::setButtonSize(int size)
{
    m_btn->setFixedSize(size, size);
}

TQtDatePeriodPicker::~TQtDatePeriodPicker()
{
    delete m_popup;
    m_popup = 0;

    delete m_formatter;
    m_formatter = 0;
}

int TQtDatePeriodPicker::isEditable() const { return m_editable; }

int TQtDatePeriodPicker::datePickerType() const { return m_pickerType; }
TQtDatePeriodPickerTypes TQtDatePeriodPicker::allowedPickerTypes() const { return m_allowed; }

int TQtDatePeriodPicker::isTimeEditable() const { return m_timeEditable; }

const TQtDatePeriodPickerAbstractFormatter* TQtDatePeriodPicker::formatter() const { return m_formatter; }

TQDate TQtDatePeriodPicker::date() const { return m_dateBegin; }
TQDate TQtDatePeriodPicker::periodBegin() const { return m_dateBegin; }
TQDate TQtDatePeriodPicker::periodEnd() const { return m_dateEnd; }

TQTime TQtDatePeriodPicker::timeBegin() const { return m_timeBegin; }
TQTime TQtDatePeriodPicker::timeEnd() const { return m_timeEnd; }

TQDateTime TQtDatePeriodPicker::dateTimePeriodBegin() const { return TQDateTime(m_dateBegin, m_timeBegin); }
TQDateTime TQtDatePeriodPicker::dateTimePeriodEnd() const { return TQDateTime(m_dateEnd, m_timeEnd); }

void TQtDatePeriodPicker::setEditable(int on)
{
    m_editable = on ? 1 : 0;
    setEnabled(m_editable ? true : false);
}

void TQtDatePeriodPicker::setDatePickerType(int type)
{
    if (m_pickerType == type) return;

    m_pickerType = type;

    if (m_popup) {
        if (sender() != m_popup) {
            if (m_popup->datePickerType() != type) m_popup->setDatePickerType(type);
        }
    }
    updateLabel_();
    emit datePickerTypeChanged(m_pickerType);
}

void TQtDatePeriodPicker::setAllowedPickerTypes(TQtDatePeriodPickerTypes types)
{
    m_allowed = types;
    if (m_popup) m_popup->setAllowedPickerTypes(types);
}

void TQtDatePeriodPicker::setTimeEditable(int on)
{
    m_timeEditable = on ? 1 : 0;
    if (m_popup) m_popup->setTimeEditable(m_timeEditable);
    updateLabel_();
}

void TQtDatePeriodPicker::setFormatter(TQtDatePeriodPickerAbstractFormatter* formatter)
{
    if (!formatter) return;

    if (m_formatter) delete m_formatter;
    m_formatter = formatter;
    updateLabel_();
}

void TQtDatePeriodPicker::setDate(const TQDate& date)
{
    m_dateBegin = date;
    m_dateEnd = date;

    if (m_popup) m_popup->setDate(date);

    updateLabel_();

    emit dateChanged(date);
}

void TQtDatePeriodPicker::setDatePeriod(const TQDate& begin, const TQDate& end)
{
    m_dateBegin = begin;
    m_dateEnd = end;

    if (m_popup) m_popup->setDatePeriod(begin, end);

    updateLabel_();

    emit datePeriodChanged(begin, end);

    if (m_timeEditable) {
        emit dateTimePeriodChanged(dateTimePeriodBegin(), dateTimePeriodEnd());
    }
}

void TQtDatePeriodPicker::setTimePeriod(const TQTime& begin, const TQTime& end)
{
    m_timeBegin = begin;
    m_timeEnd = end;

    if (m_popup) m_popup->setTimePeriod(begin, end);

    updateLabel_();

    emit timePeriodChanged(begin, end);

    if (m_timeEditable) {
        emit dateTimePeriodChanged(dateTimePeriodBegin(), dateTimePeriodEnd());
    }
}

void TQtDatePeriodPicker::setDateTimePeriod(const TQDateTime& begin, const TQDateTime& end)
{
    setDatePeriod(begin.date(), end.date());
    setTimePeriod(begin.time(), end.time());
}

void TQtDatePeriodPicker::onShowPopup_()
{
    if (!m_editable) return;
    if (!m_popup) return;

    if (m_popup->isVisible()) return;

    m_popup->reset();

    m_popup->setAllowedPickerTypes(m_allowed);
    m_popup->setDatePickerType(m_pickerType);
    m_popup->setTimeEditable(m_timeEditable);

    if (m_pickerType == TQtDPPDayType) {
        m_popup->setDate(m_dateBegin);
    } else {
        m_popup->setDatePeriod(m_dateBegin, m_dateEnd);
        if (m_timeEditable) m_popup->setTimePeriod(m_timeBegin, m_timeEnd);
    }

    adjustPopupPosition_();
    m_popup->show();
    m_popup->raise();
}

void TQtDatePeriodPicker::onPopupDateSelected_(const TQDate& date)
{
    if (m_pickerType != TQtDPPDayType) return;

    m_dateBegin = date;
    m_dateEnd = date;

    updateLabel_();

    emit dateChanged(date);
}

void TQtDatePeriodPicker::onPopupDatePeriodSelected_(const TQDate& begin, const TQDate& end)
{
    if (m_pickerType != TQtDPPPeriodType) return;

    m_dateBegin = begin;
    m_dateEnd = end;

    updateLabel_();

    emit datePeriodChanged(begin, end);
    if (m_timeEditable) emit dateTimePeriodChanged(dateTimePeriodBegin(), dateTimePeriodEnd());
}

void TQtDatePeriodPicker::onPopupTimePeriodSelected_(const TQTime& begin, const TQTime& end)
{
    if (!m_timeEditable) return;
    if (m_pickerType != TQtDPPPeriodType) return;

    m_timeBegin = begin;
    m_timeEnd = end;

    updateLabel_();

    emit timePeriodChanged(begin, end);
    emit dateTimePeriodChanged(dateTimePeriodBegin(), dateTimePeriodEnd());
}

bool TQtDatePeriodPicker::eventFilter(TQObject* obj, TQEvent* ev)
{
    if (!m_editable) return TQWidget::eventFilter(obj, ev);

    if (obj == m_label) {
        // Label clicking no longer opens the picker
    }

    if (obj == m_popup) {
        if (ev->type() == TQEvent::WindowDeactivate) {
            m_popup->close();
            emit editingFinished();
        }
    }

    return TQWidget::eventFilter(obj, ev);
}

void TQtDatePeriodPicker::showEvent(TQShowEvent* ev)
{
    TQWidget::showEvent(ev);
    adjustPopupPosition_();
}

void TQtDatePeriodPicker::updateLabel_()
{
    if (!m_label) return;

    if (!m_formatter) {
        m_label->clear();
        return;
    }

    if (m_pickerType == TQtDPPDayType) {
        m_label->setText(m_formatter->format(m_dateBegin));
        return;
    }

    if (!m_timeEditable) {
        m_label->setText(m_formatter->format(m_dateBegin, m_dateEnd));
        return;
    }

    m_label->setText(m_formatter->format(dateTimePeriodBegin(), dateTimePeriodEnd()));
}

void TQtDatePeriodPicker::adjustPopupPosition_()
{
    if (!m_popup) return;

    TQWidget* parentPage = parentWidget();
    TQWidget* topLevel = topLevelWidget();
    if (parentPage && topLevel) {
        int margin = 10;
        if (parentPage->layout()) {
            margin = parentPage->layout()->margin();
        }
        int w = 2 * topLevel->width() / 3;
        int h = 2 * topLevel->height() / 3;

        // Find lblFrom (the TQLabel with the smallest Y coordinate on parentPage whose parent is parentPage)
        TQLabel* lblFrom = 0;
        TQObjectList* list = parentPage->queryList("TQLabel");
        if (list) {
            TQObjectListIt it(*list);
            TQObject* obj;
            while ((obj = it.current()) != 0) {
                ++it;
                TQLabel* lbl = (TQLabel*)obj;
                if (lbl->parent() == parentPage) {
                    if (!lblFrom || lbl->geometry().y() < lblFrom->geometry().y()) {
                        lblFrom = lbl;
                    }
                }
            }
            delete list;
        }

        int globalX = parentPage->mapToGlobal(TQPoint(margin, 0)).x();
        int globalY = 0;
        if (lblFrom) {
            globalY = parentPage->mapToGlobal(TQPoint(0, lblFrom->geometry().bottom() + 5)).y();
        } else {
            globalY = topLevel->mapToGlobal(TQPoint(0, topLevel->height() / 4)).y();
        }

        m_popup->setGeometry(globalX, globalY, w, h);
    } else {
        TQRect g = m_popup->geometry();
        g.moveTopLeft(mapToGlobal(rect().bottomLeft()));
        m_popup->move(g.topLeft());
    }
}

#include "tqtdateperiodpicker.moc"
