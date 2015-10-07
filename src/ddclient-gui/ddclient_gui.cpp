/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient_gui.h"
#include "ddclient_gui_update_thread.h"
#include "ddclient_gui_add_dialog.h"
#include "ddclient_gui_about_dialog.h"
#include "ddclient_gui_configure_dialog.h"
#include "ddclient_gui_connect_dialog.h"
#include "ddclient_gui_captcha_dialog.h"
#include "ddclient_gui_status_bar.h"

#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cfgfile/cfgfile.h>

#include <QtGui/QStatusBar>
#include <QtGui/QMenuBar>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QLabel>
#include <QToolBar>
#include <QStringList>
#include <QStandardItem>
#include <QtGui/QContextMenuEvent>
#include <QModelIndex>
#include <QtGui/QInputDialog>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
//#include <QtGui> // this is only for testing if includes are the problem

using namespace std;


ddclient_gui::ddclient_gui(QString config_dir) : QMainWindow(NULL), config_dir(config_dir){
	setWindowTitle("DownloadDaemon Client GUI");
	this->resize(750, 500);
	setWindowIcon(QIcon("img/logoDD.png"));

	lang.set_working_dir("lang/");
	dclient = new downloadc();

	statusBar()->show();
	status_connection = new QLabel(tsl("Not connected"));
	statusBar()->insertPermanentWidget(0, status_connection);

	add_bars();
	add_list_components();

	if(QSystemTrayIcon::isSystemTrayAvailable())
		add_tray_icon();

	// connect if logindata was saved
	string file_name = config_dir.toStdString() + "ddclient-gui.conf";
	cfgfile file;
	file.open_cfg_file(file_name, false);

	if(file){ // file successfully opened
	login_data data;
	data.selected = file.get_int_value("selected");
	stringstream s;
	s << data.selected;
	data.host.push_back(file.get_cfg_value("host" + s.str()));
	data.port.push_back(file.get_int_value("port" + s.str()));
	data.pass.push_back(file.get_cfg_value("pass" + s.str()));

	set_language(file.get_cfg_value("language")); // set program language
		status_connection->setText(tsl("Not connected"));

		try{
		dclient->connect(data.host[0], data.port[0], data.pass[0], true);
		update_status(data.host[0].c_str());

		}catch(client_exception &e){
			if(e.get_id() == 2){ // daemon doesn't allow encryption

				QMessageBox box(QMessageBox::Question, tsl("Auto Connection: No Encryption Supported"), tsl("Encrypted authentication not supported by server.") + ("\n")
								+tsl("Do you want to try unsecure plain-text authentication?"), QMessageBox::Yes|QMessageBox::No);

				box.setModal(true);
				int del = box.exec();

				if(del == QMessageBox::YesRole){ // connect again
					try{
			dclient->connect(data.host[0], data.port[0], data.pass[0], false);
					}catch(client_exception &e){}
				}
			} // we don't have an error message here because it's an auto function
		}
	}

	connect(this, SIGNAL(do_full_reload()), this, SLOT(on_full_reload()), Qt::QueuedConnection);
	connect(this, SIGNAL(do_subscription_reload()), this, SLOT(on_subscription_reload()), Qt::QueuedConnection);
	connect(this, SIGNAL(do_clear_list()), this, SLOT(on_clear_list()), Qt::QueuedConnection);

	int interval = file.get_int_value("update_interval");

	if(interval == 0)
		interval = 2;

	update_thread *thread = new update_thread(this, interval);
	thread->start();
	this->thread = thread;
}


ddclient_gui::~ddclient_gui(){
	dclient->set_term(true);
	delete dclient;
	((update_thread *)thread)->terminate_yourself();
	thread->wait();
	delete thread;
}


void ddclient_gui::update_status(QString server){
	if(!check_connection()){
		return;
	}

	string answer;
	try{
		answer = dclient->get_var("downloading_active");
	}catch(client_exception &e){}

	// removing both icons/deactivating both menuentrys, even when maybe only one is shown
	activate_action->setEnabled(false);
	deactivate_action->setEnabled(false);
	configure_menu->removeAction(activate_action);
	configure_menu->removeAction(deactivate_action);

	if(answer == "1"){ // downloading active
		configure_menu->addAction(deactivate_action);
		deactivate_action->setEnabled(true);
	}else if(answer =="0"){ // downloading not active
		configure_menu->addAction(activate_action);
		activate_action->setEnabled(true);
	}else{
		// should never be reached
	}

	if(server != QString("")){ // sometimes the function is called to update the toolbar, not the status text => server is empty
		status_connection->setText(tsl("Connected to") + " " + server);
		this->server = server;
	}
}


void ddclient_gui::set_language(std::string lang_to_set){
	lang.set_language(lang_to_set);

	update_bars();
	update_list_components();

	get_content(true); // force a full list reload to change the error message language
}


void ddclient_gui::set_update_interval(int interval){
	((update_thread *) thread)->set_update_interval(interval);
}


downloadc *ddclient_gui::get_connection(){
	return dclient;
}


void ddclient_gui::get_content(bool update_full_list){

	if(update_full_list){
		list_handler->prepare_full_list_update();
		emit do_full_reload();
	}

	else{
		list_handler->prepare_subscription_update();
		emit do_subscription_reload();
	}
}


bool ddclient_gui::check_connection(bool tell_user, string individual_message){
	try{
		dclient->check_connection();

	}catch(client_exception &e){
		if(e.get_id() == 10){ //connection lost
			status_connection->setText(tsl("Not connected"));

			if(tell_user && (last_error_message != error_connected)){
				QMessageBox::information(this, tsl("No Connection to Server"), tsl(individual_message));
				last_error_message = error_connected;

				if(!this->isVisible()){ // workaround: if the user closes an error message and there is no window visible the whole programm closes!
					this->show();
					this->hide();
				}
				emit do_clear_list();
			}
			return false;
		}
	}
	return true;
}


bool ddclient_gui::check_subscritpion(){
	try{
		dclient->add_subscription(SUBS_DOWNLOADS);
		dclient->add_subscription(SUBS_CONFIG);
		return true;
	}catch(client_exception &e){
		return false;
	}
}


void ddclient_gui::clear_last_error_message(){
	last_error_message = error_none;
}


int ddclient_gui::calc_package_progress(int package_row){
	return list_handler->calc_package_progress(package_row);
}


void ddclient_gui::pause_updating(){
	((update_thread *)thread)->set_updating(false);
}


void ddclient_gui::start_updating(){
	((update_thread *)thread)->set_updating(true);
}


