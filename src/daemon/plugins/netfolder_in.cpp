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
	string result;
	handle->setopt(CURLOPT_URL, get_url());
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &result);
	handle->setopt(CURLOPT_COOKIEFILE, "");
	int res = handle->perform();
	if(res != 0) {
		return PLUGIN_ERROR;
	}
	download_container urls;
	try {
		size_t listpos = result.find("<input type=\"hidden\" name=\"list\" value=\"");
		if(listpos == string::npos) return PLUGIN_ERROR;
		listpos += 40;
		std::string list = result.substr(listpos, result.find("\"", listpos) - listpos);
		vector<string> listvec = split_string(list, ",");


		for(size_t i = 0; i < listvec.size(); ++i) {
			urls.add_download(listvec[i], "");
		}
	} catch(...) {}
	replace_this_download(urls);
	return PLUGIN_SUCCESS;
}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	return false;
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	outp.allows_resumption = false;
	outp.allows_multiple = false;

	outp.offers_premium = false;
}
