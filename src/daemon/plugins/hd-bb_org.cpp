
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
	if(inp.premium_user.empty() || inp.premium_password.empty())
		return PLUGIN_AUTH_FAIL;
	ddcurl* handle = get_handle();
	string url = get_url();
	string result;
	handle->setopt(CURLOPT_URL, url.c_str());
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &result);
	handle->setopt(CURLOPT_COOKIEFILE, "");

	//login
	string premium_user = handle->escape(inp.premium_user);
	string premium_pwd = handle->escape(inp.premium_password);
	string post="username=" + premium_user + "&password=" + premium_pwd + "&login=Login";
	handle->setopt(CURLOPT_POST, 1);
	handle->setopt(CURLOPT_COPYPOSTFIELDS, post);
	handle->setopt(CURLOPT_FOLLOWLOCATION,1);

	int res = handle->perform();
	if(res != 0)
	{
		log_string("hd-bb.org: handle failed! Please check internet connection or contact hd-bb.org",LOG_DEBUG);
		return PLUGIN_CONNECTION_ERROR;
	}
	download_container urls;
	try
	{
		//perform actual handle
		handle->setopt(CURLOPT_POST, 0);
		handle->setopt(CURLOPT_FOLLOWLOCATION,0);
		result.clear();
		handle->perform();
		//log_string("result=" + result,LOG_DEBUG);
		//trying to find password => experimental
		size_t i;
		if((i = result.find("<dd><code>Pass")) != string::npos || (i=result.find("<dd><code>Code")) != string::npos || (i=result.find("<dd><code>pass")) != string::npos|| (i=result.find("<dd><code>code"))!=string::npos)
		{
			log_string("hd-bb.org: password found on site",LOG_DEBUG);
			i = result.find(" ",i);
			i++;
			string password = result.substr(i,result.find("</code>",i)-i);
			log_string("hd-bb.org: password =" + password,LOG_DEBUG);
			urls.set_password(password);
		}
		vector<string> enco = search_all_between(result, "<script type='text/javascript'>var senc","</div></code>",0,true);
		for(size_t i = 0; i < enco.size(); i++)
		{
			//log_string("enco" + int_to_string(i+1) + "=" + enco[i],LOG_DEBUG);
			//hidden links=> simple bitwise EXOR javascript
			string senc = enco[i].substr(3,enco[i].find("'",3)-3);
			//log_string("senc=" + senc,LOG_DEBUG);
			int key;
			string links;
			for(size_t i=0; i<senc.size();i+=4)
			{
				string temp = senc.substr(i, 4);
				int dec;
				if(from_string<int>(dec, temp, std::hex));
				if(i==0)
				{
					key = dec^'$';
					log_string("hd-bb.org: Found key =" + int_to_string(key),LOG_DEBUG);
				}
				else
				{
					links += dec^key;
				}
			}
			//log_string("links="+links,LOG_DEBUG);
			vector<string> link = split_string(links, "<br />");
			for(size_t i = 0; i < link.size(); i++)
			{
				string temp = set_correct_url(trim_string(link[i]));
				if(validate_url(temp))
					urls.add_download(temp,"");
			}
		}
	}catch(...) {}
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


