/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_GUI_CONNECT_DIALOG_H
#define DDCLIENT_GUI_CONNECT_DIALOG_H

#include <QDialog>
#include <QtGui/QLineEdit>
#include <QtGui/QCheckBox>
#include <QComboBox>
#include <cfgfile/cfgfile.h>
#include "ddclient_gui.h"


class connect_dialog : public QDialog{
    Q_OBJECT

    public:
		/** Constructor
		*   @param parent MainWindow, which calls the dialog
		*   @param config_dir Configuration Directory
		*/
		connect_dialog(QWidget *parent, QString config_dir);

		/** Indicateds what a user clicked in the dialog.
		*	@returns If user clicked ok or not
		*/
		bool did_user_click_ok(){ return user_clicked_ok; }

		private:
		QComboBox *host;
		QLineEdit *port;
		QLineEdit *password;
		QComboBox *language;
		QCheckBox *save_data;
		QString config_dir;
		cfgfile file;
		login_data data;
		bool user_clicked_ok;

    private slots:
		void host_selected();
        void ok();

};

#endif // DDCLIENT_GUI_CONNECT_DIALOG_H
