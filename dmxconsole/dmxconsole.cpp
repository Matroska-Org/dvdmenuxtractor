#include <iostream>

#include "utilities.h"
#include "dmxconsole.h"

DMXConsole::~DMXConsole()
{
}

DMXConsole::DMXConsole(char *arguments[], int argumentCount)
{
	ready_ = true;

	if ((argumentCount != 7) && (argumentCount != 9))
	{
		ShowUsage();
		ready_ = false;
		return;
	}

	QString argument;
	
	for (int i = 1; i < argumentCount; ++i)
	{
		argument = arguments[i];

		if (argument == "-h")
		{
			ShowUsage();
			ready_ = false;
			return;
		}
		else if (argument == "-i")
			sourcePath_ = arguments[++i];
		else if (argument == "-o")
			destinationPath_ = arguments[++i];
		else if (argument == "-t")
			toolsPath_ = arguments[++i];
		else if (argument == "-s")
			selectionItems_ = generateSelectionItems(QString(arguments[++i]));
		else
		{
			std::cout << "ERROR: Unknown option was specified" << std::endl;
			DMXConsole::ShowUsage();
			ready_ = false;
			return;
		}
	}
}

DMX::SelectionType DMXConsole::generateSelectionItems(const QString& selectionString)
{
	QString item;
	QStringList subItems;
	DMX::SelectionType selectionItems;

	// split string using ";" as delimeter into selection item string
	QStringList items = selectionString.split(";");

	// procfess each substring
	for (int index = 0; index < items.size(); ++index)
	{
		item = items.at(index);

		// split subitems into smaller parts using "," as delimeter
		subItems = item.split(",");

		// now check if we have all the components
		if (subItems.size() == ITEM_COUNT)
		{
			DMXSelectionItem item(subItems.at(TITLE_INDEX).toInt(), 
														subItems.at(MENU_INDEX).toInt(),
														subItems.at(VIDEO_INDEX).toInt());

			// add audio tracks
			int index = 0;
			QStringList trackList = extractTrackNumbers(subItems.at(1));
			for (index = 0; index < trackList.size(); ++index)
				item.addAudioTrack(trackList.at(index).toUInt());

			// add subtitle tracks
			trackList = extractTrackNumbers(subItems.at(2));
			for (index = 0; index < trackList.size(); ++index)
				item.addSubtitleTrack(trackList.at(index).toUInt());

			selectionItems.push_back(item);
		}
		else
			fprintf(stderr, "WARNING: Incorrect selection item at index %d was found\n", index);
	}

	return selectionItems;
}

QStringList DMXConsole::extractTrackNumbers(const QString& trackNumberString)
{
	if ( trackNumberString.size() <= 2 )
		return QStringList();

	return trackNumberString.mid(1, trackNumberString.size() - 1).split(",");
}

void DMXConsole::extract()
{
	if (ready_)
	{
		DMX extractor (true);
		extractor.setExtractionParameters(sourcePath_, destinationPath_, toolsPath_, selectionItems_);
		extractor.start();
		extractor.wait();
	}
}

void DMXConsole::ShowUsage()
{
	std::cout << "USAGE: DvdMenuExtractor [<options>]\n\n"
						<< " Show usage:        -h\n"
						<< " Specify folders:   -i <dir> -o <dir> -t <dir>\n"
						<< " Specify selection: -s title, extractMenu, extractVideo, {audioTracks}, {subTracks};..."
						<< std::endl;
}
