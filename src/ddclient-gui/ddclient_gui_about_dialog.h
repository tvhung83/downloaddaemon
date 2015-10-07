/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_GUI_ABOUT_DIALOG_H
#define DDCLIENT_GUI_ABOUT_DIALOG_H

#include <QDialog>


class about_dialog : public QDialog{
    public:
        /** Constructor
        *   @param parent MainWindow, which calls the dialog
        *   @param build buildinformation
        */
        about_dialog(QWidget *parent, QString build);
};

#endif // DDCLIENT_GUI_ABOUT_DIALOG_H
