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
	if(url.find("/shows/")!=string::npos)
	{
		vector<string> splitted_url = split_string(url,"/");
		if(splitted_url[0]=="http:")
		{
			splitted_url.erase(splitted_url.begin(),splitted_url.begin()+2);
		}
		string apikey = "ED1FFEEA14679B0D";
		string fronturl = "http://api.bierdopje.com";
		string newurl = fronturl + "/" + apikey + "/" + "GetShowByName" + "/" + splitted_url[2] + "/true";
		string result;
		handle->setopt(CURLOPT_URL, newurl.c_str());
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);
		handle->setopt(CURLOPT_COOKIEFILE, "");
		int res = handle->perform();
		if(res != 0)
		{
			log_string("bierdopje.com: handle failed! Please check internet connection or contact hd-bb.org",LOG_DEBUG);
			return PLUGIN_CONNECTION_ERROR;
		}
		string status = search_between(result,"<status>","</status>");
		if(status=="true")
		{
			download_container urls;
			string id = search_between(result,"<showid>","</showid>");
			int seasons = string_to_int(search_between(result,"<seasons>","</seasons>"));
			int season = 0;
			if(url.find("/season/")!=string::npos)
			{
				season = string_to_int(splitted_url[5]);
				seasons = season;
			}
			log_string("seasons = " + int_to_string(seasons),LOG_DEBUG);
			for(int i=season;i<=seasons;i++)
			{
				newurl = fronturl + "/" + apikey + "/" + "GetSubsForSeason" + "/" + id + "/" + int_to_string(i);
				//log_string("newurl=" + newurl,LOG_DEBUG);
				handle->setopt(CURLOPT_URL, newurl.c_str());
				result.clear();
				handle->perform();
				status=search_between(result,"<status>","</status>");
				//log_string("status=" + status,LOG_DEBUG);
				//log_string("result=" + result,LOG_DEBUG);
				if(status=="true")
				{
					log_string("jeej",LOG_DEBUG);
					vector<string> links = search_all_between(result, "<downloadlink>","</downloadlink>",0,true);
					for(size_t j=0;j<links.size();j++)
					{
						log_string("link " + int_to_string(j) + ": " + links[j],LOG_DEBUG);
						urls.add_download(links[j],"");
					}
				}
			}
			replace_this_download(urls);
			return PLUGIN_SUCCESS;
		}
		else
		{
			return PLUGIN_ERROR;
		}
	}
	else
	{
		outp.download_url=url;
		return PLUGIN_SUCCESS;
	}
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp)
{
	outp.allows_resumption = false;
	outp.allows_multiple = false;

	outp.offers_premium = false;
}
