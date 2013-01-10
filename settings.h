#ifndef SETTINGS_H_____
#define SETTINGS_H_____

#include <QtGui/QDialog>

namespace Ui {
	class SettingsDialogClass;
};

class SettingsDialog: public QDialog {
	Q_OBJECT
	public:
		SettingsDialog(QWidget *parent = 0);
		~SettingsDialog();
	
	public slots:
		void load();
		void save();
		void addUpdateBlacklist();
		void addRemoveProtected();
		void removeUpdateBlacklist();
		void removeRemoveProtected();
		void addRepository();
		void deleteRepository();
		void getRepositoryList();
	private:
		Ui::SettingsDialogClass *ui;
};
#endif
