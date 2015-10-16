/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient_gui_list_handling.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <QStandardItem>
#include <QModelIndex>
using namespace std;

/*************************************************************
* Public Methods
*************************************************************/
list_handling::list_handling(main_window *my_main_window, QTreeView *list, QStandardItemModel *list_model, QItemSelectionModel *selection_model, downloadc *dclient){
	this->my_main_window = my_main_window;
	this->list = list;
	this->list_model = list_model;
	this->selection_model = selection_model;
	this->dclient = dclient;
	lang = my_main_window->GetLanguage();
}


list_handling::~list_handling(){
}


/*************************************************************
* List methods
*************************************************************/
void list_handling::clear_list(){
	QMutexLocker lock(&mx);

	list_model->setRowCount(0);
	content.clear();

	download_speed = 0;
	not_downloaded_yet = 0;
	selected_downloads_size = 0;
	selected_downloads_count = 0;
}

void list_handling::prepare_full_list_update(){
	QMutexLocker lock(&mx);

	try{
		new_content.clear();
		new_content = dclient->get_list();
	}catch(client_exception &){}
}

void list_handling::update_full_list(){
	QMutexLocker lock(&mx);

	download_speed = 0;
	not_downloaded_yet = 0;
	selected_downloads_size = 0;
	selected_downloads_count = 0;

	update_full_list_packages();

	content.clear();
	content = new_content;
}

void list_handling::prepare_subscription_update(){
	QMutexLocker lock(&mx);

	try{
		vector<update_content> tmp_updates;
		lock.unlock(); // this has to be done, otherwise the whole programm will slow down drastically
		tmp_updates = dclient->get_updates();
		lock.relock();

		new_updates.clear();
		new_updates = tmp_updates;

	}catch(client_exception &){}

	if(check_full_list_update_required())
	{ // whenever a subscription update requires a full list update, get the needed data too
		try{
			new_content.clear();
			new_content = dclient->get_list();
		}catch(client_exception &){}
	}
}


void list_handling::update_with_subscriptions(){
	QMutexLocker lock(&mx);

	if(full_list_update_required){
		update_full_list_packages();

		content.clear();
		content = new_content;
	}else{
		update_packages();
		calculate_status_bar_information();
	}
}
/*************************************************************
* End: List methods
*************************************************************/


int list_handling::calc_package_progress(int package_row){
	int progress = 0, downloads = 0, finished = 0;

	try{
		downloads = content.at(package_row).dls.size();
		for(int i = 0; i < downloads; ++i){
			if(content.at(package_row).dls.at(i).status == "DOWNLOAD_FINISHED")
				++finished;
		}

		if(downloads != 0) // we don't want x/0
			progress = (finished * 100) / downloads;

	}catch(...){}

	return progress;
}

QString list_handling::get_status_bar_text(){
	QString answer;

	// update statusbar
	if(selected_downloads_size != 0){ // something is selected and the total size is known
		answer += my_main_window->tsl("Selected Size") + ": " + QString("%1 MB").arg(selected_downloads_size);
		answer += ", " + QString("%1").arg(selected_downloads_count) + " " + my_main_window->tsl("Download(s)");

	}else{
		string time_left;

		if(download_speed > 0){
			stringstream s;
			s << (int)((not_downloaded_yet*1024)/download_speed);
			time_left = s.str();
			cut_time(time_left);

		}else
			time_left = "-";

		stringstream speed;
		speed << setprecision(1) << fixed << download_speed << " kb/s";

		answer += my_main_window->tsl("Total Speed") + ": " + speed.str().c_str() + " | " + my_main_window->tsl("Pending Queue Size");
		answer += ": " + QString("%1 MB").arg(not_downloaded_yet) + " | " + my_main_window->tsl("Time left") + ": " + time_left.c_str();
	}

	return answer;
}


/*************************************************************
* Public selection methods
*************************************************************/
void list_handling::get_selected_lines(){
	selected_lines.clear();

	// find the selected indizes and save them into vector selected_lines
	QModelIndexList selection_list = selection_model->selectedRows();

	// find row, package (yes/no), parent_row
	for(int i=0; i<selection_list.size(); ++i){
		selected_info info;
		info.row = selection_list.at(i).row();

		if(selection_list.at(i).parent() == QModelIndex()){ // no parent: we have a package
			info.package = true;
			info.parent_row = -1;
		}else{
			info.package = false;
			info.parent_row = selection_list.at(i).parent().row();
		}
	selected_lines.push_back(info);
	}

	sort(selected_lines.begin(), selected_lines.end(), list_handling::sort_selected_info);
}


