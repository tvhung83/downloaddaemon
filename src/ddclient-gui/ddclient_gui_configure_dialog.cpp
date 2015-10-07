/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient_gui_configure_dialog.h"
#include "ddclient_gui.h"
#include <string>
#include <fstream>
#include <sstream>
#include <cfgfile/cfgfile.h>

#include <QtGui/QFormLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QDialogButtonBox>
#include <QtGui/QMessageBox>
#include <QStringList>
#include <QtGui/QTextEdit>
#include <QScrollArea>

using namespace std;


configure_dialog::configure_dialog(QWidget *parent, QString config_dir) : QDialog(parent), config_dir(config_dir){
	ddclient_gui *p = (ddclient_gui *) parent;
	get_all_vars();

	setWindowTitle(p->tsl("Configure DownloadDaemon"));

	QVBoxLayout *layout = new QVBoxLayout();
	layout->addStrut(450);
	setLayout(layout);
	QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

	tabs = new QListWidget;
	//tabs->setViewMode(QListView::IconMode);
	//tabs->setIconSize(QSize(96, 84));
	tabs->setMovement(QListView::Static);
	//tabs->setMaximumWidth(128);
	//tabs->setMinimumWidth(127);
	tabs->setFixedWidth(128);

	tabs->addItem(create_list_item("General"));
	tabs->addItem(create_list_item("Download"));
	tabs->addItem(create_list_item("Password"));
	tabs->addItem(create_list_item("Logging"));
	tabs->addItem(create_list_item("Reconnect"));
	tabs->addItem(create_list_item("Proxy"));
	tabs->addItem(create_list_item("Package Extractor"));
	tabs->addItem(create_list_item("Client"));
	tabs->addItem(create_list_item("Advanced\nConfiguration"));

	tabs->setCurrentRow(0);

	pages = new QStackedWidget;
	pages->addWidget(create_general_panel());
	pages->addWidget(create_download_panel());
	pages->addWidget(create_password_panel());
	pages->addWidget(create_logging_panel());
	pages->addWidget(create_reconnect_panel());
	pages->addWidget(create_proxy_panel());
	pages->addWidget(create_extractor_panel());
	pages->addWidget(create_client_panel());
	pages->addWidget(create_advanced_panel());

	QHBoxLayout *horizontalLayout = new QHBoxLayout;
	horizontalLayout->addWidget(tabs);
	horizontalLayout->addWidget(pages, 1);

	layout->addLayout(horizontalLayout);
	layout->addWidget(button_box);

	button_box->button(QDialogButtonBox::Ok)->setDefault(true);
	button_box->button(QDialogButtonBox::Ok)->setFocus(Qt::OtherFocusReason);

	this->resize(1, 1); // set the window to the smalles possible size initially

	connect(button_box->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(ok()));
	connect(button_box->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(tabs, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(change_page(QListWidgetItem*, QListWidgetItem*)));
}


QListWidgetItem *configure_dialog::create_list_item(const string &name, const string &picture){
	ddclient_gui *p = (ddclient_gui *) this->parent();
	QListWidgetItem *button = new QListWidgetItem(tabs);

	if(!picture.empty())
		button->setIcon(QIcon(picture.c_str()));

	button->setText(p->tsl(name));
	button->setSizeHint(QSize(50, 30));
	button->setTextAlignment(Qt::AlignVCenter);
	button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

	return button;
}


QWidget *configure_dialog::create_general_panel(){
	ddclient_gui *p = (ddclient_gui *) this->parent();
	downloadc *dclient = p->get_connection();

	QWidget *page = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout();
	page->setLayout(layout);

	QGroupBox *premium_group_box = new QGroupBox(p->tsl("Premium Account"));
	QGroupBox *general_group_box = new QGroupBox(p->tsl("General Options"));
	QFormLayout *form_layout = new QFormLayout();
	QFormLayout *general_layout = new QFormLayout();
	premium_group_box->setLayout(form_layout);
	general_group_box->setLayout(general_layout);

	// premium host
	premium_host = new QComboBox();
	premium_host->addItem("");
	vector<string> host_list;
	string line = "";

	try{
		host_list = dclient->get_premium_list();
	}catch(client_exception &e){}

	// parse lines
	vector<string>::iterator it = host_list.begin();

	for(; it != host_list.end(); it++){
		premium_host->addItem(it->c_str());
	}

	// other premium items
	premium_user = new QLineEdit();
	premium_password = new QLineEdit();
	premium_password->setEchoMode(QLineEdit::Password);
	QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Save);

	form_layout->addRow(p->tsl("Host"), premium_host);
	form_layout->addRow(p->tsl("Username"), premium_user);
	form_layout->addRow(p->tsl("Password"), premium_password);
	form_layout->addRow("", button_box);

	layout->addWidget(premium_group_box);

	// general options
	bool enable = false;
	if(get_var("overwrite_files") == "1")
		enable = true;

	overwrite = new QCheckBox(p->tsl("overwrite existing Files"));
	overwrite->setChecked(enable);

	enable = false;
	if(get_var("refuse_existing_links") == "1")
		enable = true;

	refuse_existing = new QCheckBox(p->tsl("refuse existing Links"));
	refuse_existing->setChecked(enable);

	enable = false;
	if(get_var("precheck_links") == "1")
		enable = true;

	size_existing = new QCheckBox(p->tsl("check Size and Availability on adding"));
	size_existing->setChecked(enable);

	captcha_retries = new QLineEdit(get_var("captcha_retrys"));
	captcha_retries->setFixedWidth(50);

	general_layout->addRow("", overwrite);
	general_layout->addRow("", refuse_existing);
	general_layout->addRow("", size_existing);
	general_layout->addRow(p->tsl("Captcha Retries"), captcha_retries);

	connect(button_box->button(QDialogButtonBox::Save), SIGNAL(clicked()),this, SLOT(save_premium()));
	connect(premium_host, SIGNAL(currentIndexChanged(int)),this, SLOT(premium_host_changed()));

	layout->addWidget(general_group_box);
	return page;
}