/*************************************************************
* Main Window methods
*************************************************************/
QString ddclient_gui::CreateQString(const std::string &text){
	return trUtf8(text.c_str());
}


language *ddclient_gui::GetLanguage(){
	return &lang;
}


void ddclient_gui::set_downloading_active(){
	activate_action->setEnabled(false);
	deactivate_action->setEnabled(true);
	configure_menu->removeAction(activate_action);
	configure_menu->removeAction(deactivate_action); // just to be save
	configure_menu->addAction(deactivate_action);
}


void ddclient_gui::set_downloading_deactive(){
	activate_action->setEnabled(true);
	deactivate_action->setEnabled(false);
	configure_menu->removeAction(activate_action);
	configure_menu->removeAction(deactivate_action); // just to be save
	configure_menu->addAction(activate_action);
}


QString ddclient_gui::tsl(string text, ...){
	string translated = lang[text];

	size_t n;
	int i = 1;
	va_list vl;
	va_start(vl, text);
	string search("p1");

	while((n = translated.find(search)) != string::npos){
	stringstream id;
	id << va_arg(vl, char *);
	translated.replace(n-1, 3, id.str());

	i++;
	if(i>9) // maximal 9 additional Parameters
		break;

	stringstream sb;
	sb << "p" << i;
	search = sb.str();
	}

	return trUtf8(translated.c_str());
}
/*************************************************************
* End: Main Window methods
*************************************************************/


/*************************************************************
* GUI Helper Methods
*************************************************************/
void ddclient_gui::add_bars(){
	// menubar
	file_menu = menuBar()->addMenu("&" + tsl("File"));
	help_menu = menuBar()->addMenu("&" + tsl("Help"));

	connect_action = new QAction(QIcon("img/1_connect.png"), "&" + tsl("Connect"), this);
	connect_action->setShortcut(QString("Alt+C"));
	connect_action->setStatusTip(tsl("Connect to a DownloadDaemon Server"));
	file_menu->addAction(connect_action);

	configure_action = new QAction(QIcon("img/8_configure.png"), tsl("Configure"), this);
	configure_action->setShortcut(QString("Alt+P"));
	configure_action->setStatusTip(tsl("Configure DownloadDaemon Server"));
	file_menu->addAction(configure_action);

	activate_action = new QAction(QIcon("img/9_activate.png"), tsl("Activate Downloading"), this);
	activate_action->setShortcut(QString("F2"));
	activate_action->setStatusTip(tsl("Activate Downloading"));
	file_menu->addAction(activate_action);
	activate_action->setEnabled(false);

	deactivate_action = new QAction(QIcon("img/9_deactivate.png"), tsl("Deactivate Downloading"), this);
	deactivate_action->setShortcut(QString("F3"));
	deactivate_action->setStatusTip(tsl("Deactivate Downloading"));
	file_menu->addAction(deactivate_action);
	deactivate_action->setEnabled(false);
	file_menu->addSeparator();

	download_menu = file_menu->addMenu(tsl("Download"));

	activate_download_action = new QAction(QIcon("img/5_start.png"), tsl("Activate Download"), this);
	activate_download_action->setShortcut(QString("Alt+A"));
	activate_download_action->setStatusTip(tsl("Activate the selected Download"));
	download_menu->addAction(activate_download_action);

	deactivate_download_action = new QAction(QIcon("img/4_stop.png"), tsl("Deactivate Download"), this);
	deactivate_download_action->setShortcut(QString("Alt+D"));
	deactivate_download_action->setStatusTip(tsl("Deactivate the selected Download"));
	download_menu->addAction(deactivate_download_action);
	download_menu->addSeparator();

	container_action = new QAction(QIcon("img/16_container.png"), tsl("Add Download Container"), this);
	container_action->setStatusTip(tsl("Add Download Container"));
	download_menu->addAction(container_action);

	add_action = new QAction(QIcon("img/2_add.png"), tsl("Add Download"), this);
	add_action->setShortcut(QString("Alt+I"));
	add_action->setStatusTip(tsl("Add Download"));
	download_menu->addAction(add_action);

	delete_action = new QAction(QIcon("img/3_delete.png"), tsl("Delete Download"), this);
	delete_action->setShortcut(QString("DEL"));
	delete_action->setStatusTip(tsl("Delete Download"));
	download_menu->addAction(delete_action);

	delete_finished_action = new QAction(QIcon("img/10_delete_finished.png"), tsl("Delete finished Downloads"), this);
	delete_finished_action->setShortcut(QString("Ctrl+DEL"));
	delete_finished_action->setStatusTip(tsl("Delete finished Downloads"));
	download_menu->addAction(delete_finished_action);

	captcha_action = new QAction(QIcon("img/bullet_captcha.png"), "&" + tsl("Enter Captcha"), this);
	captcha_action->setStatusTip(tsl("Enter Captcha"));
	download_menu->addSeparator();
	download_menu->addAction(captcha_action);

	file_menu->addSeparator();
	select_action = new QAction(QIcon("img/14_select_all.png"), tsl("Select all"), this);
	select_action->setShortcut(QString("Ctrl+A"));
	select_action->setStatusTip(tsl("Select all"));
	file_menu->addAction(select_action);

	copy_action = new QAction(QIcon("img/11_copy_url.png"), tsl("Copy URL"), this);
	copy_action->setShortcut(QString("Ctrl+C"));
	copy_action->setStatusTip(tsl("Copy URL"));
	file_menu->addAction(copy_action);

	paste_action = new QAction(QIcon("img/11_copy_url.png"), tsl("Paste URL"), this);
	paste_action->setShortcut(QString("Ctrl+V"));
	paste_action->setStatusTip(tsl("Paste URL"));
	file_menu->addAction(paste_action);
	file_menu->addSeparator();

	quit_action = new QAction(QIcon("img/12_quit.png"), "&" + tsl("Quit"), this);
	quit_action->setShortcut(QString("Alt+F4"));
	quit_action->setStatusTip(tsl("Quit"));
	file_menu->addAction(quit_action);

	about_action = new QAction(QIcon("img/13_about.png"), "&" + tsl("About"), this);
	about_action->setShortcut(QString("F1"));
	about_action->setStatusTip(tsl("About"));
	help_menu->addAction(about_action);

	// toolbar
	up_action = new QAction(QIcon("img/6_up.png"), "&" + tsl("Increase Priority"), this);
	up_action->setStatusTip(tsl("Increase Priority of the selected Download"));

	down_action = new QAction(QIcon("img/7_down.png"), "&" + tsl("Decrease Priority"), this);
	down_action->setStatusTip(tsl("Decrease Priority of the selected Download"));

	top_action = new QAction(QIcon("img/go_top.png"), "&" + tsl("Move to Top"), this);
	top_action->setStatusTip(tsl("Move the selected Download to the top"));

	bottom_action = new QAction(QIcon("img/go_bottom.png"), "&" + tsl("Move to Bottom"), this);
	bottom_action->setStatusTip(tsl("Move the selected Download to the bottom"));

	QToolBar *connect_menu = addToolBar(tsl("Connect"));
	connect_menu->addAction(connect_action);

	QToolBar *download_menu = addToolBar(tsl("Download"));
	download_menu->addAction(add_action);
	download_menu->addAction(delete_action);
	download_menu->addAction(delete_finished_action);
	download_menu->addSeparator();
	download_menu->addAction(activate_download_action);
	download_menu->addAction(deactivate_download_action);
	download_menu->addSeparator();
	download_menu->addAction(top_action);
	download_menu->addAction(up_action);
	download_menu->addAction(down_action);
	download_menu->addAction(bottom_action);
	download_menu->addSeparator();
	download_menu->addAction(captcha_action);

	configure_menu = addToolBar(tsl("Configure"));
	configure_menu->addAction(configure_action);
	configure_menu->addAction(activate_action);

	QToolBar *donate_bar = addToolBar(tsl("Support"));
	QAction *donate_action = new QAction(QIcon("img/coins.png"), "Project-Support", this);
	donate_bar->addAction(donate_action);

	connect(connect_action, SIGNAL(triggered()), this, SLOT(on_connect()));
	connect(configure_action, SIGNAL(triggered()), this, SLOT(on_configure()));
	connect(activate_action, SIGNAL(triggered()), this, SLOT(on_downloading_activate()));
	connect(deactivate_action, SIGNAL(triggered()), this, SLOT(on_downloading_deactivate()));
	connect(activate_download_action, SIGNAL(triggered()), this, SLOT(on_activate()));
	connect(deactivate_download_action, SIGNAL(triggered()), this, SLOT(on_deactivate()));
	connect(container_action, SIGNAL(triggered()), this, SLOT(on_load_container()));

	connect(add_action, SIGNAL(triggered()), this, SLOT(on_add()));
	connect(delete_action, SIGNAL(triggered()), this, SLOT(on_delete()));
	connect(delete_finished_action, SIGNAL(triggered()), this, SLOT(on_delete_finished()));
	connect(select_action, SIGNAL(triggered()), this, SLOT(on_select()));
	connect(copy_action, SIGNAL(triggered()), this, SLOT(on_copy()));
	connect(paste_action, SIGNAL(triggered()), this, SLOT(on_paste()));
	connect(quit_action, SIGNAL(triggered()), qApp, SLOT(quit()));
	connect(about_action, SIGNAL(triggered()), this, SLOT(on_about()));
	connect(up_action, SIGNAL(triggered()), this, SLOT(on_priority_up()));
	connect(down_action, SIGNAL(triggered()), this, SLOT(on_priority_down()));
	connect(top_action, SIGNAL(triggered()), this, SLOT(on_priority_top()));
	connect(bottom_action, SIGNAL(triggered()), this, SLOT(on_priority_bottom()));
	connect(captcha_action, SIGNAL(triggered()), this, SLOT(on_enter_captcha()));

	connect(donate_action, SIGNAL(triggered()), this, SLOT(donate_sf()));
}


