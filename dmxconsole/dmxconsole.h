#ifndef DMXCONSOLE_H
#define DMXCONSOLE_H

#include "dmx.h"

class DMXConsole
{
public:
	DMXConsole(char *arguments[], int argumentCount);
  ~DMXConsole();

	void extract();

	static void ShowUsage();

private:
	bool ready_;
	QString toolsPath_;
	QString sourcePath_;
	QString destinationPath_;
	DMX::SelectionType selectionItems_;

	enum {TITLE_INDEX = 0, MENU_INDEX, VIDEO_INDEX,
				AUDIO_TRACKS_INDEX, SUBTITLE_TRACKS_INDEX, ITEM_COUNT};
	
	QStringList extractTrackNumbers(const QString& trackNumberString);
	DMX::SelectionType generateSelectionItems(const QString& selectionString);
};

#endif // DMXCONSOLE_H