QWidget *configure_dialog::create_download_panel(){
	ddclient_gui *p = (ddclient_gui *) this->parent();

	QWidget *page = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout();
	page->setLayout(layout);

	QGroupBox *time_group_box = new QGroupBox(p->tsl("Download Time"));
	QGroupBox *folder_group_box = new QGroupBox(p->tsl("Download Folder"));
	QGroupBox *options_group_box = new QGroupBox(p->tsl("Additional Download Options"));
	QVBoxLayout *t_layout = new QVBoxLayout();
	QFormLayout *f_layout = new QFormLayout();
	QVBoxLayout *o_layout = new QVBoxLayout();
	time_group_box->setLayout(t_layout);
	folder_group_box->setLayout(f_layout);
	options_group_box->setLayout(o_layout);
	QHBoxLayout *inner_t_layout = new QHBoxLayout();
	QHBoxLayout *inner_o_layout = new QHBoxLayout();

	start_time = new QLineEdit(get_var("download_timing_start"));
	start_time->setFixedWidth(100);
	end_time = new QLineEdit(get_var("download_timing_end"));
	end_time->setFixedWidth(100);
	folder = new QLineEdit(get_var("download_folder"));
	count = new QLineEdit(get_var("simultaneous_downloads"));
	count->setFixedWidth(50);
	speed = new QLineEdit(get_var("max_dl_speed"));
	speed->setFixedWidth(50);

	t_layout->addWidget(new QLabel(p->tsl("You can force DownloadDaemon to only download at specific times by entering a start and"
												"\nend time in the format hours:minutes."
												"\nLeave these fields empty if you want to allow DownloadDaemon to download permanently.")));
	t_layout->addLayout(inner_t_layout);
	inner_t_layout->addWidget(new QLabel(p->tsl("Start Time")));
	inner_t_layout->addWidget(start_time);
	inner_t_layout->addWidget(new QLabel(p->tsl("End Time")));
	inner_t_layout->addWidget(end_time);

	f_layout->addRow("", new QLabel(p->tsl("This option specifies where finished downloads should be saved on the server.")));
	f_layout->addRow("", folder);

	o_layout->addWidget(new QLabel(p->tsl("Here you can specify how many downloads may run at the same time and regulate the\n overall download speed.")));
	o_layout->addLayout(inner_o_layout);
	inner_o_layout->addWidget(new QLabel(p->tsl("Simultaneous Downloads")));
	inner_o_layout->addWidget(count);
	inner_o_layout->addWidget(new QLabel(p->tsl("Maximal Speed in kb/s")));
	inner_o_layout->addWidget(speed);

	layout->addWidget(time_group_box);
	layout->addWidget(folder_group_box);
	layout->addWidget(options_group_box);
	return page;
}


