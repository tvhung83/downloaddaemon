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
#include <fstream>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	string url = get_url();
	string result;
	ddcurl* handle = get_handle();
	handle->setopt(CURLOPT_URL, url.c_str());
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &result);
	handle->setopt(CURLOPT_FOLLOWLOCATION,1);
	handle->setopt(CURLOPT_POST, 0);
	int ret = handle->perform();
	if(ret != CURLE_OK)
		return PLUGIN_CONNECTION_ERROR;
	if(ret == CURLE_OK) 
	{
		string alias_id = search_between(result,"'","';",result.find("var __alias_id"));
		string type = search_between(result, "ticket.makeUrl( '","',",result.find("request:function(){"));
		string extra = search_between(result,"ticket.makeUrl( '" + type + "', '","'",result.find("request:function(){"));
		string add = search_between(result,"var kIjs09 = ",";</script>");
		string cmd_url = search_between(result,"var url = __base_url",";");
		string current_url = search_between(result,"'","';",result.find("var __current_url"));
		string file_key = search_between(result,"'","';",result.find("var __file_key"));
		vector<string> cur_split = split_string(current_url,"'");
		current_url = cur_split[0] + file_key + cur_split[2];
		log_string("current_url=" + current_url,LOG_DEBUG);
		vector<string> splitted_url = split_string(cmd_url,"\"");
		vector<string> temp = split_string(splitted_url[8],"+");
		string finalurl = "http://ifile.it/" + splitted_url[1] + alias_id + splitted_url[3] + type + splitted_url[5] + add + splitted_url[7] + trim_string(temp[1]) + extra;
		log_string("finallink = " + finalurl,LOG_DEBUG);
		handle->setopt(CURLOPT_URL, finalurl.c_str());
		result.clear();
		int ret = handle->perform();		
		if(ret != CURLE_OK)
			return PLUGIN_CONNECTION_ERROR;
		//log_string("result=" + result,LOG_DEBUG);
		if(result.find("status\":\"ok\"")==string::npos)
			return PLUGIN_ERROR;
		if(result.find("\"captcha\":1")!=string::npos)
		{
			//captcha not yet supported!
			log_string("captcha not supported yet",LOG_DEBUG);
			set_wait_time(600);
			return PLUGIN_LIMIT_REACHED;
		}
		if(result.find("an error has occured while processing your request")!=string::npos)
		{
			return PLUGIN_ERROR;
		}
		for(int i=0;i<2;i++)
		{
			handle->setopt(CURLOPT_URL, current_url.c_str());
			result.clear();
			ret = handle->perform();
			if(ret != CURLE_OK)
				return PLUGIN_CONNECTION_ERROR;
		}
		//log_string("result=" + result,LOG_DEBUG);
		string download_url = search_between(result,"<a target=\"_blank\" href=\"","\"");
		log_string("downloadurl =" + download_url,LOG_DEBUG);
		handle->setopt(CURLOPT_FOLLOWLOCATION,0);
		outp.download_url = download_url;
		return PLUGIN_SUCCESS;
		
	}
	return PLUGIN_ERROR;
}	

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	outp.allows_resumption = true;
	outp.allows_multiple = true;
}

