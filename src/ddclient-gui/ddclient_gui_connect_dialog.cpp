/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient_gui_connect_dialog.h"
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

using namespace std;

connect_dialog::connect_dialog(QWidget *parent, QString config_dir) : QDialog(parent), config_dir(config_dir), user_clicked_ok(false){
    ddclient_gui *p = (ddclient_gui *) parent;

    setWindowTitle(p->tsl("Connect to Host"));

    // read saved data if it exists
    string file_name = string(config_dir.toStdString()) + "ddclient-gui.conf";
    file.open_cfg_file(file_name, true);

    data.lang = file.get_cfg_value("language");
    data.selected = file.get_int_value("selected");

    bool used_default = false; // inserted default values because there were no entries

    if(data.lang == "")
	data.lang = "English";
    int i = 0;

    while(true){
	stringstream s;
	s << i;
        string tmp(file.get_cfg_value("host" + s.str()));

	if(tmp.empty())
	    break;

	data.host.push_back(tmp);
	data.port.push_back(file.get_int_value("port" + s.str()));
	data.pass.push_back(file.get_cfg_value("pass" + s.str()));
	i++;
    }

    if(data.host.empty()){
	data.lang = "English";
	data.selected = 0;
	data.host.push_back("127.0.0.1");
	data.port.push_back(56789);
	data.pass.push_back("");
        used_default = true;
    }

    QGroupBox *connect_box = new QGroupBox(p->tsl("Data"));
    QFormLayout *form_layout = new QFormLayout();
    QVBoxLayout *layout = new QVBoxLayout();
    QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect_box->setLayout(form_layout);
    layout->addWidget(new QLabel(p->tsl("Please insert Host Data")));
    layout->addWidget(connect_box);
    layout->addWidget(button_box);
    this->setLayout(layout);

    host = new QComboBox();
    host->setEditable(true);
    for(vector<string>::iterator it = data.host.begin(); it != data.host.end(); ++it)
	host->addItem(it->c_str());
    host->setCurrentIndex(data.selected);

    port = new QLineEdit(QString("%1").arg(data.port.at(data.selected)));
    port->setMaxLength(5);
    port->setFixedWidth(50);
    password = new QLineEdit(data.pass.at(data.selected).c_str());
    password->setEchoMode(QLineEdit::Password);
    language = new QComboBox();
    language->addItem("English");
    language->addItem("Deutsch");
    language->addItem(trUtf8("Español"));
    save_data = new QCheckBox();
    save_data->setChecked(true);
    button_box->button(QDialogButtonBox::Ok)->setDefault(true);
    button_box->button(QDialogButtonBox::Ok)->setFocus(Qt::OtherFocusReason);

    if(data.lang == "Deutsch")
        language->setCurrentIndex(1);
    else if(data.lang == "Espanol")
        language->setCurrentIndex(2);
    else // English is default
        language->setCurrentIndex(0);

    form_layout->addRow(new QLabel(p->tsl("IP/URL")), host);
    form_layout->addRow(new QLabel(p->tsl("Port")), port);
    form_layout->addRow(new QLabel(p->tsl("Password")), password);
    form_layout->addRow(new QLabel(p->tsl("Language")), language);
    form_layout->addRow(new QLabel(p->tsl("Save Data")), save_data);

    connect(button_box->button(QDialogButtonBox::Ok), SIGNAL(clicked()),this, SLOT(ok()));
    connect(button_box->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),this, SLOT(reject()));
    connect(host, SIGNAL(currentIndexChanged(int)), this, SLOT(host_selected()));

    if(used_default){ // we have to clear it or the save function will be confused
        data.host.clear();
        data.port.clear();
        data.pass.clear();
    }
}


void connect_dialog::host_selected(){
    string host = this->host->currentText().toStdString();
    port->clear();
    password->clear();

    try{
	for(size_t i = 0; i != data.host.size(); i++){
	    if(host == data.host.at(i)){
		port->insert(QString("%1").arg(data.port.at(i)));
		password->insert(data.pass.at(i).c_str());
		break;
	    }
	}
    }catch(...){}
}