const std::vector<selected_info> &list_handling::get_selected_lines_data(){
	return selected_lines;
}


bool list_handling::check_selected(){
	if(selected_lines.empty()){
		this->my_main_window->last_error_message = error_selected;
		return false;
	}

	return true;
}


void list_handling::select_list(){
	list->expandAll();
	list->selectAll();
}


void list_handling::deselect_list(){
	list->clearSelection();
}


const std::vector<package> &list_handling::get_list_content()
{
	return content;
}
/*************************************************************
* End: Public selection methods
*************************************************************/
/*************************************************************
* End: Public Methods
*************************************************************/


/*************************************************************
* Full list update helper methods
*************************************************************/
void list_handling::update_full_list_packages(){
	vector<package>::iterator old_it = content.begin();
	vector<package>::iterator new_it = new_content.begin();
	vector<download>::iterator dit;

	int line_nr = 0;
	bool expanded;

	QStandardItem *pkg;

	vector<view_info> info = get_current_view();
	deselect_list();
	list->setAnimated(false);

	// loop all packages
	while((old_it != content.end()) && (new_it != new_content.end())){

		// compare package items
		QModelIndex index = compare_one_package(line_nr, pkg, old_it, new_it, info, expanded);

		// compare downloads
		compare_downloads(index, new_it, old_it, info);

		if((old_it->name != new_it->name) || (old_it->password != new_it->password)){
			pkg = list_model->item(line_nr, 1);
			if(new_it->password != "")
				pkg->setIcon(QIcon("img/key.png"));
			else if(old_it->password  != "")
				pkg->setIcon(QIcon(""));
			pkg->setText(QString(new_it->name.c_str()));
		}

		if(expanded)
			list->expand(index);

		++old_it;
		++new_it;
		++line_nr;
	}

	if(old_it != content.end()){ // there are more old lines than new ones
		while(old_it != content.end()){

			// delete packages out of model
			list_model->removeRow(line_nr);
			++old_it;
		}

	}else if(new_it != new_content.end()){ // there are more new lines then old ones
		while(new_it != new_content.end()){

			// insert new package lines
			pkg = create_new_package(*new_it, line_nr);

			QModelIndex index = list_model->index(line_nr, 0, QModelIndex()); // downloads need the parent index

			//insert downloads
			int dl_line = 0;
			for(dit = new_it->dls.begin(); dit != new_it->dls.end(); ++dit){ // loop all downloads of that package

				create_new_download(*dit, pkg, dl_line);
				++dl_line;
			}

			list->expand(index);

			++line_nr;
			++new_it;
		}
	}

	list->setAnimated(true);
}


QModelIndex list_handling::compare_one_package(int line_nr, QStandardItem *&pkg, const vector<package>::iterator &old_it,
											   const vector<package>::iterator &new_it, const vector<view_info> &info, bool &expanded){
	vector<view_info>::const_iterator vit;

	QModelIndex index = list_model->index(line_nr, 0, QModelIndex()); // downloads need the parent index
	expanded = false;

	if(old_it->id != new_it->id){
		pkg = list_model->item(line_nr, 0);
		pkg->setText(QString("%1").arg(new_it->id));
	}

	// recreate expansion and selection
	for(vit = info.begin(); vit != info.end(); ++vit){
		if((vit->id == new_it->id) && (vit->package)){
			if(vit->expanded) // package is expanded
				expanded = true;

			if(vit->selected){ // package is selected
				for(int i=0; i<5; i++)
					selection_model->select(list_model->index(line_nr, i, QModelIndex()), QItemSelectionModel::Select);
			}
			break;
		}
	}

	return index;
}


