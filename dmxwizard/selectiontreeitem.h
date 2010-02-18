#ifndef SELECTIONTREEITEM_H
#define SELECTIONTREEITEM_H

#include <stdint.h>

// forward declaration
class QTreeWidgetItem;

class SelectionTreeItem : public QTreeWidgetItem
{
public:

  SelectionTreeItem(QTreeWidget *parent, int16_t title, bool isMenuItem);
	SelectionTreeItem(const SelectionTreeItem &other);
  ~SelectionTreeItem();

	int16_t title()		const;
	bool isMenuItem() const;

private:
	int16_t title_;
	bool isMenuItem_;
};

#endif // SELECTIONTREEITEM_H
