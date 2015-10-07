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
#include <sstream>
using namespace std;

template <class T>
bool from_string(T& t, const std::string& s, std::ios_base& (*f)(std::ios_base&))
{
	std::istringstream iss(s);
	return !(iss >> f >> t).fail();
}


size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	ddcurl* handle = get_handle();
	string url = get_url();
	if(url.find(".multiupload.com:")==string::npos)
	{
		string result;
		result.clear();
		handle->setopt(CURLOPT_HEADER, 1);
		handle->setopt(CURLOPT_URL, url.c_str());
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);
		handle->setopt(CURLOPT_COOKIEFILE, "");
	
		int res = handle->perform();
		if(res != 0)
		{
			log_string("multiupload.com: handle failed! Please check internet connection or contact hd-bb.org",LOG_DEBUG);
			return PLUGIN_CONNECTION_ERROR;
		}
		download_container urls;
		try
		{
			if(result.find("the link you have clicked is not available")!=string::npos || result.find("Invalid link")!=string::npos || 
			result.find("The file has been deleted because it was violating our")!=string::npos || result.find("No htmlCode read")!=string::npos)
			{
				return PLUGIN_FILE_NOT_FOUND;
			}
			if(result.find(">UNKNOWN ERROR<")!=string::npos)
			{
				log_string("multiupload.com: Unknown error caused by multiupload.com",LOG_WARNING);
				return PLUGIN_ERROR;
			}
			if(url.find("_")==string::npos)
			{
				vector<string> links = search_all_between(result,"<div id=\"downloadbutton_","</a></div>");
				for(size_t i=0; i<links.size();i++)
				{
					string directMultiuploadLink = search_between(links[i],"<a href=\"","\"");
					if(directMultiuploadLink.find("mediafire")!=string::npos)
					{
						replace_all(directMultiuploadLink,"mediafire.com?","mediafire.com/?");
					}
					if(validate_url(directMultiuploadLink))
						urls.add_download(directMultiuploadLink,"");
				}
			}
			else
			{
				//just a simple redirect
				log_string("simple redirect!",LOG_DEBUG);
				size_t urlpos = result.find("location:");
				if(urlpos == string::npos)
				{
					log_string("Multiupload.com: urlpos is at end of file",LOG_DEBUG);
					return PLUGIN_ERROR;
				}
				urlpos += 10;
				string newurl = result.substr(urlpos, result.find_first_of("\r\n", urlpos) - urlpos);
				urls.add_download(set_correct_url(newurl),"");
			}		
		}catch(...) {}
		replace_this_download(urls);
		return PLUGIN_SUCCESS;
	}
	else
	{
		//direct download link
		outp.download_url = url;
		return PLUGIN_SUCCESS;
	}
}

bool get_file_status(plugin_input &inp, plugin_output &outp)
{
	string url = inp.url;
	if(url.find(".multiupload.com:")==string::npos)
		return false;
	else
		return true;
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp)
{
	outp.allows_resumption = false;
	outp.allows_multiple = false;

	outp.offers_premium = false;
}