void ddclient_gui::update_bars(){
	file_menu->setTitle("&" + tsl("File"));
	help_menu->setTitle("&" + tsl("Help"));
	download_menu->setTitle( tsl("Download"));

	connect_action->setText("&" + tsl("Connect"));
	connect_action->setStatusTip(tsl("Connect to a DownloadDaemon Server"));

	configure_action->setText(tsl("Configure"));
	configure_action->setStatusTip(tsl("Configure DownloadDaemon Server"));

	activate_action->setText(tsl("Activate Downloading"));
	activate_action->setStatusTip(tsl("Activate Downloading"));

	deactivate_action->setText(tsl("Deactivate Downloading"));
	deactivate_action->setStatusTip(tsl("Deactivate Downloading"));

	activate_download_action->setText(tsl("Activate Download"));
	activate_download_action->setStatusTip(tsl("Activate the selected Download"));

	deactivate_download_action->setText(tsl("Deactivate Download"));
	deactivate_download_action->setStatusTip(tsl("Deactivate the selected Download"));

	container_action->setText(tsl("Add Download Container"));
	container_action->setStatusTip(tsl("Add Download Container"));

	add_action->setText(tsl("Add Download"));
	add_action->setStatusTip(tsl("Add Download"));

	delete_action->setText(tsl("Delete Download"));
	delete_action->setStatusTip(tsl("Delete Download"));

	delete_finished_action->setText(tsl("Delete finished Downloads"));
	delete_finished_action->setStatusTip(tsl("Delete finished Downloads"));

	select_action->setText(tsl("Select all"));
	select_action->setStatusTip(tsl("Select all"));

	copy_action->setText(tsl("Copy URL"));
	copy_action->setStatusTip(tsl("Copy URL"));

	paste_action->setText(tsl("Paste URL"));
	paste_action->setStatusTip(tsl("Paste URL"));

	quit_action->setText("&" + tsl("Quit"));
	quit_action->setStatusTip(tsl("Quit"));

	about_action->setText("&" + tsl("About"));
	about_action->setStatusTip(tsl("About"));

	up_action->setText("&" + tsl("Increase Priority"));
	up_action->setStatusTip(tsl("Increase Priority of the selected Download"));

	down_action->setText("&" + tsl("Decrease Priority"));
	down_action->setStatusTip(tsl("Decrease Priority of the selected Download"));

	top_action->setText("&" + tsl("Move to Top"));
	top_action->setStatusTip(tsl("Move the selected Download to the top"));

	bottom_action->setText("&" + tsl("Move to Bottom"));
	bottom_action->setStatusTip(tsl("Move the selected Download to the bottom"));
}


void ddclient_gui::add_list_components(){
	// create treeview which will show all the data later
	list_model = new QStandardItemModel();
	QStringList column_labels;
	column_labels << tsl("ID") << tsl("Title") << tsl("URL") << tsl("Time left") << tsl("Status");

	list = new QTreeView();
	list_model->setHorizontalHeaderLabels(column_labels);
	list->setModel(list_model);

	selection_model = new QItemSelectionModel(list_model);
	list->setSelectionModel(selection_model);
	list->setSelectionMode(QAbstractItemView::ExtendedSelection);
	list->setSelectionBehavior(QAbstractItemView::SelectRows);
	// this makes the status bar change whenever the selection changes
	connect(selection_model, SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(on_subscription_reload()), Qt::QueuedConnection);

	status_bar *status_bar_delegate = new status_bar(this);
	list->setItemDelegateForColumn(4, status_bar_delegate);
	list->setAnimated(true);

	setCentralWidget(list);
	list_handler = new list_handling(this, list, list_model, selection_model, dclient);
}