QWidget *configure_dialog::create_password_panel(){
	ddclient_gui *p = (ddclient_gui *) this->parent();

	QWidget *page = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout();
	page->setLayout(layout);

	QGroupBox *group_box = new QGroupBox(p->tsl("Change Password"));
	QFormLayout *form_layout = new QFormLayout();
	group_box->setLayout(form_layout);

	old_password = new QLineEdit();
	old_password->setEchoMode(QLineEdit::Password);
	new_password = new QLineEdit();
	new_password->setEchoMode(QLineEdit::Password);
	QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Save);

	form_layout->addRow(p->tsl("Old Password"), old_password);
	form_layout->addRow(p->tsl("New Password"), new_password);
	form_layout->addRow("", button_box);

	connect(button_box->button(QDialogButtonBox::Save), SIGNAL(clicked()),this, SLOT(save_password()));

	layout->addWidget(group_box);
	return page;
}


QWidget *configure_dialog::create_logging_panel(){
	ddclient_gui *p = (ddclient_gui *) this->parent();

	QWidget *page = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout();
	page->setLayout(layout);

	QGroupBox *procedure_group_box = new QGroupBox(p->tsl("Log Procedure"));
	QGroupBox *activity_group_box = new QGroupBox(p->tsl("Logging Activity"));
	QFormLayout *p_form_layout = new QFormLayout();
	QFormLayout *a_form_layout = new QFormLayout();
	procedure_group_box->setLayout(p_form_layout);
	activity_group_box->setLayout(a_form_layout);

	// log procedure
	procedure = new QComboBox();
	procedure->setEditable(true);
	procedure->addItem(p->tsl("stdout - Standard output"));
	procedure->addItem(p->tsl("stderr - Standard error output"));
	procedure->addItem(p->tsl("syslog - Syslog-daemon"));
	procedure->addItem(p->tsl("file:") + " " + p->tsl("enter path"));

	QString old_output = get_var("log_procedure");

	if(old_output == "stdout")
		procedure->setCurrentIndex(0);
	else if(old_output == "stderr")
		procedure->setCurrentIndex(1);
	else if(old_output == "syslog")
		procedure->setCurrentIndex(2);
	else{ // own file selected
		procedure->removeItem(3);

		string file_output = old_output.toStdString();
		file_output.replace(0, 5, ""); // we have to delete the "file:" part in English and insert the correct translation

		while(file_output.size() > 0 && isspace(file_output[0])) // delete blanks at the beginning if existing
			file_output.erase(0, 1);

		procedure->addItem(p->tsl("file:") + " " + file_output.c_str());
		procedure->setCurrentIndex(3);
	}

	// log activity
	activity = new QComboBox();
	activity->addItem(p->tsl("Debug"));
	activity->addItem(p->tsl("Info"));
	activity->addItem(p->tsl("Warning"));
	activity->addItem(p->tsl("Severe"));
	activity->addItem(p->tsl("Off"));

	QString old_activity = get_var("log_level");

	if(old_activity == "DEBUG")
		activity->setCurrentIndex(0);
	else if(old_activity == "INFO")
		activity->setCurrentIndex(1);
	else if(old_activity == "WARNING")
		activity->setCurrentIndex(2);
	else if(old_activity == "SEVERE")
		activity->setCurrentIndex(3);
	else if(old_activity == "OFF")
		activity->setCurrentIndex(4);

	p_form_layout->addRow("", new QLabel(p->tsl("This option specifies how logging should be done\n(Standard output, Standard error"
												"output, Syslog-daemon, own file).")));
	p_form_layout->addRow("", procedure);
	a_form_layout->addRow("", new QLabel(p->tsl("This option specifies DownloadDaemons logging activity.")));
	a_form_layout->addRow("", activity);

	layout->addWidget(procedure_group_box);
	layout->addWidget(activity_group_box);
	return page;
}


