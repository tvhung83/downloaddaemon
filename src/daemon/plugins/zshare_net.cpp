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

	handle->setopt(CURLOPT_POST,1);
	handle->setopt(CURLOPT_COPYPOSTFIELDS, "referer2=&download=1&imageField");
	handle->setopt(CURLOPT_URL, url.c_str());
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &resultstr);

	int success = handle->perform();

	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}
	//get real link
	url = search_between(resultstr, "link_enc=new Array(", ");");
	vector<string> letters = search_all_between(url, "'", "'");

	//no link can be found -> file deleted
	if (letters.size() == 0)
		return PLUGIN_FILE_NOT_FOUND;

	//wait-time until download can start
	set_wait_time(50);
	url.clear();

	//get link from pseudo-encrypted string
	for(size_t i = 0; i < letters.size(); ++i) {
		if(letters[i] != ",")
		url += letters[i];
	}

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
