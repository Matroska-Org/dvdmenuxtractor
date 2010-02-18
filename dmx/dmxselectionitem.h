#ifndef DMX_SELECTION_ITEM
#define DMX_SELECTION_ITEM

#include <vector>
#include <stdint.h>

// Denotes each selected title
class DMXSelectionItem
{
public:

	typedef std::vector<size_t> TracksContainerType;

	DMXSelectionItem(int16_t title, bool isMenu, bool enableVideo);
	~DMXSelectionItem();

	// Getters
	bool isMenu() const;
	int16_t title() const;
	bool isVideoEnabled() const;
	const TracksContainerType& audioTracks() const;
	const TracksContainerType& subtitleTracks() const;

	void disableVideo();
	bool addAudioTrack(size_t trackIndex);
	bool addSubtitleTrack(size_t trackIndex);

private:
	int16_t title_;
	bool menu_;
	bool videoEnabled_;

	TracksContainerType audioTracks_;
	TracksContainerType subtitleTracks_;
};


#endif // DMX_SELECTION_ITEM
