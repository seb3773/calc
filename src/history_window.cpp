#include "history_window.h"
#include "calc_button.h"
#include "translation.h"
#include "embedded_icons.h"
#include "icon_utils.h"
#include <tqlayout.h>
#include <tqpainter.h>
#include <tqevent.h>
#include <tqfont.h>
#include <tqtooltip.h>
#include <tqobjectlist.h>

static void applyIconToButton(CalcButton* btn, const unsigned char* data, unsigned int len, const TQString& tooltip) {
	if (!btn) return;
	btn->setCustomIcon(data, len);
	btn->addMode(ModeNormal, "", tooltip);
}

HistoryItemWidget::HistoryItemWidget(const CalcHistoryItem &item, TQWidget *parent)
	: TQWidget(parent), m_item(item), m_hovered(false)
{
	setBackgroundMode(TQt::PaletteBackground);
	
	TQVBoxLayout *lay = new TQVBoxLayout(this, 6, 2);
	
	m_lblExpr = new TQLabel(item.expression, this);
	m_lblExpr->setAlignment(TQt::AlignRight);
	TQFont fExpr = m_lblExpr->font();
	fExpr.setPointSize(fExpr.pointSize() - 1);
	m_lblExpr->setFont(fExpr);
	lay->addWidget(m_lblExpr);
	
	m_lblResult = new TQLabel(item.result_str, this);
	m_lblResult->setAlignment(TQt::AlignRight);
	TQFont fResult = m_lblResult->font();
	fResult.setBold(true);
	fResult.setPointSize(fResult.pointSize() + 1);
	m_lblResult->setFont(fResult);
	lay->addWidget(m_lblResult);
}

void HistoryItemWidget::updateItemSizes(double scale)
{
	int exprSize = static_cast<int>(9.0 * scale);
	int resultSize = static_cast<int>(13.0 * scale);
	if (exprSize < 7) exprSize = 7;
	if (resultSize < 9) resultSize = 9;

	TQFont fExpr = m_lblExpr->font();
	fExpr.setPointSize(exprSize);
	m_lblExpr->setFont(fExpr);

	TQFont fResult = m_lblResult->font();
	fResult.setPointSize(resultSize);
	fResult.setBold(true);
	m_lblResult->setFont(fResult);

	TQVBoxLayout *lay = dynamic_cast<TQVBoxLayout*>(layout());
	if (lay) {
		int margin = static_cast<int>(6.0 * scale);
		int spacing = static_cast<int>(2.0 * scale);
		if (margin < 4) margin = 4;
		if (spacing < 1) spacing = 1;
		lay->setMargin(margin);
		lay->setSpacing(spacing);
	}

	updateGeometry();
}

HistoryItemWidget::~HistoryItemWidget()
{
}

void HistoryItemWidget::setColors(const TQColor &bg, const TQColor &fg, const TQColor &displayBg)
{
	(void)displayBg;
	m_normalBg = bg;
	int val = (bg.red() + bg.green() + bg.blue()) / 3;
	m_hoverBg = (val < 128) ? bg.light(120) : bg.dark(108);

	TQColor curBg = m_hovered ? m_hoverBg : m_normalBg;
	setPaletteBackgroundColor(curBg);
	m_lblExpr->setPaletteBackgroundColor(curBg);
	m_lblResult->setPaletteBackgroundColor(curBg);

	int fgVal = (fg.red() + fg.green() + fg.blue()) / 3;
	TQColor exprFg = (fgVal < 128) ? fg.light(130) : fg.dark(120);
	m_lblExpr->setPaletteForegroundColor(exprFg);
	m_lblResult->setPaletteForegroundColor(fg);
	update();
}

void HistoryItemWidget::paintEvent(TQPaintEvent *e)
{
	(void)e;
	if (!m_normalBg.isValid()) return;

	TQPainter p(this);
	p.fillRect(rect(), m_hovered ? m_hoverBg : m_normalBg);

	int val = (m_normalBg.red() + m_normalBg.green() + m_normalBg.blue()) / 3;
	TQColor lineColor = (val > 128) ? m_normalBg.dark(110) : m_normalBg.light(115);
	if (val < 10) {
		lineColor = TQColor(40, 40, 40);
	}
	p.setPen(lineColor);
	p.drawLine(0, height() - 1, width(), height() - 1);
}

