/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef PLUGIN_HELPERS_H_INCLUDED
#define PLUGIN_HELPERS_H_INCLUDED
#define IS_PLUGIN
#include "captcha.h"
#include <sstream>
#include <config.h>
#include "../dl/download.h"
#include "../dl/download_container.h"
#include <reg_ex/reg_ex.h>

/*
The following types are imported from download.h and may be/have to be used for writing plugins:
enum plugin_status { PLUGIN_SUCCESS = 1, PLUGIN_ERROR, PLUGIN_LIMIT_REACHED, PLUGIN_FILE_NOT_FOUND, PLUGIN_CONNECTION_ERROR, PLUGIN_SERVER_OVERLOADED,
					 PLUGIN_INVALID_HOST, PLUGIN_INVALID_PATH, PLUGIN_CONNECTION_LOST, PLUGIN_WRITE_FILE_ERROR, PLUGIN_AUTH_FAIL };

struct plugin_output {
	std::string download_url;
	std::string download_filename;
	bool allows_resumption;
	bool allows_multiple;
	bool offers_premium;
	filesize_t file_size;
	plugin_status file_online;
};

struct plugin_input {
	std::string premium_user;
	std::string premium_password;
	std::string url;
};

You also might be interested in checking ../dl/download_container.h to see what you can do with the result of get_dl_container().
Also, the functions below may help you writing your plugin by implementing some basic things you might need and simplifying tasks
you would usually do with the result of get_dl_container().
*/

// forward declaration
plugin_status plugin_exec(plugin_input &pinp, plugin_output &poutp);

#include <vector>
#include "../tools/helperfunctions.h"

/** Set the wait-time for the currently active download. Set this whenever you have to wait for the host. DownloadDaemon will count down
 *	that time until it reaches 0. This is just for the purpose of displaying a status to the user.
 *	@param seconds Seconds to wait
 */
void set_wait_time(int seconds);

/** returns the currently set wait-time for your download
 *	@returns wait time in seconds
 */
int get_wait_time();

/** Get the URL for your download, which was entered by the user
 *	@returns the url
 */
const char* get_url();

/** allows you to change the URL in the download list
 *	@param url URL to set
 */
void set_url(const char* url);

/** allows you to automatically remove the www. from the link.  This is the way all plugins work*/
std::string set_correct_url(std::string url);

/** gives you a pointer to the main download-list. With this, you can do almost anything with any download
 *	@returns pointer to the main download list
 */
download_container* get_dl_container();

/** return download pointer
*/
download* get_dl_ptr();

/** this returns the curl-handle which will be used for downloading later. You may need it for setting special options like cookies, etc.
 *	@returns the curl handle
 */
ddcurl* get_handle();

/** passing a download_container object to that function will delete the current download and replace it by all the links specified in lst.
 *	this is mainly for decrypter-plugins for hosters that contain several download-links of other hosters
 *	@param lst the download list to use for replacing
 */
void replace_this_download(download_container &lst);

/** this function replaces some html-encoded characters with native ansi characters (eg replaces &quot; with " and &lt; with <)
 *	@param s string in which to replace
 */
void replace_html_special_chars(std::string& s);

/** convert anything with an overloaded operator << to string (eg int, long, double, ...)
 *	@param p1 any type to convert into std::string
 *	@returns the result of the conversion
 */
template <class PARAM>
std::string convert_to_string(PARAM p1);

/** Get text between two strings (searches for first occurence and the next one after it)
 *	@param searchIn string to search in
 *	@param before text before the desired string
 *	@param after text after the desired string
 *	@param start_from start searching from this position in the string (and ignore matches before it)
 *	@returns text between before and after, an empty string if before is not found, from before to the end of searchIn if after is not found
 */
std::string search_between(const std::string& searchIn, const std::string& before, const std::string& after="", size_t start_from = 0);

/** same as above, but returns all occurences from searchIn that match
	There is the additional parameter by_end. If this is set to true, the after-string will be used for locating substrings
	(this means, we search for "after", then search backward to "before" and take everything in between. This is useful if the "before" string
	is not useful for identification, but the after string is */
std::vector<std::string> search_all_between(const std::string& searchIn, const std::string& before, const std::string& after, size_t start_from = 0, bool by_end = false);


/////////////////////// IMPLEMENTATION ////////////////////////


download_container *dl_list;
download           *dl_ptr;
int dlid;
int max_retrys;
std::string gocr;
std::string host;
std::string share_directory;
captcha     Captcha;
std::mutex p_mutex;

