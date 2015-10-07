/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient_gui_captcha_dialog.h"
#include <string>
#include <fstream>
#include <sstream>

#include <QtGui/QGroupBox>
#include <QtGui/QFormLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QDialogButtonBox>
#include <QtGui/QMessageBox>
#include <QMovie>

using namespace std;

captcha_dialog::captcha_dialog(QWidget *parent, std::string image, std::string question, int id) : QDialog(parent), id(id){
    ddclient_gui *p = (ddclient_gui *) parent;

	setWindowTitle(p->tsl("Enter Captcha"));

	QVBoxLayout *layout = new QVBoxLayout();
	this->setLayout(layout);
	QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Ok);

	QMovie *captcha = new QMovie(image.c_str());
	QLabel *captcha_label = new QLabel();
	captcha_label->setWindowFlags(Qt::FramelessWindowHint);
	captcha_label->setMovie(captcha);
	captcha->start();
	layout->addWidget(captcha_label);

	if(question != ""){
		QLabel *question_label = new QLabel(QString(question.c_str()));
		layout->addWidget(question_label);
	}

	answer_label = new QLineEdit();
	layout->addWidget(answer_label);

	button_box->button(QDialogButtonBox::Ok)->setDefault(true);
	button_box->button(QDialogButtonBox::Ok)->setFocus(Qt::OtherFocusReason);

	layout->addWidget(button_box);
	connect(button_box->button(QDialogButtonBox::Ok), SIGNAL(clicked()),this, SLOT(ok()));
}


void captcha_dialog::ok(){
	ddclient_gui *p = (ddclient_gui *) parent();

	downloadc *dclient = p->get_connection();

	try{
		dclient->captcha_resolve(id, answer_label->text().toStdString());
	}catch(client_exception &e){}

	emit done(0);
}

