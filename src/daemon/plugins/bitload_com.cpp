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

//Only Free-User support
plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	ddcurl* handle = get_handle();
	std::string resultstr;
	string url = get_url();
	string new_url = url + "?c=Free";
	handle->setopt(CURLOPT_COOKIEFILE, "");
	handle->setopt(CURLOPT_FOLLOWLOCATION, 0);
	handle->setopt(CURLOPT_URL, new_url);//url.c_str());
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &resultstr);
	handle->setopt(CURLOPT_HTTPGET,true);
	handle->setopt(CURLOPT_NOBODY,false);
	//need 2 run times
	int success = handle->perform();
	success = handle->perform();
	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}
		//get real link
	url= search_between(resultstr, "</div>\n\t<a href=\"","\"");
	if (url.size() == 0)
		return PLUGIN_FILE_NOT_FOUND;

	outp.download_url = url;

	return PLUGIN_SUCCESS;

}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		outp.allows_resumption = true;
		outp.allows_multiple = true;
	} else {
		outp.allows_resumption = false;
		outp.allows_multiple = true;
	}
	outp.offers_premium = false;
}
