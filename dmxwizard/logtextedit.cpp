#include <QDate>
#include <QFile>
#include <QTextStream>
#include "logtextedit.h"
#include "outputreader.h"

LogTextEdit::LogTextEdit(QWidget *parent)
	: QTextEdit(parent), notifier_(OutputReader::instance())
{
	// set flags
	setReadOnly(true);
	setLineWrapMode(QTextEdit::NoWrap);

	if (!notifier_.redirect())
	{
		setHtml("ERROR: Could not start output redirection");
		notifier_.cancelRedirect();
		return;
	}

	// setup slots
	connect(&notifier_, SIGNAL(readyReadStandardError()), this, SLOT(standardErrorReady()));
	connect(&notifier_, SIGNAL(readyReadStandardOutput()), this, SLOT(standardOutputReady()));
}

LogTextEdit::~LogTextEdit()
{
}

void LogTextEdit::standardErrorReady()
{
	static const QString format("%1;%2 - WARNING: %3");
	
	int index = 0;
	QString message = notifier_.readError();
	QStringList messages = message.split('\n', QString::SkipEmptyParts);
	for (index = 0; index < messages.size(); ++index)
		append(format.arg(QDate::currentDate().toString(), QTime::currentTime().toString(), messages.at(index)));
}

void LogTextEdit::standardOutputReady()
{
	static const QString format("%1;%2: %3");

	int index = 0;
	QString message = notifier_.readOutput();
	QStringList messages = message.split('\n', QString::SkipEmptyParts);
	for (index = 0; index < messages.size(); ++index)
		append(format.arg(QDate::currentDate().toString(), QTime::currentTime().toString(), messages.at(index)));
}
