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
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		std::string result;
		ddcurl* handle = get_handle();

		string url = get_url();
		vector<string> splitted_url = split_string(url, "/");
		url = url.substr(0, url.find("?"));
                //url += "?setlang=en";

                std::string api_data = "apikey=lhF2IeeprweDfu9ccWlxXVVypA5nA3EL&id_0=" + splitted_url[4];

                handle->setopt(CURLOPT_URL, "http://uploaded.net/api/filemultiple");
		handle->setopt(CURLOPT_POST, 1);
		handle->setopt(CURLOPT_COPYPOSTFIELDS, api_data.c_str());
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);
		if(handle->perform()) {
			return PLUGIN_CONNECTION_ERROR;
		}
		if (result.find("offline") != std::string::npos)
			return PLUGIN_FILE_NOT_FOUND;

		string filename = result.substr(result.find_last_of(",")+1,result.size());
		filename = filename.substr(0,filename.size()-1);
		outp.download_filename = filename;

		result.clear();
		std::string post_data = "id=" + inp.premium_user + "&pw=" + inp.premium_password;
                handle->setopt(CURLOPT_URL, "http://uploaded.net/io/login");
                handle->setopt(CURLOPT_REFERER, "http://uploaded.net");
		handle->setopt(CURLOPT_POST, 1);
		handle->setopt(CURLOPT_COPYPOSTFIELDS, post_data.c_str());
		handle->setopt(CURLOPT_HEADER, 1);
		handle->setopt(CURLOPT_FOLLOWLOCATION, 0);
		handle->setopt(CURLOPT_COOKIEFILE, "");
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);
		handle->perform();
		handle->setopt(CURLOPT_POST, 0);
		handle->setopt(CURLOPT_COPYPOSTFIELDS, "");

		if(result.find("User and password do not match!") != string::npos) {
			return PLUGIN_AUTH_FAIL;
		}
                result.clear();
                handle->setopt(CURLOPT_POST, 0);
                handle->setopt(CURLOPT_HEADER, 0);

		outp.download_url = url;
		return PLUGIN_SUCCESS;
	}

	// ul.to moved to recatcha :( can't solve this with gocr, so the free-download code is disabled for now.
	return PLUGIN_AUTH_FAIL;
/*
	ddcurl* handle = get_handle();
	std::string resultstr;
	handle->setopt(CURLOPT_COOKIEFILE, "");
	handle->setopt(CURLOPT_URL, get_url());
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &resultstr);
	handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
	int success = handle->perform();

	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}

	if(resultstr.find("File doesn't exist") != std::string::npos || resultstr.find("404 Not Found") != string::npos ||
	   resultstr.find("The file status can only be queried by premium users") != string::npos) {
		return PLUGIN_FILE_NOT_FOUND;
	}

	size_t pos;
	if((pos = resultstr.find("You have reached a maximum number of downloads or used up your Free-Traffic!")) != std::string::npos) {
		pos = resultstr.find("(Or wait ", pos);
		pos += 9;
		size_t end = resultstr.find(' ', pos);
		std::string wait_time = resultstr.substr(pos, end - pos);
		set_wait_time(atoi(wait_time.c_str()) * 60);
		return PLUGIN_LIMIT_REACHED;
	}

	if(resultstr.find("Just <b>1</b> download is possible at the same time for free user") != string::npos) {
		set_wait_time(20);
		return PLUGIN_ERROR;
	}

	if((pos = resultstr.find("name=\"download_form\"")) == std::string::npos || resultstr.find("Filename:") == std::string::npos) {
		return PLUGIN_ERROR;
	}

	pos = resultstr.find("http://", pos);
	size_t end = resultstr.find('\"', pos);
	outp.download_url = resultstr.substr(pos, end - pos);

	std::string filename;
	pos = resultstr.find("Filename:");
	pos = resultstr.find("<b>", pos) + 3;
	end = resultstr.find("</b>", pos);
	filename = resultstr.substr(pos, end - pos);
	trim_string(filename);
	std::string filetype;
	pos = resultstr.find("Filetype:");
	pos = resultstr.find("<td>", pos) + 4;
	end = resultstr.find("</td>", pos);
	filetype = resultstr.substr(pos, end - pos);
	trim_string(filetype);
	filename += filetype;
	outp.download_filename = filename;
	return PLUGIN_SUCCESS;
*/
}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	std::string url = get_url();
	std::string result;
	ddcurl handle;
	handle.setopt(CURLOPT_LOW_SPEED_LIMIT, (long)10);
	handle.setopt(CURLOPT_LOW_SPEED_TIME, (long)20);
	handle.setopt(CURLOPT_CONNECTTIMEOUT, (long)30);
	handle.setopt(CURLOPT_NOSIGNAL, 1);
	handle.setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle.setopt(CURLOPT_WRITEDATA, &result);
	handle.setopt(CURLOPT_COOKIEFILE, "");
	handle.setopt(CURLOPT_FOLLOWLOCATION, true);
	handle.setopt(CURLOPT_URL, url.c_str());
	int res = handle.perform();
	handle.cleanup();
	if(res != 0) {
		outp.file_online = PLUGIN_CONNECTION_LOST;
		return true;
	}
	try {
		size_t n = result.find(">Filesize:");
		if(n == string::npos) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		n = result.find("<td>", n) + 4;
		size_t end = result.find("</td>", n) - 1;
		string size_str = result.substr(n, end - n);
		trim_string(size_str);

		const char * oldlocale = setlocale(LC_NUMERIC, "C");

		size_t size = 0;
		if(size_str.find("KB") != string::npos)
			size = strtod(size_str.c_str(), NULL) * 1024;
		else if(size_str.find("MB") != string::npos)
			size = strtod(size_str.c_str(), NULL) * (1024*1024);

		setlocale(LC_NUMERIC, oldlocale);

		outp.file_size = size;
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
