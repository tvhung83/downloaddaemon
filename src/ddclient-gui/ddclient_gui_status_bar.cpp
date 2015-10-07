/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient_gui_status_bar.h"

#include <QModelIndex>
#include <QProgressBar>
#include <QStyledItemDelegate>
#include <QApplication>
#include "ddclient_gui.h"

using namespace std;


status_bar::status_bar(QObject *parent) : QStyledItemDelegate(parent){
}


status_bar::~status_bar(){
}


QSize status_bar::sizeHint(const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const{
    return QSize(60,16);
}


void status_bar::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const{
    string content = index.data().toString().toStdString();

    if(content == "" && index.column() == 4){ // we have a package, because there is no data inside
        ddclient_gui *p = (ddclient_gui *) parent();

        int progress = p->calc_package_progress(index.row());

        QStyleOptionProgressBar progressBarOption;
        progressBarOption.rect = option.rect;
        progressBarOption.minimum = 0;
        progressBarOption.maximum = 100;
        progressBarOption.progress = progress;
        progressBarOption.text = QString::number(progress) + "%";
        progressBarOption.textVisible = true;
		progressBarOption.state = QStyle::State_Enabled | QStyle::State_Active;

        QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);

    }else if(index.column() == 4){ // we have a download

        size_t pos = content.find("%");
        if(pos == string::npos) // there is no % in content => we can't show a value inside the progress bar
			QStyledItemDelegate::paint(painter, option, index);

        else{
            string s = content.substr(0, content.find("%"));
            s = s.substr(s.find_last_of(" "));
            s = s.substr(0, s.find("."));
            int progress = atoi(s.c_str());
            QStyleOptionProgressBar progressBarOption;
            progressBarOption.rect = option.rect;

            if((progress > 100) || (progress <= 0)){
                progressBarOption.minimum = 0;
                progressBarOption.maximum = 1;
                progressBarOption.progress = 0;
            }else{
                progressBarOption.minimum = 0;
                progressBarOption.maximum = 100;
                progressBarOption.progress = progress;
            }

            progressBarOption.text = content.c_str();
            progressBarOption.textVisible = true;
			progressBarOption.state = QStyle::State_Enabled | QStyle::State_Active;

            QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);
        }
    }else
		QStyledItemDelegate::paint(painter, option, index);
}
