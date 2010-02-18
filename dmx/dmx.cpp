#include "dmx.h"
#include "utilities.h"
#include "chaptermanager.h"

#include <QDir>
#include <QTime>
#include <QMutexLocker>

DMX::DMX(bool consoleMode)
	: ifoFile_(0), consoleMode_(consoleMode), needsAbort_(false)
{
}

DMX::~DMX()
{
	abort();

	QMutex mutex;
	mutex.lock();
	delete ifoFile_;
	ifoFile_ = 0;
	mutex.unlock();
}

void DMX::abort()
{
	QMutex mutex;
	mutex.lock();
	needsAbort_ = true;
	mutex.unlock();

	wait();
}

void DMX::processTitle(int16_t title, int index)
{
	const QString editionUID = Utilities::CreateUID();

	unsigned stepIndex = 0;
	bool menu = ((index < 0) && !title) || ((index >= 0) && selection_[index].isMenu());
	VobParser *aVobParser = 0;
	QString str;
	QString text = "Step %1 of 2: %3...";

	do
	{
		printf("Treating Title %d %s VOB file(s)\n", title, menu ? "Menu" : "");

		stepIndex = 1;

		str = text.arg(stepIndex++).arg("Building VOB map");
		
		if (consoleMode_)
			printf(qPrintable(str + '\n'));
		else
			emit stepChanged(str);

		aVobParser = buildVobParser(title, menu);

		str = text.arg(stepIndex++).arg("Splitting and demuxing");
		
		if (consoleMode_)
			printf(qPrintable(str + '\n'));
		else
			emit stepChanged(str);
		
		demux(aVobParser, index, editionUID, title, menu);

		delete aVobParser;
		
		menu = !menu && ((index < 0) || selection_[index].isMenu());
	} while (menu && !needsAbort_);
}

bool DMX::setExtractionParameters(const QString& sourcePath, const QString& destinationPath, const QString& toolsPath, const SelectionType& selectedItems)
{
	QMutex mutex;
	QMutexLocker locker (&mutex);

	// set selection
	selection_ = selectedItems;

	// check if require directories exist
	if (!QFile::exists(sourcePath))
	{
		fprintf(stderr, "Input folder '%s' doesn't exist\n", qPrintable(sourcePath));
		return false;
	}

	// set source path
	sourcePath_ = sourcePath;

	if (!QFile::exists(toolsPath))
	{
		fprintf(stderr, "Tools folder '%s' doesn't exist\n", qPrintable(toolsPath));
		return false;
	}

	// set "mkvmerge.exe" path
	toolsPath_ = toolsPath;

	// try to create output
	if (!QFile::exists(destinationPath))
	{
		if (!QDir().mkpath(destinationPath))
		{
			fprintf(stderr,"Cannot create output folder: '%s'\n", qPrintable(destinationPath));
			return false;
		}
		else
			printf("Successfully created output folder: '%s'\n", qPrintable(destinationPath));
	}

	// set output path
	destinationPath_ = destinationPath;

	return true;
}

void DMX::run()
{
	// try to open input file
	if (!sourcePath_.size() || !loadIFOFile(sourcePath_))
		return;

	QMutex mutex;
	mutex.lock();
	needsAbort_ = false;
	mutex.unlock();

	if (selection_.size()) // if selection is available
	{
		for (size_t index = 0; index < selection_.size(); ++index)
		{
			int16_t title = selection_[index].title();
			
			if (title >= 0)
				processTitle(title, index);
		}
	} 
	else // if no selection is available, process all titles
	{
		for (int16_t title = 0; title <= ifoFile_->NumberOfTitles(); ++title)
			processTitle(title, -1);
	}
}

IFOFile* DMX::OpenIFOFile(const QString& path)
{
	IFOFile *file = 0;
	QDir info (path);
	
	try
	{
		file = new IFOFile(info.absolutePath());
	}
	catch(IFOException&)
	{
		// TODO : More verbose
		fprintf(stderr, "Couldn't use input directory as DVD source\n");
		return 0;
	}

	return file;
}

