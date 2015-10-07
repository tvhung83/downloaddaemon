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
	// not needed here.
	// std::string *blubb = (std::string*)userp;
	// blubb->append((char*)buffer, nmemb);
	return nmemb;
}

size_t header_cb( void *ptr, size_t size, size_t nmemb, void *userdata) {
	std::string *d = static_cast<std::string*>(userdata);
	std::string header((char*)ptr, nmemb);
	if (header.find("Location:") != std::string::npos) {
		(*d) = header.substr(10);
		(*d) = d->substr(0, d->find("\r\n"));
		log_string("linksafe.me: found Location: " + *d, LOG_DEBUG);
	}
	return nmemb;
}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	// simple plugin: We just have to check the Location: header and replace_this_download with the result
	ddcurl* handle = get_handle();
	string result;
	handle->setopt(CURLOPT_URL, get_url());
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_COOKIEFILE, "");
	handle->setopt(CURLOPT_HEADERFUNCTION, header_cb);
	handle->setopt(CURLOPT_WRITEHEADER, &result);
	handle->perform();
	if (result.size() == 0) {
		return PLUGIN_ERROR;
	}
	download_container d;
	d.add_download(result);
	replace_this_download(d);
	return PLUGIN_SUCCESS;
}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	return false;
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	outp.allows_resumption = false;
	outp.allows_multiple = true;

	outp.offers_premium = false;
}