void list_handling::compare_downloads(QModelIndex &index, std::vector<package>::iterator &new_it, std::vector<package>::iterator &old_it, vector<view_info> &info){
	int dl_line = 0;
	vector<download>::iterator old_dit = old_it->dls.begin();
	vector<download>::iterator new_dit = new_it->dls.begin();
	vector<view_info>::iterator vit;
	QStandardItem *pkg;
	QStandardItem *dl;

	pkg = list_model->itemFromIndex(index);
	if(pkg == NULL)
		return;

	// compare every single download of the package
	while((old_dit != old_it->dls.end()) && (new_dit != new_it->dls.end())){
		compare_one_download(*new_dit, *old_dit, pkg, dl_line);

		// recreate selection if existed
		for(vit = info.begin(); vit != info.end(); ++vit){
			if((vit->id == new_dit->id) && !(vit->package)){
				if(vit->selected){ // download is selected
					for(int i=0; i<5; ++i)
						selection_model->select(index.child(dl_line, i), QItemSelectionModel::Select);

					selected_downloads_size += (double)new_dit->size / 1048576;
					++selected_downloads_count;
					break;
				}
			}
		}

		++old_dit;
		++new_dit;
		++dl_line;
	}

	if(old_dit != old_it->dls.end()){ // there are more old lines than new ones
		while(old_dit != old_it->dls.end()){

			// delete packages out of model
			for(int i=0; i<5; ++i){
				dl = pkg->takeChild(dl_line, i);
				delete dl;
			}

			pkg->takeRow(dl_line);
			++old_dit;
		}

	}else if(new_dit != new_it->dls.end()){ // there are more new lines than old ones
		while(new_dit != new_it->dls.end()){
			// insert new download linkes
			create_new_download(*new_dit, pkg, dl_line);

			++dl_line;
			++new_dit;
		}
		list->collapse(index);
		list->expand(index);
	}
}


void list_handling::compare_one_download(const download &new_download, const download &old_download, QStandardItem *pkg, int dl_line){
	QStandardItem *dl;

	if(old_download.id != new_download.id){
		dl = pkg->child(dl_line, 0);
		dl->setText(QString("%1").arg(new_download.id));
	}

	if(old_download.title != new_download.title){
		dl = pkg->child(dl_line, 1);
		dl->setText(QString(new_download.title.c_str()));
	}

	if(old_download.url != new_download.url){
		dl = pkg->child(dl_line, 2);
		dl->setText(QString(new_download.url.c_str()));
	}

	if((new_download.status != old_download.status) || (new_download.downloaded != old_download.downloaded) || (new_download.size != old_download.size) ||
	   (new_download.wait != old_download.wait) || (new_download.error != old_download.error) || (new_download.speed != old_download.speed)){

		string color, status_text, time_left;
		color = build_status(status_text, time_left, new_download);

		dl = pkg->child(dl_line, 3);
		dl->setText(QString(time_left.c_str()));

		string colorstring = "img/bullet_" + color + ".png";

		dl = pkg->child(dl_line, 4);
		dl->setText(QString(my_main_window->CreateQString(status_text.c_str())));
		dl->setIcon(QIcon(colorstring.c_str()));
	}
}
/*************************************************************
* End: Full list update helper methods
*************************************************************/


/*************************************************************
* Subscription update helper methods
*************************************************************/

bool list_handling::check_full_list_update_required(){
	vector<update_content>::iterator up_it = new_updates.begin();

	full_list_update_required = true;
	for(; up_it != new_updates.end(); ++up_it){
		if(up_it->reason == R_MOVEUP)
			return true;
        else if(up_it->reason == R_MOVEDOWN)
			return true;
		else if(up_it->reason == R_MOVETOP)
			return true;
		else if(up_it->reason == R_MOVEBOTTOM)
			return true;
	}

	full_list_update_required = false;
	return full_list_update_required;
}