bool DMX::loadIFOFile(const QString& path)
{
	QMutex mutex;
	QMutexLocker locker (&mutex);
	
	// reset last file
	delete ifoFile_;
	ifoFile_ = OpenIFOFile(path);

	if (ifoFile_ == 0)
		return false;

	return true;
}

VobParser* DMX::buildVobParser(int16_t title, bool menu)
{
	VobParser* parser = 0;
	if (consoleMode_)
		printf("0%%");
	else
		emit progressChanged(0);
	
	try 
	{
		parser = new VobParser(qPrintable(sourcePath_), title, menu);
	} catch (...)
	{
		if (consoleMode_)
			printf("\r\r");
		
		printf("No VOB file(s) found in %s for Title %d\n", qPrintable(sourcePath_), title);
	}

	if (consoleMode_)
		printf("\r\r100%%\n");
	else
		emit progressChanged(100);
	
	return parser;
}

void DMX::demuxAudioTrack(int16_t title, bool isMenu, const AudioTrackList& _audioTracks, size_t _stream, CompositeDemuxWriter& demuxer, const QString& filename)
{
	if (_stream < 0 || _stream >= _audioTracks.size())
		return;

	static const QString muxArgumentsFormat(" --track-name 0:\"aud-%1\" --language 0:%2 --timecodes 0:\"%3_%4.tmc\" \"%3.%4\"");

	const audio_attr_t *_attr = _audioTracks[_stream];
	// get the possible ID for this track for this Cell/PGC
	uint8_t _ID = ifoFile_->GetAudioId(_stream, title, isMenu);

	Writer *_muxer = 0;
	
	// if (_attr->lang_code == 0)
	QString lang ("und");
	QString langSuffix (QString("_%1_un").arg(_ID));
	
	if (_attr->lang_code != 0)
	{
		lang = QString("%1%2").arg(QString(_attr->lang_code >> 8), QString(_attr->lang_code & 0xFF));
		langSuffix = QString("_%1_%2%3").arg(QString::number(_ID), QString(_attr->lang_code >> 8), QString(_attr->lang_code & 0xFF));
	}

	QString muxArguments;
	QString fullPrefix (destinationPath_ + QDir::separator() + filename + langSuffix);

	switch (_audioTracks[_stream]->audio_format)
	{
	case 0:
		muxArguments = muxArgumentsFormat.arg(filename + langSuffix, lang, fullPrefix, "ac3");		
		_muxer = new AC3DemuxWriter(fullPrefix, 0x80 + _ID);

		if (!demuxer.AddDemuxer(SUBSTREAM_AC3_LOW + _ID, _muxer, muxArguments))
			delete _muxer;
		break;

	case 2:
	case 3:
		muxArguments = muxArgumentsFormat.arg(filename + langSuffix, lang, fullPrefix, "mpa");		
		_muxer = new MPADemuxWriter(fullPrefix, 0xC0 + _ID);

		if (!demuxer.AddDemuxer(AUDIO_STREAM + _ID, _muxer, muxArguments))
			delete _muxer;
		break;

	case 4:
		// TODO possibly other LPCM formats ?
		muxArguments = muxArgumentsFormat.arg(filename + langSuffix, lang, fullPrefix, "wav");
		_muxer = new LPCMDemuxWriter(fullPrefix, 0xA0 + _ID, 48000, 16, 2);

		if (!demuxer.AddDemuxer(SUBSTREAM_PCM_LOW + _ID, _muxer, muxArguments))
			delete _muxer;
		break;

	case 6:
		muxArguments = muxArgumentsFormat.arg(filename + langSuffix, lang, fullPrefix, "dts");
		_muxer = new DTSDemuxWriter(fullPrefix, 0x88 + _ID);

		if (!demuxer.AddDemuxer(SUBSTREAM_DTS_LOW + _ID, _muxer, muxArguments))
			delete _muxer;
		break;

	default:
		fprintf(stderr, "Unknown Audio Format: %d\n", _attr->audio_format);
	}
}

