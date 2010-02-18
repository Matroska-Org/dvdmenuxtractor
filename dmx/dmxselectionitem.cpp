#include <algorithm>
#include "dmxselectionitem.h"

DMXSelectionItem::DMXSelectionItem(int16_t title, bool isMenu, bool enableVideo = 1)
	: title_(title), menu_(isMenu), videoEnabled_(enableVideo)
{
	// override for VMG
	if (!title)
		menu_ = true;
}

DMXSelectionItem::~DMXSelectionItem()
{
}

int16_t DMXSelectionItem::title() const
{
	return title_;
}

bool DMXSelectionItem::isMenu() const
{
	return menu_;
}

bool DMXSelectionItem::isVideoEnabled() const
{
	return videoEnabled_;
}

const DMXSelectionItem::TracksContainerType& DMXSelectionItem::audioTracks() const
{
	return audioTracks_;
}

const DMXSelectionItem::TracksContainerType& DMXSelectionItem::subtitleTracks() const
{
	return subtitleTracks_;
}

void DMXSelectionItem::disableVideo()
{
	videoEnabled_ = false;
}

bool DMXSelectionItem::addAudioTrack(size_t trackIndex)
{
	// check if audio track was already selected
	if (std::count(audioTracks_.begin(), audioTracks_.end(), trackIndex) == 0)
	{
		audioTracks_.push_back(trackIndex);
		return true;
	}

	return false;
}

bool DMXSelectionItem::addSubtitleTrack(size_t trackIndex)
{
	// check if audio track was already selected
	if (std::count(subtitleTracks_.begin(), subtitleTracks_.end(), trackIndex) == 0)
	{
		subtitleTracks_.push_back(trackIndex);
		return true;
	}

	return false;
}
