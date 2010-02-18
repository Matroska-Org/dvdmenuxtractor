#ifndef DMX_H
#define DMX_H

#include <vector>
#include <QThread>
#include "dmxselectionitem.h"
#include "vobparser/IFOFile.h"

class DMX : public QThread
{
	Q_OBJECT

public:
	typedef std::vector<DMXSelectionItem> SelectionType;

  DMX(bool consoleMode = false);
  ~DMX();

	static IFOFile* OpenIFOFile(const QString& path);
	bool setExtractionParameters(const QString& sourcePath, const QString& destinationPath, const QString& toolsPath, const SelectionType& selection);
	
signals:
	// Signal is emitted when the current step progress is changed
	void progressChanged(int);

	// Signal is emitted when the step is changed
	void stepChanged(const QString&);

public slots:
	void abort();

protected:
	void run();

private:
	IFOFile *ifoFile_;
	bool consoleMode_;
	QString toolsPath_;
	QString sourcePath_;
	QString destinationPath_;
	SelectionType selection_;
	volatile bool needsAbort_;
	
	bool loadIFOFile(const QString& path);
	void processTitle(int16_t title, int index);

	VobParser* buildVobParser(int16_t title, bool isMenu);
	
	void demux(VobParser* aVobParser, int selectionIndex, const QString& editionUID, int16_t title, bool isMenu);
	void demuxAudioTrack(int16_t title, bool isMenu, const AudioTrackList& _audioTracks, size_t _stream, CompositeDemuxWriter& demuxer, const QString& filename);
	void demuxSubtitleTrack(int16_t title, bool isMenu, const SubtitleTrackList& _subTracks, size_t _stream,  CompositeDemuxWriter& demuxer, const QString& filename, const uint32_t *_palette, uint16_t _width, uint16_t _height);
};

#endif // DMX_H
