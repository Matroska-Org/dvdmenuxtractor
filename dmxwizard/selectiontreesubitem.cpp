#include <QTreeWidgetItem>
#include "selectiontreesubitem.h"

SelectionTreeSubItem::SelectionTreeSubItem(QTreeWidgetItem *parent, const SelectionTreeItem &titleItem, size_t index, SelectionTreeSubItem::SubItemType type)
	: QTreeWidgetItem(parent)
	,index_(index)
	,type_(type)
	,titleItem_(titleItem)
{
	static const QString audioTrackTitleFormat ("Audio Track %1");
	static const QString subTrackTitleFormat ("Subtitle Track %1");
	
	if (type == AUDIO_TRACK)
		setText(0, audioTrackTitleFormat.arg(index));
	else
		setText(0, subTrackTitleFormat.arg(index));

	setFlags(flags() | Qt::ItemIsTristate);
	setCheckState(0, Qt::Checked);
}

SelectionTreeSubItem::SelectionTreeSubItem(const SelectionTreeSubItem &other)
	: QTreeWidgetItem(other)
	,index_(other.index_)
	,type_(other.type_)
	,titleItem_(other.titleItem_)
{
	setCheckState(0, other.checkState(0));
}

SelectionTreeSubItem::~SelectionTreeSubItem()
{
}

size_t SelectionTreeSubItem::index() const
{
	return index_;
}

SelectionTreeSubItem::SubItemType SelectionTreeSubItem::type() const
{
	return type_;
}

const SelectionTreeItem& SelectionTreeSubItem::titleItem() const
{
	return titleItem_;
}