void list_handling::update_packages(){
	vector<package>::iterator pkg_it;
	vector<download>::iterator dl_it;
	vector<update_content>::iterator up_it = new_updates.begin();
	QStandardItem *pkg_gui;
	int line_nr, dl_line;
	bool exists;

	for(; up_it != new_updates.end(); ++up_it){
		exists = false;

		if((up_it->sub == SUBS_CONFIG) && (up_it->var_name == "downloading_active")){

			// update toolbar
			if(up_it->value == "1")
				my_main_window->set_downloading_active();
			else
				my_main_window->set_downloading_deactive();

			continue;
		}

		if(up_it->sub != SUBS_DOWNLOADS) // we just need SUBS_DOWNLOADS information
			continue;

		if(up_it->package){ // dealing with a package update

			if(up_it->reason == R_NEW){ // new package

				for(pkg_it = content.begin(); pkg_it != content.end(); ++pkg_it){
					if(up_it->id == pkg_it->id) // found right package
						exists = true;
				}

				if(exists) // got a new update even though we already have the package in the list
					continue;

				content.push_back(*up_it);
				line_nr = list_model->rowCount();
				create_new_package(*up_it, line_nr);

				continue;
			}

			// handling update and delete
			line_nr = 0;
			for(pkg_it = content.begin(); pkg_it != content.end(); ++pkg_it){

				if(up_it->id == pkg_it->id){ // found right package

					if(up_it->reason == R_UPDATE){
						pkg_it->name = up_it->name;
						pkg_it->password = up_it->password;

						pkg_gui = list_model->item(line_nr, 1);
						pkg_gui->setText(QString(up_it->name.c_str()));

						if(up_it->password != "")
							pkg_gui->setIcon(QIcon("img/key.png"));
						else
							pkg_gui->setIcon(QIcon());

					}else if(up_it->reason == R_DELETE){
						list_model->removeRow(line_nr);
						content.erase(pkg_it);
					}
					break;
				}

				++line_nr;
			}

		}else{ // dealing with a download

			// find right package
			line_nr = 0;
			for(pkg_it = content.begin(); pkg_it != content.end(); ++pkg_it){
				if(up_it->pkg_id == pkg_it->id){ // found right package
					exists = true;
					break;
				}
				++line_nr;
			}

			if(!exists) // couldn't find right package
				continue;

			exists = false;

			dl_line = 0;
			for(dl_it = pkg_it->dls.begin(); dl_it != pkg_it->dls.end(); ++dl_it){
				if(up_it->id == dl_it->id){ // found right download
					exists = true;
					break;
				}
				++dl_line;
			}

			QModelIndex index = list_model->index(line_nr, 0, QModelIndex()); // downloads need the parent index
			pkg_gui = list_model->itemFromIndex(index);
			if(pkg_gui == NULL)
				continue;

			if(up_it->reason == R_NEW){ // new download

				if(exists) // download already exists
					continue;

				pkg_it->dls.push_back(*up_it); // automatic cast from update_content to download via cast operator

				dl_line = pkg_it->dls.size() - 1;
				create_new_download(*up_it, pkg_gui, dl_line);
				list->expand(index);

				continue;

			}else if(up_it->reason == R_UPDATE){
				if(!exists) // couldn't find right download
					continue;

				compare_and_update_one_download(*dl_it, *up_it, dl_line, pkg_gui);

			}else if(up_it->reason == R_DELETE){
				if(!exists) // couldn't find right download
					continue;

				pkg_gui->takeRow(dl_line);
				pkg_it->dls.erase(dl_it);
			}
		}
	}
}


void list_handling::compare_and_update_one_download(download &old_download, const download &new_download, int dl_line, QStandardItem *pkg_gui){
	QStandardItem *dl_gui;
	string color, status_text, time_left;

	if(old_download.title != new_download.title){
		old_download.title = new_download.title;

		dl_gui = pkg_gui->child(dl_line, 1);
		if(dl_gui != NULL)
			dl_gui->setText(QString(new_download.title.c_str()));
	}

	if(old_download.url != new_download.url){
		old_download.url = new_download.url;

		dl_gui = pkg_gui->child(dl_line, 2);
		if(dl_gui != NULL)
			dl_gui->setText(QString(new_download.url.c_str()));
	}

	if((old_download.status != new_download.status) || (old_download.downloaded !=new_download.downloaded) || (old_download.size != new_download.size) ||
	   (old_download.wait != new_download.wait) || (old_download.error != new_download.error) || (old_download.speed != new_download.speed)){

		old_download.status = new_download.status;
		old_download.downloaded = new_download.downloaded;
		old_download.size = new_download.size;
		old_download.wait = new_download.wait;
		old_download.error = new_download.error;
		old_download.speed = new_download.speed;
		old_download.title = new_download.title;

		color = build_status(status_text, time_left, old_download);

		dl_gui = pkg_gui->child(dl_line, 3);
		if(dl_gui != NULL)
			dl_gui->setText(QString(time_left.c_str()));

		string colorstring = "img/bullet_" + color + ".png";

		dl_gui = pkg_gui->child(dl_line, 4);
		if(dl_gui != NULL){
			dl_gui->setText(QString(my_main_window->CreateQString(status_text.c_str())));
			dl_gui->setIcon(QIcon(colorstring.c_str()));
		}

	}
}