void HistoryItemWidget::mouseReleaseEvent(TQMouseEvent *e)
{
	if (e->button() == TQt::LeftButton) {
		emit clicked(m_item);
	}
}

void HistoryItemWidget::enterEvent(TQEvent *e)
{
	(void)e;
	m_hovered = true;
	TQColor curBg = m_hoverBg;
	setPaletteBackgroundColor(curBg);
	m_lblExpr->setPaletteBackgroundColor(curBg);
	m_lblResult->setPaletteBackgroundColor(curBg);
	update();
}

void HistoryItemWidget::leaveEvent(TQEvent *e)
{
	(void)e;
	m_hovered = false;
	TQColor curBg = m_normalBg;
	setPaletteBackgroundColor(curBg);
	m_lblExpr->setPaletteBackgroundColor(curBg);
	m_lblResult->setPaletteBackgroundColor(curBg);
	update();
}


HistoryWindow::HistoryWindow(TQWidget *parent)
	: TQDialog(parent, "HistoryWindow", false, WType_TopLevel | WStyle_Customize | WStyle_Title | WStyle_SysMenu | WStyle_MinMax),
	  m_scroll(NULL), m_container(NULL), m_bottomBar(NULL), m_btnClear(NULL), m_lblEmpty(NULL)
{
	setCaption(tr_str("History"));
	resize(320, 480);

	TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 0, 0);

	m_scroll = new TQScrollView(this);
	m_scroll->setFrameStyle(TQFrame::NoFrame);
	m_scroll->setHScrollBarMode(TQScrollView::AlwaysOff);
	m_scroll->setVScrollBarMode(TQScrollView::Auto);
	m_scroll->setResizePolicy(TQScrollView::AutoOneFit);
	mainLayout->addWidget(m_scroll, 1);

	m_container = new TQWidget(m_scroll->viewport(), "HistoryContainer");
	m_container->setBackgroundMode(TQt::PaletteBackground);
	new TQVBoxLayout(m_container, 0, 0);
	m_scroll->addChild(m_container);

	m_bottomBar = new TQWidget(this);
	TQHBoxLayout *botLayout = new TQHBoxLayout(m_bottomBar, 4, 4);
	botLayout->addStretch(1);

	m_btnClear = new CalcButton(m_bottomBar, "btnClearHistory");
	m_btnClear->setFlat(true);
	m_btnClear->setButtonType(CalcButton::TypeOther);
	applyIconToButton(m_btnClear, trash_png, trash_png_len, tr_str("Clear all"));
	m_btnClear->setFixedSize(36, 36);
	m_btnClear->setFocusPolicy(TQWidget::NoFocus);
	connect(m_btnClear, TQ_SIGNAL(clicked(void)), this, TQ_SLOT(slotClearClicked(void)));
	botLayout->addWidget(m_btnClear);

	mainLayout->addWidget(m_bottomBar);
}

HistoryWindow::~HistoryWindow()
{
}

void HistoryWindow::setHistory(const std::vector<CalcHistoryItem> &items)
{
	m_items = items;
	
	const TQObjectList *list = m_container->children();
	if (list) {
		TQObjectList copy = *list;
		TQObjectListIt it(copy);
		TQObject *obj;
		while ((obj = it.current()) != 0) {
			++it;
			if (obj->isWidgetType()) {
				delete obj;
			}
		}
	}
	
	delete m_container->layout();
	
	TQVBoxLayout *lay = new TQVBoxLayout(m_container, 8, 4);

	if (items.empty()) {
		m_lblEmpty = new TQLabel(tr_str("No history yet"), m_container);
		m_lblEmpty->setAlignment(TQt::AlignCenter);
		TQFont f = m_lblEmpty->font();
		f.setItalic(true);
		m_lblEmpty->setFont(f);
		m_lblEmpty->setPaletteForegroundColor(m_textFg);
		m_lblEmpty->show();
		lay->addWidget(m_lblEmpty);
	} else {
		m_lblEmpty = NULL;
		for (std::vector<CalcHistoryItem>::const_reverse_iterator it = items.rbegin(); it != items.rend(); ++it) {
			HistoryItemWidget *w = new HistoryItemWidget(*it, m_container);
			w->setColors(m_panelBg, m_textFg, m_displayBg);
			connect(w, TQ_SIGNAL(clicked(const CalcHistoryItem&)), this, TQ_SLOT(slotItemClicked(const CalcHistoryItem&)));
			w->show();
			lay->addWidget(w);
		}
	}
	lay->addStretch(1);
	
	updateLayoutSizes();
}

