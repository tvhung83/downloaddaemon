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

	handle->setopt(CURLOPT_COOKIEFILE, "");
	handle->setopt(CURLOPT_FOLLOWLOCATION, 0);
	handle->setopt(CURLOPT_URL, url.c_str());
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &resultstr);
	int success = handle->perform();

	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}

	if (resultstr.find("File not found.") != std::string::npos)
		return PLUGIN_FILE_NOT_FOUND;

	string post_form = search_between(resultstr, "method=\"post\">","</button>");
	string name = search_between(post_form, "input type=\"hidden\" name=\"","\"");
	string value = search_between(post_form, "value=\"","\"");
	string name2 = search_between(post_form, "button name=\"","\"");
	string postdata = name + "=" +value+ "&" +name2+ "=";

	set_wait_time(60);
	sleep(30);

	resultstr.clear();
	handle->setopt(CURLOPT_HTTPPOST,1);
	handle->setopt(CURLOPT_FOLLOWLOCATION, 0);
	handle->setopt(CURLOPT_URL, url.c_str());
	handle->setopt(CURLOPT_COPYPOSTFIELDS, postdata);
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &resultstr);
	success = handle->perform();

	post_form = search_between(resultstr, "method=\"post\">","</button>");
	name = search_between(post_form, "input type=\"hidden\" name=\"","\"");
	value = search_between(post_form, "value=\"","\"");
	name2 = search_between(post_form, "button name=\"","\"");
	postdata = name + "=" +value+ "&" +name2+ "=";

	// prepare handle for DD
	handle->setopt(CURLOPT_HTTPPOST,1);
	handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
	handle->setopt(CURLOPT_URL, url.c_str());
	handle->setopt(CURLOPT_COPYPOSTFIELDS, postdata);

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
