/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_GUI_MAIN_WINDOW_H
#define DDCLIENT_GUI_MAIN_WINDOW_H

#include <Qt/qstring.h>
#include <string>
#include <language/language.h>


enum error_message{error_none, error_selected, error_connected};

/** This is an abstract basic class that is used as an interface to include all public methods of the main window (ddclient_gui) */
class main_window{
	public:
		virtual QString CreateQString(const std::string &text) = 0;
		virtual language *GetLanguage() = 0;
		virtual void set_downloading_active() = 0;
		virtual void set_downloading_deactive() = 0;
		virtual QString tsl(std::string text, ...) = 0;

		error_message last_error_message;
};

#endif // DDCLIENT_GUI_MAIN_WINDOW_H
