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
#include <crypt/base64.h>
#include <iostream>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

int overload_counter = 0;

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	ddcurl* handle = get_handle();
	std::string resultstr;
	string url = get_url();
	resultstr.clear();
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		handle->setopt(CURLOPT_SSL_VERIFYPEER, 0);
		std::string post_data = "user=" + inp.premium_user + "&pass=" + inp.premium_password;
		handle->setopt(CURLOPT_URL, "https://www.share-online.biz/user/login");
		handle->setopt(CURLOPT_REFERER, "http://www.share-online.biz/");
		handle->setopt(CURLOPT_POST, 1);
		handle->setopt(CURLOPT_COPYPOSTFIELDS, post_data.c_str());
		handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
		handle->setopt(CURLOPT_COOKIEFILE, "");
		handle->setopt(CURLOPT_COOKIE, "http://www.share-online.biz; page_language=english");
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &resultstr);
		handle->perform();

		if(resultstr.find("login_error") != string::npos)
			return PLUGIN_AUTH_FAIL;
		handle->setopt(CURLOPT_POST, 0);
		handle->setopt(CURLOPT_HEADER, 0);
		handle->setopt(CURLOPT_URL, url);
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &resultstr);
		int success = handle->perform();

		if(success != 0) {
			return PLUGIN_CONNECTION_ERROR;
		}

		//is file deleted?
		if (resultstr.find("dl_failure") != std::string::npos)
			return PLUGIN_FILE_NOT_FOUND;

		// if cookie/session error, wait 5sec and try it again
		if (resultstr.find("Unable to validate cookie/session") != std::string::npos) {
			overload_counter = 0;
			set_wait_time(5);
			return PLUGIN_SERVER_OVERLOADED;
		}

		//get real link
		string code = search_between(resultstr, "dl=\"","\"");
		string data = base64_decode(code);
		if (data.empty())
			return PLUGIN_FILE_NOT_FOUND;

		outp.download_url = data;
		return PLUGIN_SUCCESS;
		}
		else{
			// Free support disabled because of re-captcha
			return PLUGIN_AUTH_FAIL;

			handle->setopt(CURLOPT_COOKIE, "http://www.share-online.biz; page_language=english");
			handle->setopt(CURLOPT_URL, url.c_str());
			handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
			handle->setopt(CURLOPT_WRITEDATA, &resultstr);

			int success = handle->perform();

			// if IP block from Server wait 10min
			if (resultstr.find("Share-Online - Access to content denied") != std::string::npos){
				overload_counter = 0;
				set_wait_time(600);
				return PLUGIN_SERVER_OVERLOADED;
			}

			if(success != 0) {
				return PLUGIN_CONNECTION_ERROR;
			}

			//is file deleted?
			if (resultstr.find("dl_failure") != std::string::npos)
				return PLUGIN_FILE_NOT_FOUND;

			//get free User Link
			url = search_between(resultstr, "var url=\"","\"");
			resultstr.clear();
			handle->setopt(CURLOPT_POST,1);
			handle->setopt(CURLOPT_COPYPOSTFIELDS, "dl_free=1&choice=free");
			handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
			handle->setopt(CURLOPT_URL, url.c_str());
			handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
			handle->setopt(CURLOPT_WRITEDATA, &resultstr);
			success = handle->perform();

			if(success != 0) {
				return PLUGIN_CONNECTION_ERROR;
			}

			//this can be true very often, somethimes you need to wait very long time to get a Downloadticket
			//if downloadlink can't get at 30 retrys set waittime to 10 min to prevent IP block from server
			if (resultstr.find("No free slots for free users!") != std::string::npos){
				if (overload_counter < 30){
					overload_counter++;
					set_wait_time(3);
					return PLUGIN_SERVER_OVERLOADED;
				}
				else{
					overload_counter = 0;
					set_wait_time(600);
					return PLUGIN_SERVER_OVERLOADED;
				}
			}
			//set back counter to 0
			overload_counter = 0;

			if (resultstr.find("This file is too big for you remaining download volume!") != std::string::npos){
				set_wait_time(1800);
				return PLUGIN_SERVER_OVERLOADED;
			}
			//wait-time until download can start
			string wait = search_between(resultstr, "var wait=",";");
			set_wait_time(atoi(wait.c_str()));

			//get real link
			string code = search_between(resultstr, "dl=\"","\"");
			string data = base64_decode(code);

			outp.download_url = data;
			return PLUGIN_SUCCESS;

		}
}
bool get_file_status(plugin_input &inp, plugin_output &outp) {
	string url = get_url();
	string filename, fileid;
	vector<string> splitted_url = split_string(url, "/");

	if(splitted_url.size() < 5) return true;

	url = "http://www.share-online.biz/linkcheck/linkcheck.php";
	string postdata = "links=" + splitted_url[4];

	std::string result;
	ddcurl status;
	status.setopt(CURLOPT_LOW_SPEED_LIMIT, (long)10);
	status.setopt(CURLOPT_LOW_SPEED_TIME, (long)20);
	status.setopt(CURLOPT_CONNECTTIMEOUT, (long)30);
	status.setopt(CURLOPT_NOSIGNAL, 1);
	status.setopt(CURLOPT_HTTPPOST,1);
	status.setopt(CURLOPT_HEADER, 0);
	status.setopt(CURLOPT_COPYPOSTFIELDS, postdata);
	status.setopt(CURLOPT_WRITEFUNCTION, write_data);
	status.setopt(CURLOPT_WRITEDATA, &result);
	status.setopt(CURLOPT_COOKIEFILE, "");
	status.setopt(CURLOPT_URL, url.c_str());
	int res = status.perform();
	status.cleanup();
	if(res != 0) {
		outp.file_online = PLUGIN_CONNECTION_LOST;
		return true;
	}
	try {
		vector<string> answer = split_string(result, ";");
		long size = 0;
		if(answer[1] == "OK")
			size = atoi(answer[3].c_str());
		if(size == 0) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		outp.file_online = PLUGIN_SUCCESS;
		outp.file_size = size;
		outp.download_filename = answer[2];
	} catch(...) {
		outp.file_online = PLUGIN_FILE_NOT_FOUND;
	}
	return true;
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