void connect_dialog::ok(){
    string host = this->host->currentText().toStdString();

    int port = this->port->text().toInt();
    string password = this->password->text().toStdString();
    string language = this->language->currentText().toStdString();

    if(language == trUtf8("Español").toStdString())
        language = "Espanol";

    bool error_occured = false;
    ddclient_gui *p = (ddclient_gui *) parent();
    p->set_language(language);
    downloadc *dclient = p->get_connection();

    try{
        dclient->set_term(true);
        dclient->connect(host, port, password, true);

    }catch(client_exception &e){
        if(e.get_id() == 2){ // daemon doesn't allow encryption

            QMessageBox box(QMessageBox::Question, p->tsl("No Encryption"), p->tsl("Encrypted authentication not supported by server.") + ("\n")
                           + p->tsl("Do you want to try unsecure plain-text authentication?"), QMessageBox::Yes|QMessageBox::No);

            box.setModal(true);
            int del = box.exec();

            if(del == QMessageBox::YesRole){ // connect again
                try{
                    dclient->connect(host, port, password, false);
                }catch(client_exception &e){

                    // standard connection error handling
                    if(e.get_id() == 1){ // wrong host/port
                        QMessageBox::warning(this,  p->tsl("Connection failed"), p->tsl("Connection failed (wrong IP/URL or Port).")  + ("\n")
                                              + p->tsl("Please try again."));
                        error_occured = true;

                    }else if(e.get_id() == 3){ // wrong password
                        QMessageBox::warning(this,  p->tsl("Authentication Error"), p->tsl("Wrong Password, Authentication failed."));
                        error_occured = true;

                    }else if(e.get_id() == 4){ // wrong password
                        QMessageBox::warning(this,  p->tsl("Authentication Error"), p->tsl("Unknown Error while Authentication."));
                        error_occured = true;

                    }else{ // we have some other connection error
                        QMessageBox::warning(this,  p->tsl("Authentication Error"), p->tsl("Unknown Error while Authentication."));
                        error_occured = true;
                    }

                }
            }

        // standard connection error handling
        }else if(e.get_id() == 1){ // wrong host/port
            QMessageBox::warning(this,  p->tsl("Connection failed"), p->tsl("Connection failed (wrong IP/URL or Port).")  + ("\n")
                                  + p->tsl("Please try again."));
            error_occured = true;

        }else if(e.get_id() == 3){ // wrong password
            QMessageBox::warning(this,  p->tsl("Authentication Error"), p->tsl("Wrong Password, Authentication failed."));
            error_occured = true;

        }else if(e.get_id() == 4){ // wrong password
            QMessageBox::warning(this,  p->tsl("Authentication Error"), p->tsl("Unknown Error while Authentication."));
            error_occured = true;

        }else{ // we have some other connection error
            QMessageBox::warning(this,  p->tsl("Authentication Error"), p->tsl("Unknown Error while Authentication."));
            error_occured = true;
        }
    }

    // save data if no error occured => connection established
    if(!error_occured){

        // updated statusbar of parent
		p->update_status(host.c_str());

        // write login_data to a file
        if(save_data->isChecked()){ // user clicked checkbox to save data

	    file.set_cfg_value("language", language);

	    bool found = false;
	    size_t i = 0;
	    try{

		for(; i != data.host.size(); i++){
		    if(host == data.host.at(i)){
			stringstream s, port_s;
			s << i;
			port_s << port;
			file.set_cfg_value("selected", s.str());
			file.set_cfg_value("host" + s.str(), host);
			file.set_cfg_value("port" + s.str(), port_s.str());
			file.set_cfg_value("pass" + s.str(), password);

			found = true;
			break;
		    }
		}

		if(!found){ // new host => add a new entry
		    stringstream s, port_s;
		    s << i;
		    port_s << port;
		    file.set_cfg_value("host" + s.str(), host);
		    file.set_cfg_value("port" + s.str(), port_s.str());
		    file.set_cfg_value("pass" + s.str(), password);
		    file.set_cfg_value("selected", s.str());
		}
	    }catch(...){}
        }

		user_clicked_ok = true;
        emit done(0);
    }
}

