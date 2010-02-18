#include <QFile>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include "dmxlogwidget.h"

DMXLogWidget::DMXLogWidget(QWidget *parent)
    : QWidget(parent)
{
	ui.setupUi(this);
	setWindowFlags(windowFlags() | Qt::Tool);

	//setup slots
	connect(ui.saveButton, SIGNAL(clicked()), this, SLOT(saveButtonClicked()));
}

DMXLogWidget::~DMXLogWidget()
{

}

void DMXLogWidget::closeEvent(QCloseEvent *event)
{
	emit closed();
	event->accept();
}

void DMXLogWidget::saveButtonClicked()
{
	QString filename = QFileDialog::getSaveFileName(this, "Save as...");

	if (filename.size())
	{
		QFile output (filename);
		if (!output.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QMessageBox::critical(this, "Error", "Could not open file for writing");
			return;
		}

		QTextStream out (&output);
		out << ui.textEdit->toPlainText();
	}
}