void ddclient_gui::update_list_components(){
	QStringList column_labels;
	column_labels << tsl("ID") << tsl("Title") << tsl("URL") << tsl("Time left") << tsl("Status");
	list_model->setHorizontalHeaderLabels(column_labels);
}


void ddclient_gui::add_tray_icon(){
	tray_menu = new QMenu(this);
	tray_menu->addAction(connect_action);
	tray_menu->addAction(configure_action);
	tray_menu->addSeparator();
	tray_menu->addAction(activate_action);
	tray_menu->addAction(deactivate_action);
	tray_menu->addSeparator();
	tray_menu->addAction(paste_action);
	tray_menu->addSeparator();
	tray_menu->addAction(about_action);
	tray_menu->addAction(quit_action);

	tray_icon = new QSystemTrayIcon(QIcon("img/logoDD.png"), this);
	tray_icon->setContextMenu(tray_menu);
	tray_icon->show();

	connect(tray_icon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(on_activate_tray_icon(QSystemTrayIcon::ActivationReason)));
}


void ddclient_gui::update_status_bar(){
	if(!check_connection()){
		status_connection->setText(tsl("Not connected"));
		list_handler->clear_list();
		return;
	}

	QString status_bar_text = tsl("Connected to");
	status_bar_text += " " + server + " | ";
	status_bar_text += list_handler->get_status_bar_text();

	status_connection->setText(status_bar_text);
	if(QSystemTrayIcon::isSystemTrayAvailable()){

		// replace every | with a \n for a better display of the tray icon text
		string status_bar_text_string = status_bar_text.toStdString();
		size_t old_pos = 0;
		size_t new_pos;
		while((new_pos = status_bar_text_string.find("|", old_pos)) != std::string::npos){
			old_pos = new_pos + 2;
			status_bar_text_string.replace(new_pos, 1, "\n");
		}
		tray_icon->setToolTip(CreateQString(status_bar_text_string.c_str()));
	}
}
/*************************************************************
* End: GUI Helper Methods
*************************************************************/


/*************************************************************
* Slot helper methods
*************************************************************/
void ddclient_gui::delete_download_helper(int id, int &dialog_answer){
	try{
		dclient->delete_download(id, dont_know);

	}catch(client_exception &e){
		if(e.get_id() == 7){

			if((dialog_answer != QMessageBox::YesToAll) && (dialog_answer != QMessageBox::NoToAll)){ // file exists and user didn't choose YesToAll or NoToAll before
				stringstream s;
				s << id;
				QMessageBox file_box(QMessageBox::Question, tsl("Delete File"), tsl("Do you want to delete the downloaded File for Download %p1?", s.str().c_str()),
									 QMessageBox::YesToAll|QMessageBox::Yes|QMessageBox::No|QMessageBox::NoToAll, this);
				file_box.setModal(true);
				dialog_answer = file_box.exec();
			}

			if((dialog_answer == QMessageBox::YesToAll) || (dialog_answer == QMessageBox::Yes)){
				try{
					dclient->delete_download(id, del_file);

				}catch(client_exception &e){}

			}else{ // don't delete file
				try{
					dclient->delete_download(id, dont_delete);

				}catch(client_exception &e){
						QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting Download(s)."));
				}
			}
		}else{ // some error occured
			QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting Download(s)."));
		}
	}
}


void ddclient_gui::delete_finished_error_handling_helper(int id, int &dialog_answer){
	if((dialog_answer != QMessageBox::YesToAll) && (dialog_answer != QMessageBox::NoToAll)){ // file exists and user didn't choose YesToAll or NoToAll before
		stringstream s;
		s << id;
		QMessageBox file_box(QMessageBox::Question, tsl("Delete File"), tsl("Do you want to delete the downloaded File for Download %p1?", s.str().c_str()),
							 QMessageBox::YesToAll|QMessageBox::Yes|QMessageBox::No|QMessageBox::NoToAll, this);
		file_box.setModal(true);
		dialog_answer = file_box.exec();
	}

	if((dialog_answer == QMessageBox::YesToAll) || (dialog_answer == QMessageBox::Yes)){
		try{
			dclient->delete_download(id, del_file);

		}catch(client_exception &e){
			stringstream s;
			s << id;
			QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting File of Download %p1.", s.str().c_str()));
		}

	}else{ // don't delete file
		try{
			dclient->delete_download(id, dont_delete);
		}catch(client_exception &e){
			QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting Download(s)."));
		}
	}
}


void ddclient_gui::delete_file_helper(int id){
	try{
		dclient->delete_file(id);
	}catch(client_exception &e){
		if(e.get_id() == 19){ // file error
			stringstream s;
			s << id;
			QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting File of Download %p1.", s.str().c_str()));
		}
	}
}
/*************************************************************
* End: Slot helper methods
*************************************************************/


/*************************************************************
* Slots
*************************************************************/
void ddclient_gui::on_about(){
	QString build("Build: "); // has to be done here, if not compile time is obsolete
	build += __DATE__;
	build += ", ";
	build += __TIME__;
	build += "\nQt ";
	build += QT_VERSION_STR;

	about_dialog dialog(this, build);
	dialog.setModal(true);
	dialog.exec();

	if(!this->isVisible()){ // workaround: if the user closes an error message and there is no window visible the whole programm closes!
		this->show();
		this->hide();
	}
}


void ddclient_gui::on_select(){
	if(!check_connection(false))
		return;

	list_handler->select_list();
}


void ddclient_gui::on_connect(){
	this->show();
	pause_updating();

	connect_dialog dialog(this, config_dir);
	dialog.setModal(true);
	dialog.exec();

	if(dialog.did_user_click_ok() != true){ // stangely exec returns a nonusable value, so this has to be done manually
		start_updating();
		this->get_content(true);
		dclient->set_term(false); // whenever you tried to connect and failed, then set term is true => has to be set to false again
		return;
	}

	int interval = ((update_thread *)thread)->get_update_interval();
	((update_thread *)thread)->terminate_yourself();
	thread->wait();
	delete thread;

	dclient->set_term(false); // the old thread got terminated, now we have to set term to false again (set to true is in connect_dialog::ok)

	update_thread *new_thread = new update_thread(this, interval);
	new_thread->start();
	thread = new_thread;

	get_content(true);
	start_updating();
}


