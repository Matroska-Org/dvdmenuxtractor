#ifndef DMX_SELECTIONTREE_H
#define DMX_SELECTIONTREE_H

#include "dmx.h"
#include <QTreeWidget>

class DMXSelectionTree : public QTreeWidget
{
	Q_OBJECT

public:
  DMXSelectionTree(QWidget *parent);
  ~DMXSelectionTree();

	DMX::SelectionType getSelection() const;
	bool loadIFOFile(const QString& path);

private:
	IFOFile *ifoFile_;
};

#endif // DMX_SELECTIONTREE_H