QWidget *configure_dialog::create_reconnect_panel(){
	ddclient_gui *p = (ddclient_gui *) this->parent();
	downloadc *dclient = p->get_connection();

	QWidget *page = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout();
	page->setLayout(layout);

	reconnect_group_box = new QGroupBox(p->tsl("enable reconnecting"));
	reconnect_group_box->setCheckable(true);
	bool enable = false;
	if(get_var("enable_reconnect") == "1")
		enable = true;
	reconnect_group_box->setChecked(enable);
	QFormLayout *form_layout = new QFormLayout();
	reconnect_group_box->setLayout(form_layout);

	QHBoxLayout *policy_layout = new QHBoxLayout();
	QWidget *policy_widget = new QWidget();
	policy_widget->setLayout(policy_layout);

	// reconnect policy
	reconnect_policy = new QComboBox();
	reconnect_policy->addItem(p->tsl("Hard"));
	reconnect_policy->addItem(p->tsl("Continue"));
	reconnect_policy->addItem(p->tsl("Soft"));
	reconnect_policy->addItem(p->tsl("Pussy"));
	QString old_policy = get_var("reconnect_policy", ROUTER_T);

	if(old_policy == "HARD")
		reconnect_policy->setCurrentIndex(0);
	else if(old_policy == "CONTINUE")
		reconnect_policy->setCurrentIndex(1);
	else if(old_policy == "SOFT")
		reconnect_policy->setCurrentIndex(2);
	else if(old_policy == "PUSSY")
		reconnect_policy->setCurrentIndex(3);

	QPushButton *help_button = new QPushButton("?");
	help_button->setFixedWidth(20);
	policy_layout->addWidget(reconnect_policy);
	policy_layout->addWidget(help_button);

	// router model
	model = new QListWidget();
	QStringList model_input;
	string line = "", old_model;
	int selection = 0, i = 0;
	vector<string> model_list;

	old_model = (get_var("router_model", ROUTER_T)).toStdString();
	model_input << "";
	router_model_list.push_back("");

	try{
		model_list = dclient->get_router_list();
	}catch(client_exception &e){}

	// parse lines
	vector<string>::iterator it = model_list.begin();

	for(; it != model_list.end(); it++){
		model_input << it->c_str();
		router_model_list.push_back(*it);

		i++;
		if(*it == old_model)
			selection = i;
	}
	if(selection == 0 && !old_model.empty()) {
		// the model is not in the list - it's a custom line like file:..., add it to the list and select it
		model_input << old_model.c_str();
		router_model_list.push_back(old_model);
		selection = i + 1;
	}

	model->addItems(model_input);
	QListWidgetItem *sel_item = model->item(selection);
	sel_item->setSelected(true);

	// router data
	model_search = new QLineEdit();
	model_search->setText(old_model.c_str());
	ip = new QLineEdit(get_var("router_ip", ROUTER_T));
	username = new QLineEdit(get_var("router_username", ROUTER_T));
	password = new QLineEdit();
	password->setEchoMode(QLineEdit::Password);

	form_layout->addRow(p->tsl("Reconnect Policy"), policy_widget);
	form_layout->addRow(p->tsl("Model"), model_search);
	form_layout->addRow("", model);
	form_layout->addRow(p->tsl("IP"), ip);
	form_layout->addRow(p->tsl("Username"), username);
	form_layout->addRow(p->tsl("Password"), password);

	connect(help_button, SIGNAL(clicked()), this, SLOT(help()));
	connect(model_search, SIGNAL(textEdited(const QString &)), this, SLOT(search_in_model()));
	connect(model, SIGNAL(itemSelectionChanged()), this, SLOT(router_model_sel_changed()));

	layout->addWidget(reconnect_group_box);
	return page;
}


