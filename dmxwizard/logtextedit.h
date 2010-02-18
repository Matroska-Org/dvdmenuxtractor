#ifndef LOGTEXTEDIT_H
#define LOGTEXTEDIT_H

#include <QTextEdit>

// forward declaration
class OutputReader;

class LogTextEdit : public QTextEdit
{
	Q_OBJECT

public:
  LogTextEdit(QWidget *parent);
  ~LogTextEdit();

private slots:
	void standardErrorReady();
	void standardOutputReady();

private:
	OutputReader& notifier_;
};

#endif // LOGTEXTEDIT_H
