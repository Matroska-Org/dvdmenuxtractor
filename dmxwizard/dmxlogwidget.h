#ifndef DMXLOGWIDGET_H
#define DMXLOGWIDGET_H

#include <QWidget>
#include "ui_dmxlogwidget.h"

class DMXLogWidget : public QWidget
{
    Q_OBJECT

public:
  DMXLogWidget(QWidget *parent = 0);
  ~DMXLogWidget();

signals:
	void closed();

protected:
	virtual void closeEvent(QCloseEvent *event);

private slots:
	void saveButtonClicked();

private:
    Ui::DMXLogWidgetClass ui;
};

#endif // DMXLOGWIDGET_H
