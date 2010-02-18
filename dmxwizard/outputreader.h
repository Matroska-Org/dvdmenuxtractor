/**********************************************************/
/* Description: Contains OutputReader class declaration	  */
/**********************************************************/
#ifndef OUTPUTREADER_H
#define OUTPUTREADER_H

#include <stdio.h>
#include <QObject>
#include <QBasicTimer>

class OutputReader : public QObject
{
	Q_OBJECT

public:

	static OutputReader& instance();

	/* Starts redirecting stdout and stderr to itself */
	bool redirect();
	
	/* Cancels redirecting stdout and stderr to itself */
	bool cancelRedirect();

	/* Reads stderr content, when available */
	QString readError() const;
	
	/* Reads stdout content, when available */
	QString readOutput() const;

signals:
	/* These signals are emitted when stdout  */
	/*  and/or stderr are ready to be read	  */
	void readyReadStandardError();
	void readyReadStandardOutput();

protected:
	void timerEvent(QTimerEvent *event);

private:
	static const int TIMEOUT = 1000;

	bool redirecting_;

	QString stdoutFileName; // file where stdout would be redirected
	QString stderrFileName; // file where stderr would be redirected

  int stdout_desc;	// stdout descriptor before redirecting
	int stderr_desc;	// stderr descriptor before redirecting

	FILE *stderrStream;	// file stream for standard error
	FILE *stdoutStream; // file stream for standard output

	QBasicTimer timer;	// Timer to watch for output change

	// singleton pattern, no copying
	OutputReader(const OutputReader&);
	OutputReader& operator= (const OutputReader&);
	OutputReader(QObject *parent = 0);
	~OutputReader();
};


#endif // OUTPUTREADER_H