QWidget *configure_dialog::create_proxy_panel(){
	ddclient_gui *p = (ddclient_gui *) this->parent();

	QWidget *page = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout();
	page->setLayout(layout);

	QGroupBox *group_box = new QGroupBox(p->tsl("Proxy"));
	QFormLayout *form_layout = new QFormLayout();
	group_box->setLayout(form_layout);

	QLabel *proxy_retry_explanation = new QLabel(p->tsl("If a connection to a server fails when using a proxy, should DownloadDaemon retry with\nanother proxy?"));
	proxy_retry = new QCheckBox(p->tsl("retry"));
	if(get_var("assume_proxys_online") == "0")
		proxy_retry->setChecked(true);

	QLabel *proxy_explanation = new QLabel(p->tsl("You can provide a list of proxys to use proxy-alternation. For hosters with an IP-based"
												  "\nbandwidth-limit, this can bypass such restrictions by using different proxies.\n"
												  "\nOne entry per Line. User, Password and Port are optional."
												  "\nFormat: <user>:<password>@<ip-address>:<port>"
												  "\nFor example: tom:mypass@123.456.7.89:5115 or tom@hello.de:1234 or hello.de"));

	string proxy_list = this->get_var("proxy_list").toStdString();
	size_t n;

	while((n = proxy_list.find(";")) != std::string::npos){
		proxy_list.replace(n, 1, "\n");
	}


	QTextEdit *proxy_edit = new QTextEdit();
	//proxy_edit->setFixedHeight(100);
	proxy = new QTextDocument(proxy_list.c_str(), proxy_edit);
	proxy_edit->setDocument(proxy);

	form_layout->addRow("", proxy_retry_explanation);
	form_layout->addRow("", proxy_retry);
	form_layout->addRow("", new QLabel(""));
	form_layout->addRow("", proxy_explanation);
	form_layout->addRow("", proxy_edit);

	layout->addWidget(group_box);
	return page;
}


QWidget *configure_dialog::create_client_panel(){
	ddclient_gui *p = (ddclient_gui *) this->parent();

	QWidget *page = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout();
	page->setLayout(layout);

	QGroupBox *group_box = new QGroupBox(p->tsl("Client"));
	QFormLayout *form_layout = new QFormLayout();
	group_box->setLayout(form_layout);

	// read saved language if it exists
	string file_name = string(config_dir.toStdString()) + "ddclient-gui.conf";
	cfgfile file;
	file.open_cfg_file(file_name, true);

	string lang = file.get_cfg_value("language");
	int interval = file.get_int_value("update_interval");

	if(lang == "")
		lang = "English";

	if(interval == 0)
		interval = 2;

	language = new QComboBox();
	language->setFixedWidth(100);
	language->addItem("English");
	language->addItem("Deutsch");
	language->addItem(trUtf8("Español"));

	if(lang == "Deutsch")
		language->setCurrentIndex(1);
	else if(lang == "Espanol")
		language->setCurrentIndex(2);
	else // English is default
		language->setCurrentIndex(0);

	update_interval = new QLineEdit(QString("%1").arg(interval));
	update_interval->setFixedWidth(100);

	form_layout->addRow(new QLabel(p->tsl("Language")), language);
	form_layout->addRow(new QLabel(p->tsl("Update Interval in s")), update_interval);

	layout->addWidget(group_box);
	return page;
}

QWidget *configure_dialog::create_extractor_panel(){
	ddclient_gui *p = (ddclient_gui *) this->parent();

	QWidget *page = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout();
	page->setLayout(layout);

	QGroupBox *group_box = new QGroupBox(p->tsl("Package Extractor"));
	QFormLayout *form_layout = new QFormLayout();
	group_box->setLayout(form_layout);

	bool enable = false;
	if(get_var("enable_pkg_extractor") == "1")
		enable = true;

	enable_extractor = new QCheckBox(p->tsl("enable") + " " + p->tsl("Package Extractor"));
	enable_extractor->setChecked(enable);

	enable = false;
	if(get_var("delete_extracted_archives") == "1")
		enable = true;

	delete_extracted = new QCheckBox(p->tsl("delete extracted Archives"));
	delete_extracted->setChecked(enable);

	QLabel *extractor_explanation = new QLabel(p->tsl("Supply a list of passwords that should always be tried to extract archives. If you set a"
												  "\npassword for a specific package, that password will be used. If you set no password,"
												  "\nExtraction will be tried without a password and with this list."
												  "\nOne entry per Line."));

	string password_list = this->get_var("pkg_extractor_passwords").toStdString();
	size_t n;

	while((n = password_list.find(";")) != std::string::npos){
		password_list.replace(n, 1, "\n");
	}

	QTextEdit *passwords_edit = new QTextEdit();
	//passwords_edit->setFixedHeight(100);
	extractor_passwords = new QTextDocument(password_list.c_str(), passwords_edit);
	passwords_edit->setDocument(extractor_passwords);

	form_layout->addRow("", enable_extractor);
	form_layout->addRow("", delete_extracted);
	form_layout->addRow("", new QLabel(""));
	form_layout->addRow("", extractor_explanation);
	form_layout->addRow("", passwords_edit);

	layout->addWidget(group_box);
	return page;
}

