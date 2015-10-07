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

	// Seite einlesen und in resultstr speichern
	handle->setopt(CURLOPT_URL, url.c_str());
	handle->setopt(CURLOPT_COOKIEFILE, "");
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &resultstr);
	int success = handle->perform();
	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}
	size_t waitpos = resultstr.find("You have download 1 File in the last");
	string waittime = search_between(resultstr, "Please wait ", " miniuts");
	if(waitpos != string::npos) {
		set_wait_time(atoi(waittime.c_str())*60);
		return PLUGIN_LIMIT_REACHED;
	 }


	//get the post-data/real link
	string fileID = search_between(resultstr, "name=\"fileID\" value=\"", "\"");
	string dlSession = handle->escape(search_between(resultstr, "name=\"dlSession\" value=\"","\""));
	string userID = search_between(resultstr, "name=\"userID\" value=\"","\"");
	string password = search_between(resultstr, "name=\"password\" value=\"","\"");
	string lang = search_between(resultstr, "name=\"lang\" value=\"","\"");
	string postdata ="fileID=" + fileID + "&dlSession=" + dlSession + "&userID=" + userID + "&password=" + password + "&lang=" + lang;
	url = search_between(resultstr, "id=\"download\" action=\"", "\"");


	set_wait_time(60);
	resultstr.clear();
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
		outp.allows_multiple = false;
	}
	outp.offers_premium = false;
}
