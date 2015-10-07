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
#include <cctype>
#include <string>
using namespace std;


bool is_number(const std::string& s)
{
	for (size_t i = 0; i < s.length(); i++) {
		if (!std::isdigit(s[i]))
			return false;
	}
	return true;
}

string getId(string url)
{
	vector<string> splitted_url = split_string(url, "/");
	string id;
	if(splitted_url[4][0]=='r')
	{
		id = splitted_url[4] + "-" + splitted_url[5];
	}
	else
	{
		id = splitted_url[4];
	}
		
	/*else if(is_number(splitted_url[5]))
		id = splitted_url[4] + "-" + splitted_url[5];
	else
	{
		log_string("filesonic.com: cannot find id",LOG_DEBUG);
		return "";
	}*/
	return id;
}


size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

string getDomainAPI() 
{
	try 
	{
		ddcurl handle;
		string result;
	        handle.setopt(CURLOPT_FOLLOWLOCATION, 1);
		handle.setopt(CURLOPT_URL, "http://api.filesonic.com/utility?method=getFilesonicDomainForCurrentIp&format=xml");
		handle.setopt(CURLOPT_HTTPPOST, 0);
		handle.setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle.setopt(CURLOPT_WRITEDATA, &result);
		handle.setopt(CURLOPT_COOKIEFILE, "");
		int ret = handle.perform();
		//log_string("filesonic.com: result=" + result, LOG_DEBUG);
		if(ret != CURLE_OK)
			return "";
		string domain = search_between(result,"<response>","</response>");
		//log_string("domain=" + domain,LOG_DEBUG);
		if (!domain.empty()) { return "http://www" + domain; }
        } catch (...) {}
        return "";
}

//Only Premium-User support
plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	ddcurl* handle = get_handle();
	//dcurl* fileexists = get_handle();
	//ddcurl* get_domain = get_handle();
	string url = get_url();
	string result;
	result.clear();
	if(url.find("/folder/")==string::npos)
	{
		if(inp.premium_user.empty() || inp.premium_password.empty()) {
			return PLUGIN_AUTH_FAIL; // free download not supported yet
		}
	
		//encode login data
		string premium_user = handle->escape(inp.premium_user);
		string premium_pwd = handle->escape(inp.premium_password);
		string id = getId(url);
		log_string("filesonic.com: id = " + id, LOG_DEBUG);
		if(id=="")
			return PLUGIN_ERROR;
		url = "http://api.filesonic.com/link?method=getDownloadLink&u=" + premium_user + "&p=" + premium_pwd + "&format=xml&ids=" + id;
		for(int i=0;i<6;i++) //ask 6 times if password!
		{
			handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
			handle->setopt(CURLOPT_URL, url.c_str());
			handle->setopt(CURLOPT_HTTPPOST, 0);
			handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
			handle->setopt(CURLOPT_WRITEDATA, &result);
			handle->setopt(CURLOPT_COOKIEFILE, "");
			int ret = handle->perform();
			log_string("filesonic.com: result=" + result, LOG_DEBUG);
			if(ret != CURLE_OK)
				return PLUGIN_CONNECTION_ERROR;
			if(ret == 0) 
			{
				if (result.find("FSApi_Auth_Exception")!=string::npos)
				{
					return PLUGIN_AUTH_FAIL;
				}
				string status = search_between(result,"<status>","</status");
				if (status == "NOT_AVAILABLE")
				{
					return PLUGIN_FILE_NOT_FOUND;
				}
				if(status == "PASSWORD_REQUIRED" || status == "WRONG_PASSWORD")
				{
					//password protected => ask user for password
					string rslt="http://ghost-zero.webs.com/password.png";
					handle->setopt(CURLOPT_URL, rslt);
					result.clear();
					handle->perform();
					std::string captcha_text = Captcha.process_image(result, "png", "", -1, false, false, captcha::SOLVE_MANUAL);
					url = "http://api.filesonic.com/link?method=getDownloadLink&u=" + premium_user + "&p=" + premium_pwd + "&format=xml&ids=" + id + "&passwords[" + id + "]=" + captcha_text;
				}
				string url = search_between(result,"<url><![CDATA[","]]></url>");
				outp.download_url = url.c_str();
				//log_string("url = " + url,LOG_DEBUG);
				return PLUGIN_SUCCESS;
			}
		}
		return PLUGIN_ERROR;
	}
	else
	{
		download_container urls;
		try
		{
			size_t pos = url.find("filesonic.com");
			pos+=13;
			string id = url.substr(pos);
			string domain = getDomainAPI();
			if(domain=="")
				return PLUGIN_ERROR;
			string url = domain + id;
			log_string("url=" + url,LOG_DEBUG);
			handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
			handle->setopt(CURLOPT_URL, url.c_str());
			handle->setopt(CURLOPT_HTTPPOST, 0);
			handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
			handle->setopt(CURLOPT_WRITEDATA, &result);
			handle->setopt(CURLOPT_COOKIEFILE, "");
			int ret = handle->perform();
			if(ret != CURLE_OK)
				return PLUGIN_CONNECTION_ERROR;
			if(result.find(">No links to show<")!=string::npos || result.find("Folder do not exist<") !=string::npos 
			|| result.find(">The requested folder do not exist or was deleted by the owner") != string::npos
			|| result.find(">If you want, you can contact the owner of the referring site to tell him about this mistake") != string::npos) 
				return PLUGIN_FILE_NOT_FOUND;
			result = search_between(result,"<tbody>","</tbody>");
			//log_string("filesonic.com: result=" + result, LOG_DEBUG);
			vector<string> alink = search_all_between(result,"</span><a href=\"","</a></td>",0,true);
			if(alink.empty())
			{
				return PLUGIN_FILE_NOT_FOUND;
			}
			for(size_t i = 0; i < alink.size(); i++)
			{
				vector<string> split = split_string(alink[i],"\">");
				urls.add_download(split[0],split[1]);
			}
		}catch(...) {return PLUGIN_ERROR;}
		replace_this_download(urls);
		return PLUGIN_SUCCESS;
	}
	
}

bool get_file_status(plugin_input &inp, plugin_output &outp) 
{
	string url = get_url();
	if(url.find("/folder/")!=string::npos)
		return false;
	string id = getId(url);
	if(id=="")
	{
		outp.file_online = PLUGIN_ERROR;
		return false;
	}
	string result;
	url = "http://api.filesonic.com/link?method=getInfo&format=xml&ids=" + id;
	ddcurl handle;
	handle.setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle.setopt(CURLOPT_WRITEDATA, &result);
	handle.setopt(CURLOPT_COOKIEFILE, "");
	handle.setopt(CURLOPT_URL, url.c_str());
	int res = handle.perform();
	handle.cleanup();
	if(res != 0) 
	{
		outp.file_online = PLUGIN_CONNECTION_LOST;
		return true;
	}
	try 
	{
		string status = search_between(result,"<status>","</status>");
		if (status == "NOT_AVAILABLE")
		{
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		long size = 0;
		size = atoi(search_between(result,"<size>","</size>").c_str());
		string filename = search_between(result,"<filename>","</filename>");
		outp.file_online = PLUGIN_SUCCESS;
		outp.file_size = size;
		outp.download_filename = filename;
	}
	catch(...) 
	{
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
