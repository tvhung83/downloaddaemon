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
#include <QtGui/QApplication>
#include <QLibraryInfo>
#include <QTranslator>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <cstdlib>
#include <string>
#include <iostream>
int main(int argc, char *argv[])
{
    // getting working dir
    std::string working_dir = argv[0];

    if(working_dir.find_first_of("/\\") == std::string::npos) {
        // no / in argv[0]? this means that it's in the path or in ./ (windows) let's find out where.
        std::string env_path;
        env_path = std::getenv("PATH");
        std::string curr_path;
        size_t curr_pos = 0, last_pos = 0;
        bool found = false;

        while((curr_pos = env_path.find_first_of(";:", curr_pos)) != std::string::npos) {
            curr_path = env_path.substr(last_pos, curr_pos -  last_pos);
	    curr_path += '/';
	    curr_path += argv[0];

            QFile file(curr_path.c_str());
            if(file.exists()) {
                found = true;
                break;
            }

            last_pos = ++curr_pos;
        }

        if(!found) {
            // search the last folder of $PATH which is not included in the while loop
            curr_path = env_path.substr(last_pos, curr_pos - last_pos);
            curr_path += argv[0];

            QFile file(curr_path.c_str());
            if(file.exists()) {
                found = true;
            }
        }

        if(found) {
            // successfully located the file..
            working_dir = curr_path;
        }
    }

    working_dir = working_dir.substr(0, working_dir.find_last_of("/\\"));
    if(working_dir.find_last_of("/\\") != std::string::npos) {
        working_dir = working_dir.substr(0, working_dir.find_last_of("/\\"));
    } else {
	// needed if it's started with ./ddclient-gui, so we get ./../share/
        working_dir += "/..";
    }
    working_dir += "/share/ddclient-gui/";
    #ifndef _WIN32
        char* abs_path = realpath(working_dir.c_str(), NULL);
        if(abs_path != NULL) {
            working_dir = abs_path;
            working_dir += "/";
            free(abs_path);
        }
    #endif

    QDir::setCurrent(working_dir.c_str());

    std::string config_dir;

    // getting config dir
    #ifdef _WIN32
        config_dir = getenv("APPDATA");
        config_dir += "\\ddclient-gui\\";
    #else
        config_dir = getenv("HOME");
        config_dir += "/.ddclient-gui/";
    #endif

    QDir dir(config_dir.c_str());
    if(!dir.exists()) {
        if(!dir.mkpath(dir.dirName())) {
            config_dir = "";
        }
    }

    QApplication a(argc, argv);

    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(&qtTranslator);

    ddclient_gui c(config_dir.c_str());

    c.show();
    return a.exec();
}

