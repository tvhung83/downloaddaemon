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
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

void form_opt(const string &result, size_t &pos, const string &var, string &val) {
	pos = result.find(var, pos);
	pos = result.find("value=", pos) + 6;
	val = result.substr(pos, result.find(">", pos) - pos);
	pos += val.size();

}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	string url = get_url();
	//log_string("url = "+url,LOG_DEBUG);
	if(url.find("/list/")==string::npos && url.find("/links/")==string::npos)
	{
		if(inp.premium_user.empty() || inp.premium_password.empty())
			return PLUGIN_AUTH_FAIL;
		ddcurl* handle = get_handle();
		outp.allows_multiple = true;
		outp.allows_resumption = true;
		outp.download_url = inp.url;
		handle->setopt(CURLOPT_USERPWD, inp.premium_user + ":" + inp.premium_password);
		return PLUGIN_SUCCESS;
	}
	else
	{
		log_string("hotfile.com: folder detected",LOG_DEBUG);
		ddcurl* handle = get_handle();
		string result;
		handle->setopt(CURLOPT_URL, url.c_str());
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);
		handle->setopt(CURLOPT_COOKIEFILE, "");
		int res = handle->perform();
		if(res != 0)
		{
			log_string("hotfile.com: handle failed! Please check internet connection or contact hd-bb.org",LOG_DEBUG);
			return PLUGIN_CONNECTION_ERROR;
		}
		download_container urls;
		try
		{
			if(result.find("Empty Directory")!=string::npos)
				return PLUGIN_FILE_NOT_FOUND;
			if(url.find("/list/")!=string::npos)
			{
				string fpname = search_between(result,"-2px;\" />","</td>");
				fpname = trim_string(fpname);
				if(fpname == "")
				{
					fpname = "Hotfile.com folder";
				}
				urls.set_pkg_name(fpname);
				result = search_between(result,"<div id=\"main_content\">","</div>");
				vector<string> alink = search_all_between(result,"<td style=\"","</a></td>",0,true);
				for(size_t i=0;i<alink.size();i++)
				{
					string link = search_between(alink[i],"<a href=\"","\"");
					if(validate_url(link))
						urls.add_download(link,"");
				}
			}
			else
			{
				string finallink = search_between(result,"name=\"url\" id=\"url\" class=\"textfield\" value=\"","\"");
				if(finallink == "")
				{
					finallink = search_between(result,"name=\"forum\" id=\"forum\" class=\"textfield\" value=\"[URL=","]http");
					if(finallink == "")
					{
						finallink = "http://hotfile.com/dl/" + search_between(result,"\"http://hotfile.com/dl/","\"");
					}
				}
				if(finallink == "")
					return PLUGIN_ERROR;
				urls.add_download(finallink, "");
			}
		}catch(...) {return PLUGIN_ERROR;}
		replace_this_download(urls);
		return PLUGIN_SUCCESS;
	}
// Hotfile.com free support is disabled because of recatcha. Only premium works
#if 0
	CURL* handle = get_handle();
	outp.allows_multiple = false;
	outp.allows_resumption = false;

	std::string resultstr;
	string url = get_url();
	url += "&lang=en";
	curl_easy_setopt(handle, CURLOPT_URL, get_url());
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &resultstr);
	int success = curl_easy_perform(handle);

	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}

	if(resultstr.find("This file is either removed") != std::string::npos || resultstr.find("404 - Not Found") != std::string::npos) {
		return PLUGIN_FILE_NOT_FOUND;
	}

	size_t pos;
	if((pos = resultstr.find("function starthtimer(){")) == std::string::npos) {
		return PLUGIN_ERROR;
	}
	pos = resultstr.find("d.getTime()+", pos);
	pos += 12;
	string wait_t = resultstr.substr(pos, resultstr.find(";", pos) - pos);
	int wait = atoi(wait_t.c_str());
	wait /= 1000;
	if(wait > 0) {
		set_wait_time(wait);
		return PLUGIN_LIMIT_REACHED;
	}

	set_wait_time(60);

	pos = resultstr.find("action=\"", pos);
	pos += 8;
	size_t end = resultstr.find('\"', pos);

	url = "http://hotfile.com" + resultstr.substr(pos, end - pos);
	url += "&lang=en";

	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(handle, CURLOPT_POST, 1);
	//curl_easy_setopt(prepare_handle, CURLOPT_COPYPOSTFIELDS, "dl.start=\"Free\"");
	string action, tm, tmhash, wait_var, waithash, upidhash;
	form_opt(resultstr, pos, "action", action);
	form_opt(resultstr, pos, "tm", tm);
	form_opt(resultstr, pos, "tmhash", tmhash);
	form_opt(resultstr, pos, "wait", wait_var);
	form_opt(resultstr, pos, "waithash", waithash);
	form_opt(resultstr, pos, "upidhash", upidhash);
	string post_data = "action=" + action + "&tm=" + tm + "&tmhash=" + tmhash + "&wait=" + wait_var + "&waithash=" + waithash + "&upidhash=" + upidhash;
	curl_easy_setopt(handle, CURLOPT_COPYPOSTFIELDS, post_data.c_str());

	resultstr.clear();

	int to_wait = 60;
	while(to_wait > 0) {
		set_wait_time(to_wait--);
		sleep(1);
	}

	curl_easy_perform(handle);
	curl_easy_setopt(handle, CURLOPT_POST, 0);
	curl_easy_setopt(handle, CURLOPT_COPYPOSTFIELDS, "");

	pos = resultstr.find("<td><a href=\"");
	if(pos == string::npos) {
		return PLUGIN_ERROR;
	}
	pos += 13;
	end = resultstr.find("\"", pos);
	outp.download_url = resultstr.substr(pos, end - pos);

	return PLUGIN_SUCCESS;
#endif
}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	std::string url = get_url();
	url += "&lang=en";
	std::string result;
	ddcurl handle(true);
	handle.setopt(CURLOPT_LOW_SPEED_LIMIT, (long)10);
	handle.setopt(CURLOPT_LOW_SPEED_TIME, (long)20);
	handle.setopt(CURLOPT_CONNECTTIMEOUT, (long)30);
	handle.setopt(CURLOPT_NOSIGNAL, 1);
	handle.setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle.setopt(CURLOPT_WRITEDATA, &result);
	handle.setopt(CURLOPT_COOKIEFILE, "");
	handle.setopt(CURLOPT_URL, url.c_str());
	int res = handle.perform();
	handle.cleanup();

	if(res != 0) {
		outp.file_online = PLUGIN_CONNECTION_LOST;
		return true;
	}
	try {
		size_t n = result.find("Downloading:");
		if(n == string::npos) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		n = result.find("<strong>", n);
		if(n == string::npos) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		n += 8;
		size_t end = result.find(" ", n);
		long size = atol(result.substr(n, end - n).c_str());
		size *= 1024 * 1024; // yes, rapidshare uses this factor.
		outp.file_online = PLUGIN_SUCCESS;
		outp.file_size = (long)size;
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