void ddclient_gui::on_add(){
	pause_updating();
	if(!check_connection(true, "Please connect before adding Downloads."))
		return;

	add_dialog dialog(this);
	dialog.setModal(true);
	dialog.exec();

	get_content(true);
	start_updating();
}


void ddclient_gui::on_delete(){
	pause_updating();
	if(!check_connection(true, "Please connect before deleting Downloads."))
		return;

	list_handler->get_selected_lines();

	if(!list_handler->check_selected())
		return;

	// make sure user wants to delete downloads
	QMessageBox box(QMessageBox::Question, tsl("Delete Downloads"), tsl("Do you really want to delete\nthe selected Download(s)?"),
                  QMessageBox::Yes|QMessageBox::No, this);
	box.setModal(true);
	int del = box.exec();

	if(del != QMessageBox::Yes) // user clicked no to abort deleteing
		return;

	vector<selected_info>::const_iterator it;
	const vector<selected_info> &selected_lines = list_handler->get_selected_lines_data();
	int id;
	const vector<package> &content = list_handler->get_list_content();

	vector<download>::const_iterator dit;
	int parent_row = -1;
	int dialog_answer = QMessageBox::No; // possible answers are YesToAll, Yes, No, NoToAll

	for(it = selected_lines.begin(); it < selected_lines.end(); it++){
		if(it->package){ // we have a package
			parent_row = it->row;
			id = content.at(parent_row).id;

			// delete every download of that package, then the package
			for(dit = content.at(parent_row).dls.begin(); dit != content.at(parent_row).dls.end(); ++dit)
				delete_download_helper(dit->id, dialog_answer);

			try{
				dclient->delete_package(id);
			}catch(client_exception &e){}

		}else{ // we have a real download
			if(it->parent_row == parent_row) // we already deleted the download because we deleted the whole package
				continue;

			id = content.at(it->parent_row).dls.at(it->row).id;
			delete_download_helper(id, dialog_answer);
		}
	}

	 get_content(true);
	 start_updating();
}


void ddclient_gui::on_delete_finished(){
	pause_updating();
	if(!check_connection(true, "Please connect before deleting Downloads."))
		return;

	const vector<package> &content = list_handler->get_list_content();

	vector<int>::iterator it;
	vector<int> finished_ids;
	vector<package>::const_iterator content_it;
	vector<download>::const_iterator download_it;
	int id;
	int package_count = 0;

	vector<int> package_deletes; // prepare structure to save how many downloads of which package will be deleted
	for(unsigned int i = 0; i < content.size(); ++i)
		package_deletes.push_back(0);

	// delete all empty packages
	for(content_it = content.begin(); content_it < content.end(); ++content_it){
		if(content_it->dls.size() == 0){
			try{
				dclient->delete_package(content_it->id);
			}catch(client_exception &e){}
		}
	}

	// find all finished downloads
	for(content_it = content.begin(); content_it < content.end(); ++content_it){
		for(download_it = content_it->dls.begin(); download_it < content_it->dls.end(); ++download_it){
			if(download_it->status == "DOWNLOAD_FINISHED"){
				finished_ids.push_back(download_it->id);
				package_deletes[package_count]++;
			}
		}
		++package_count;
	}

	if(!finished_ids.empty()){
		// make sure user wants to delete downloads
		QMessageBox box(QMessageBox::Question, tsl("Delete Downloads"), tsl("Do you really want to delete\nall finished Download(s)?"),
						QMessageBox::Yes|QMessageBox::No, this);
		box.setModal(true);
		int del = box.exec();

		if(del != QMessageBox::Yes){ // user clicked no to cancel deleting
			return;
		}

		int dialog_answer = QMessageBox::No; // possible answers are YesToAll, Yes, No, NoToAll
		for(it = finished_ids.begin(); it < finished_ids.end(); ++it){
			id = *it;

			try{
				dclient->delete_download(id, dont_know);
			}catch(client_exception &e){
				if(e.get_id() == 7)
					delete_finished_error_handling_helper(id, dialog_answer);
				else // some error occured
					QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting Download(s)."));
			}
		}

	}
	list_handler->deselect_list();

	// delete all empty packages
	int i = 0;
	for(it = package_deletes.begin(); it < package_deletes.end(); ++it){
		unsigned int package_size = *it;

		if(content.at(i).dls.size() <= package_size){ // if we deleted every download inside a package
			try{
				dclient->delete_package(content.at(i).id);
			}catch(client_exception &e){}
		}
		++i;
	}

	get_content(true);
	start_updating();
}


void ddclient_gui::on_delete_file(){
	pause_updating();
	if(!check_connection(true, "Please connect before deleting Files."))
		return;

	list_handler->get_selected_lines();

	if(!list_handler->check_selected())
		return;

	// make sure user wants to delete files
	QMessageBox box(QMessageBox::Question, tsl("Delete Files"), tsl("Do you really want to delete\nthe selected File(s)?"),
					QMessageBox::Yes|QMessageBox::No, this);
	box.setModal(true);
	int del = box.exec();

	if(del != QMessageBox::Yes) // user clicked no to delete
		return;

	vector<selected_info>::const_iterator it;
	const vector<selected_info> &selected_lines = list_handler->get_selected_lines_data();
	int id;
	const vector<package> &content = list_handler->get_list_content();

	vector<download>::const_iterator dit;
	int parent_row = -1;
	for(it = selected_lines.begin(); it < selected_lines.end(); ++it){
		if(it->package){ // we have a package
			parent_row = it->row;
			id = content.at(parent_row).id;

			// delete every file of that package
			for(dit = content.at(parent_row).dls.begin(); dit != content.at(parent_row).dls.end(); ++dit)
				delete_file_helper(dit->id);

		}else{ // we have a real download
			 if(it->parent_row == parent_row) // we already deleted the file because we selected the package
				continue;

			id = content.at(it->parent_row).dls.at(it->row).id;
			delete_file_helper(id);
		}
	}

	list_handler->deselect_list();
	get_content(true);
	start_updating();
}


