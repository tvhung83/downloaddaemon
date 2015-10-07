/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "language.h"
#include <fstream>


language::language() : lang("English"){
}


void language::set_working_dir(std::string working_dir){
    std::lock_guard<std::mutex> lock(mx);
    this->working_dir = working_dir;
}


bool language::set_language(std::string lang){
    std::lock_guard<std::mutex> lock(mx);
    texts.clear();

    // einlesen der datei
    if(lang != "English"){ // default language is english
        size_t found;

        if(working_dir == "")
            return false;


        std::string file_name = working_dir + lang;
        std::ifstream ifs(file_name.c_str(), std::fstream::in); // open file

        if(ifs.good()){ // file successfully opened
            std::string line;

            while((!ifs.eof())){ // loop through lines
                getline(ifs, line);

                replace_all(line, "\\n", "\n");
                replace_all(line, "\\t", "\t");

                std::string lang_string;

                found = line.find("->"); // english and language string are separated by ->

                if (found != std::string::npos && line[0] != '#'){ // split line in english and language string and insert it
                    lang_string = line.substr(found+2);
                    line = line.substr(0, found);

                    texts.insert(std::pair<std::string, std::string>(line, lang_string));
                }
            }

            ifs.close(); // close file
        }else // error at opening
            return false;

    }

    this->lang = lang;
    return true;
}


std::string language::operator[](std::string index){
    std::lock_guard<std::mutex> lock(mx);

    if(lang == "English") // default language is English
        return index;

    if(texts.find(index) == texts.end()) // if index can't be found return index
        return index;

    else
        return texts[index];
}


void language::replace_all(std::string& str, const std::string& old, const std::string& new_s){
    size_t n;

    while((n = str.find(old)) != std::string::npos){
        str.replace(n, old.length(), new_s);
    }
}