QWidget *configure_dialog::create_advanced_panel(){
	ddclient_gui *p = (ddclient_gui *) this->parent();

	QWidget *page = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout();
	page->setLayout(layout);

	QScrollArea *scroll = new QScrollArea(page);
	QGroupBox *group_box = new QGroupBox(p->tsl("Advanced Configuration"), scroll);

	QFormLayout *form_layout = new QFormLayout();
	group_box->setLayout(form_layout);

	std::map<std::string, std::pair<std::string, bool> >::iterator it;

	for(it = all_variables.begin(); it != all_variables.end(); ++it){
		QLineEdit *tmp = new QLineEdit((it->second.first).c_str());
		tmp->setFixedWidth(300);
		advanced.push_back(tmp);
		form_layout->addRow((it->first).c_str(), tmp);
	}

	scroll->setWidget(group_box);
	layout->addWidget(scroll);
	return page;
}

void configure_dialog::get_all_vars(){
	ddclient_gui *p = (ddclient_gui *) this->parent();
	downloadc *dclient = p->get_connection();

	string answer;

	if(!p->check_connection(false))
		return;

	try{
		this->all_variables = dclient->get_var_list();
	}catch(client_exception &e){}

	return;
}


QString configure_dialog::get_var(const string &var, var_type typ){

	if(typ == NORMAL_T){ // get normal var
		if(all_variables.find(var) == all_variables.end()) // if var can't be found return default value
			return QString("");

		return QString(all_variables[var].first.c_str());
	}

	ddclient_gui *p = (ddclient_gui *) this->parent();
	downloadc *dclient = p->get_connection();

	string answer;

	if(!p->check_connection(false))
		return QString("");

	try{
		if(typ == ROUTER_T)  // get router var
			answer = dclient->get_router_var(var);
		else // get premium var
			answer = dclient->get_premium_var(var);
	}catch(client_exception &e){}

	return QString(answer.c_str());
}


void configure_dialog::set_var(const string &var, const std::string &value, var_type typ){
	ddclient_gui *p = (ddclient_gui *) this->parent();
	downloadc *dclient = p->get_connection();

	try{
		if(typ == NORMAL_T) // set normal var
			dclient->set_var(var, value);
		else if(typ == ROUTER_T)  // set router var
			dclient->set_router_var(var, value);
	}catch(client_exception &e){}
}


void configure_dialog::change_page(QListWidgetItem *current, QListWidgetItem *previous){
	if(!current)
		current = previous;

	pages->setCurrentIndex(tabs->row(current));
}


void configure_dialog::help(){
	ddclient_gui *p = (ddclient_gui *) this->parent();

	string help("HARD cancels all downloads if a reconnect is needed\n"
				"CONTINUE only cancels downloads that can be continued after the reconnect\n"
				"SOFT will wait until all other downloads are finished\n"
				"PUSSY will only reconnect if there is no other choice \n\t(no other download can be started without a reconnect)");

	QMessageBox::information(this, p->tsl("Help"), p->tsl(help));
}


void configure_dialog::search_in_model(){
	QStringList model_list;

	string search_input = model_search->text().toStdString();

	for(size_t i=0; i<search_input.length(); i++) // lower everything for case insensitivity
		search_input[i] = tolower(search_input[i]);

	vector<string>::iterator it;
	size_t index = 0;
	for(it = router_model_list.begin(); it != router_model_list.end(); it++){
		string lower = *it;

		for(size_t i=0; i<lower.length(); i++) // lower everything for case insensitivity
			lower[i] = tolower(lower[i]);

		if(lower.find(search_input) == 0) {
			model->setCurrentItem(model->item(index));
			break;
		}
		++index;
	}
}

void configure_dialog::router_model_sel_changed(){
	model_search->setText(model->selectedItems()[0]->text());
}


