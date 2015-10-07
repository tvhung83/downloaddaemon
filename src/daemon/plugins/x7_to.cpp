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

//Only Free-User and Free-User Account support
plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	ddcurl* handle = get_handle();
	std::string resultstr;
	string url = get_url();
	int ret = 0;
	resultstr.clear();

	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		string post = "id="+inp.premium_user+"&pw="+inp.premium_password;
		handle->setopt(CURLOPT_URL, "http://x7.to/james/login");
		handle->setopt(CURLOPT_POST, 1);
		handle->setopt(CURLOPT_FOLLOWLOCATION, 0);
		handle->setopt(CURLOPT_REFERER, "http://x7.to/");
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &resultstr);
		handle->setopt(CURLOPT_COPYPOSTFIELDS, post);
		handle->setopt(CURLOPT_COOKIEFILE, "");
		ret = handle->perform();

		if(ret != CURLE_OK)
			return PLUGIN_CONNECTION_ERROR;

		resultstr.clear();
		handle->setopt(CURLOPT_URL, "http://x7.to/my");
		handle->setopt(CURLOPT_POST, 0);
		handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &resultstr);

		ret = handle->perform();
		if(ret != CURLE_OK)
			return PLUGIN_CONNECTION_ERROR;

		// If the Account is a free account, set resume and multiple Downloads to false
		if (resultstr.find(">Free<") != std::string::npos){
			outp.allows_resumption = false;
			outp.allows_multiple = false;
		}
		else {
			outp.allows_resumption = true;
			outp.allows_multiple = true;
		}

		resultstr.clear();
	}

	handle->setopt(CURLOPT_URL, url.c_str());
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &resultstr);

	ret = handle->perform();
	if(ret != CURLE_OK)
		return PLUGIN_CONNECTION_ERROR;

	if (resultstr.find("The requested file is larger than 1,00 GB, only premium members will be able to download the file!") != std::string::npos)
		return PLUGIN_AUTH_FAIL;

	if (resultstr.find("File not found!") != std::string::npos)
		return PLUGIN_FILE_NOT_FOUND;

	vector<string> splitted_url = split_string(url, "/");

	string new_url = "http://x7.to/james/ticket/dl/" + splitted_url[3];

	resultstr.clear();
	handle->setopt(CURLOPT_URL, new_url);
	handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
	handle->setopt(CURLOPT_HTTPPOST, 1);
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &resultstr);
	handle->setopt(CURLOPT_COPYPOSTFIELDS, "");
	ret = handle->perform();

	if(ret != CURLE_OK)
		return PLUGIN_CONNECTION_ERROR;

	if (resultstr.find("limit-parallel") != std::string::npos){
		set_wait_time(10);
		return PLUGIN_NO_PARALLEL;
	}

	if (resultstr.find("limit-dl") != std::string::npos){
		set_wait_time(1800);
		return PLUGIN_SERVER_OVERLOADED;
	}

	set_wait_time(atoi(search_between(resultstr, "wait:",",").c_str()));

	outp.download_url = search_between(resultstr, "url:'","'").c_str();
	return PLUGIN_SUCCESS;
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		outp.allows_resumption = false;
		outp.allows_multiple = false;
	}

	else {
		outp.allows_resumption = false;
		outp.allows_multiple = false;
	}
	// For Free-Account
	outp.offers_premium = true;
}
