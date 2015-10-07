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
	ddcurl* handle = get_handle();
	handle->setopt(CURLOPT_COOKIEFILE, "");
	std::string url = get_url();
	std::string result;
	handle->setopt(CURLOPT_URL, url.c_str());
	handle->setopt(CURLOPT_WRITEDATA, &result);
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);

	int res = handle->perform();
	if(res != 0) {
		return PLUGIN_ERROR;
	}

	if(result.find("this file is no longer available") != string::npos) {
		return PLUGIN_FILE_NOT_FOUND;
	}

	size_t n = result.find("<div class=\"basicBtn\">");
	if(n == string::npos) {
		return PLUGIN_ERROR;
	}
	n = result.find("<a href=\"", n) + 9;
	url = "http://www.filefactory.com" + result.substr(n, result.find("\"", n) - n);
	result.clear();
	handle->setopt(CURLOPT_URL, url.c_str());
	res = handle->perform();
	if(res != 0) {
		return PLUGIN_ERROR;
	}

	n = result.find("<div id=\"downloadLink\" style=\"display: none;\">");
	if(n == string::npos) {
		return PLUGIN_ERROR;
	}
	n = result.find("<a href=\"", n) + 9;
	outp.download_url = result.substr(n, result.find("\"", n) - n);
	n = result.find("link will be available below in ") + 32;
	n = result.find(">", n) + 1;
	string wait_string = result.substr(n, result.find("<", n) - n);
	int wait_seconds = atoi(wait_string.c_str());
	if(wait_seconds <= 31) {
		set_wait_time(30);
		return PLUGIN_SUCCESS;
	} else {
		set_wait_time(wait_seconds);
		return PLUGIN_LIMIT_REACHED;
	}
}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	std::string url = get_url();
	std::string result;
	ddcurl handle;
	handle.setopt(CURLOPT_LOW_SPEED_LIMIT, (long)10);
	handle.setopt(CURLOPT_LOW_SPEED_TIME, (long)20);
	handle.setopt(CURLOPT_CONNECTTIMEOUT, (long)30);
	handle.setopt(CURLOPT_NOSIGNAL, 1);
	handle.setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle.setopt(CURLOPT_WRITEDATA, &result);
	handle.setopt(CURLOPT_COOKIEFILE, "");
	handle.setopt(CURLOPT_FOLLOWLOCATION, true);
	handle.setopt(CURLOPT_URL, url.c_str());
	int res = handle.perform();
	handle.cleanup();
	if(res != 0) {
		outp.file_online = PLUGIN_CONNECTION_LOST;
		return true;
	}
	try {
		size_t n = result.find("<span>");
		if(n == string::npos) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		n += 6;
		size_t end = result.find("file", n) - 1;

		vector<string> value = split_string(result.substr(n, end - n), " ");

		const char * oldlocale = setlocale(LC_NUMERIC, "C");

		size_t size = 0;
		if(value[1].find("KB") != string::npos)
			size = strtod(value[0].c_str(), NULL) * 1024;
		else if(value[1].find("MB") != string::npos)
			size = strtod(value[0].c_str(), NULL) * (1024*1024);

		setlocale(LC_NUMERIC, oldlocale);

		outp.file_size = size;
	} catch(...) {
		outp.file_online = PLUGIN_FILE_NOT_FOUND;
	}
	return true;
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		outp.allows_resumption = true;
		outp.allows_multiple = true;
	} else {
		outp.allows_resumption = false;
		outp.allows_multiple = false;
	}
	outp.offers_premium = false;
}
