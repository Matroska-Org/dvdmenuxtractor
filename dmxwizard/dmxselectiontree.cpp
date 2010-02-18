#include "dmxselectiontree.h"
#include "selectiontreeitem.h"
#include "selectiontreesubitem.h"

DMXSelectionTree::DMXSelectionTree(QWidget *parent)
: QTreeWidget(parent), ifoFile_(0)
{
	setColumnCount(1);
	setHeaderLabels(QStringList("DVD"));
	setAlternatingRowColors(true);
}

DMXSelectionTree::~DMXSelectionTree()
{
	delete ifoFile_;
	ifoFile_ = 0;
}

bool DMXSelectionTree::loadIFOFile(const QString &path)
{
	clear();
	setHeaderLabels(QStringList(QString("DVD - %1").arg(path)));

	// reset last file
	delete ifoFile_;
	ifoFile_ = DMX::OpenIFOFile(path);

	if (ifoFile_ == 0)
		return false;

	bool menu = false;
	SelectionTreeItem *item = 0;
	SelectionTreeSubItem *subItem = 0;
	QTreeWidgetItem *videoStreamItem = 0;
	QTreeWidgetItem *audioTrackItem = 0;
	QTreeWidgetItem *subtitleTrackItem = 0;

	for (int16_t title = 0; title <= ifoFile_->NumberOfTitles(); ++title)
	{
		menu = !title;

		do
		{
			item = new SelectionTreeItem(this, title, menu);

			// add video stream item
			videoStreamItem = new QTreeWidgetItem(item, QStringList("Video Stream"));
			videoStreamItem->setFlags(videoStreamItem->flags() | Qt::ItemIsTristate);
			videoStreamItem->setCheckState(0, Qt::Checked);

			// get audio tracks
			const AudioTrackList& audioTracks = ifoFile_->AudioTracks(title, menu);

			if (audioTracks.size())
			{
				audioTrackItem = new QTreeWidgetItem(item, QStringList("Audio Tracks"));
				audioTrackItem->setFlags(audioTrackItem->flags() | Qt::ItemIsTristate);
				audioTrackItem->setCheckState(0, Qt::Checked);

				for (size_t index = 0; index < audioTracks.size(); ++index)
					subItem = new SelectionTreeSubItem(audioTrackItem, *item, index, SelectionTreeSubItem::AUDIO_TRACK);
			}
			
			// get subtitle tracks
			const SubtitleTrackList& subTracks = ifoFile_->SubsTracks(title, menu);

			if (subTracks.size())
			{
				subtitleTrackItem = new QTreeWidgetItem(item, QStringList("Subtitle Tracks"));
				subtitleTrackItem->setFlags(subtitleTrackItem->flags() | Qt::ItemIsTristate);
				subtitleTrackItem->setCheckState(0, Qt::Checked);

				for (size_t index = 0; index < subTracks.size(); ++index)
					subItem = new SelectionTreeSubItem(subtitleTrackItem, *item, index, SelectionTreeSubItem::SUBTITLE_TRACK);
			}
			
			menu = !menu;
		} while (menu);
	}

	return true;
}

DMX::SelectionType DMXSelectionTree::getSelection() const
{
	DMX::SelectionType selection;
	QTreeWidgetItem *trackListItem = 0;

	for (int index = 0; index < topLevelItemCount(); ++index)
	{
		// if processing title item
		if ( SelectionTreeItem *item = dynamic_cast<SelectionTreeItem*>(topLevelItem(index)) )
		{
			// if the title item is (partially) checked
			if (item->checkState(0) != Qt::Unchecked)
			{
				// create selection item instance
				DMXSelectionItem selectionItem(item->title(), item->isMenuItem(), true);

				// check if video stream is selected
				if ((item->childCount() > 0) && (item->child(0)->checkState(0) == Qt::Unchecked))
					selectionItem.disableVideo();

				for (int index = 0; index < item->childCount(); ++index)
				{
					trackListItem = item->child(index);

					// check if track list item is checked
					if (item->checkState(0) != Qt::Unchecked)
					{
						for (int index = 0; index < trackListItem->childCount(); ++index)
						{
							// process track subitem
							if (SelectionTreeSubItem *subItem = dynamic_cast<SelectionTreeSubItem*>(trackListItem->child(index)))
							{
								// if sub item is checked
								if (subItem->checkState(0) == Qt::Checked)
								{
									if (subItem->type() == SelectionTreeSubItem::AUDIO_TRACK)
										selectionItem.addAudioTrack(subItem->index());

									if (subItem->type() == SelectionTreeSubItem::SUBTITLE_TRACK)
											selectionItem.addSubtitleTrack(subItem->index());
								}
							}
						}
					}
				}

				selection.push_back(selectionItem);
			}
		}
	}

	return selection;
}