void ddclient_gui::on_activate(){
	pause_updating();
	if(!check_connection(true, "Please connect before activating Downloads."))
		return;

	list_handler->get_selected_lines();

	if(!list_handler->check_selected())
		return;

	vector<selected_info>::const_iterator it;
	const std::vector<selected_info> &selected_lines = list_handler->get_selected_lines_data();
	int id;
	const vector<package> &content = list_handler->get_list_content();

	vector<download>::const_iterator dit;
	int parent_row = -1;
	for(it = selected_lines.begin(); it < selected_lines.end(); ++it){
		if(it->package){ // we have a package
			parent_row = it->row;
			id = content.at(parent_row).id;

			// activate every download of that package
			for(dit = content.at(parent_row).dls.begin(); dit != content.at(parent_row).dls.end(); ++dit){
				try{
					dclient->activate_download(dit->id);
				}catch(client_exception &e){}
			}

		}else{ // we have a real download
			 if(it->parent_row == parent_row) // we already activated the donwload because we selected the package
				continue;

			id = content.at(it->parent_row).dls.at(it->row).id;
			try{
				dclient->activate_download(id);
			}catch(client_exception &e){}
		}
	}

	get_content(true);
	start_updating();
}


void ddclient_gui::on_deactivate(){
	pause_updating();
	if(!check_connection(true, "Please connect before deactivating Downloads."))
		return;

	list_handler->get_selected_lines();

	if(!list_handler->check_selected())
		return;

	vector<selected_info>::const_iterator it;
	const std::vector<selected_info> &selected_lines = list_handler->get_selected_lines_data();
	int id;
	const vector<package> &content = list_handler->get_list_content();

	vector<download>::const_iterator dit;
	int parent_row = -1;
	for(it = selected_lines.begin(); it < selected_lines.end(); ++it){
		if(it->package){ // we have a package
			parent_row = it->row;
			id = content.at(parent_row).id;

			// activate every download of that package
			for(dit = content.at(parent_row).dls.begin(); dit != content.at(parent_row).dls.end(); ++dit){
				try{
					dclient->deactivate_download(dit->id);
				}catch(client_exception &e){}
			}

		}else{ // we have a real download
			if(it->parent_row == parent_row) // we already activated the donwload because we selected the package
				continue;

			id = content.at(it->parent_row).dls.at(it->row).id;
			try{
				dclient->deactivate_download(id);
			}catch(client_exception &e){}
		}
	}

	get_content(true);
	start_updating();
}


void ddclient_gui::on_priority_up(){
	pause_updating();
	if(!check_connection(true, "Please connect before increasing Priority."))
		return;

	list_handler->get_selected_lines();

	if(!list_handler->check_selected())
		return;

	vector<selected_info>::const_iterator it;
	const std::vector<selected_info> &selected_lines = list_handler->get_selected_lines_data();
	int id;
	const vector<package> &content = list_handler->get_list_content();

	for(it = selected_lines.begin(); it<selected_lines.end(); ++it){
		if(!(it->package)) // we have a real download
			id = content.at(it->parent_row).dls.at(it->row).id;
		else // we have a package selected
			id = content.at(it->row).id;

		try{
			if(it->package)
				dclient->package_priority_up(id);
			else
				dclient->priority_up(id);
		}catch(client_exception &e){}
	}

	get_content(true);
	start_updating();
}


void ddclient_gui::on_priority_down(){
	pause_updating();
	if(!check_connection(true, "Please connect before decreasing Priority."))
		return;

	list_handler->get_selected_lines();

	if(!list_handler->check_selected())
		return;

	vector<selected_info>::const_reverse_iterator rit;
	const std::vector<selected_info> &selected_lines = list_handler->get_selected_lines_data();
	int id;
	const vector<package> &content = list_handler->get_list_content();

	for(rit = selected_lines.rbegin(); rit<selected_lines.rend(); ++rit){
		if(!(rit->package)) // we have a real download
			id = content.at(rit->parent_row).dls.at(rit->row).id;
		else // we have a package selected
			id = content.at(rit->row).id;

		try{
			if(rit->package)
				dclient->package_priority_down(id);
			else
				dclient->priority_down(id);
		}catch(client_exception &e){}
	}

	get_content(true);
	start_updating();
}


void ddclient_gui::on_priority_top(){
	pause_updating();

	if(!check_connection(true, "Please connect before decreasing Priority."))
		return;

	list_handler->get_selected_lines();

	if(!list_handler->check_selected())
		return;

	vector<selected_info>::const_reverse_iterator rit;
	const std::vector<selected_info> &selected_lines = list_handler->get_selected_lines_data();
	int id;
	const vector<package> &content = list_handler->get_list_content();

	for(rit = selected_lines.rbegin(); rit<selected_lines.rend(); ++rit){
		if(!(rit->package)) // we have a real download
			id = content.at(rit->parent_row).dls.at(rit->row).id;
		else // we have a package selected
			id = content.at(rit->row).id;

		try{
			if(rit->package)
				dclient->package_priority_top(id);
			else
				dclient->priority_top(id);
		}catch(client_exception &e){}
	}

	get_content(true);
	start_updating();
}

void ddclient_gui::on_priority_bottom(){
	pause_updating();
	if(!check_connection(true, "Please connect before decreasing Priority."))
		return;

	list_handler->get_selected_lines();

	if(!list_handler->check_selected())
		return;

	vector<selected_info>::const_reverse_iterator rit;
	const std::vector<selected_info> &selected_lines = list_handler->get_selected_lines_data();
	int id;
	const vector<package> &content = list_handler->get_list_content();

	for(rit = selected_lines.rbegin(); rit<selected_lines.rend(); ++rit){
		if(!(rit->package)) // we have a real download
			id = content.at(rit->parent_row).dls.at(rit->row).id;
		else // we have a package selected
			id = content.at(rit->row).id;

		try{
			if(rit->package)
				dclient->package_priority_bottom(id);
			else
				dclient->priority_bottom(id);
		}catch(client_exception &e){}
	}

	get_content(true);
	start_updating();
}

void ddclient_gui::on_enter_captcha(){
	pause_updating();
	if(!check_connection(true, "Please connect before entering a Captcha."))
		return;

	list_handler->get_selected_lines();

	if(!list_handler->check_selected())
		return;

	vector<selected_info>::const_iterator it;
	const std::vector<selected_info> &selected_lines = list_handler->get_selected_lines_data();
	int id;
	const vector<package> &content = list_handler->get_list_content();

	for(it = selected_lines.begin(); it<selected_lines.end(); ++it){
		if(!(it->package)) // we have a real download
			id = content.at(it->parent_row).dls.at(it->row).id;
		else // we have a package selected
			continue;

		string type, question;

		try{
			string image = dclient->get_captcha(id, type, question);
			if(image == "")
				continue;

			// Create temporary image
			fstream captcha_image(string("/tmp/captcha." + type).c_str(), fstream::out);
			captcha_image << image;
			captcha_image.close();

			captcha_dialog dialog(this, "/tmp/captcha." + type, question, id);
			dialog.setModal(true);
			dialog.exec();

			if(!this->isVisible()){ // workaround: if the user closes an error message and there is no window visible the whole programm closes!
				this->show();
				this->hide();
			}
		}catch(client_exception &e){}
	}

	get_content(true);
	start_updating();
}


