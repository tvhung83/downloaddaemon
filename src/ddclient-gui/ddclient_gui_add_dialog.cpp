/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient_gui_add_dialog.h"
#include "ddclient_gui.h"
#include <sstream>

#include <QtGui/QGroupBox>
#include <QtGui/QFormLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QDialogButtonBox>
#include <QtGui/QMessageBox>
#include <QtGui/QTextEdit>


using namespace std;

add_dialog::add_dialog(QWidget *parent) : QDialog(parent){
    ddclient_gui *p = (ddclient_gui *) parent;

    setWindowTitle(p->tsl("Add Downloads"));

	tabs = new QListWidget;
	//tabs->setViewMode(QListView::IconMode);
	//tabs->setIconSize(QSize(96, 84));
	tabs->setMovement(QListView::Static);
	tabs->setFixedWidth(128);

	tabs->addItem(create_list_item("Many Downloads"));
	tabs->addItem(create_list_item("Single Download"));

	tabs->setCurrentRow(0);

	pages = new QStackedWidget;
	pages->addWidget(create_many_downloads_page());
	pages->addWidget(create_single_downloads_page());

	// fill packages combobox
	downloadc *dclient = p->get_connection();
	vector<package> pkgs;
	vector<package>::iterator it;

	try{
		pkgs = dclient->get_packages();
	}catch(client_exception &e){}

	for(it = pkgs.begin(); it != pkgs.end(); it++){
		if(it->name != string("")){
			package_single->addItem(QString("%1").arg(it->id) + ": " + it->name.c_str());
			package_many->addItem(QString("%1").arg(it->id) + ": " + it->name.c_str());
		}else{
			package_single->addItem(QString("%1").arg(it->id));
			package_many->addItem(QString("%1").arg(it->id));
		}
		package_info info = {it->id, it->name};
		packages.push_back(info);
	}

	QHBoxLayout *horizontalLayout = new QHBoxLayout;
	horizontalLayout->addWidget(tabs);
	horizontalLayout->addWidget(pages, 1);

	QVBoxLayout *layout = new QVBoxLayout();
	QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
	button_box->button(QDialogButtonBox::Ok)->setDefault(true);
	button_box->button(QDialogButtonBox::Ok)->setFocus(Qt::OtherFocusReason);

	layout->addLayout(horizontalLayout);
	layout->addWidget(button_box);
	this->setLayout(layout);

	this->resize(1, 1); // set the window to the smalles possible size initially
	tabs->adjustSize();

    connect(button_box->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(ok()));
    connect(button_box->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(separate_packages, SIGNAL(stateChanged(int)), this, SLOT(separate_packages_toggled()));
    connect(fill_title_single, SIGNAL(stateChanged(int)), this, SLOT(fill_title_toggled()));
	connect(tabs, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(change_page(QListWidgetItem*, QListWidgetItem*)));
}


QListWidgetItem *add_dialog::create_list_item(const string &name, const string &picture){
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


QWidget *add_dialog::create_single_downloads_page(){
	ddclient_gui *p = (ddclient_gui *) this->parent();

	QWidget *page = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout();
	page->setLayout(layout);
	QFormLayout *single_layout = new QFormLayout();

	//layout->set

	//layout->addWidget(new QLabel(p->tsl("Add single Download (no Title necessary)")));
	layout->addLayout(single_layout);

	// single download
	url_single = new QLineEdit();
	title_single = new QLineEdit();
	title_single->setFixedWidth(200);
	package_single = new QComboBox();
	package_single->setFixedWidth(200);
	package_single->setEditable(true);
	fill_title_single = new QCheckBox(p->tsl("fill Title from URL"));

	single_layout->addRow(new QLabel(p->tsl("Package")), package_single);
	single_layout->addRow(new QLabel(p->tsl("Title")), title_single);
	single_layout->addRow(new QLabel(""), fill_title_single);
	single_layout->addRow(new QLabel(p->tsl("URL")), url_single);

	return page;
}


QWidget *add_dialog::create_many_downloads_page(){
	ddclient_gui *p = (ddclient_gui *) this->parent();

	QWidget *page = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout();
	page->setLayout(layout);

	QFormLayout *many_package_layout = new QFormLayout();
	layout->addWidget(new QLabel(p->tsl("Add many Downloads (one per Line, no Title necessary).")));

	// many downloads
	QTextEdit *add_many_edit = new QTextEdit();
	add_many = new QTextDocument(add_many_edit);
	add_many_edit->setDocument(add_many);
	package_many = new QComboBox();
	package_many->setEditable(true);
	separate_packages = new QCheckBox(p->tsl("Separate into different Packages"));
	fill_title = new QCheckBox(p->tsl("fill Title from URL"));

	many_package_layout->addRow(new QLabel(p->tsl("Package")), package_many);
	many_package_layout->addRow(new QLabel(""), separate_packages);
	many_package_layout->addRow(new QLabel(""), fill_title);

	layout->addWidget(new QLabel(p->tsl("Separate URL and Title like this: http://something.aa/bb::a fancy Title")));
	layout->addLayout(many_package_layout);
	layout->addWidget(add_many_edit);

	return page;
}


void add_dialog::find_parts(vector<new_download> &all_dls){
    ddclient_gui *p = (ddclient_gui *) parent();
    downloadc *dclient = p->get_connection();
    int package_id;
    bool error_occured = false;
    int error = 0;

    // prepare imaginary file names
    vector<new_download>::iterator it = all_dls.begin();
    for(; it != all_dls.end(); ++it){
        it->file_name = find_title(it->url);
    }


    // decide packages based on the file names
    vector<new_download>::iterator inner_it;

    for(it = all_dls.begin(); it != all_dls.end(); ++it){
        if(it->package == -1){ // found a download without a package

            // create a new package
            try{
                package_id = dclient->add_package();
            }catch(client_exception &e){ // there is no reason to continue if we failed here
                QMessageBox::warning(this,  p->tsl("Error"), p->tsl("Failed to create Package."));
                return;
            }

            it->package = package_id;

            // find all other downloads with the same file name
            for(inner_it = it+1; inner_it != all_dls.end(); ++inner_it){

                if(inner_it->package == -1){ // no package yet
                    if(inner_it->file_name == it->file_name)
                        inner_it->package = package_id;
                }
            }
        }
    }


    // finally send downloads
    for(it = all_dls.begin(); it != all_dls.end(); ++it){
        try{
			if((this->fill_title->isChecked()) && it->title.size() < 1)
                it->title = find_title(it->url, false);

            dclient->add_download(it->package, it->url, it->title);
        }catch(client_exception &e){
            error_occured = true;
            error = e.get_id();
        }

    }



    if(error_occured){
        if(error == 6)
            QMessageBox::warning(this,  p->tsl("Error"), p->tsl("Failed to create Package."));
        else if(error == 13)
            QMessageBox::warning(this,  p->tsl("Invalid URL"), p->tsl("At least one inserted URL was invalid."));
    }

}


string add_dialog::find_title(string url, bool strip){
    string file_name = url;

    if(file_name.size() < 1)
        return "";

    // cut away some things
    size_t n;

    n = file_name.find_last_of("//");
    if((n != (file_name.length())-1) && (n != std::string::npos)) // cut everything till the last / if there is something after it, eg: http://cut.me/bla => bla
        file_name = file_name.substr(n+1);

    else if(n != std::string::npos){ // there is a / at the end, delete it and do it again
        file_name.erase(n, 1);

        n = file_name.find_last_of("//");
        if(n != ((file_name.length())-1) && (n != std::string::npos)) // cut everything till the last / if there is something after it, eg: http://cut.me/bla => bla
            file_name = file_name.substr(n+1);

    }

    n = file_name.find_last_of("?");
    if(n != std::string::npos) // cut everything from the last ?, eg: test?var1=5 => test
        file_name = file_name.substr(0, n);

    string cut_me = ".html";
    while((n = file_name.find(cut_me)) != std::string::npos) // cut .html
            file_name.replace(n, cut_me.length(), "");

    cut_me = ".htm";
    while((n = file_name.find(cut_me)) != std::string::npos) // cut .htm
            file_name.replace(n, cut_me.length(), "");

    if(strip){
        for (size_t i = 0; i < file_name.size(); ++i){ // strip numbers
            if(isdigit(file_name[i])){
                file_name.erase(i, 1);
                --i; // otherwise the next letter will not be looked at
            }
        }

    for (size_t i = 0; i < file_name.size(); ++i) // make it case insensitive
        file_name[i] = tolower(file_name[i]);

    }

    return file_name;
}


void add_dialog::separate_packages_toggled(){
    if(separate_packages->isChecked())
        package_many->setEnabled(false);
    else
        package_many->setEnabled(true);
}


void add_dialog::fill_title_toggled(){
    if(fill_title_single->isChecked())
        title_single->setEnabled(false);
    else
        title_single->setEnabled(true);
}


void add_dialog::change_page(QListWidgetItem *current, QListWidgetItem *previous){
	if(!current)
		current = previous;

	pages->setCurrentIndex(tabs->row(current));
}


void add_dialog::ok(){
    ddclient_gui *p = (ddclient_gui *) parent();
    downloadc *dclient = p->get_connection();

    string title = title_single->text().toStdString();
    string url = url_single->text().toStdString();
    string many = add_many->toPlainText().toStdString();
    string package_single = this->package_single->currentText().toStdString();
    string package_many = this->package_many->currentText().toStdString();

    bool separate = separate_packages->isChecked();

	// exchange every :: inside the title (of single download) with a blank
    size_t title_find;
	title_find = title.find("::");

    while(title_find != string::npos){
        title.at(title_find) = ' ';
		title_find = title.find("::");
    }

    // fill title from url if checked
    if(fill_title_single->isChecked())
        title = find_title(url, false);

    // find out if we have an existing or new package
    int package_single_id = -1;
    int package_many_id = -1;
    vector<package_info>::iterator it = packages.begin();

    if(package_single.size() != 0){ // we don't have to check if there's no package name given
        for(; it != packages.end(); ++it){ // first single package
            stringstream s;
            s << it->id;
            if(package_single == s.str()){
                package_single_id = it->id;
                break;
            }

            s << ": " << it->name;
            if(package_single == s.str()){
                package_single_id = it->id;
                break;
            }
        }
    }

    if((package_many.size() != 0) && !separate){ // we don't have to check if there's no package name given and we don't have to separate into packages
        for(it = packages.begin(); it != packages.end(); ++it){ // now many package
            stringstream s;
            s << it->id;
            if(package_many == s.str()){
                package_many_id = it->id;
                break;
            }

            s << ": " << it->name;
            if(package_many == s.str()){
                package_many_id = it->id;
                break;
            }
        }
    }


    bool error_occured = false;
    int error = 0;
    string line;
    size_t lineend = 1, urlend;

    // add single download
    if(!url.empty()){
        try{
            if(package_single_id == -1) // create a new package
                package_single_id = dclient->add_package(package_single);

            dclient->add_download(package_single_id, url, title);
        }catch(client_exception &e){
            error_occured = true;
            error = e.get_id();
        }
    }

    // add many downloads
    vector<new_download> all_dls; // in case we have to separate into packages
    new_download dl = {"", "", "", -1};

    // parse lines
    while(many.length() > 0 && lineend != string::npos){
        lineend = many.find("\n"); // termination character for line

        if(lineend == string::npos){ // this is the last line (which ends without \n)
            line = many.substr(0, many.length());
            many = "";

        }else{ // there is still another line after this one
            line = many.substr(0, lineend);
            many = many.substr(lineend+1);
        }

        // parse url and title
		urlend = line.find("::");

        if(urlend != string::npos)
		{
			url = line.substr(0, urlend);
			line = line.substr(urlend+1);

			urlend = line.find("::");
			while(urlend != string::npos){ // exchange every | with a blank (there can be some | inside the title too)
					line.at(urlend) = ' ';
					urlend = line.find("::");
			}
			title = line;
        }else{ // no title
                url = line;
                title = "";
        }

        if(separate){ // don't send it now, find fitting packages first
            dl.url = url;
            dl.title = title;
            all_dls.push_back(dl);

        }else{ // send a single download
            try{
                if(package_many_id == -1) // create a new package
                    package_many_id = dclient->add_package(package_many);

				if((this->fill_title->isChecked()) && title.size() < 1)
                    title = find_title(url, false);

                dclient->add_download(package_many_id, url, title);
            }catch(client_exception &e){
                error_occured = true;
                error = e.get_id();
            }
        }
    }

    if(all_dls.size() > 0){
        find_parts(all_dls);
    }

    if(error_occured){
        if(error == 6)
            QMessageBox::warning(this,  p->tsl("Error"), p->tsl("Failed to create Package."));
        else if(error == 13)
            QMessageBox::warning(this,  p->tsl("Invalid URL"), p->tsl("At least one inserted URL was invalid."));
    }

    emit done(0);
}

