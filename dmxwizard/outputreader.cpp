#if defined(WIN32) || defined(WIN64)
#include <io.h>
#endif
#include <QDir>
#include <fcntl.h>
#include <fstream>
#include <QWidget>
#include <QTimerEvent>
#include "outputreader.h"

OutputReader::OutputReader(QObject *parent /* = 0*/)
  : QObject(parent)
	,redirecting_(false)
	,stdoutFileName("stdout.txt")
	,stderrFileName("stderr.txt")
	,stderrStream(0)
	,stdoutStream(0)
{
}

OutputReader::~OutputReader()
{
	if (redirecting_)
		cancelRedirect();
}

void OutputReader::timerEvent(QTimerEvent *event)
{
	// if the event is intended for us
	if ((event->timerId() == timer.timerId()) && redirecting_)
	{
		QFile stdoutFile (stdoutFileName);
		QFile stderrFile (stderrFileName);
	
		static qint64 stderrFileSize = 0;
		static qint64 stdoutFileSize = 0;

		qint64 size = stderrFile.size();

		if (size > stderrFileSize) // new data is available
		{
			stderrFileSize = size;
			emit readyReadStandardError();
		}

		size = stdoutFile.size();

		if (size > stdoutFileSize) // new data is available
		{
			stdoutFileSize = size;
			emit readyReadStandardOutput();
		}
	}
	else
		QObject::timerEvent(event);
}

bool OutputReader::redirect() 
{
	if (redirecting_)
		return true;

	// save old descriptors
#if defined(WIN32) || defined(WIN64)
	stdout_desc = _dup(1);
	stderr_desc = _dup(2);
#else
	stdout_desc = dup(1);
	stderr_desc = dup(2);
#endif

	if ((stdout_desc == -1) || (stderr_desc == -1))
  	return false;

	// redirect stdout and stderr to corresponding files
	stdoutStream = freopen(stdoutFileName.toLatin1(), "wc", stdout);
	stderrStream = freopen(stderrFileName.toLatin1(), "wc", stderr);

	if ((stderrStream == 0) || (stdoutStream == 0))
		return false;

	// disable buffering
  setvbuf(stdout, 0, _IONBF, 0);
  setvbuf(stderr, 0, _IONBF, 0);

	redirecting_ = true;

	// start monitoring
	timer.start(TIMEOUT, this);
	
  return redirecting_;
}

bool OutputReader::cancelRedirect()
{
	// close streams
	if (stderrStream != 0)
		fclose(stderrStream);
	
	if (stdoutStream != 0)
		fclose(stdoutStream);

	// set back standard file descriptors
#if defined(WIN32) || defined(WIN64)
	if (( -1 == _dup2(stdout_desc, 1)) 
	  ||( -1 == _dup2(stderr_desc, 2)))
			return false;
#else
	if (( -1 == dup2(stdout_desc, 1)) 
	  ||( -1 == dup2(stderr_desc, 2)))
			return false;
#endif

	// stop monitoring
	timer.stop();

	stderrStream = 0;
	stdoutStream = 0;

	redirecting_ = false;

	return true;
}

QString OutputReader::readOutput() const
{
	static std::ifstream::pos_type position = 0;
	
	QString output;
	std::string data;
	std::ifstream stream (qPrintable(stdoutFileName));
	if (stream.is_open())
	{
		if (stream.seekg(position).good())
			do
			{
				position = stream.tellg();
				if(getline(stream, data))
					output.append(data.data()).append('\n');
				else
					break;
			}
			while (true);
	}

	return output;
}

QString OutputReader::readError() const
{
	static std::ifstream::pos_type position = 0;
	
	QString error;
	std::string data;
	std::ifstream stream (qPrintable(stderrFileName));
	if (stream.is_open())
	{
		if (stream.seekg(position).good())
			do
			{
				position = stream.tellg();
				if(getline(stream, data))
					error.append(data.data()).append('\n');
				else
					break;
			}
			while (true);
	}

	return error;
}

OutputReader& OutputReader::instance()
{
	static OutputReader reader;
	return reader;
}