void ddclient_gui::on_configure(){
	pause_updating();
	if(!check_connection(true, "Please connect before configurating DownloadDaemon."))
		return;

	configure_dialog dialog(this, config_dir);
	dialog.setModal(true);
	dialog.exec();

	if(!this->isVisible()){ // workaround: if the user closes an error message and there is no window visible the whole programm closes!
		this->show();
		this->hide();
	}

	get_content(true);
	start_updating();
}


void ddclient_gui::on_downloading_activate(){
	if(!check_connection(true, "Please connect before you activate Downloading."))
		return;

	try{
		dclient->set_var("downloading_active", "1");
	}catch(client_exception &e){}

	// update toolbar
	set_downloading_active();

	if(!this->isVisible()){ // workaround: if the user closes an error message and there is no window visible the whole programm closes!
		this->show();
		this->hide();
	}
}


void ddclient_gui::on_downloading_deactivate(){
	if(!check_connection(true, "Please connect before you deactivate Downloading."))
		return;

	try{
		dclient->set_var("downloading_active", "0");
	}catch(client_exception &e){}

	// update toolbar
	set_downloading_deactive();

	if(!this->isVisible()){ // workaround: if the user closes an error message and there is no window visible the whole programm closes!
		this->show();
		this->hide();
	}
}


void ddclient_gui::on_copy(){
	if(!check_connection(true, "Please connect before copying URLs."))
		return;

	list_handler->get_selected_lines();

	if(!list_handler->check_selected()){
		return;
	}

	vector<selected_info>::const_iterator it;
	const std::vector<selected_info> &selected_lines = list_handler->get_selected_lines_data();
	QString clipboard_data;
	vector<download>::const_iterator dit;
	int parent_row = -1;
	const vector<package> &content = list_handler->get_list_content();

	for(it = selected_lines.begin(); it < selected_lines.end(); ++it){
		if(it->package){ // we have a package
			parent_row = it->row;

			// copy every url of that package
			for(dit = content.at(parent_row).dls.begin(); dit != content.at(parent_row).dls.end(); ++dit){
				clipboard_data += dit->url.c_str();
				clipboard_data += '\n';
			}

		}else{ // we have a real download
			 if(it->parent_row == parent_row) // we already copied the url because we selected the package
				continue;

			 clipboard_data += content.at(it->parent_row).dls.at(it->row).url.c_str();
			 clipboard_data += '\n';
		}
	}

	QApplication::clipboard()->setText(clipboard_data);
}


void ddclient_gui::on_paste(){
	if(!check_connection(true, "Please connect before adding Downloads."))
		return;

	string text = QApplication::clipboard()->text().toStdString();

	bool error_occured = false;
	size_t lineend = 1, urlend;
	int package = -1, error = 0;
	string line, url, title;

	// parse lines
	while(text.length() > 0 && lineend != string::npos){
		lineend = text.find("\n"); // termination character for line

		if(lineend == string::npos){ // this is the last line (which ends without \n)
			line = text.substr(0, text.length());
			text = "";

		}else{ // there is still another line after this one
			line = text.substr(0, lineend);
			text = text.substr(lineend+1);
		}

		// parse url and title
		urlend = line.find("|");

		if(urlend != string::npos){ // we have a title or rapishare.com
			if(line.find("/#!linklist")==std::string::npos){
				url = line.substr(0, urlend);
				line = line.substr(urlend+1);

				urlend = line.find("|");
				while(urlend != string::npos){ // exchange every | with a blank (there can be some | inside the title too)
					line.at(urlend) = ' ';
					urlend = line.find("|");
				}
				title = line;
			}else{
				url = line;
				title = "";
			}

		}else{ // no title
			url = line;
			title = "";
		}

		// send a single download
		try{
			if(package == -1) // create a new package
				package = dclient->add_package();

			dclient->add_download(package, url, title);
		}catch(client_exception &e){
			error_occured = true;
			error = e.get_id();
		}
	}

	if(error_occured){
		if(error == 6)
			QMessageBox::warning(this,  tsl("Error"), tsl("Failed to create Package."));
		else if(error == 13)
			QMessageBox::warning(this,  tsl("Invalid URL"), tsl("At least one inserted URL was invalid."));

		if(!this->isVisible()){ // workaround: if the user closes an error message and there is no window visible the whole programm closes!
			this->show();
			this->hide();
		}
	}
}


void ddclient_gui::on_set_password(){
	pause_updating();
	if(!check_connection(true, "Please connect before changing Packages."))
		return;

	list_handler->get_selected_lines();

	if(!list_handler->check_selected())
		return;

	vector<selected_info>::const_iterator it;
	const std::vector<selected_info> &selected_lines = list_handler->get_selected_lines_data();
	int id;
	const vector<package> &content = list_handler->get_list_content();

	for(it = selected_lines.begin(); it < selected_lines.end(); ++it){
		if(it->package){ // we have a package
			id = content.at(it->row).id;

		bool ok;
		string old_pass = dclient->get_package_var(id, "PKG_PASSWORD");

		QString pass = QInputDialog::getText(this, tsl("Enter Package Password"), tsl("Enter Package Password"), QLineEdit::Normal, old_pass.c_str(), &ok);
		if(!ok)
			return;

		try{
			dclient->set_package_var(id, "PKG_PASSWORD", pass.toStdString());
		}catch(client_exception &e){}

		}else{} // we have a real download, but we don't need it
	}

	get_content(true);
	start_updating();
}


void ddclient_gui::on_set_name(){
	pause_updating();
	if(!check_connection(true, "Please connect before changing the Download List."))
		return;

	list_handler->get_selected_lines();

	if(!list_handler->check_selected())
		return;

	vector<selected_info>::const_iterator it;
	const std::vector<selected_info> &selected_lines = list_handler->get_selected_lines_data();
	int id;
	const vector<package> &content = list_handler->get_list_content();

	for(it = selected_lines.begin(); it < selected_lines.end(); ++it){
		if(it->package){ // we have a package
			id = content.at(it->row).id;

			bool ok;
			stringstream s;
			s << id;
			QString name = QInputDialog::getText(this, tsl("Enter Title"), tsl("Enter Title of Package %p1", s.str().c_str()), QLineEdit::Normal, "", &ok);
			if(!ok)
				return;

			try{
				dclient->set_package_var(id, "PKG_NAME", name.toStdString());
			}catch(client_exception &e){}

		}else{ // we have a real download
			id = content.at(it->parent_row).dls.at(it->row).id;

			bool ok;
			stringstream s;
			s << id;
			QString name = QInputDialog::getText(this, tsl("Enter Title"), tsl("Enter Title of Download %p1", s.str().c_str()), QLineEdit::Normal, "", &ok);
			if(!ok)
				return;

			try{
				dclient->set_download_var(id, "DL_TITLE", name.toStdString());
			}catch(client_exception &e){
				if(e.get_id() == 18)
					QMessageBox::information(this, tsl("Error"), tsl("Running or finished Downloads can't be changed."));
			}
		}
	}

	get_content(true);
	start_updating();
}


