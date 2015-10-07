/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

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


	if (result.find("This video has been removed by the user.") != std::string::npos)
		return PLUGIN_FILE_NOT_FOUND;

	string url = search_between(result, "url_encoded_fmt_stream_map=", "\"");
	url = handle->unescape(url);
	url = handle->unescape(url);
	url = handle->unescape(url);

	url = url.substr(0, url.find("x-flv")+4);
	url = url.substr(url.rfind("url=")+4,url.length());

	outp.download_url = url;

	string title = search_between(result, "name=\"title\" content=\"", "\">");
	make_valid_filename(title);

	outp.download_filename = title + ".flv";

	return PLUGIN_SUCCESS;
}


extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		outp.allows_resumption = false;
		outp.allows_multiple = true;
	} else {
		outp.allows_resumption = false;
		outp.allows_multiple = true;
	}
	outp.offers_premium = false; // no login support yet
}
