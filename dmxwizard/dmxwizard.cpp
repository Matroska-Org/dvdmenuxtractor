#include <QSettings>
#include <QDirModel>
#include <QFileDialog>
#include <QCloseEvent>

#include "dmxwizard.h"

DMXWizard::DMXWizard(QWidget *parent)
	: QDialog(parent), logWidget_(0)
{
	ui.setupUi(this);
	
	// hide image labels
	ui.errorImageLabel->setVisible(false);
	ui.warningImageLabel->setVisible(false);

	setupSlots();
	loadSettings();

	// set first page
	ui.stackedWidget->setCurrentIndex(SETTINGS_PAGE_INDEX);
	ui.infoLabel->setText(tr("Welcome to DVD Menu Extraction Wizard!\nThe Wizard will guide You through the extraction process"));
}

DMXWizard::~DMXWizard()
{
	storeSettings();
}

void DMXWizard::pathEditTextChanged(const QString&)
{
	bool ready = true;

	// set default styles
	QPalette palette = ui.dvdPathEdit->palette();
	palette.setColor(QPalette::Base, Qt::white);
	ui.dvdPathEdit->setPalette(palette);
	ui.toolsPathEdit->setPalette(palette);
	ui.outputPathEdit->setPalette(palette);

	// hide all message and image labels
	ui.errorInfoLabel->clear();
	ui.warningInfoLabel->clear();
	ui.errorImageLabel->hide();
	ui.warningImageLabel->hide();

	// set background to "red" if incorrect paths are specified
	// and show corresponding message
	if (!QFile::exists(ui.dvdPathEdit->text()))
	{	
		palette.setColor(QPalette::Base, Qt::red);
		ui.dvdPathEdit->setPalette(palette);
		ready = false;
	}

	if (!QFile::exists(ui.toolsPathEdit->text()))
	{	
		palette.setColor(QPalette::Base, Qt::red);
		ui.toolsPathEdit->setPalette(palette);
		ready = false;
	}

	if (!ui.outputPathEdit->text().size())
	{
		palette.setColor(QPalette::Base, Qt::red);
		ui.outputPathEdit->setPalette(palette);
		ready = false;
	}
	else if (QFile::exists(ui.outputPathEdit->text())) // just a warning in case the output folder exists
	{
		palette.setColor(QPalette::Base, Qt::yellow);
		ui.outputPathEdit->setPalette(palette);
		
		ui.warningImageLabel->show();
		ui.warningInfoLabel->setText(tr("Specified path already exists. Contents will be overwritten"));
	}

	// show error info if needed
	if (!ready)
	{
		ui.errorImageLabel->show();
		ui.errorInfoLabel->setText(tr("Missing or incorrect path was specified"));
	}

	ui.nextButton->setEnabled(ready);
}

void DMXWizard::browseButtonClicked()
{
	QString directory = QFileDialog::getExistingDirectory(this, "Select directory...");

	if (!directory.size())
		return;

	if (QPushButton *senderObject = dynamic_cast<QPushButton*>(sender()))
	{
		if (senderObject == ui.dvdBrowseButton)
		{
			if (!(directory.endsWith('/') || directory.endsWith('\\')))
				directory.append('/');

			ui.dvdPathEdit->setText(directory);
		}
		else if (senderObject == ui.outputBrowseButton)
			ui.outputPathEdit->setText(directory);
		else if (senderObject == ui.toolsBrowseButton)
			ui.toolsPathEdit->setText(directory);
	}
}

void DMXWizard::loadSettings()
{
	QSettings settings ("DMX", "MatroskaTeam");
	ui.dvdPathEdit->setText(settings.value("Wizard/DVDPath").toString());
	ui.outputPathEdit->setText(settings.value("Wizard/OutputPath").toString());
	ui.toolsPathEdit->setText(settings.value("Wizard/ToolsPath").toString());
}

void DMXWizard::storeSettings()
{
	QSettings settings ("DMX", "MatroskaTeam");
	settings.setValue("Wizard/DVDPath", ui.dvdPathEdit->text());
	settings.setValue("Wizard/OutputPath", ui.outputPathEdit->text());
	settings.setValue("Wizard/ToolsPath", ui.toolsPathEdit->text());
}