void list_handling::calculate_status_bar_information()
{
	vector<package>::iterator pkg_it;
	vector<download>::iterator dl_it;
	vector<view_info>::iterator vit;
	vector<view_info> info = get_current_view();

	download_speed = 0;
	not_downloaded_yet = 0;
	selected_downloads_size = 0;
	selected_downloads_count = 0;

	for(pkg_it = content.begin(); pkg_it != content.end(); ++pkg_it){
		for(dl_it = pkg_it->dls.begin(); dl_it != pkg_it->dls.end(); ++dl_it){

			for(vit = info.begin(); vit != info.end(); ++vit){
				if(!(vit->package) && (vit->id == dl_it->id) && (vit->selected)){ // the download we have is selected

					if(dl_it->size != 0 && dl_it->size != 1){
						selected_downloads_size += (double)dl_it->size / 1048576;
						selected_downloads_count++;
					}
					break;
				}
			}

			if(dl_it->status == "DOWNLOAD_RUNNING" && dl_it->speed != 0 && dl_it->speed != -1)
				download_speed += (double)dl_it->speed / 1024;

			if(dl_it->size != 0 && dl_it->size != 1){
				if(dl_it->status == "DOWNLOAD_RUNNING" || dl_it->status == "DOWNLOAD_PENDING" || dl_it->status == "DOWNLOAD_WAITING"){

					if(dl_it->downloaded == 0 || dl_it->downloaded == 1)
						not_downloaded_yet += (double)dl_it->size / 1048576;
					else
						not_downloaded_yet += ((double)dl_it->size / 1048576) - (double)dl_it->downloaded / 1048576;
				}
			}
		}
	}
}
/*************************************************************
* End: Subscription update helper methods
*************************************************************/


