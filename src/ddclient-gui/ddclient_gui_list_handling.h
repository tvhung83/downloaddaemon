/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_GUI_LIST_HANDLING_H
#define DDCLIENT_GUI_LIST_HANDLING_H

#include <config.h>
#include <downloadc/downloadc.h>
#include <language/language.h>
#include <vector>

#include <QString>
#include <QTreeView>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QMutex>

#include "ddclient_gui_main_window.h"

struct selected_info{
	bool package;
	int row;
	int parent_row;
};

struct view_info{
	bool package;
	int id;
	int package_id;
	bool expanded;
	bool selected;
};

class list_handling{

	public:
		list_handling(main_window *my_main_window, QTreeView *list, QStandardItemModel *list_model, QItemSelectionModel *selection_model, downloadc *dclient);
		~list_handling();

		// List methods
		void clear_list();
		void prepare_full_list_update();
		void update_full_list();
		void prepare_subscription_update();
		void update_with_subscriptions();

		/** Calculates the packages progress by comparing the number of all package downloads and finished package downloads
		*   @param package_row row where the package is stored in the view
		*   @returns package progress in %
		*/
		int calc_package_progress(int package_row);
		QString get_status_bar_text();

		// Public selection methods
		void get_selected_lines();
		const std::vector<selected_info> &get_selected_lines_data();
		bool check_selected();
		void select_list();
		void deselect_list();
		const std::vector<package> &get_list_content();

	private:
		main_window *my_main_window;
		QMutex mx;
		downloadc *dclient;
		language *lang;

		QTreeView *list;
		QStandardItemModel *list_model;
		QItemSelectionModel *selection_model;

		std::vector<selected_info> selected_lines;
		std::vector<package> content;
		std::vector<package> new_content;
		std::vector<update_content> new_updates;

		bool full_list_update_required;

		uint64_t not_downloaded_yet;
		uint64_t selected_downloads_size;
		int selected_downloads_count;
		double download_speed;

		// Full list update helper methods
		void update_full_list_packages();
		QModelIndex compare_one_package(int line_nr, QStandardItem *&pkg, const std::vector<package>::iterator &old_it,
										const std::vector<package>::iterator &new_it, const std::vector<view_info> &info, bool &expanded);
		void compare_downloads(QModelIndex &index, std::vector<package>::iterator &new_it, std::vector<package>::iterator &old_it, std::vector<view_info> &info);
		void compare_one_download(const download &new_download, const download &old_download, QStandardItem *pkg, int dl_line);

		// Subscription update helper methods
		bool check_full_list_update_required();
		void update_packages();
		void compare_and_update_one_download(download &old_download, const download &new_download, int dl_line, QStandardItem *pkg_gui);
		void calculate_status_bar_information();

		// General update helper methods
		std::string build_status(std::string &status_text, std::string &time_left, const download &dl);
		void cut_time(std::string &time_left);
		QStandardItem *create_new_package(const package &pkg, int line_nr);
		void create_new_download(const download &download, QStandardItem *pkg, int dl_line);

		//Private selection helper methods
		static bool sort_selected_info(selected_info i1, selected_info i2);
		std::vector<view_info> get_current_view();
};

#endif // DDCLIENT_GUI_LIST_HANDLING_H
