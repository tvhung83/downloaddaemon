/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define PLUGIN_CAN_PRECHECK
#include "plugin_helpers.h"
#include <curl/curl.h>
#include <cstdlib>
#include <iostream>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	ddcurl* prepare_handle = get_handle();

	std::string resultstr;
	std::string url = get_url();
	replace_all(url, "/ru/", "/de/");
	replace_all(url, "/de/", "/de/");
	replace_all(url, "/es/", "/de/");
	replace_all(url, "/pt/", "/de/");

	prepare_handle->setopt(CURLOPT_URL, url.c_str());
	prepare_handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	prepare_handle->setopt(CURLOPT_WRITEDATA, &resultstr);
	int success = prepare_handle->perform();

	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}

	if(resultstr.find("file does not exist") != string::npos) { // FILE NOT FOUND
		return PLUGIN_FILE_NOT_FOUND;
	}

	prepare_handle->setopt(CURLOPT_POST, 1);
	prepare_handle->setopt(CURLOPT_COPYPOSTFIELDS, "gateway_result=1");
	resultstr.clear();
	success = prepare_handle->perform();

	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}

	string next_url = "http://depositfiles.com" + search_between(resultstr, ".load('", "'");
	set_wait_time(61);
	sleep(61);
	prepare_handle->setopt(CURLOPT_URL, next_url.c_str());
	prepare_handle->setopt(CURLOPT_POST, 0);
	resultstr.clear();
	prepare_handle->perform();
	outp.download_url = search_between(resultstr, "action=\"", "\"");
	return PLUGIN_SUCCESS;
}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	std::string url = get_url();
	replace_all(url, "/ru/", "/de/");
	replace_all(url, "/de/", "/de/");
	replace_all(url, "/es/", "/de/");
	replace_all(url, "/pt/", "/de/");
	std::string result;
	ddcurl handle;
	handle.setopt(CURLOPT_LOW_SPEED_LIMIT, (long)10);
	handle.setopt(CURLOPT_LOW_SPEED_TIME, (long)20);
	handle.setopt(CURLOPT_CONNECTTIMEOUT, (long)30);
	handle.setopt(CURLOPT_NOSIGNAL, 1);
	handle.setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle.setopt(CURLOPT_WRITEDATA, &result);
	handle.setopt(CURLOPT_COOKIEFILE, "");
	handle.setopt(CURLOPT_URL, url.c_str());
	int res = handle.perform();
	handle.cleanup();
	if(res != 0) {
		outp.file_online = PLUGIN_CONNECTION_LOST;
		return true;
	}
	try {
		string size_s = search_between(result, "Dateigröße: <b>", "</b></span>");
		if(size_s.empty()) return PLUGIN_FILE_NOT_FOUND;
		vector<string> value = split_string(size_s, "&nbsp;");
		const char * oldlocale = setlocale(LC_NUMERIC, "C");
		double size = atof(value[0].c_str());
		setlocale(LC_NUMERIC, oldlocale);
		if(value[1] == "MB")
			size *= 1024*1024;
		else if(value[1] == "KB")
			size *= 1024;

		outp.file_online = PLUGIN_SUCCESS;
		outp.file_size = size;
	} catch(...) {
		outp.file_online = PLUGIN_FILE_NOT_FOUND;
	}
	return true;
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	//if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
	//	outp.allows_resumption = true;
	//	outp.allows_multiple = true;
	//} else {
		outp.allows_resumption = false;
		outp.allows_multiple = false;
	//}
	outp.offers_premium = false;
}
