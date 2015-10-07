/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_GUI_CAPTCHA_DIALOG_H
#define DDCLIENT_GUI_CAPTCHA_DIALOG_H

#include <QDialog>
#include <QtGui/QLineEdit>
#include <QComboBox>
#include <cfgfile/cfgfile.h>
#include "ddclient_gui.h"


class captcha_dialog : public QDialog{
	Q_OBJECT

	public:
		/** Constructor
		*   @param parent MainWindow, which calls the dialog
		*   @param image Place of captcha image
		*	@param question Question asked with captcha
		*	@param id ID of download
		*/
		captcha_dialog(QWidget *parent, std::string image, std::string question, int id);

	private:
		int id;
		QLineEdit *answer_label;

	private slots:
		void ok();

};

#endif // DDCLIENT_GUI_CAPTCHA_DIALOG_H