void ddclient_gui::on_set_url(){
	pause_updating();
	if(!check_connection(true, "Please connect before changing the Download List."))
		return;

	list_handler->get_selected_lines();

	if(!list_handler->check_selected())
		return;

	vector<selected_info>::const_iterator it;
	const std::vector<selected_info> &selected_lines = list_handler->get_selected_lines_data();
	int id;
	const vector<package> &content = list_handler->get_list_content();

	for(it = selected_lines.begin(); it < selected_lines.end(); ++it){
		if(it->package){ // we have a package, but don't need it

		}else{ // we have a real download
			id = content.at(it->parent_row).dls.at(it->row).id;

			bool ok;
			stringstream s;
			s << id;
			QString url = QInputDialog::getText(this, tsl("Enter URL"), tsl("Enter URL of Download %p1", s.str().c_str()), QLineEdit::Normal, "", &ok);
			if(!ok)
				return;

			try{
				dclient->set_download_var(id, "DL_URL", url.toStdString());
			}catch(client_exception &e){
				if(e.get_id() == 18)
					QMessageBox::information(this, tsl("Error"), tsl("Running or finished Downloads can't be changed."));
			}
		}
	}

	get_content(true);
	start_updating();
}


void ddclient_gui::on_load_container(){
	if(!check_connection(true, "Please connect before adding Containers."))
		return;

	QFileDialog dialog(this,tsl("Add Download Container"), "", "*.rsdf ; *.dlc ; *.ccf");
	dialog.setModal(true);
	dialog.setFileMode(QFileDialog::ExistingFiles);

	QStringList file_names;
	if(dialog.exec() == QDialog::Accepted )
		file_names = dialog.selectedFiles();

	for (int i = 0; i < file_names.size(); ++i){ // loop every file name

	fstream f;
	string fn = file_names.at(i).toStdString();
	f.open(fn.c_str(), fstream::in);
	string content;

	for(std::string tmp; getline(f, tmp); content += tmp); // read data into string
		try{
			if (fn.rfind("rsdf") == fn.size() - 4 || fn.rfind("RSDF") == fn.size() - 4)
				dclient->pkg_container("RSDF", content);
			else if (fn.rfind("ccf") == fn.size() - 3 || fn.rfind("CCF") == fn.size() - 3)
				dclient->pkg_container("CCF", content);
			else
				dclient->pkg_container("DLC", content);
		}catch(client_exception &e){}
	}
}


void ddclient_gui::on_activate_tray_icon(QSystemTrayIcon::ActivationReason reason){
	switch (reason) {
	case QSystemTrayIcon::Trigger:
	case QSystemTrayIcon::DoubleClick:
		if(this->isVisible())
			this->hide();
		else{
			this->show();
			this->setWindowState(Qt::WindowActive);
		}

		break;
	default:
		break;
	}
}


void ddclient_gui::on_full_reload(){
	list_handler->update_full_list();
	update_status_bar();
}


void ddclient_gui::on_subscription_reload(){
	list_handler->update_with_subscriptions();
	update_status_bar();
}

void ddclient_gui::on_clear_list(){
	list_handler->clear_list();
	update_status_bar();
}

void ddclient_gui::donate_sf(){
	QDesktopServices::openUrl(QUrl(QString("http://sourceforge.net/donate/index.php?group_id=278029")));
}
/*************************************************************
* End: Slots
*************************************************************/


void ddclient_gui::contextMenuEvent(QContextMenuEvent *event){
	QMenu menu(this);

	QAction* delete_file_action = new QAction(QIcon("img/15_delete_file.png"), tsl("Delete File"), this);
	delete_file_action->setShortcut(QString("Ctrl+F"));
	delete_file_action->setStatusTip(tsl("Delete File"));

	QAction* set_password_action = new QAction(QIcon("img/package.png"), tsl("Enter Package Password"), this);
	set_password_action->setStatusTip(tsl("Enter Package Password"));

	QAction* set_name_action = new QAction(QIcon("img/download_package.png"), tsl("Enter Title"), this);
	set_name_action->setStatusTip(tsl("Enter Title"));

	QAction* set_url_action = new QAction(QIcon("img/bullet_black.png"), tsl("Enter URL"), this);
	set_url_action->setStatusTip(tsl("Enter URL"));

	menu.addAction(container_action);
	menu.addSeparator();
	menu.addAction(activate_download_action);
	menu.addAction(deactivate_download_action);
	menu.addSeparator();
	menu.addAction(delete_action);
	menu.addAction(delete_file_action);
	menu.addSeparator();
	menu.addAction(select_action);
	menu.addAction(copy_action);
	menu.addSeparator();
	menu.addAction(set_password_action);
	menu.addAction(set_name_action);
	menu.addAction(set_url_action);
	menu.addAction(captcha_action);

	connect(delete_file_action, SIGNAL(triggered()), this, SLOT(on_delete_file()));
	connect(set_password_action, SIGNAL(triggered()), this, SLOT(on_set_password()));
	connect(set_name_action, SIGNAL(triggered()), this, SLOT(on_set_name()));
	connect(set_url_action, SIGNAL(triggered()), this, SLOT(on_set_url()));

	menu.exec(event->globalPos());
}

void ddclient_gui::resizeEvent(QResizeEvent* event){
	event = event;
	double width = list->width();

	width -= 250;
	list->setColumnWidth(0, 100); // fixed sizes
	list->setColumnWidth(3, 100);

	if(width > 600){ // use different ratio if window is a bit bigger then normal
		list->setColumnWidth(4, 300);
		width -= 300;
		list->setColumnWidth(1, 0.3*width);
		list->setColumnWidth(2, 0.7*width);
	}else{
		list->setColumnWidth(1, 0.2*width);
		list->setColumnWidth(2, 0.3*width);
		list->setColumnWidth(4, 0.55*width);
	}
}
