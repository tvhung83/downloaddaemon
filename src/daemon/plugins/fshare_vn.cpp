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
	string url = get_url();
	log_string("fshare.vn: START [" + url + "]", LOG_DEBUG);
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
		handle->setopt(CURLOPT_SSL_VERIFYPEER, 0L);
		handle->setopt(CURLOPT_COOKIEFILE, "");

		int ret = handle->perform();
		// log_string("fshare.vn: result=" + result + ", ret=" + to_string(ret), LOG_DEBUG);
		if(ret != CURLE_OK)
			return PLUGIN_CONNECTION_ERROR;
		
		string csrf = search_between(result,"type=\"hidden\" value=\"","\" name=\"fs_csrf\"");
		log_string("fshare.vn: csrf=" + csrf, LOG_DEBUG);

		string data = "LoginForm%5Bemail%5D=" + handle->escape(inp.premium_user) + "&LoginForm%5Bpassword%5D=" + handle->escape(inp.premium_password) +
				  "&fs_csrf=" + csrf + "&LoginForm%5BrememberMe%5D=1";
		handle->setopt(CURLOPT_POST, 1);
		handle->setopt(CURLOPT_COPYPOSTFIELDS, data.c_str());
		log_string("fshare.vn: data=" + data, LOG_DEBUG);

		result.clear();
		ret = handle->perform();
		log_string("fshare.vn: result=" + to_string(result.find("/account/profile")), LOG_DEBUG);
		if(ret != CURLE_OK)
			return PLUGIN_CONNECTION_ERROR;
		
		if(result.find("/account/profile") == string::npos) {
			return PLUGIN_AUTH_FAIL;
		}

		handle->setopt(CURLOPT_URL, url);
		handle->setopt(CURLOPT_POST, 0);
		handle->setopt(CURLOPT_COPYPOSTFIELDS, "");
		handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
		handle->setopt(CURLOPT_HEADER, 1);
		handle->setopt(CURLOPT_NOBODY, 1);
		handle->setopt(CURLOPT_WRITEDATA, &result);
		char *premium_url;
		result.clear();
		ret = handle->perform();
		// log_string("fshare.vn: ret=" + to_string(ret) + ", result=" + to_string(result.find("/account/profile")), LOG_DEBUG);
		ret = handle->getinfo(CURLINFO_EFFECTIVE_URL, &premium_url);
		// log_string("fshare.vn: result=" + result, LOG_DEBUG);
		log_string("fshare.vn: ret=" + to_string(ret) + ", premium_url=" + premium_url, LOG_DEBUG);
		handle->cleanup();
		if((CURLE_OK == ret) && premium_url) {
			outp.download_url = premium_url;
			return PLUGIN_SUCCESS;
		} else {
			return PLUGIN_CONNECTION_ERROR;
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
