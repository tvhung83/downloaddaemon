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
	if(url.find("/list/")==string::npos)
	{
		if(inp.premium_user.empty() || inp.premium_password.empty()) {
			return PLUGIN_AUTH_FAIL; // free download not supported yet
		}
		string result;
		ddcurl* handle = get_handle();
		string data = "loginUserName=" + handle->escape(inp.premium_user) + "&loginUserPassword=" + handle->escape(inp.premium_password) +
				  "&autoLogin=on&recaptcha_response_field=&recaptcha_challenge_field=&recaptcha_shortencode_field=&loginFormSubmit=Login";
		handle->setopt(CURLOPT_URL, "http://fileserve.com/login.php");
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);
		handle->setopt(CURLOPT_COPYPOSTFIELDS, data.c_str());

		int ret = handle->perform();
		if(ret != CURLE_OK)
			return PLUGIN_CONNECTION_ERROR;
		if(ret == CURLE_OK) {
			if(result.find("Invalid login.") != string::npos) {
				return PLUGIN_AUTH_FAIL;
			}
		}

		outp.download_url = get_url();
		return PLUGIN_SUCCESS;
	}
	else
	{
		string result;
		ddcurl* handle = get_handle();
		handle->setopt(CURLOPT_URL, get_url());
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);

		int ret = handle->perform();
		if(ret != CURLE_OK)
			return PLUGIN_CONNECTION_ERROR;
		if(ret == CURLE_OK) 
		{
			download_container urls;
			try
			{
				if(result.find(">Total file size: 0 Bytes<") != string::npos || result.find("Fileserve - 404 - Page not found") != string::npos || 
				result.find("<h4>The file or page you tried to access is no longer accessible") != string::npos) 
				{
					return PLUGIN_FILE_NOT_FOUND;
				}
				string fpname = search_between(result,"<h1>Viewing public folder ","</h1>");
				result = search_between(result,"<td class=\"m35\">","</table>");
				vector<string> alink = search_all_between(result,"<a href=\"","\" class=\"sheet_icon");
				if(alink.empty())
				{
					return PLUGIN_FILE_NOT_FOUND;
				}
				//urls.add_download("http://fileserve.com" + alink,"");	
				urls.set_pkg_name(trim_string(fpname));
				for(size_t i = 0; i < alink.size(); i++)
				{
					//log_string("alink = " + alink[i],LOG_DEBUG);
					urls.add_download("http://fileserve.com" + alink[i],"");
				}
			}
			catch(...) {return PLUGIN_ERROR;}
			replace_this_download(urls);
			return PLUGIN_SUCCESS;
		}
	}
	return PLUGIN_CONNECTION_ERROR;
}	

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	if(inp.url.find("/list/")!=string::npos)
		return false;
	ddcurl handle;
	string result;
	string post = "submit=Check+Urls&urls=" + inp.url;
	handle.setopt(CURLOPT_URL, "http://fileserve.com/link-checker.php");
	handle.setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle.setopt(CURLOPT_WRITEDATA, &result);
	handle.setopt(CURLOPT_COPYPOSTFIELDS, post.c_str());
	handle.setopt(CURLOPT_POST, 1);
	int ret = handle.perform();
	if(ret != CURLE_OK) return true;
	result = search_between(result, "<div class=\"link_checker\">", "</div>");
	result = search_between(result, "<table>", "</table>");
	if(result.empty()) {
		outp.file_online = PLUGIN_FILE_NOT_FOUND;
		return true;
	}
	try {
		vector<string> filenames = search_all_between(result, "<tr>","</tr>",0,true);
		if(filenames[1].find("fileserve.com/file/")!=string::npos)
		{
			vector<string> splitted = search_all_between(filenames[1],"<td>","</td>",0,true);
			outp.download_filename = trim_string(splitted[1]);
			string size_total = trim_string(splitted[2]);
			vector<string> size_split = split_string(size_total, " ");
			filesize_t size = string_to_long(size_split[0]);
			if(size_split[1] == "MB")
				size *= 1024*1024;
			else if(size_split[1] == "KB")
				size *= 1024;
			else if(size_split[1] == "GB")
				size *= 1024*1024*1024;
			outp.file_size = size;
			if(splitted[3]!="Available&nbsp;<img src=\"/images/green_alerts.jpg\"/>" || trim_string(splitted[1]) == "--" || size_total == "--" )
				outp.file_online = PLUGIN_FILE_NOT_FOUND;
		}		
	} catch(...) {
		outp.file_online = PLUGIN_FILE_NOT_FOUND;
		return true;
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