void configure_dialog::premium_host_changed(){
	string host = premium_host->currentText().toStdString();

	premium_user->clear();
	premium_password->clear();

	if(host != ""){
		premium_user->insert(get_var(host, PREMIUM_T));
		premium_password->insert("...");
	}
}


void configure_dialog::ok(){
	ddclient_gui *p = (ddclient_gui *) parent();
	downloadc *dclient = p->get_connection();

	// getting user input
	string overwrite = "0", refuse_existing = "0", precheck_links = "0";
	if(this->overwrite->isChecked())
		overwrite = "1";
	if(this->refuse_existing->isChecked())
		refuse_existing = "1";
	if(size_existing->isChecked())
		precheck_links = "1";
	string captcha_retries = this->captcha_retries->text().toStdString();

	string start_time = this->start_time->text().toStdString();
	string end_time = this->end_time->text().toStdString();
	string save_dir = folder->text().toStdString();
	string count = this->count->text().toStdString();
	string speed = this->speed->text().toStdString();

	string log_output;
	int selection = procedure->currentIndex();

	switch (selection){
		case 0:	 log_output = "stdout";
					break;
		case 1:	 log_output = "stderr";
					break;
		case 2:	 log_output = "syslog";
					break;
		case 3:	 log_output = procedure->currentText().toStdString();

					size_t n; // we have to make sure the string starts with "file:" in English and not another language
					string old = p->tsl("file:").toStdString();

					while((n = log_output.find(old)) != std::string::npos) // delete every "file:" in the current language
							log_output.replace(n, old.length(), "");


					while(log_output.size() > 0 && isspace(log_output[0])) // delete blanks at the beginning if existing
						log_output.erase(0, 1);

					log_output = "file:" + log_output; // add the English one
					break;
	}

	string activity_level;
	selection = activity->currentIndex();

	switch (selection){
		case 0:	 activity_level = "DEBUG";
					break;
		case 1:	 activity_level = "INFO";
				break;
		case 2:	 activity_level = "WARNING";
					break;
		case 3:	 activity_level = "SEVERE";
					break;
		case 4:	 activity_level = "OFF";
					break;
	}

	string reconnect_enable = "0";
	if(reconnect_group_box->isChecked())
		reconnect_enable = "1";
	string policy, router_model, router_ip, router_user, router_pass;

	if(reconnect_enable == "1"){ // other contents of reconnect panel are only saved if reconnecting is enabled
		selection = reconnect_policy->currentIndex();

		switch (selection){
			case 0:	 policy = "HARD";
						break;
			case 1:	 policy = "CONTINUE";
						break;
			case 2:	 policy = "SOFT";
						break;
			case 3:	 policy = "PUSSY";
						break;
		}

		router_model = model_search->text().toStdString();
		router_ip = ip->text().toStdString();
		router_user = username->text().toStdString();
		router_pass = password->text().toStdString();
	}

	string proxy_online = "1"; // retry on true means proxy_online is false!
	if(this->proxy_retry->isChecked())
		proxy_online = "0";
	string proxy_list = proxy->toPlainText().toStdString();
	size_t n;

	while((n = proxy_list.find("\n")) != std::string::npos){
		proxy_list.replace(n, 1, ";");
	}

	string language = this->language->currentText().toStdString();
	if(language == trUtf8("Español").toStdString())
		language = "Espanol";

	int update_interval = this->update_interval->text().toInt();

	string enable_extractor = "0", delete_extracted = "0";
	if(this->enable_extractor->isChecked())
		enable_extractor = "1";
	if(this->delete_extracted->isChecked())
		delete_extracted = "1";

	string passwords_list = extractor_passwords->toPlainText().toStdString();

	while((n = passwords_list.find("\n")) != std::string::npos){
		passwords_list.replace(n, 1, ";");
	}


	// check connection
	if(!p->check_connection(true, "Please connect before configurating DownloadDaemon."))
		return;

	// overwrite files
	if(overwrite != get_var("overwrite_files").toStdString())
		set_var("overwrite_files", overwrite);

	// refuse existing links
	if(refuse_existing != get_var("refuse_existing_links").toStdString())
		set_var("refuse_existing_links", refuse_existing);

	// precheck_links
	if(precheck_links != get_var("precheck_links").toStdString())
		set_var("precheck_links", precheck_links);

	// captcha retries
	if(captcha_retries != get_var("captcha_retrys").toStdString())
		set_var("captcha_retrys", captcha_retries);

	// download times
	if(start_time != get_var("download_timing_start").toStdString())
		set_var("download_timing_start", start_time);
	if(end_time != get_var("download_timing_end").toStdString())
		set_var("download_timing_end", end_time);

	// download folder
	if(save_dir != get_var("download_folder").toStdString())
		set_var("download_folder", save_dir);

	// download count
	if(count != get_var("simultaneous_downloads").toStdString())
		set_var("simultaneous_downloads", count);

	// download speed
	if(speed != get_var("max_dl_speed").toStdString())
		set_var("max_dl_speed", speed);

	// log output
	if(log_output != get_var("log_procedure").toStdString())
		set_var("log_procedure", log_output);

	// log activity
	if(activity_level != get_var("log_level").toStdString())
		set_var("log_level", activity_level);

	// assume proxys online
	if(proxy_online != get_var("assume_proxys_online").toStdString())
		set_var("assume_proxys_online", proxy_online);

	// proxy list
	if(proxy_list != get_var("proxy_list").toStdString())
		set_var("proxy_list", proxy_list);

	// enable package extractor
	if(enable_extractor != get_var("enable_pkg_extractor").toStdString())
		set_var("enable_pkg_extractor", enable_extractor);

	// delete extracted archives
	if(delete_extracted != get_var("delete_extracted_archives").toStdString())
		set_var("delete_extracted_archives", delete_extracted);

	// extractor passwords
	if(passwords_list != get_var("pkg_extractor_passwords").toStdString())
		set_var("pkg_extractor_passwords", passwords_list);


	// reconnect enable
	set_var("enable_reconnect", reconnect_enable);

	if(reconnect_enable == "1"){

		// policy
		set_var("reconnect_policy", policy, ROUTER_T);

		try{
			// router model
			dclient->set_router_model(router_model);
		}catch(client_exception &e){}

		// router ip
		set_var("router_ip", router_ip, ROUTER_T);

		// router user
		set_var("router_username", router_user, ROUTER_T);

		// user pass
		set_var("router_password", router_pass, ROUTER_T);
	}

	std::map<std::string, std::pair<std::string, bool> >::iterator it;
	std::vector<QLineEdit *>::iterator gui_it = advanced.begin();

	for(it = all_variables.begin(); it != all_variables.end(); ++it){
		if((*gui_it)->text().toStdString() != it->second.first){
			set_var((it->first).c_str(), (*gui_it)->text().toStdString());
		}

		++gui_it;
	}

	p->set_language(language);
	p->set_update_interval(update_interval);

	// save data to config file
	string file_name = string(config_dir.toStdString()) + "ddclient-gui.conf";
	stringstream s;
	s << update_interval;
	cfgfile file;
	file.open_cfg_file(file_name, true);

	file.set_cfg_value("language", language);
	file.set_cfg_value("update_interval", s.str());

	emit done(0);
}


void configure_dialog::save_premium(){
	ddclient_gui *p = (ddclient_gui *) parent();
	downloadc *dclient = p->get_connection();

   string host = premium_host->currentText().toStdString();

	if(host != ""){ // selected a valid host
		string user = premium_user->text().toStdString();
		string pass = premium_password->text().toStdString();

		// check connection
		if(!p->check_connection(true, "Please connect before configurating DownloadDaemon."))
			return;

		string answer;

		// host, user and password together
		try{
			dclient->set_premium_var(host, user, pass);
		}catch(client_exception &e){}
	}
}


void configure_dialog::save_password(){
	ddclient_gui *p = (ddclient_gui *) parent();
	downloadc *dclient = p->get_connection();

	// getting user input
	string old_pass = old_password->text().toStdString();
	string new_pass = new_password->text().toStdString();

	// check connection
	if(!p->check_connection(true, "Please connect before configurating DownloadDaemon."))
		return;

	string answer;

	// save password
	try{
		dclient->set_var("mgmt_password", new_pass, old_pass);
	}catch(client_exception &e){
		if(e.get_id() == 12){ //password fail
			QMessageBox::information(this, p->tsl("Password Error"), p->tsl("Failed to change the Password."));
		}
	}
	emit done(0);
}
