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
#include <string>
#include <algorithm>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	ddcurl* handle = get_handle();
	string result;
	size_t pos;
	int res;
	download_container urls;

	handle->setopt(CURLOPT_URL, get_url());
	handle->setopt(CURLOPT_HEADER, 1);
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &result);
	handle->setopt(CURLOPT_COOKIEFILE, "");
	res = handle->perform();
	string url = get_url();
	log_string("Serienjunkies.org: trying to decrypt " + url,LOG_DEBUG);
	if(res != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}

	if (result.find("Du hast zu oft das Captcha falsch eingegeben!")  != std::string::npos) {
		log_string("Serienjunkies.org: You have entered wrong Captcha too many times",LOG_DEBUG);
		set_wait_time(1800);
		return PLUGIN_SERVER_OVERLOADED;
	}
	if (result.find("Du hast das Download-Limit")  != std::string::npos) {
		log_string("Serienjunkies.org: download limit reached",LOG_DEBUG);
		set_wait_time(1800);
		return PLUGIN_SERVER_OVERLOADED;
	}

	while((pos = result.find("<INPUT TYPE=\"HIDDEN\" NAME=\"s\" VALUE=\"")) != string::npos) {
		handle->setopt(CURLOPT_URL, get_url());
		handle->setopt(CURLOPT_HEADER, 1);
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);
		handle->setopt(CURLOPT_COOKIEFILE, "");
		res = handle->perform();
		string url = get_url();
		log_string("Serienjunkies.org: trying to decrypt " + url,LOG_DEBUG);
		if(res != 0) {
			return PLUGIN_CONNECTION_ERROR;
		}
		if (result.find("Du hast zu oft das Captcha falsch eingegeben!")  != std::string::npos) {
			log_string("Serienjunkies.org: You have entered wrong Captcha too many times",LOG_DEBUG);
			set_wait_time(1800);
			return PLUGIN_SERVER_OVERLOADED;
		}
		if (result.find("Du hast das Download-Limit")  != std::string::npos) {
			log_string("Serienjunkies.org: download limit reached",LOG_DEBUG);
			set_wait_time(1800);
			return PLUGIN_SERVER_OVERLOADED;
		}
		string value = search_between(result, "<INPUT TYPE=\"HIDDEN\" NAME=\"s\" VALUE=\"","\"");

		string captchaAddress = "http://download.serienjunkies.org" + search_between(result, "<TD><IMG SRC=\"","\"");
		handle->setopt(CURLOPT_HEADER, 0);
		handle->setopt(CURLOPT_FOLLOWLOCATION,0);
		handle->setopt(CURLOPT_POST, 0);
		handle->setopt(CURLOPT_URL, captchaAddress);

		result.clear();
		handle->perform();
		std::string captcha_text = Captcha.process_image(result, "png", "", -1, false, false, captcha::SOLVE_MANUAL);
		string postdata = "s=" + value + "&c=" + captcha_text + "&action=download";
		result.clear();
		handle->setopt(CURLOPT_URL, get_url());
		handle->setopt(CURLOPT_HEADER, 1);
		handle->setopt(CURLOPT_FOLLOWLOCATION,0);
		handle->setopt(CURLOPT_POST, 1);
		handle->setopt(CURLOPT_COPYPOSTFIELDS, postdata);
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);
		res = handle->perform();

		if(res != 0) {
			return PLUGIN_CONNECTION_ERROR;
		}
	}

	vector<string> links = search_all_between(result, "<FORM ACTION=\"","\" STYLE=\"display:",0,true);
	for(size_t i = 0; i < links.size(); i++) {
        if ((links[i].find("mirror") == std::string::npos) &&
        (links[i].find("firstload.com") == std::string::npos)){

            //firstload
            result.clear();
			handle->setopt(CURLOPT_URL, links[i]);
			handle->setopt(CURLOPT_POST, 0);
			handle->setopt(CURLOPT_HEADER, 1);
			handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
			handle->setopt(CURLOPT_WRITEDATA, &result);
			res = handle->perform();
			string temp = search_between(result, "Location: ", "X-Server");
			temp = temp.substr(0,temp.size()-2);
			urls.add_download(temp, "");
		}
	}
	replace_this_download(urls);
	return PLUGIN_SUCCESS;
}

bool get_file_status(plugin_input &inp, plugin_output &outp)
{
	return false;
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp)
{
	outp.allows_resumption = false;
	outp.allows_multiple = false;

	outp.offers_premium = false;
}