void DMX::demuxSubtitleTrack(int16_t title, bool isMenu, const SubtitleTrackList& _subTracks, size_t _stream,  CompositeDemuxWriter& demuxer, const QString& filename, const uint32_t *_palette, uint16_t _width, uint16_t _height)
{
	if (_stream < 0 || _stream >= _subTracks.size())
		return;

	static const QString subMuxArgumentsFormat(" --track-name 0:\"sub-%1\" --language 0:%2 \"%3\"");

	const subp_attr_t *_attr = 0;
	const QString prefix = destinationPath_ + QDir::separator() + filename;

	// get the possible IDs for this track for this Cell/PGC
	IdArray _IDs = ifoFile_->GetSubsId(_stream, title, isMenu);

	Writer * _muxer = 0;
	QString lang, langSuffix;
	
	for (IdArray::size_type _IDidx=0; _IDidx < _IDs.size(); ++_IDidx)
	{
		_attr = _subTracks[_stream];
		
		// if (_attr->lang_code == 0)
		lang = "und";
		langSuffix = QString("_%1_un").arg(_IDs.at(_IDidx));

		if (_attr->lang_code != 0)
		{
			lang = QString("%1%2 \"").arg(QString(_attr->lang_code >> 8), QString(_attr->lang_code & 0xFF));
			langSuffix = QString("_%1_%2%3").arg(QString::number(_IDs.at(_IDidx)), QString(_attr->lang_code >> 8), QString(_attr->lang_code & 0xFF));		
		}

		_muxer = new SubDemuxWriter(QString(prefix + langSuffix), 0x20 + _IDs.at(_IDidx), _width, _height, _palette, _attr->lang_code, _attr->lang_extension == 9);

		QString commandLine = subMuxArgumentsFormat.arg(filename + langSuffix, lang, prefix + langSuffix + ".idx");
		if (!demuxer.AddDemuxer(SUBSTREAM_SUB_LOW + _IDs.at(_IDidx), _muxer, commandLine))
			delete _muxer;
	}
}

