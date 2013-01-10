#ifndef PROGRESSWIDGET_H__
#define PROGRESSWIDGET_H__

#include <QtGui/QWidget>
#include "mpkghandler.h"
#include <QMessageBox>
#include <QKeyEvent>

namespace Ui {
	class ProgressWidgetClass;
}

class ProgressWidget: public QDialog {
	Q_OBJECT
	public:
		ProgressWidget(QWidget *parent = 0);
        void clear();
		~ProgressWidget();
	private:
		Ui::ProgressWidgetClass *ui;

	public slots:
        void updateDataProcessing(progressMessage message);
		void cancelActions();
        void closeEvent(QCloseEvent *event);

    signals:
        void abort(bool value);
};

class MpkgErrorBox: public QMessageBox{
    Q_OBJECT
public:
    MpkgErrorBox(QWidget *parent = 0);

public slots:
    void showError(errorMessage message);

signals:
    void sigButtonClick(int button);
};
#endif // PROGRESSWIDGET_H__
