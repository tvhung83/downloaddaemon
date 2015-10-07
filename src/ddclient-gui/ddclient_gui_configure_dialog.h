/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_GUI_CONFIGURE_DIALOG_H
#define DDCLIENT_GUI_CONFIGURE_DIALOG_H

#include <QDialog>
#include <QtGui/QGroupBox>
#include <QComboBox>
#include <QtGui/QLineEdit>
#include <QListWidget>
#include <QtGui/QCheckBox>
#include <QTextDocument>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStackedWidget>
#include <string>
#include <vector>
#include <map>

class configure_dialog : public QDialog{
	Q_OBJECT

	public:
		/** Constructor
		*   @param parent MainWindow, which calls the dialog
		*/
		configure_dialog(QWidget *parent, QString config_dir);

	private:

		enum var_type {NORMAL_T, ROUTER_T, PREMIUM_T};
		QListWidgetItem *create_list_item(const std::string &name, const std::string &picture = "");
		QWidget *create_general_panel();
		QWidget *create_download_panel();
		QWidget *create_password_panel();
		QWidget *create_logging_panel();
		QWidget *create_reconnect_panel();
		QWidget *create_proxy_panel();
		QWidget *create_client_panel();
		QWidget *create_extractor_panel();
		QWidget *create_advanced_panel();
		void get_all_vars();
		QString get_var(const std::string &var, var_type typ = NORMAL_T);
		void set_var(const std::string &var, const std::string &value, var_type typ = NORMAL_T);

		QString config_dir;
		QListWidget *tabs;
		QStackedWidget *pages;
		std::map<std::string, std::pair<std::string, bool> > all_variables;

		// general
		QComboBox *premium_host;
		QCheckBox *overwrite;
		QCheckBox *refuse_existing;
		QCheckBox *size_existing;
		QLineEdit *captcha_retries;
		QLineEdit *premium_user;
		QLineEdit *premium_password;

		// download
		QLineEdit *start_time;
		QLineEdit *end_time;
		QLineEdit *folder;
		QLineEdit *count;
		QLineEdit *speed;

		// password
		QLineEdit *old_password;
		QLineEdit *new_password;

		// log
		QComboBox *activity;
		QComboBox *procedure;

		// reconnect
		std::vector<std::string> router_model_list;
		QGroupBox *reconnect_group_box;
		QComboBox *reconnect_policy;
		QLineEdit *model_search;
		QListWidget *model;
		QLineEdit *ip;
		QLineEdit *username;
		QLineEdit *password;

		// proxy
		QTextDocument *proxy;
		QCheckBox *proxy_retry;

		// client
		QComboBox *language;
		QLineEdit *update_interval;

		// extractor
		QTextDocument *extractor_passwords;
		QCheckBox *enable_extractor;
		QCheckBox *delete_extracted;

		// advanced configuration
		std::vector<QLineEdit *> advanced;

	private slots:
		void change_page(QListWidgetItem *current, QListWidgetItem *previous);
		void help();
		void search_in_model();
		void router_model_sel_changed();
		void premium_host_changed();
		void ok();
		void save_premium();
		void save_password();
};

#endif // DDCLIENT_GUI_CONFIGURE_DIALOG_H
