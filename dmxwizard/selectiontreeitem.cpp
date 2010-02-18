#include <QTreeWidgetItem>
#include "selectiontreeitem.h"

SelectionTreeItem::SelectionTreeItem(QTreeWidget *parent, int16_t title, bool isMenuItem)
	: QTreeWidgetItem(parent)
	,title_(title)
	,isMenuItem_(isMenuItem)
{
	static const QString vmgTitleFormat(QObject::tr("VMG"));
	static const QString vtsTitleFormat(QObject::tr("Title %1"));
	static const QString vtsmTitleFormat(QObject::tr("Title %1 Menu"));
	
	if (isMenuItem && title)
		setText(0, vtsmTitleFormat.arg(QString::number(title), 2, '0'));
	else if (isMenuItem)
		setText(0, vmgTitleFormat);
	else
		setText(0, vtsTitleFormat.arg(QString::number(title), 2, '0'));

	setFlags(flags() | Qt::ItemIsTristate);
	setCheckState(0, Qt::Checked);
}

SelectionTreeItem::SelectionTreeItem(const SelectionTreeItem &other)
	: QTreeWidgetItem(other)
	,title_(other.title_)
	,isMenuItem_(other.isMenuItem_)
{
	setCheckState(0, other.checkState(0));
}

SelectionTreeItem::~SelectionTreeItem()
{

}

bool SelectionTreeItem::isMenuItem() const
{
	return isMenuItem_;
}

int16_t SelectionTreeItem::title() const
{
	return title_;
}
