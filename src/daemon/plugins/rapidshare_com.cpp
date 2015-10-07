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
#define PLUGIN_WANTS_POST_PROCESSING
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

bool use_premium = true; // if the premium limit is exceeded, this is set to false and we restart the download from the beginning without premium

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	get_handle()->setopt(CURLOPT_SSL_VERIFYPEER, 0);

	string url = get_url();
	string result;
	result.clear();
	if(url.find("/users/")==std::string::npos && url.find("#!linklist|")==std::string::npos)
	{
		log_string("Rapidshare.com: no folder",LOG_DEBUG);
		if(!inp.premium_user.empty() && !inp.premium_password.empty() && use_premium) {
			// string result;
			ddcurl* handle = get_handle();
			outp.allows_multiple = true;
			outp.allows_resumption = true;
			string post_data="sub=getaccountdetails&login=" + ddcurl::escape(inp.premium_user) + "&password=" + ddcurl::escape(inp.premium_password) +"&withpublicid=3&withcookie=1&cbid=1&cbf=rs.jsonp.callback";
			handle->setopt(CURLOPT_URL, string("https://api.rapidshare.com/cgi-bin/rsapi.cgi?" + post_data).c_str());
			handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
			handle->setopt(CURLOPT_WRITEDATA, &result);
			handle->setopt(CURLOPT_COOKIEFILE, "");
			int res = handle->perform();
			if(res == 0) {
				if(result.find("Login failed") != string::npos) {
					return PLUGIN_AUTH_FAIL;
				}

				outp.download_url = get_url();
				// get the handle ready (get cookies)
				handle->setopt(CURLOPT_POST, 0);
                size_t pos = result.find("cookie=");
                if(pos == string::npos) {
                    log_string("rapidshare.com: rapidshare server did not return a cookie", LOG_ERR);
					return PLUGIN_AUTH_FAIL;
                }

				string tmp = result.substr(pos + 7);
                pos = tmp.find_first_of(" \r\n\\r\\n");
                tmp = tmp.substr(0, pos);
				handle->setopt(CURLOPT_COOKIE, string("enc=" + tmp).c_str());
				return PLUGIN_SUCCESS;
			}
			return PLUGIN_ERROR;
		}
		use_premium = true;

		ddcurl* handle = get_handle();

		std::string resultstr;
		string url = get_url();
		vector<string> splitted_url;
		if(url.find("#!download") == string::npos)
			splitted_url = split_string(url, "/");
		else
		{
			// new rapidshare links
			splitted_url = split_string(url, "|");
			splitted_url.pop_back();
		}
		string filename = splitted_url.back();
		string fileid = *(splitted_url.end() - 2);
		string dispatch_url = "https://api.rapidshare.com/cgi-bin/rsapi.cgi?sub=download&fileid=" + fileid + "&filename=" + filename + "&try=1&cbf=RSAPIDispatcher&cbid=1";
		handle->setopt(CURLOPT_URL, dispatch_url);
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &resultstr);
		int success = handle->perform();

		if(success != 0) {
			return PLUGIN_CONNECTION_ERROR;
		}

		vector<string> dispatch_data = split_string(resultstr, ",");
		if(dispatch_data.size() >= 2) {
		// everything seems okay
		size_t waitpos = dispatch_data[1].find("need to wait ");
		if(waitpos != string::npos) {
			waitpos += 13;
			if(waitpos >= dispatch_data[1].size()) return PLUGIN_FILE_NOT_FOUND;
			set_wait_time(atoi(dispatch_data[1].substr(waitpos).c_str()));
			return PLUGIN_LIMIT_REACHED;
		}
		if(dispatch_data[1].find("You need RapidPro to download more files") != string::npos) {
			set_wait_time(30);
			return PLUGIN_LIMIT_REACHED;
		}

		if(dispatch_data[1].find("DL:") == string::npos) return PLUGIN_FILE_NOT_FOUND;

		url = "http://" + dispatch_data[1].substr(dispatch_data[1].find(":") + 1) + "/cgi-bin/rsapi.cgi?sub=download&fileid=" + fileid;
		url += "&filename=" + filename + "&dlauth=" + dispatch_data[2];
		handle->setopt(CURLOPT_URL, url.c_str());
		set_wait_time(atoi(dispatch_data[3].c_str()) + 1);
		outp.download_filename = handle->unescape(filename);
		outp.download_url = url;
		return PLUGIN_SUCCESS;
	}
	return PLUGIN_ERROR;
	}
	else
	{
		string id,result;
		ddcurl* handle = get_handle();
		size_t n = url.find("/users/");
		if(n==std::string::npos)
		{
			n = url.find("/#!linklist|");
			if (n == string::npos)
			return PLUGIN_ERROR;
			n += 12;
			id = url.substr(n);
			log_string("Rapidshare.com: id_linklist=" + id,LOG_DEBUG);
			while(id.find("|")!=string::npos) //because the function does not seems to work with |
			{
				replace_all(id,"|","");
			}
			log_string("Rapidshare.com: id_linklist=" + id,LOG_DEBUG);
		}
		else
		{
			n +=7;
			id = url.substr(n);
			vector<string> splitted_url = split_string(id, "/");
			id = splitted_url[0];
			log_string("Rapidshare.com: id_user=" + id,LOG_DEBUG);
		}
		string newurl = "http://rapidshare.com/cgi-bin/rsapi.cgi?sub=viewlinklist&linklist=" + id + "&cbf=RSAPIDispatcher&cbid=1";
		//log_string("Rapidshare.com: url=" + newurl,LOG_DEBUG);
		handle->setopt(CURLOPT_URL, newurl);
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);
		handle->setopt(CURLOPT_COOKIEFILE, "");
		int res = handle->perform();
		if(res != 0)
		{
			return PLUGIN_CONNECTION_ERROR;
		}
		//rapidshare removed linklists/folders from there API
		if(result.find("Invalid routine called")!=string::npos)
		{
			log_string("Rapidshare removed linklists, please contact rapidshare.com",LOG_WARNING);
			return PLUGIN_ERROR;
		}
		replace_all(result,"\\\"","\"");
		//log_string("Rapidshare.com: result=" + result,LOG_DEBUG);
		vector<string> links = split_string(result, ",");
		download_container urls;
		for(size_t i = 4; i < links.size()-1; i=i+10)
		{
			//log_string("Rapidshare.com: splitted-link =" + links[i],LOG_DEBUG);
			size_t urlpos = links[i].find("\"");
			if(urlpos == string::npos)
			{
				log_string("Rapidshare.com: urlpos is at end of file",LOG_DEBUG);
				return PLUGIN_ERROR;
			}
			urlpos += 1;
			//log_string("Safelinking.net: urlpos=" + int_to_string(urlpos),LOG_DEBUG);
			string temp1 = links[i].substr(urlpos, links[i].find("\"", urlpos) - urlpos);
			string temp2 = links[i+1].substr(urlpos, links[i+1].find("\"", urlpos) - urlpos);
			string temp = "http://rapidshare.com/files/" + temp1 + "/" + temp2;
			log_string("Rapidshare.com: link=" + temp,LOG_DEBUG);
			urls.add_download(temp, "");
		}
		replace_this_download(urls);
		return PLUGIN_SUCCESS;
	}
}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	string url = get_url();
	if(url.find("/users/")!=std::string::npos || url.find("#!linklist|")!=std::string::npos)
		return false;
	string filename, fileid;
	if(url.find("#!download") == string::npos) {
		vector<string> splitted_url = split_string(url, "/");
		if(splitted_url.size() <= 2) return true;
		filename = splitted_url.back();
		fileid = *(splitted_url.end() - 2);
	} else {
		vector<string> splitted_url = split_string(url, "|");
		if(splitted_url.size() < 5) return true;
		filename = *(splitted_url.end() - 2);
		fileid = *(splitted_url.end() - 3);
	}
	url = "http://api.rapidshare.com/cgi-bin/rsapi.cgi?sub=checkfiles&files=" + fileid + "&filenames=" + filename;
	std::string result;
	ddcurl handle;
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
		vector<string> answer = split_string(result, ",");
		long size = 0;
		if(answer.size() >= 3)
			size = atoi(answer[2].c_str());
		if(size == 0) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		outp.file_online = PLUGIN_SUCCESS;
		outp.file_size = size;
		if (filename.empty())
			outp.download_filename = answer[1];
		else
			outp.download_filename = filename;
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

void post_process_download(plugin_input &inp) {
	if(inp.premium_user.empty() || inp.premium_password.empty()) return;
	string filename = dl_list->get_output_file(dlid);
	struct pstat st;
	if(pstat(filename.c_str(), &st) != 0) {
		return;
	}
	if(st.st_size > 1024 * 100 /* 100kb */ ) {
		return;
	}
	ifstream ifs(filename.c_str());
	string tmp;
	char buf[1024];
	while(ifs) {
		ifs.read(buf, 1024);
		tmp.append(buf, ifs.gcount());
	}

	if(tmp.find("You have exceeded the download limit.") != string::npos) {
		use_premium = false;
		dl_list->set_status(dlid, DOWNLOAD_PENDING);
		dl_list->set_error(dlid, PLUGIN_SUCCESS);
		remove(filename.c_str());
		dl_list->set_output_file(dlid, "");
	}
}
