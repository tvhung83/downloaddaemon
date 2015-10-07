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
#include <fstream>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	string url = get_url();
	if(url.find("/folder/")==string::npos)
	{
		if(inp.premium_user.empty() || inp.premium_password.empty()) {
			return PLUGIN_AUTH_FAIL; // free download not supported yet
		}
		string result;
		ddcurl* handle = get_handle();
		handle->setopt(CURLOPT_URL, "https://www.fshare.vn/login");
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);

		int ret = handle->perform();
		if(ret != CURLE_OK)
			return PLUGIN_CONNECTION_ERROR;
		if(ret == CURLE_OK) {
			handle->cleanup();
			string csrf = search_between(result,"type=\"hidden\" value=\"","\" name=\"fs_csrf\"");

			string data = "LoginForm%5Bemail%5D=" + handle->escape(inp.premium_user) + "&LoginForm%5Bpassword%5D=" + handle->escape(inp.premium_password) +
					  "&fs_csrf=" + csrf + "&LoginForm%5BrememberMe%5D=1";
			handle->setopt(CURLOPT_URL, "https://www.fshare.vn/login");
			handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
			handle->setopt(CURLOPT_WRITEDATA, &result);
			handle->setopt(CURLOPT_COPYPOSTFIELDS, data.c_str());

			ret = handle->perform();
			if(ret != CURLE_OK)
				return PLUGIN_CONNECTION_ERROR;
			if(ret == CURLE_OK) {
				if(result.find("Invalid login.") != string::npos) {
					return PLUGIN_AUTH_FAIL;
				}
			}
			handle->cleanup();

			handle->setopt(CURLOPT_URL, url);
			handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
			handle->setopt(CURLOPT_WRITEDATA, &result);
			char *premium_url;
			ret = handle->getinfo(CURLINFO_EFFECTIVE_URL, &premium_url);
			if((CURLE_OK == ret) && premium_url) {
				outp.download_url = premium_url;
				return PLUGIN_SUCCESS;
			} else {
				return PLUGIN_CONNECTION_ERROR;
			}
		}

	}
	return PLUGIN_CONNECTION_ERROR;
}	

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		outp.allows_resumption = true;
		outp.allows_multiple = true;
	} else {
		outp.allows_resumption = false;
		outp.allows_multiple = false;
	}
	outp.offers_premium = true;
}
