#ifndef KCALC_HISTORY_WINDOW_H
#define KCALC_HISTORY_WINDOW_H

#include <tqdialog.h>
#include <tqwidget.h>
#include <tqscrollview.h>
#include <tqlabel.h>
#include <tqcolor.h>
#include <tqevent.h>
#include <vector>
#include "calc.h"

class CalcButton;

class HistoryItemWidget : public TQWidget {
	TQ_OBJECT
public:
	HistoryItemWidget(const CalcHistoryItem &item, TQWidget *parent = 0);
	~HistoryItemWidget();

	const CalcHistoryItem& item() const { return m_item; }
	void setColors(const TQColor &bg, const TQColor &fg, const TQColor &displayBg);
	void updateItemSizes(double scale);

signals:
	void clicked(const CalcHistoryItem &item);

protected:
	virtual void paintEvent(TQPaintEvent *e);
	virtual void mouseReleaseEvent(TQMouseEvent *e);
	virtual void enterEvent(TQEvent *e);
	virtual void leaveEvent(TQEvent *e);

private:
	CalcHistoryItem m_item;
	TQLabel *m_lblExpr;
	TQLabel *m_lblResult;
	TQColor m_normalBg;
	TQColor m_hoverBg;
	bool m_hovered;
};

class HistoryWindow : public TQDialog {
	TQ_OBJECT
public:
	HistoryWindow(TQWidget *parent = 0);
	~HistoryWindow();

	void setHistory(const std::vector<CalcHistoryItem> &items);
	void setColors(const TQColor &panelBg, const TQColor &textFg, const TQColor &displayBg);
	void retranslateUi();

signals:
	void itemSelected(const CalcHistoryItem &item);
	void clearHistoryRequested();

protected:
	virtual void resizeEvent(TQResizeEvent *e);

private slots:
	void slotItemClicked(const CalcHistoryItem &item);
	void slotClearClicked();

private:
	void updateLayoutSizes();

	TQScrollView *m_scroll;
	TQWidget *m_container;
	TQWidget *m_bottomBar;
	CalcButton *m_btnClear;
	TQLabel *m_lblEmpty;
	std::vector<CalcHistoryItem> m_items;

	TQColor m_panelBg;
	TQColor m_textFg;
	TQColor m_displayBg;
};

#endif // KCALC_HISTORY_WINDOW_H