/*************************************************************
* General update helper methods
*************************************************************/
string list_handling::build_status(string &status_text, string &time_left, const download &dl){
	string color;
	color = "white";
	status_text = time_left = "";

	if(dl.status == "DOWNLOAD_RUNNING"){
		color = "green";

		if(dl.error == "Captcha") {
			color = "captcha";

			status_text = (*lang)["Enter Captcha"] + ".";
			time_left = "";

		}else if(dl.wait > 0 && dl.error == "PLUGIN_SUCCESS"){ // waiting time > 0
			status_text = (*lang)["Download running. Waiting."];
			stringstream time;
			time << dl.wait;
			time_left =  time.str();
			cut_time(time_left);

		}else if(dl.wait > 0 && dl.error != "PLUGIN_SUCCESS"){
			color = "red";

			status_text = (*lang)["Error"] + ": " + (*lang)[dl.error] + " " + (*lang)["Retrying soon."];
			stringstream time;
			time << dl.wait;
			time_left =  time.str();
			cut_time(time_left);

		}else{ // no waiting time
			stringstream stream_buffer, time_buffer;
			stream_buffer << (*lang)["Running"];

			if(dl.speed != 0 && dl.speed != -1){ // download speed known
				stream_buffer << "@" << setprecision(1) << fixed << (double)dl.speed / 1024 << " kb/s";

				download_speed += (double)dl.speed / 1024;
			}

			stream_buffer << ": ";

			if(dl.size == 0 || dl.size == 1){ // download size unknown
				stream_buffer << "0.00% - ";
				time_left = "";

				if(dl.downloaded == 0 || dl.downloaded == 1) // nothing downloaded yet
					stream_buffer << "0.00 MB/ 0.00 MB";
				else // something downloaded
					stream_buffer << setprecision(1) << fixed << (double)dl.downloaded / 1048576 << " MB/ 0.00 MB";

			}else{ // download size known
				if(dl.downloaded == 0 || dl.downloaded == 1){ // nothing downloaded yet
					stream_buffer << "0.00% - 0.00 MB/ " << fixed << (double)dl.size / 1048576 << " MB";

					not_downloaded_yet += (double)dl.size / 1048576;

					if(dl.speed != 0 && dl.speed != -1){ // download speed known => calc time left
						time_buffer << (int)(dl.size / dl.speed);
						time_left = time_buffer.str();
						cut_time(time_left);
					}else
						time_left = "";

				}else{ // download size known and something downloaded
					stream_buffer << setprecision(1) << fixed << (double)dl.downloaded / (double)dl.size * 100 << "% - ";
					stream_buffer << setprecision(1) << fixed << (double)dl.downloaded / 1048576 << " MB/ ";
					stream_buffer << setprecision(1) << fixed << (double)dl.size / 1048576 << " MB";

					not_downloaded_yet += ((double)dl.size / 1048576) - (double)dl.downloaded / 1048576;

					if(dl.speed != 0 && dl.speed != -1){ // download speed known => calc time left
						time_buffer << (int)((dl.size - dl.downloaded) / dl.speed);
						time_left = time_buffer.str();
						cut_time(time_left);
					}else
						time_left = "";
				}
			}
			status_text = stream_buffer.str();
		}

	}else if(dl.status == "DOWNLOAD_INACTIVE"){
		if(dl.error == "PLUGIN_SUCCESS"){
			color = "yellow";

			if(dl.size == dl.downloaded && dl.size != 0) // download is finished but got inactivated!
				status_text = (*lang)["Download Inactive and Finished."];
			else
				status_text = (*lang)["Download Inactive."];

			time_left = "";

		}else{ // error occured
			color = "red";

			status_text = (*lang)["Inactive. Error"] + ": " + (*lang)[dl.error];
			time_left = "";
		}

	}else if(dl.status == "DOWNLOAD_PENDING"){
		time_left = "";

		if(dl.error == "PLUGIN_SUCCESS"){

			if(dl.size > 0){
				stringstream text;
				text << (*lang)["Download Pending."] << " " << (*lang)["Size"] << ": ";
				text << setprecision(1) << fixed << (double)dl.size / 1048576 << " MB";
				status_text = text.str();

				not_downloaded_yet += (double)dl.size / 1048576;

			}else{
				status_text = (*lang)["Download Pending."];
			}

		}else{ //error occured
			color = "red";

			status_text = (*lang)["Error"] + ": " + (*lang)[dl.error];
		}

	}else if(dl.status == "DOWNLOAD_WAITING"){
		color = "yellow";

		status_text = (*lang)["Have to wait."];
		stringstream time;
		time << dl.wait;
		time_left =  time.str();
		cut_time(time_left);

		if(dl.size > 0)
			not_downloaded_yet += ((double)dl.size / 1048576);

	}else if(dl.status == "DOWNLOAD_FINISHED"){
		color = "star";

		status_text = (*lang)["Download Finished."];
		time_left = "";

	}else if(dl.status == "DOWNLOAD_RECONNECTING") {
		color = "yellow";

		status_text = (*lang)["Reconnecting..."];
		time_left = "";
	}else{ // default, column 4 has unknown input
		status_text = (*lang)["Status not detected."];
		time_left = "";
	}

	return color;
}


void list_handling::cut_time(string &time_left){
	long time_span = atol(time_left.c_str());
	int hours = 0, mins = 0, secs = 0;
	stringstream stream_buffer;

	secs = time_span % 60;
	if(time_span >= 60) // cut time_span down to minutes
		time_span /= 60;
	else { // we don't have minutes
		stream_buffer << secs << " s";
		time_left = stream_buffer.str();
		return;
	}

	mins = time_span % 60;
	if(time_span >= 60) // cut time_span down to hours
		time_span /= 60;
	else { // we don't have hours
		stream_buffer << mins << ":";
		if(secs < 10)
			stream_buffer << "0";
		stream_buffer << secs << "m";
		time_left = stream_buffer.str();
		return;
	}

	hours = time_span;
	stream_buffer << hours << "h, ";
	if(mins < 10)
		stream_buffer << "0";
	stream_buffer << mins << ":";
	if(secs < 10)
		stream_buffer << "0";
	stream_buffer << secs << "m";

	time_left = stream_buffer.str();
	return;
}


QStandardItem *list_handling::create_new_package(const package &pkg, int line_nr){

	QStandardItem *pkg_remember = new QStandardItem(QIcon("img/package.png"), QString("%1").arg(pkg.id));
	pkg_remember->setEditable(false);
	list_model->setItem(line_nr, 0, pkg_remember);

	QStandardItem *pkg_gui = new QStandardItem(QString(pkg.name.c_str()));
	pkg_gui->setEditable(false);

	if(pkg.password != "")
		pkg_gui->setIcon(QIcon("img/key.png"));
	list_model->setItem(line_nr, 1, pkg_gui);

	for(int i=2; i<5; i++){
		pkg_gui = new QStandardItem(QString(""));
		pkg_gui->setEditable(false);
		list_model->setItem(line_nr, i, pkg_gui);
	}

	return pkg_remember;
}


