/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define PLUGIN_WANTS_POST_PROCESSING
#define PLUGIN_CAN_PRECHECK
#include "plugin_helpers.h"
#include <curl/curl.h>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	ddcurl* handle = get_handle();
	string result;
	bool done = false;
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &result);
	handle->setopt(CURLOPT_COOKIEFILE, "");
	handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
	string urlprefix;
	if(inp.url.find("http://www.") != string::npos)
		urlprefix = "http://www.netload.in/";
	else
		urlprefix = "http://netload.in/";

	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		handle->setopt(CURLOPT_URL, "http://netload.in/index.php?lang=en");
		string post_data = "txtuser=" + inp.premium_user + "&txtpass=" + inp.premium_password + "&txtcheck=login&txtlogin=";
		handle->setopt(CURLOPT_POST, 1);
		handle->setopt(CURLOPT_COPYPOSTFIELDS, post_data.c_str());
		if(handle->perform() != 0) {
		return PLUGIN_ERROR;
		}
		if(result.find("forgot the password?") != string::npos || result.find("you didn't insert a password or it might be invalid") != string::npos) {
			return PLUGIN_AUTH_FAIL;
		}
		handle->setopt(CURLOPT_POST, 0);
		handle->setopt(CURLOPT_COPYPOSTFIELDS, "");
		outp.download_url = get_url();
		return PLUGIN_SUCCESS;
	}

	// so we can never get in an infinite loop..
	int while_tries = 0;
	while(!done && while_tries < 100) {
		++while_tries;
		handle->setopt(CURLOPT_URL, get_url());
		result.clear();
		int res = handle->perform();
		if(res != 0) {
			return PLUGIN_CONNECTION_ERROR;
		}

		if(result.find("dl_first_filename") == string::npos) {
			return PLUGIN_FILE_NOT_FOUND;
		}

		string captcha_url;
		try {
			size_t n = result.find("<div class=\"dl_first_filename\">") + 31;
			outp.download_filename = result.substr(n, result.find("<span style=\"color:", n) - n);
			trim_string(outp.download_filename);
			replace_html_special_chars(outp.download_filename);
			if((n = result.find("<a class=\"download_fast_link\" href=\"")) != string::npos) {
				// in swizerland, you don't have to enter a captcha or something like that.
				// it's as easy as this if you download from there. No idea why.
				n += 36;
				outp.download_url = urlprefix + result.substr(n, result.find("\"", n) - n);
				replace_html_special_chars(outp.download_url);
				return PLUGIN_SUCCESS;
			}

			n = result.find("<div class=\"Free_dl\"><a href=\"") + 30;
			string url = urlprefix + result.substr(n, result.find("\">", n) - n);
			replace_html_special_chars(url);
			handle->setopt(CURLOPT_URL, url.c_str());
			result.clear();
			res = handle->perform();
			if(res != 0) {
				return PLUGIN_ERROR;
			}

			n = result.find("share/includes/captcha.php?t=");
			captcha_url = urlprefix + result.substr(n, result.find("\"", n) - n);

			n = result.find("<div id=\"downloadDiv\">");
			n = result.find("action=\"", n) + 8;
			url = urlprefix + result.substr(n, result.find("\">", n) - n);
			string post_data;
			n = result.find("file_id", n);
			n = result.find("value=\"", n) + 7;
			post_data = "file_id=" + result.substr(n, result.find("\"", n) - n);

			handle->setopt(CURLOPT_URL, captcha_url.c_str());
			result.clear();
			res = handle->perform();
			if(res != 0) {
				return PLUGIN_ERROR;
			}

			std::string captcha_text = Captcha.process_image(result, "png", "-m 2 -a 20 -C 0-9", 4, true, false, captcha::SOLVE_AUTOMATIC);
			log_string("netload: Solved captcha: " + captcha_text, LOG_DEBUG);
			if(captcha_text.empty()) continue;
			post_data += "&captcha_check=" + captcha_text + "&start=";

			handle->setopt(CURLOPT_URL, url.c_str());
			handle->setopt(CURLOPT_POST, 1);
			handle->setopt(CURLOPT_COPYPOSTFIELDS, post_data.c_str());
			result.clear();
			res = handle->perform();
			handle->setopt(CURLOPT_POST, 0);

			if(res != 0) {
				return PLUGIN_ERROR;
			}

			if(result.find("You may forgot the security code or it might be wrong") != string::npos) {
				log_string("netload: The entered captcha was incorrect", LOG_DEBUG);
				continue;
			}

			if(result.find("file is currently unavailable") != string::npos || result.find("offline") != string::npos || result.find("unknown_file_id") != string::npos) {
				return PLUGIN_FILE_NOT_FOUND;
			}

			if((n = result.find("You can download the next file in")) != string::npos) {
				n = result.find("countdown(", n) + 10;
				int wait_secs = atoi(result.substr(n, result.find(",", n) - n).c_str()) / 100;
				set_wait_time(wait_secs);
				return PLUGIN_LIMIT_REACHED;
			}
			n = result.find("Your download will start automatically.");
			n = result.find("href=\"", n);

			if(n == string::npos) continue;
			n += 6;
			outp.download_url = result.substr(n, result.find("\"", n) - n);

			set_wait_time(20);
			done = true;

		} catch(std::exception &e) {
			return PLUGIN_ERROR;
		}
	}
	return PLUGIN_SUCCESS;
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
	handle.setopt(CURLOPT_URL, url.c_str());
	handle.setopt(CURLOPT_FOLLOWLOCATION, true);
	int res = handle.perform();
	handle.cleanup();
	if(res != 0) {
		outp.file_online = PLUGIN_CONNECTION_LOST;
		return true;
	}
	try {
		size_t n;
		if((n = result.find("dl_first_filename")) == string::npos) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		n = result.find(">, ", n) + 3;
		size_t end = n;
		while(!isspace(result[++end]));
		string value = result.substr(n, end - n);
		n = end + 1;
		end = result.find("<", n);
		string unit = result.substr(n, end - n);
		trim_string(value);
		trim_string(unit);
		const char * oldlocale = setlocale(LC_NUMERIC, "C");
		double value_d = strtod(value.c_str(), NULL);
		setlocale(LC_NUMERIC, oldlocale);
		if(unit == "MB")
			value_d *= 1024*1024;
		else if(unit == "KB")
			value_d *= 1024;

		outp.file_size = value_d;

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
	outp.offers_premium = true; // no login support yet
}


void post_process_download(plugin_input &inp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		return;
	} else {
		string filename = dl_list->get_output_file(dlid);
		struct pstat st;
		if(pstat(filename.c_str(), &st) != 0) {
			return;
		}
		if(st.st_size > 1024 * 1024 /* 1 mb */ ) {
			return;
		}
		ifstream ifs(filename.c_str());
		string tmp;
		size_t n = 0;
		while(getline(ifs, tmp)) {
			if((n = tmp.find("You could download your next file in")) != string::npos) {
				n = tmp.find("countdown(", n) + 10;
				int wait_secs = atoi(tmp.substr(n, tmp.find(",", n) - n).c_str()) / 100;
				set_wait_time(wait_secs);
				dl_list->set_status(dlid, DOWNLOAD_WAITING);
				dl_list->set_error(dlid, PLUGIN_SERVER_OVERLOADED);
				break;
			}
		}

	}
}
