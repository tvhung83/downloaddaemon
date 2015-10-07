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
#include <crypt/base64.h>
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
		log_string("relink.us: failed to fetch URL", LOG_WARNING);
		return PLUGIN_ERROR;
	}
	size_t start = result.find("<table border=\"0\" cellspacing=\"0\" align=\"center\">");
	string dlcurl = "http://relink.us/" + search_between(result, "<a href=\"", "\"", start);
	if(!dlcurl.empty() && dlcurl.find("&dlc=") != string::npos) {
		handle->setopt(CURLOPT_URL, dlcurl);
		download_container d;
		result.clear();
		handle->perform();
		decode_dlc(result, &d);
		replace_this_download(d);
		return PLUGIN_SUCCESS;	
	}
	return PLUGIN_ERROR;
}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	return false;
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	outp.allows_resumption = false;
	outp.allows_multiple = false;

	outp.offers_premium = false;
}
