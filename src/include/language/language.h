/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_WX_LANGUAGE_H
#define DDCLIENT_WX_LANGUAGE_H

#include <config.h>
#include <map>
#include <string>
#include <vector>

#ifndef USE_STD_THREAD
#include <boost/thread.hpp>
namespace std {
    using namespace boost;
}
#else
#include <thread>
#include <mutex>
#endif



/** Language Class, translates a string to a given language */
class language{
    public:

        /** Defaultconstructor */
        language();

        /** Sets the working directory
        *    @param working_dir Working Directory
        */
        void set_working_dir(std::string working_dir);

        /** Sets the language by reading in the language file.
        *    @param lang Language wich is set
        *    @returns Successful nor not
        */
        bool set_language(std::string lang);

        /** Operator []
        *    @param index Index where the language Object is accessed at
        *    @returns String
        */
        std::string operator[](std::string index);

        /** Repaces all occurences of old in str with new_s
        *    @param str string to search in
        *    @param old string to replace
        *    @param new_s replacement
        */
        static void replace_all(std::string& str, const std::string& old, const std::string& new_s);

    private:

        std::map<std::string, std::string> texts;
        std::string lang;
        std::string working_dir;
        std::mutex mx;
};

#endif // DDCLIENT_WX_LANGUAGE_H


