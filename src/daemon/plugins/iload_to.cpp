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
	string url = get_url();
	download_container urls;

	if (url.find("/go/") != std::string::npos) {
		handle->setopt(CURLOPT_HEADER, 1);
		handle->setopt(CURLOPT_URL, url);
		handle->setopt(CURLOPT_FOLLOWLOCATION,1);
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);
		res = handle->perform();
		if(res != 0) {
			log_string("iload.to: could not receive forward url: " + url,LOG_DEBUG);
			return PLUGIN_CONNECTION_ERROR;
		}

		string fileid;
		if (url.find("merged") != std::string::npos) 
			fileid = result.substr(result.find("Location:") + 10, result.size());
		else
			fileid = result.substr(result.find("Location:", result.find("Location:") + 10) + 10, result.size());

		fileid = fileid.substr(0, fileid.find("HTTP")-4);
		if (url.find("merged") != std::string::npos)
			url = fileid;
		else
			url = "http://iload.to" + fileid;

		log_string("iload.to: url which contains captcha: " + url, LOG_DEBUG);
	}
	result.clear();
	handle->setopt(CURLOPT_URL, url);
	handle->setopt(CURLOPT_HEADER, 1);
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &result);
	handle->setopt(CURLOPT_FOLLOWLOCATION, 0);
	handle->setopt(CURLOPT_COOKIEFILE, "");
	res = handle->perform();
	log_string("iload.to: trying to decrypt " + url,LOG_DEBUG);
	if(res != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}

	while((pos = result.find("captcha")) != string::npos) {
		string value = search_between(result, "<img src=\"","\"", result.find("CaptchaForm"));
		string captchaAddress = "http://iload.to" + value;
		handle->setopt(CURLOPT_HEADER, 0);
		handle->setopt(CURLOPT_FOLLOWLOCATION, 0);
		handle->setopt(CURLOPT_POST, 0);
		handle->setopt(CURLOPT_URL, captchaAddress);
		result.clear();
		handle->perform();
		std::string captcha_text = Captcha.process_image(result, "jpg", "", -1, false, false, captcha::SOLVE_MANUAL);
		string postdata = "captcha=" + captcha_text;
		result.clear();
		handle->setopt(CURLOPT_URL, url);
		handle->setopt(CURLOPT_HEADER, 1);
		handle->setopt(CURLOPT_FOLLOWLOCATION, 0);
		handle->setopt(CURLOPT_POST, 1);
		handle->setopt(CURLOPT_COPYPOSTFIELDS, postdata);
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);
		res = handle->perform();

		if(res != 0) {
			return PLUGIN_CONNECTION_ERROR;
		}
	}
	string temp = search_between(result, "<div class=\"links link-stream-files\">", "</div>");
	vector<string> links = search_all_between(temp, "href=\"", "\" style=", 0, true);

	for(size_t i = 0; i < links.size(); i++) {
		links[i] = handle->unescape(links[i]);
		if (links[i].find("?http") != std::string::npos)
			links[i] = links[i].substr(links[i].find("?")+1,links[i].size());
		urls.add_download(links[i], "");
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
