#include <QtGui/QApplication>
#include "dmxwizard.h"

int main(int argc, char *argv[])
{	
	QApplication app(argc, argv);
	DMXWizard wizard;
	wizard.show();
	app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

	return app.exec();
}
