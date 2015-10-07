/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "captcha.h"


#include <string>
#include <fstream>
#include <cstring>
#include <iostream>
#include "../global.h"


#include <sys/stat.h>
using namespace std;

std::string captcha::process_image(const string &img_data, const string &img_type, const string &gocr_options,
								   int required_chars, bool use_db, bool keep_whitespaces, captcha::solve_type solver) {
	std::unique_lock<std::recursive_mutex> lock(mx);
	set_imgtype(img_type);
	image = img_data;
	struct pstat st;
	captcha_exception e;
	if(solver == captcha::SOLVE_AUTOMATIC && !gocr.empty() && pstat(gocr.c_str(), &st) == 0) {
		if(retry_count > max_retrys) {
			e.set_problem(captcha_exception::AUTOSOLVE_MAXTRIES);
			throw e;
		}
		string img_fn = "/tmp/captcha_" + host + "." + img_type;
		ofstream ofs(img_fn.c_str());
		ofs << image;
		ofs.close();
		string to_exec = gocr + " " + gocr_options;
		if(use_db) {
			to_exec += " -p " + share_directory + "/plugins/captchadb/" + host;
			#ifdef __CYGWIN__
			to_exec += "\\\\";
			#else
			to_exec += "/";
			#endif
		}
		to_exec += " " + img_fn + " 2> /dev/null";
		log_string("captcha: " + to_exec, LOG_DEBUG);
		FILE* cap_result = popen(to_exec.c_str(), "r");
		if(cap_result == NULL) {
			captcha_exception e;
			throw e;
		}

		char cap_res_string[256];
		memset(cap_res_string, 0, 256);
		fgets(cap_res_string, 256, cap_result);

		pclose(cap_result);
		remove(img_fn.c_str());

		string final = cap_res_string;
		if(!keep_whitespaces) {
			for(size_t i = 0; i < final.size(); ++i) {
				if(isspace(final[i])) {
					final.erase(i, 1);
					--i;
				}
			}
		}

		if(required_chars == -1) {
			++retry_count;
		} else {
			final = final.substr(0, required_chars);
			if(final.size() == (size_t)required_chars) {
				++retry_count;
			} else {
				final = "";
			}

		}
		return final;
	} else {
		if(retry_count > max_retrys) {
			e.set_problem(captcha_exception::MANUALSOLVE_MAXTRIES);
			throw e;
		}
		if(!global_config.get_bool_value("captcha_manual")) {
			e.set_problem(captcha_exception::MANUALSOLVE_DISABLED);
			throw e;
		}
		int pkg_id = global_download_list.pkg_that_contains_download(dlid);
		dlindex dl(pkg_id, dlid);
		string res;
		lock.unlock();
		if(!global_download_list.solve_captcha(dl, *this, res)) {
			e.set_problem(captcha_exception::MANUALSOLVE_TIMEOUT);
			throw e;
		} else {
			return res;
		}
	}
}

std::string captcha::get_image() const {
	std::unique_lock<std::recursive_mutex> lock(mx);
	return image;
}

std::string captcha::get_imgtype() const {
	std::unique_lock<std::recursive_mutex> lock(mx);
	return img_type;
}

void captcha::set_image(const std::string &img) {
	std::unique_lock<std::recursive_mutex> lock(mx);
	image = img;
}

void captcha::set_imgtype(const std::string &type) {
	std::unique_lock<std::recursive_mutex> lock(mx);
	img_type = type;
}

void captcha::set_result(const std::string &res) {
	std::unique_lock<std::recursive_mutex> lock(mx);
	result = res;
}

std::string captcha::get_result() const {
	std::unique_lock<std::recursive_mutex> lock(mx);
	return result;
}

void captcha::set_question(const std::string &q) {
	std::unique_lock<std::recursive_mutex> lock(mx);
	question = q;
}

std::string captcha::get_question() const {
	std::unique_lock<std::recursive_mutex> lock(mx);
	return question;
}

void captcha::setup(const std::string &gocr, int max_retrys, const std::string &data_dir, int dlid, const std::string &host) {
	std::unique_lock<std::recursive_mutex> lock(mx);
	this->gocr = gocr;
	this->max_retrys = max_retrys;
	this->dlid = dlid;
	this->share_directory = data_dir;
	this->host = host;
	this->retry_count = 0;
}