void list_handling::create_new_download(const download &new_download, QStandardItem *pkg, int dl_line){
	string color, status_text, time_left;
	color = build_status(status_text, time_left, new_download);

	QStandardItem *dl = new QStandardItem(QIcon("img/bullet_black.png"), QString("%1").arg(new_download.id));
	dl->setEditable(false);
	pkg->setChild(dl_line, 0, dl);

	dl = new QStandardItem(QString(new_download.title.c_str()));
	dl->setEditable(false);
	pkg->setChild(dl_line, 1, dl);

	dl = new QStandardItem(QString(new_download.url.c_str()));
	dl->setEditable(false);
	pkg->setChild(dl_line, 2, dl);

	dl = new QStandardItem(QString(time_left.c_str()));
	dl->setEditable(false);
	pkg->setChild(dl_line, 3, dl);

	string colorstring = "img/bullet_" + color + ".png";
	dl = new QStandardItem(QIcon(colorstring.c_str()), my_main_window->CreateQString(status_text.c_str()));
	dl->setEditable(false);
	pkg->setChild(dl_line, 4, dl);
}
/*************************************************************
* End: General update helper methods
*************************************************************/


/*************************************************************
* Private selection helper methods
*************************************************************/
bool list_handling::sort_selected_info(selected_info i1, selected_info i2){
	if(i1.package && i2.package) // we have two packages
		return (i1.row < i2.row);

	if(i1.package){ // i1 is a package, i2 is a download
		if(i1.row == i2.parent_row) // see if they are from the same package
			return true; // package is smaller

		return (i1.row < i2.parent_row);
	}

	if(i2.package){ // i1 is a download, i2 is a package
		if(i2.row == i1.parent_row) // see if they are from the same package
			return false; // package is smaller

		return (i1.parent_row < i2.row);
	}

	// we have two downloads
	return (i1.parent_row < i2.parent_row);
}


vector<view_info> list_handling::get_current_view(){
	get_selected_lines();
	vector<view_info> info;

	vector<package>::iterator pit = content.begin();
	vector<selected_info>::iterator sit = selected_lines.begin();
	vector<download>::iterator dit;
	vector<view_info>::iterator vit;
	int line = 0;

	// loop all packages to see which are expanded
	for(; pit != content.end(); ++pit){
		view_info curr_info;
		curr_info.package = true;
		curr_info.id = pit->id;
		curr_info.package_id = -1;

		curr_info.expanded = list->isExpanded(list_model->index(line, 0, QModelIndex()));
		curr_info.selected = false;
		info.push_back(curr_info);

		// loop all downloads to save information for later
		for(dit = pit->dls.begin(); dit != pit->dls.end(); ++dit){
			view_info curr_d_info;
			curr_d_info.package = false;
			curr_d_info.id = dit->id;
			curr_d_info.package_id = pit->id;
			curr_d_info.expanded = false;
			curr_d_info.selected = false;
			info.push_back(curr_d_info);
		}
		++line;
	}

	// loop all selected lines to save that too
	for(; sit != selected_lines.end(); ++sit){
		if(sit->package){ // package
			unsigned int row = sit->row;
			if(content.size() < row)
				break; // shouldn't happen => someone deleted content!

			 // loop info to find the package with the right id
			int id = content.at(row).id;

			for(vit = info.begin(); vit != info.end(); ++vit){ // find package with that in info
				if((vit->id == id) && (vit->package)){
					vit->selected = true;
					break;
				}
			}

		}else{ // download
			unsigned int row = sit->row;
			unsigned int parent_row = sit->parent_row;
			if(content.size() < parent_row)
				break; // shouldn't happen => someone deleted content!
			if(content.at(sit->parent_row).dls.size() < row)
				break;

			// loop info to find the download with the right id
			int id = (content.at(parent_row)).dls.at(row).id; // found real id!

			for(vit = info.begin(); vit != info.end(); ++vit){ // find download with that in info
				if((vit->id == id) && !(vit->package)){
					vit->selected = true;
					break;
				}
			}
		}
	}

	return info;
}
/*************************************************************
* End: Private selection helper methods
*************************************************************/
