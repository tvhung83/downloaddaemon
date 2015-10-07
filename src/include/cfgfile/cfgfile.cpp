/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include"cfgfile.h"
#include<fstream>
#include<string>
#include<sstream>
#include<vector>
#ifndef DDCLIENT_GUI
    #ifndef USE_STD_THREAD
	#include <boost/thread.hpp>
	namespace std {
	    using namespace boost;
	}
    #else
	#include <thread>
    #endif
#endif
using namespace std;


cfgfile::cfgfile()
	: comment_token("#"), is_writeable(false), eqtoken('=') {
}

cfgfile::cfgfile(std::string &fp, bool open_writeable)
	: filepath(fp), comment_token("#"), is_writeable(open_writeable), eqtoken('=') {
	open_cfg_file(filepath, is_writeable);
	comment_token = "#";
}

cfgfile::cfgfile(std::string &fp, std::string &comm_token, char eq_token, bool open_writeable)
	: filepath(fp), comment_token(comm_token), is_writeable(open_writeable), eqtoken(eq_token) {
	open_cfg_file(filepath, is_writeable);
}

cfgfile::~cfgfile() {
	file.close();
}

void cfgfile::open_cfg_file(const std::string &fp, bool open_writeable) {
	file.close();
	filepath = fp;
	if (open_writeable) {
		file.open(filepath.c_str(), fstream::out | fstream::in);
		is_writeable = true;
	}
	else {
		file.open(filepath.c_str(), fstream::in);
		is_writeable = false;
	}
}

std::string cfgfile::get_cfg_value(const std::string &cfg_identifier) {
#ifndef DDCLIENT_GUI
	unique_lock<recursive_mutex> lock(mx);
#endif
	if(!file.is_open() || !file.good()) {
#ifndef DDCLIENT_GUI
		lock.unlock();
#endif
		reload_file();
#ifndef DDCLIENT_GUI
		lock.lock();
#endif
	}

	CfgIter cit = cfg_cache.find(cfg_identifier);
	if(cit != cfg_cache.end()) {
        return cit->second;
	}
	file.seekg(0);
	std::string buff, identstr, val;
	size_t eqloc;
	while(file.good() && getline(file, buff)) {
		buff = buff.substr(0, buff.find(comment_token));
		eqloc = buff.find(eqtoken);
		identstr = buff.substr(0, eqloc);
		trim(identstr);
		if(identstr == cfg_identifier) {
			val = buff.substr(eqloc +1);
			trim(val);
			cfg_cache[cfg_identifier] = val;
			return val;
		}
	}

	// get the value from the default-config
	if(default_config.empty()) return "";

	cfgfile defconf(default_config, false);
	val = defconf.get_cfg_value(cfg_identifier);
	if(!val.empty() && is_writeable) {
		set_cfg_value(cfg_identifier, val);
	}
	cfg_cache[cfg_identifier] = val;
	return val;
}

bool cfgfile::get_bool_value(const std::string &cfg_identifier) {
	std::string result = get_cfg_value(cfg_identifier);
	for(size_t i = 0; i < result.size(); ++i) {
		result[i] = tolower(result[i]);
	}
	if(result == "1" || result == "true" || result == "yes")
		return true;
	return false;
}

long cfgfile::get_int_value(const std::string &cfg_identifier) {
	std::stringstream ss;
	ss << get_cfg_value(cfg_identifier);
        long res = 0;
	ss >> res;
	return res;
}

bool cfgfile::set_cfg_value(const std::string &cfg_identifier, const std::string &value) {
#ifndef DDCLIENT_GUI
	unique_lock<recursive_mutex> lock(mx);
#endif
	std::string cfg_value = value;
	trim(cfg_value);
	if(!is_writeable) {
		return false;
	}
	if(!file.is_open() || !file.good()) {
#ifndef DDCLIENT_GUI
		lock.unlock();
#endif
		reload_file();
#ifndef DDCLIENT_GUI
		lock.lock();
#endif
	}

	file.seekg(0);
	vector<std::string> cfgvec;
	std::string buff, newbuff, identstr, val;
	size_t eqloc;
	bool done = false;

	while(file.good()) {
		getline(file, buff);
		cfgvec.push_back(buff);
	}

	file.seekg(0);
	if(file.bad()) {
		return false;
	}

	for(vector<std::string>::iterator it = cfgvec.begin(); it != cfgvec.end(); ++it) {
		newbuff = it->substr(0, it->find(comment_token));
		eqloc = newbuff.find(eqtoken);
		if(eqloc == string::npos) {
			continue;
		}
		identstr = newbuff.substr(0, eqloc);
		trim(identstr);
		if(identstr == cfg_identifier) {
			// to protect from substr-segfaults
			*it += " ";
			*it = it->substr(0, eqloc + 1);
			*it += " ";
			*it += cfg_value;
			done = true;
			break;
		}
	}

	if(!done) {
		cfgvec.push_back(cfg_identifier + " = " + cfg_value);
	}

	// write to file.. need to close/reopen it in order to delete the existing content
	file.close();
	file.open(filepath.c_str(), fstream::in | fstream::out | fstream::trunc);
	for(vector<std::string>::iterator it = cfgvec.begin(); it != cfgvec.end(); ++it) {
		file << *it;
		if(it != cfgvec.end() - 1) {
			file << '\n';
		}
	}
	cfg_cache[cfg_identifier] = value;
#ifndef DDCLIENT_GUI
	lock.unlock();
#endif
	reload_file();

	return true;
}

std::string cfgfile::get_comment_token() const {
	return comment_token;
}

void cfgfile::set_comment_token(const std::string &token) {
	comment_token = token;
}

void cfgfile::reload_file() {
#ifndef DDCLIENT_GUI
	lock_guard<recursive_mutex> lock(mx);
#endif
	if(file.is_open()) {
		file.close();
	}

	if(is_writeable) {
		file.open(this->filepath.c_str(), fstream::in | fstream::out);
	} else {
		file.open(this->filepath.c_str(), fstream::in);
	}
}

void cfgfile::close_cfg_file() {
#ifndef DDCLIENT_GUI
	lock_guard<recursive_mutex> lock(mx);
#endif
	file.close();
}

inline bool cfgfile::writeable() const {
	return is_writeable;
}

inline std::string cfgfile::get_filepath() const {
	return filepath;
}

bool cfgfile::list_config(std::string& resultstr) {
#ifndef DDCLIENT_GUI
	unique_lock<recursive_mutex> lock(mx);
#endif
	if(!file.is_open()) {
		return false;
	}
	std::string buff;
	resultstr = "";
	file.seekg(0);
	while(!file.eof() && file.good()) {
		getline(file, buff);
		trim(buff);
		if(buff.empty()) {
			continue;
		} else {
			if(buff.find(comment_token, 0) != std::string::npos) {
				buff = buff.substr(0, buff.find(comment_token, 0));
			}
			resultstr += buff;
			resultstr += '\n';
		}
	}
	return true;
}

void cfgfile::trim(std::string &str) const {
	while(str.length() > 0 && isspace(str[0])) {
		str.erase(str.begin());
	}
	while(str.length() > 0 && isspace(*(str.end() - 1))) {
		str.erase(str.end() -1);
	}
}

void cfgfile::set_default_config(const std::string &defconf) {
#ifndef DDCLIENT_GUI
	lock_guard<recursive_mutex> lock(mx);
#endif
	default_config = defconf;
}

std::string cfgfile::get_default_config() {
#ifndef DDCLIENT_GUI
	lock_guard<recursive_mutex> lock(mx);
#endif
	return default_config;
}