void HistoryWindow::setColors(const TQColor &panelBg, const TQColor &textFg, const TQColor &displayBg)
{
	m_panelBg = panelBg;
	m_textFg = textFg;
	m_displayBg = displayBg;

	setPaletteBackgroundColor(panelBg);
	m_scroll->setPaletteBackgroundColor(panelBg);
	m_scroll->viewport()->setPaletteBackgroundColor(panelBg);
	m_container->setPaletteBackgroundColor(panelBg);
	
	if (m_btnClear) {
		TQPalette btnPal = m_btnClear->palette();
		btnPal.setColor(TQPalette::Active, TQColorGroup::Background, panelBg);
		btnPal.setColor(TQPalette::Inactive, TQColorGroup::Background, panelBg);
		btnPal.setColor(TQPalette::Active, TQColorGroup::Button, panelBg);
		btnPal.setColor(TQPalette::Inactive, TQColorGroup::Button, panelBg);
		btnPal.setColor(TQPalette::Active, TQColorGroup::ButtonText, textFg);
		btnPal.setColor(TQPalette::Inactive, TQColorGroup::ButtonText, textFg);
		btnPal.setColor(TQPalette::Active, TQColorGroup::Foreground, textFg);
		btnPal.setColor(TQPalette::Inactive, TQColorGroup::Foreground, textFg);
		m_btnClear->setPalette(btnPal);
		m_btnClear->setBackgroundColor(panelBg);
	}
	
	m_bottomBar->setPaletteBackgroundColor(panelBg);

	setHistory(m_items);
}

void HistoryWindow::retranslateUi()
{
	setCaption(tr_str("History"));
	if (m_btnClear) {
		TQToolTip::add(m_btnClear, tr_str("Clear all"));
	}
	if (m_lblEmpty) {
		m_lblEmpty->setText(tr_str("No history yet"));
	}
}

void HistoryWindow::updateLayoutSizes()
{
	double scale = static_cast<double>(height()) / 480.0;
	if (scale < 0.5) scale = 0.5;

	if (m_btnClear) {
		int btn_sz = static_cast<int>(36.0 * scale);
		if (btn_sz < 24) btn_sz = 24;
		m_btnClear->setFixedSize(btn_sz, btn_sz);
	}

	if (m_lblEmpty) {
		int emptySize = static_cast<int>(12.0 * scale);
		if (emptySize < 8) emptySize = 8;
		TQFont f = m_lblEmpty->font();
		f.setPixelSize(emptySize);
		m_lblEmpty->setFont(f);
	}

	if (m_container) {
		const TQObjectList *list = m_container->children();
		if (list) {
			TQObjectListIt it(*list);
			TQObject *obj;
			while ((obj = it.current()) != 0) {
				++it;
				HistoryItemWidget *w = dynamic_cast<HistoryItemWidget*>(obj);
				if (w) {
					w->updateItemSizes(scale);
				}
			}
		}
		m_container->updateGeometry();
	}

	if (m_container && m_scroll) {
		int w = m_scroll->visibleWidth();
		m_container->resize(w, m_container->sizeHint().height());
		m_scroll->resizeContents(w, m_container->height());
	}
}

void HistoryWindow::resizeEvent(TQResizeEvent *e)
{
	TQDialog::resizeEvent(e);
	updateLayoutSizes();
}

void HistoryWindow::slotItemClicked(const CalcHistoryItem &item)
{
	emit itemSelected(item);
}

void HistoryWindow::slotClearClicked()
{
	emit clearHistoryRequested();
}

#include "history_window.moc"