void DMX::demux(VobParser *aVobParser, int selectionIndex, const QString &editionUID, int16_t title, bool menu)
{
	// get list of all cells
	const CellsListType *CellsList = ifoFile_->GetCellsList(title, menu);
	
	if (!CellsList)
		return;

	static const QString demuxArguments (" --track-name 0:\"video\" --timecodes 0:\"%1_m2v.tmc\" \"%1.m2v\"");
	static const QString btnMuxArgumentsFormat(" --track-name 0:\"btn-%1\" --timecodes 0:\"%2_btn.tmc\" \"%2.btn\"");
	
	try
	{
		QString filename;
		QString muxCommand;

		// init filename
		if (title == 0)
			filename = "VMG";
		else
			filename = QString("VTS%1%2").arg(menu ? "M" : "").arg(title, 2, 10, QChar('0'));

		const QString prefix = destinationPath_ + QDir::separator() + filename;

		if (aVobParser != 0)
		{
			aVobParser->Reset();

			const AudioTrackList & _audioTracks = ifoFile_->AudioTracks(title, menu);
			const SubtitleTrackList & _subTracks = ifoFile_->SubsTracks(title, menu);
			
			double _fps = 0;
			uint16_t _width = 0, _height = 0;
			
			ifoFile_->VideoSize(title, menu, _width, _height, _fps);
			const uint32_t * _palette = ifoFile_->GetPalette(title, menu);

			CompositeDemuxWriter &demuxer = aVobParser->GetDemuxer();

			//emit progressChanged(CellsList->count());

			demuxer.Reset();
			printf("Processing %s\n", qPrintable(filename));

			if ((selectionIndex < 0) || (selection_[selectionIndex].isVideoEnabled()))
			{
				Writer *_muxer = new VideoDemuxWriter(prefix, _fps);

				QString commandLine = demuxArguments.arg(prefix);
				if (!demuxer.AddDemuxer(VIDEO_STREAM, _muxer, commandLine))
					delete _muxer;
			}

			size_t _stream = 0;

			QString lang;
			QString langSuffix;

			// decide which audio streams to process
			if (_audioTracks.size())
			{
				if (selectionIndex > -1)
				{
					const std::vector<size_t>& selectedAudioTracks = selection_[selectionIndex].audioTracks();
					
					for (size_t index = 0; index < selectedAudioTracks.size(); ++index)
					{
						_stream = selectedAudioTracks[index];
						demuxAudioTrack(title, menu, _audioTracks, _stream,  demuxer, filename);
					}
				}
				else
				{
					for (_stream = 0; _stream < _audioTracks.size(); ++_stream)
						demuxAudioTrack(title, menu, _audioTracks, _stream, demuxer, filename);
				}
			}

			// process subtitle streams
			if (_subTracks.size())
			{
				if (selectionIndex > -1)
				{
					const std::vector<size_t>& selectedSubTracks = selection_[selectionIndex].subtitleTracks();

					for (size_t index = 0; index < selectedSubTracks.size(); ++index)
					{
						_stream = selectedSubTracks[index];
						demuxSubtitleTrack(title, menu, _subTracks, _stream,  demuxer, filename, _palette, _width, _height);
					}
				}
				else
				{
					for (_stream = 0; _stream < _subTracks.size(); ++_stream)
						demuxSubtitleTrack(title, menu, _subTracks, _stream, demuxer, filename, _palette, _width, _height);
				}

				// create a possible button demuxer too
				Writer *_muxer = new BtnDemuxWriter(prefix, _width, _height);

				QString commandLine = btnMuxArgumentsFormat.arg(filename, prefix);
				if (!demuxer.AddDemuxer(SUBSTREAM_PCI, _muxer, commandLine))
					delete _muxer;
			}

			if (consoleMode_)
				printf("0%%");
			else
				emit progressChanged(0);
			
			const uint32_t maximum = aVobParser->GetPacketCount();
			
			while(aVobParser->ParseNextPacket(*CellsList) && !needsAbort_)
			{
				if (consoleMode_)
					printf("\r\r\r%d%%", aVobParser->GetPacketIndex() * 100 / maximum);
				else
					emit progressChanged(aVobParser->GetPacketIndex() * 100 / maximum);
			}
			printf("\n");

			// create the command line using the list of used files
			// always put video first
			if (demuxer.FileExists(VIDEO_STREAM))
				muxCommand += demuxer.GetString(VIDEO_STREAM);

			for (_stream = 0; _stream < 256; _stream++)
			{
				if (_stream == VIDEO_STREAM)
					continue;
				
				if (demuxer.FileExists(_stream))
					muxCommand += demuxer.GetString(_stream);
			}
		}

		CellsListType *CellsListDone = (CellsListType *)CellsList;
		CellsListDone->arrange();

		bool addChapters = false;
		ChapterManager chapterEditor(2 /*indent count*/);

		if (menu)
			addChapters = chapterEditor.generateMenuScript(*ifoFile_, prefix, title, editionUID);
		else
			addChapters = chapterEditor.generateScript(*ifoFile_, prefix, title, editionUID);

		if (addChapters)
		{
			muxCommand += " --chapters \"" + prefix + ChapterManager::CHAPTER_SUFFIX + "\"";
			muxCommand += " --segmentinfo \"" + prefix + ChapterManager::INFO_SUFFIX + "\"";
		}

#if (defined(WIN32) || defined(WIN64))
		muxCommand.insert(0, "\"" + toolsPath_ + "\\mkvmerge\" -o \"" + prefix + ".mkv\"");
		QFile muxBatchFile(prefix + "_" + QTime::currentTime().toString("hh_mm_ss") + ".bat");
#else
		muxCommand.insert(0, toolsPath_ + "/mkvmerge -o \"" + prefix + ".mkv\"");
		QFile muxBatchFile(prefix + "_" + QTime::currentTime().toString("hh_mm_ss") + ".sh");
#endif

		if (!muxBatchFile.open( QIODevice::WriteOnly | QIODevice::Text ))
			fprintf(stderr, "Could not create batch file\n");
		
#if !defined(WIN32) && !defined(WIN64)
		QString shellString ("#!/bin/sh\n\n");
		muxBatchFile.write(shellString.toUtf8());
#endif
		muxBatchFile.write(muxCommand.toUtf8());
		muxBatchFile.close();

		printf("Done demuxing %s\n", qPrintable(filename));
	}
	catch(VobParserException e)
	{
		fprintf(stderr, "Vob Parser Exception Occurred: %s\n", e.what());
	}
}