void DMXWizard::setupSlots()
{
	// setup slots for buttons
	connect(ui.backButton, SIGNAL(clicked()), this, SLOT(backButtonClicked()));
	connect(ui.nextButton, SIGNAL(clicked()), this, SLOT(nextButtonClicked()));
	connect(ui.stopButton, SIGNAL(clicked()), &dmx, SLOT(abort()));
	connect(ui.dvdBrowseButton, SIGNAL(clicked()), this, SLOT(browseButtonClicked()));
	connect(ui.outputBrowseButton, SIGNAL(clicked()), this, SLOT(browseButtonClicked()));
	connect(ui.toolsBrowseButton, SIGNAL(clicked()), this, SLOT(browseButtonClicked()));

	// setup slots for edits
	connect(ui.dvdPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(pathEditTextChanged(const QString&)));
	connect(ui.outputPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(pathEditTextChanged(const QString&)));
	connect(ui.toolsPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(pathEditTextChanged(const QString&)));

	// setup log widget
	connect(ui.logButton, SIGNAL(toggled(bool)), &logWidget_, SLOT(setVisible(bool)));
	connect(&logWidget_, SIGNAL(closed()), ui.logButton, SLOT(toggle()));

	// setup slots for extraction
	connect(&dmx, SIGNAL(finished()), this, SLOT(extractionFinished()));
	connect(&dmx, SIGNAL(progressChanged(int)), ui.progressBar, SLOT(setValue(int)));
	connect(&dmx, SIGNAL(stepChanged(const QString&)), this, SLOT(extractionStepChanged(const QString&)));
}

void DMXWizard::nextButtonClicked()
{
	if (ui.stackedWidget->currentIndex() == SETTINGS_PAGE_INDEX)
	{
		// try to load DVD IFO
		if (!ui.selectionTree->loadIFOFile(ui.dvdPathEdit->text()))
			ui.nextButton->setEnabled(false);

		// set button states
		ui.backButton->setEnabled(true);

		// turn to the "selection page"
		ui.stackedWidget->setCurrentIndex(SELECTION_PAGE_INDEX);
	}
	else if (ui.stackedWidget->currentIndex() == SELECTION_PAGE_INDEX)
	{
		ui.nextButton->setEnabled(false);

		// turn to the "extraction page"
		ui.stackedWidget->setCurrentIndex(EXTRACTION_PAGE_INDEX);

		// get selection
		DMX::SelectionType selection = ui.selectionTree->getSelection();
		if (selection.size())
		{
			// unhide
			ui.stopButton->show();
			ui.progressBar->show();

			// set button states
			ui.backButton->setEnabled(false);
			ui.stopButton->setEnabled(true);

			ui.progressInfoLabel->setText(tr("Please wait...\nYou can stop the process at any moment by clicking the \"Stop\" button."));

			dmx.setExtractionParameters(ui.dvdPathEdit->text(), ui.outputPathEdit->text(),
											ui.toolsPathEdit->text(), ui.selectionTree->getSelection());

			dmx.start();
		}
		else // nothing was selected
		{
			ui.stopButton->hide();
			ui.progressBar->hide();
			ui.progressInfoLabel->clear();

			ui.stepInfoLabel->setText(tr("Nothing was selected, nothing to do..."));
			
			ui.backButton->setEnabled(true);
			ui.stopButton->setEnabled(false);
		}
	}
}

void DMXWizard::backButtonClicked()
{
	// if going back from the "selection page"
	if (ui.stackedWidget->currentIndex() == SELECTION_PAGE_INDEX)
	{
		// set button states
		ui.backButton->setEnabled(false);
		ui.nextButton->setEnabled(true);

		ui.stackedWidget->setCurrentIndex(SETTINGS_PAGE_INDEX);
	}
	else if (ui.stackedWidget->currentIndex() == EXTRACTION_PAGE_INDEX)
	{
		// set button states
		ui.backButton->setEnabled(true);
		ui.nextButton->setEnabled(true);

		ui.stackedWidget->setCurrentIndex(SELECTION_PAGE_INDEX);
	}
}

void DMXWizard::extractionFinished()
{
	// set button states
	ui.backButton->setEnabled(true);
	ui.stopButton->setEnabled(false);

	// chage text
	ui.stepInfoLabel->setText("Extraction finished!");
}

void DMXWizard::extractionStepChanged(const QString &text)
{
	ui.stepInfoLabel->setText(text);
	
	// send the message to the log
	printf(qPrintable(text + '\n'));
}

void DMXWizard::closeEvent(QCloseEvent *evt)
{
	// stop extraction if in progress before quitting
	if (dmx.isRunning())
		dmx.abort();

	evt->accept();
}