void set_wait_time(int seconds) {
	dl_ptr->set_wait(seconds);
}

int get_wait_time() {
	return dl_ptr->get_wait();
}

const char* get_url() {
	return dl_ptr->get_url().c_str();
}

void set_url(const char* url) {
	dl_ptr->set_url(url);
}

std::string set_correct_url(std::string url)
{
	std::string newurl;
	try {
		size_t pos = url.find("www.");
		if(pos == std::string::npos) return url;
		pos += 4;
		newurl = "http://" + url.substr(pos);
	} catch(...) {}
	return newurl;
}
		

download_container* get_dl_container() {
	return dl_list;
}

download* get_dl_ptr(){
	return dl_ptr;
}

	ddcurl* get_handle() {
	return dl_ptr->get_handle();
}

void replace_this_download(download_container &lst) {
	dl_list->insert_downloads(dl_list->get_list_position(dlid), lst);
	dl_list->set_status(dlid, DOWNLOAD_DELETED);
}

void replace_html_special_chars(std::string& s) {
	replace_all(s, "&quot;", "\"");
	replace_all(s, "&lt;", "<");
	replace_all(s, "&gt;", ">");
	replace_all(s, "&apos;", "'");
	replace_all(s, "&amp;", "&");
}


template <class PARAM>
std::string convert_to_string(PARAM p1) {
	std::stringstream ss;
	ss << p1;
	return ss.str();
}

std::string search_between(const std::string& searchIn, const std::string& before, const std::string& after, size_t search_from) {
	size_t pos = searchIn.find(before, search_from);
	if(pos == std::string::npos) return "";
	pos += before.size(); // go to the position right after the search term
	if(pos >= searchIn.size()) return "";
	if(after!="")
		return searchIn.substr(pos, searchIn.find(after, pos) - pos);
	else
		return searchIn.substr(pos);
}

std::vector<std::string> search_all_between(const std::string& searchIn, const std::string& before, const std::string& after, size_t start_from, bool by_end) {
	std::vector<std::string> result;
	while(true) {
		size_t pos = 0;
		if(!by_end) {
			pos = searchIn.find(before, start_from);
			if(pos == std::string::npos) return result;
		} else {
			pos = searchIn.find(after, start_from);
			if(pos == std::string::npos) return result;
			pos = searchIn.rfind(before, pos);
		}
		pos += before.size();
		if(pos >= searchIn.size()) return result;
		result.push_back(searchIn.substr(pos, searchIn.find(after, pos) - pos));
		start_from = pos + result.back().size() + after.size();
	}
	return result;
}


/** This function is just a wrapper for defining globals and calling your plugin */
extern "C" plugin_status plugin_exec_wrapper(download_container* dlc, download* pdl, int id, plugin_input& pinp, plugin_output& poutp,
											 int max_captcha_retrys, const std::string &gocr_path, const std::string &root_dir) {
	std::lock_guard<std::mutex> lock(p_mutex);
	dl_ptr = pdl;
	dl_list = dlc;
	dlid = id;
	max_retrys = max_captcha_retrys;
	gocr = gocr_path;
	host = dlc->get_host(id);
	share_directory = root_dir;
	Captcha.setup(gocr, max_retrys, share_directory, dlid, host);
	return plugin_exec(pinp, poutp);
}

#ifdef PLUGIN_CAN_PRECHECK
bool get_file_status(plugin_input &inp, plugin_output &outp);
extern "C" bool get_file_status_init(download_container &dlc, download* pdl, int id, plugin_input &inp, plugin_output &outp) {
	std::lock_guard<std::mutex> lock(p_mutex);
	dl_list = &dlc;
	dl_ptr = pdl;
	dlid = id;
	return get_file_status(inp, outp);
}
#endif

#ifdef PLUGIN_WANTS_POST_PROCESSING
void post_process_download(plugin_input&);
extern "C" void post_process_dl_init(download_container& dlc, download *pdl, int id, plugin_input& pinp) {
	std::lock_guard<std::mutex> lock(p_mutex);
	dl_list = &dlc;
	dl_ptr = pdl;
	dlid = id;
	host = dlc.get_host(id);
	post_process_download(pinp);
}
#endif

// so every plugin gets its own implementation and uses its own retry-count
#include "captcha.cpp"

#endif // PLUGIN_HELPERS_H_INCLUDED
