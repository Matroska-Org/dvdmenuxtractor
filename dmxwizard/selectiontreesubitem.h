#ifndef SELECTION_TREE_SUB_ITEM_H
#define SELECTION_TREE_SUB_ITEM_H

// forward declaration
class QTreeWidgetItem;
class SelectionTreeItem;

class SelectionTreeSubItem : public QTreeWidgetItem
{
public:
	enum SubItemType { AUDIO_TRACK, SUBTITLE_TRACK };
	SelectionTreeSubItem(QTreeWidgetItem *parent, const SelectionTreeItem& titleItem, size_t index, SubItemType type);
	SelectionTreeSubItem(const SelectionTreeSubItem& other);
	~SelectionTreeSubItem();

	size_t index() const;
	SubItemType type() const;
	const SelectionTreeItem& titleItem() const;

private:
	size_t index_;
	SubItemType type_;
	const SelectionTreeItem &titleItem_;
};

#endif // SELECTION_TREE_SUB_ITEM_H
