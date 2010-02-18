#ifndef DMXWIZARD_H
#define DMXWIZARD_H

#include <QDialog>
#include "ui_dmxwizard.h"
#include "dmxlogwidget.h"

class DMXWizard : public QDialog
{
    Q_OBJECT

public:
   DMXWizard(QWidget *parent = 0);
   ~DMXWizard();

protected:
	void closeEvent(QCloseEvent* evt);

private slots:
	void backButtonClicked();
	void nextButtonClicked();
	void browseButtonClicked();
	void extractionFinished();
	void pathEditTextChanged(const QString& path);
	void extractionStepChanged(const QString& text);

private:
  Ui::DMXWizardClass ui;
	
	DMX dmx;
	DMXLogWidget logWidget_;

	enum {SETTINGS_PAGE_INDEX = 0, SELECTION_PAGE_INDEX, EXTRACTION_PAGE_INDEX};

	void setupSlots();
	void loadSettings();
	void storeSettings();
};

#endif // DMXWIZARD_H
