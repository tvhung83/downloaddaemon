/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_GUI_H
#define DDCLIENT_GUI_H

#include <config.h>
#include <downloadc/downloadc.h>
#include <language/language.h>
#include <vector>
#include <QThread>

#include <QMainWindow>
#include <QString>
#include <QtGui/QMenu>
#include <QtGui/QLabel>
#include <QTreeView>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QMutex>
#include <QClipboard>
#include <QSystemTrayIcon>

#include "ddclient_gui_main_window.h"
#include "ddclient_gui_list_handling.h"


struct login_data{
    std::vector<std::string> host;
    std::vector<int> port;
    std::vector<std::string> pass;
    int selected;
    std::string lang;
};


class ddclient_gui : public QMainWindow, main_window{
    Q_OBJECT

    public:
        /** Constructor
        *   @param config_dir Configuration Directory of Program
        */
        ddclient_gui(QString config_dir);

        /** Destructor */
        ~ddclient_gui();

        /** Updates Status of the Program, should be called after Connecting to a Server
        *    @param server Location of the Server, which the client is connected to
        */
        void update_status(QString server);

        /** Setter for Language
        *    @param lang_to_set Language
        */
        void set_language(std::string lang_to_set);

        /** Setter for Update Interval
        *    @param interval Interval to set
        */
        void set_update_interval(int interval);

        /** Getter for Download-Client Object (used for Communication with Daemon)
        *    @returns Download-Client Object
        */
        downloadc *get_connection();

        /** Gets the download list from Daemon and emits do_reload() signal
		*   @param update the whole list (if false -> use subscriptions)
        */
		void get_content(bool update_full_list = false);

        /** Checks connection
        *    @param tell_user show to user via message box if connection is lost
        *    @param individual_message part of a message to send to user
        *    @returns connection available or not
        */
        bool check_connection(bool tell_user = false, std::string individual_message = "");

        /** Checks if there is subscription enabled and subscribes to SUBS_DOWNLOADS, if so
        *   @return enabled or not
        */
        bool check_subscritpion();

        /** Clears the error message cache, so that they will be shown again */
        void clear_last_error_message();

        /** Calculates the packages progress by comparing the number of all package downloads and finished package downloads
        *   @param package_row row where the package is stored in the view
        *   @returns package progress in %
        */
        int calc_package_progress(int package_row);

		/** Pauses updating */
		void pause_updating();

		/** Starts updating (again) */
		void start_updating();

		// Main Window methods
		/** Creates a QString out of a normal string
		*	@param text String to create the return string from
		*	@returns string as QString
		*/
		QString CreateQString(const std::string &text);

		/** Returns the used language object
		*	@returns language object
		*/
		language *GetLanguage();

		/** Sets downloading active in the GUI */
		void set_downloading_active();

		/** Sets downloading deactive in the GUI */
		void set_downloading_deactive();

		/** Translates a string and changes it into a wxString
		*    @param text string to translate
		*    @returns translated wxString
		*/
		QString tsl(std::string text, ...);

		/** Indicates the last error message that got thrown */
		error_message last_error_message;
		// End: Main Window methods

    signals:
		void do_full_reload();
		void do_subscription_reload();
		void do_clear_list();

    private:
		// GUI helper methods
		void add_bars();
        void update_bars();
        void add_list_components();
        void update_list_components();
        void add_tray_icon();
		void update_status_bar();

		// Slot helper methods
		void delete_download_helper(int id, int &dialog_answer);
		void delete_finished_error_handling_helper(int id, int &dialog_answer);
		void delete_file_helper(int id);

        downloadc *dclient;
        language lang;
		QString server;

        QString config_dir;
        QThread *thread;
		QLabel *status_connection;

		list_handling *list_handler;
		QTreeView *list;
		QStandardItemModel *list_model; // this is copied to and used only in list handling
		QItemSelectionModel *selection_model; // this is copied to and used only in list handling

        QMenu *file_menu;
        QMenu *help_menu;
        QMenu *download_menu;
        QAction *connect_action;
        QAction *configure_action;
        QAction *activate_action;
        QAction *deactivate_action;
        QAction *activate_download_action;
        QAction *deactivate_download_action;
        QAction *container_action;
        QAction *add_action;
        QAction *delete_action;
        QAction *delete_finished_action;
        QAction *select_action;
        QAction *copy_action;
        QAction *paste_action;
        QAction *quit_action;
        QAction *about_action;
        QAction *down_action;
        QAction *up_action;
        QAction *top_action;
        QAction *bottom_action;
		QAction *captcha_action;
		QToolBar *configure_menu;
        QMenu *tray_menu;
        QSystemTrayIcon *tray_icon;

    private slots:
        void on_about();
        void on_select();
        void on_connect();
        void on_add();
        void on_delete();
        void on_delete_finished();
        void on_delete_file();
        void on_activate();
        void on_deactivate();
        void on_priority_up();
        void on_priority_down();
        void on_priority_top();
        void on_priority_bottom();
		void on_enter_captcha();
        void on_configure();
        void on_downloading_activate();
        void on_downloading_deactivate();
        void on_copy();
        void on_paste();
        void on_set_password();
        void on_set_name();
        void on_set_url();
        void on_load_container();
        void on_activate_tray_icon(QSystemTrayIcon::ActivationReason reason);
		void on_full_reload();
		void on_subscription_reload();
		void on_clear_list();
        void donate_sf();

    protected:
        void contextMenuEvent(QContextMenuEvent *event);
        void resizeEvent(QResizeEvent *event);
};

#endif // DDCLIENT_GUI_H
