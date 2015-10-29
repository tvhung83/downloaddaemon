/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <config.h>
#include "helperfunctions.h"
#include "curl_callbacks.h"
#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include <cstring>
#include <vector>
#include <cstdlib>

#ifndef USE_STD_THREAD
#include <boost/thread.hpp>
namespace std {
	using namespace boost;
}
#else
#include <thread>
#include <mutex>
#endif

#include <sys/stat.h>
#include <sys/param.h>
#ifdef HAVE_SYSLOG_H
	#include <syslog.h>
#endif
#include <errno.h>

#include <cfgfile/cfgfile.h>
#include "../dl/download.h"
#include "../dl/download_container.h"
#include "../dl/curl_speeder.h"
#include "../global.h"
using namespace std;

namespace {
	// anonymous namespace to make it file-global
	mutex logfile_mutex;
}

std::string possible_vars = ",enable_resume,enable_reconnect,downloading_active,download_timing_start,download_timing_end,download_folder,"
                            "simultaneous_downloads,log_level,log_procedure,mgmt_max_connections,mgmt_port,mgmt_password,mgmt_accept_enc,max_dl_speed,"
                            "bind_addr,dlist_file,auth_fail_wait,write_error_wait,plugin_fail_wait,connection_lost_wait,refuse_existing_links,overwrite_files,"
                            "recursive_ftp_download,assume_proxys_online,proxy_list,enable_pkg_extractor,pkg_extractor_passwords,download_to_subdirs,"
                            "precheck_links,captcha_retrys,delete_extracted_archives,captcha_manual,captcha_max_wait,";
std::string insecure_vars = ",daemon_umask,gocr_binary,tar_path,unrar_path,unzip_path,post_download_script,";


int string_to_int(const std::string str) {
	stringstream ss;
	ss << str;
	int i;
	ss >> i;
	return i;
}

const std::string& trim_string(std::string &str) {
	size_t pos = str.find_first_not_of("\t\r\n\v\f ");
	size_t end = str.find_last_not_of("\t\r\n\v\f ");
	if(pos == string::npos)
		str.clear();
	else
		str = str.substr(pos, end + 1);
	return str;
}

bool validate_url(std::string &url) {
	bool valid = true;
	// needed for security reason - so the dlist file can't be corrupted
	if(url.find("http://") != 0 && url.find("ftp://") != 0 && url.find("https://") != 0) {
		valid = false;
	}
	size_t pos = url.find('/') + 2;
	if(url.find('.', pos +1) == std::string::npos) {
		valid = false;
	}
	return valid;
}

void log_string(const std::string logstr, int level) {

	std::string desiredLogLevel = global_config.get_cfg_value("log_level");
	int desiredLogLevelInt = LOG_DEBUG;

	time_t rawtime;
	time(&rawtime);
	std::string log_date = ctime(&rawtime);
	log_date.erase(log_date.length() - 1);

	if(desiredLogLevel == "OFF") {
		return;
	} else if (desiredLogLevel == "SEVERE") {
		desiredLogLevelInt = LOG_ERR;
	} else if (desiredLogLevel == "WARNING") {
		desiredLogLevelInt = LOG_WARNING;
	} else if (desiredLogLevel == "DEBUG") {
		desiredLogLevelInt = LOG_DEBUG;
	} else if(desiredLogLevel == "INFO") {
		desiredLogLevelInt = LOG_INFO;
	}

	if(desiredLogLevelInt < level) {
		return;
	}

	std::string log_procedure = global_config.get_cfg_value("log_procedure");
	if(log_procedure.empty()) {
		log_procedure = "syslog";
	}

	std::stringstream to_log;
	std::stringstream to_syslog;
	to_log << '[' << log_date << "] ";
	if(level == LOG_ERR) {
		to_log << "SEVERE: ";
		to_syslog << "SEVERE: ";
	} else if(level == LOG_WARNING) {
		to_log << "WARNING: ";
		to_syslog << "WARNING: ";
	} else if(level == LOG_DEBUG) {
		to_log << "DEBUG: ";
		to_syslog << "DEBUG: ";
	} else if(level == LOG_INFO) {
		to_log << "INFO: ";
		to_syslog << "INFO: ";
	}

	to_log << logstr << '\n';
	to_syslog << logstr;

	lock_guard<mutex> lock(logfile_mutex);
	if(log_procedure == "stdout") {
		cout << to_log.str() << flush;
	} else if(log_procedure == "stderr") {
		cerr << to_log.str() << flush;
	} else if(log_procedure.find("file:") == 0) {
		log_procedure = log_procedure.substr(5);
		trim_string(log_procedure);
		ofstream ofs(log_procedure.c_str(), ios::app);
		if(ofs) ofs.write(to_log.str().c_str(), to_log.str().size());
		ofs.close();
	} else {
		#ifdef HAVE_SYSLOG_H
		openlog("DownloadDaemon", LOG_PID, LOG_DAEMON);
		syslog(level, to_syslog.str().c_str(), 0);
		closelog();
		#else
		cerr << "DownloadDaemon compiled without syslog support! Either recompile with syslog support or change the log_procedure!";
		#endif
	}
}


void replace_all(std::string& searchIn, std::string searchFor, std::string ReplaceWith) {
	size_t old_pos = 0;
	size_t new_pos;
	while((new_pos = searchIn.find(searchFor, old_pos)) != std::string::npos) {
		old_pos = new_pos + 2;
		searchIn.replace(new_pos, searchFor.length(), ReplaceWith);
	}
}

std::string long_to_string(long i) {
	stringstream ss;
	ss << i;
	std::string ret;
	ss >> ret;
	return ret;

}

std::string int_to_string(int i) {
	stringstream ss;
	ss << i;
	std::string ret;
	ss >> ret;
	return ret;

}

long string_to_long(std::string str) {
	stringstream ss;
	ss << str;
	long ret;
	ss >> ret;
	return ret;
}

bool variable_is_valid(std::string &variable) {
	trim_string(variable);

	if(possible_vars.find("," + variable + ",") != std::string::npos) {
		return true;
	}

	if(global_config.get_bool_value("insecure_mode") && insecure_vars.find("," + variable + ",") != std::string::npos) {
		return true;
	}

	return false;
}

bool proceed_variable(const std::string &variable, std::string value) {
	if(variable == "download_folder") {
		correct_path(value);
		std::string pd = program_root + "/plugins/";
		correct_path(pd);
		std::string rcp = program_root + "/reconnect/";
		correct_path(rcp);
		if(value == pd || value == rcp || value.find("/etc") == 0) {
			return false;
		}
	} else if(variable == "max_dl_speed") {
		filesize_t dl_speed = atol(value.c_str()) * 1024;
		curl_speeder::instance()->set_glob_speed(dl_speed);
	} else if(variable == "downloading_active") {
		if(value == "1") {
			global_download_list.start_next_downloadable();
		}
	}

	return true;
}

bool router_variable_is_valid(std::string &variable) {
	trim_string(variable);
	std::string possible_vars = ",reconnect_policy,router_ip,router_username,router_password,new_ip_wait,reconnect_tries,ip_server,";

	if(possible_vars.find("," + variable + ",") != std::string::npos) {
		return true;
	}
	return false;
}

void correct_path(std::string &path) {
	trim_string(path);
	if(path.length() == 0) {
		path = program_root;
		return;
	}

	if(path[0] == '~') {
		path.replace(0, 1, "$HOME");
	}
	substitute_env_vars(path);
	if(path[0] != '/' && path[0] != '\\' && path.find(":\\") == std::string::npos && path.find(":/") == string::npos) {
		path.insert(0, program_root);
	}

	/*
	int path_max;
	#ifdef PATH_MAX
	path_max = PATH_MAX;
	#else
	path_max = pathconf (path, _PC_PATH_MAX);
	if(path_max <= 0)
		path_max = 4096;
	#endif
	char* real_path = new char[path_max];
	realpath(path.c_str(), real_path);
	if(real_path[0] != '\0')
		path = real_path;
	delete [] real_path;
	*/

	char* real_path = realpath(path.c_str(), NULL);
	if (real_path != NULL) {
		path = real_path;
		free(real_path);
	}

	// remove slashes at the end
	while(*(path.end() - 1) == '/' || *(path.end() - 1) == '\\') {
		path.erase(path.end() - 1);
	}

	std::string::iterator it = path.begin();
#ifdef __CYGWIN__
	if(it != path.end())
		++it; // required to make network-paths possible that start with \\ or //
#endif // __CYGWIN__

	for(; it != path.end(); ++it) {
		if((*it == '/' || *it == '\\') && (*(it + 1) == '/' || *(it + 1) == '\\')) {
			path.erase(it);
		}
	}
}

std::string get_env_var(const std::string &var) {
	const char* res = getenv(var.c_str());
	if(res == NULL) return "";
	return res;
}

void substitute_env_vars(std::string &str) {
	size_t pos = 0, old_pos = 0;
	string var_to_replace;
	string result;
	while((pos = str.find('$', old_pos)) != string::npos) {
		var_to_replace = str.substr(pos + 1, str.find_first_of("$/\\ \n\r\t\0", pos + 1) - pos - 1);
		result = get_env_var(var_to_replace);
		str.replace(pos, var_to_replace.length() + 1, result);
		old_pos = pos + 1;

	}
}

bool mkdir_recursive(std::string dir) {
	if (dir.size() < 2) return false;
	correct_path(dir);
	dir += "/";

	struct stat st;
	string curr;
	for(size_t i = 0; i < dir.size(); ++i) {
		if ((dir[i] == '/' || dir[i] == '\\') && i != 0) {
			curr = dir.substr(0, i);
			if(stat(curr.c_str(), &st) == 0) continue;
			errno = 0;
			if(mkdir(curr.c_str(), 0777) != 0) {
				log_string("mkdir_recursive() failed for: '" + curr + "'. error: " + strerror(errno), LOG_ERR);
				return false;
			}
		}
	}
	return true;
}

std::string filename_from_url(const std::string &url) {
	std::string fn;
	if(url.find("/") != std::string::npos) {
		fn = url.substr(url.find_last_of("/\\"));
		fn = fn.substr(1, fn.find('?') - 1);
	}
	make_valid_filename(fn);
	return fn;
}

/** Functor to compare 2 chars case-insensitive, used as lexicographical_compare callback */
struct lt_nocase : public std::binary_function<char, char, bool> {
	bool operator()(char x, char y) const {
		return toupper(static_cast<unsigned char>(x)) < toupper(static_cast<unsigned char>(y));
	}
};


bool CompareNoCase( const std::string& s1, const std::string& s2 ) {
	return std::lexicographical_compare( s1.begin(), s1.end(), s2.begin(), s2.end(), lt_nocase());
}

void make_valid_filename(std::string &fn) {
	replace_all(fn, "\r\n", "");
	replace_all(fn, "\r", "");
	replace_all(fn, "\n", "");
	replace_all(fn, "/", "");
	replace_all(fn, "\\", "");
	replace_all(fn, ":", "");
	replace_all(fn, "*", "");
	replace_all(fn, "?", "");
	replace_all(fn, "\"", "");
	replace_all(fn, "<", "");
	replace_all(fn, ">", "");
	replace_all(fn, "|", "");
	replace_all(fn, "@", "");
	replace_all(fn, "'", "");
	replace_all(fn, "\"", "");
	replace_all(fn, ";", "");
	replace_all(fn, "[", "");
	replace_all(fn, "]", "");
}

std::string ascii_hex_to_bin(std::string ascii_hex) {
	std::string result;
	for(size_t i = 0; i < ascii_hex.size(); i = i + 2) {
		char hex[5] = "0x00";
		hex[2] = ascii_hex[i];
		hex[3] = ascii_hex[i + 1];
		result.push_back(strtol(hex, NULL, 16));
	}
	return result;
}

bool fequal(double p1, double p2) {
	if(p1 > (p2 - 0.0001) && p1 < p2 + 0.0001)
		return true;
	return false;
}

std::vector<std::string> split_string(const std::string& inp_string, const std::string& seperator, bool respect_escape) {
	std::vector<std::string> ret;
	size_t n = 0, last_n = 0;
	while(true) {
		n = inp_string.find(seperator, n);

		if(respect_escape && (n != string::npos) && (n != 0)) {
			// count the number of escape-backslashes before the occurence.. if %2 == 0, this char is not escaped
			int num_escapes = 0;
			int i = (int)n-1; // has to be int.. size_t will not work			
			while(i >= 0 && inp_string[i] == '\\') {
				++num_escapes; --i;
			}
			if(num_escapes % 2 != 0) {
				++n;
				continue;
			}
		}

		ret.push_back(inp_string.substr(last_n, n - last_n));

		if(n == std::string::npos) break;
		n += seperator.size();
		last_n = n;

	}
	return ret;
}

string escape_string(string s, string escape_chars, const string& escape_sequence) {
	escape_chars.append(escape_sequence); // the escape-sequence itsself has to be escaped, too
	size_t n = 0;
	while(true) {
		n = s.find_first_of(escape_chars, n);
		if(n == string::npos) return s;
			s.insert(n, escape_sequence);
			n += escape_sequence.size() + 1;
	}
}

string unescape_string(string s, string escape_chars, const string &escape_sequence) {
	escape_chars.append(escape_sequence);
	size_t n = s.find(escape_sequence);
	bool escape = false;
	while(true) {
		if(n == string::npos) return s;
		
		if(escape) {
			n = s.find(escape_sequence, n + 1);
			escape = false;
		} else {
			s.erase(n, escape_sequence.size());
			escape = true;
		}
	}
}


bool decode_dlc(const std::string& content, download_container* container)
{
	return loadcontainer(".dlc", content, container);
}

std::mutex container_mutex;
bool loadcontainer(const std::string extension, const std::string& content, download_container* container) //(".rsdf", *.ccf, *.dlc);
{
	lock_guard<mutex> lock(container_mutex);
	std::string filename = "/tmp/dd_container_file" + extension;
	log_string("Extension:" + extension,LOG_DEBUG);
	ofstream ofs(filename.c_str());
	if(!ofs.good()) {
		log_string("Could not decode " + extension + " container: Unable to write temporary file", LOG_ERR);
		return false;
	}
	ofs.write(content.c_str(), content.size());
	ofs.close();
	ddcurl handle;
	handle.setopt(CURLOPT_URL, "http://dcrypt.it/decrypt/upload");
	struct curl_httppost* post = NULL;
	struct curl_httppost* last = NULL;
	curl_formadd(&post, &last, CURLFORM_COPYNAME, "dlcfile", CURLFORM_FILE, filename.c_str(), CURLFORM_END);
	handle.setopt(CURLOPT_HTTPPOST, post);
	handle.setopt(CURLOPT_WRITEFUNCTION, write_to_string);
	std::string result;
	handle.setopt(CURLOPT_WRITEDATA, &result);
	if(handle.perform() != CURLE_OK) {
		log_string("Failed to decrypt " + extension + " container: Couldn't contact decryption-server", LOG_ERR);
		curl_formfree(post);
		return false;
	}
	//log_string("Loadcontainer: result" + result, LOG_DEBUG);
	handle.cleanup();
	curl_formfree(post);
    try {
        vector<string> links;
        if (result.find("Sorry") != std::string::npos)
            {
                result.clear();
                ddcurl handle;
                handle.setopt(CURLOPT_URL, "http://posativ.org/decrypt/dlc");
                struct curl_httppost* post = NULL;
                struct curl_httppost* last = NULL;
                curl_formadd(&post, &last, CURLFORM_COPYNAME, "file", CURLFORM_FILE, filename.c_str(), CURLFORM_END);
                handle.setopt(CURLOPT_HTTPPOST, post);
                handle.setopt(CURLOPT_WRITEFUNCTION, write_to_string);
                std::string result;
                handle.setopt(CURLOPT_WRITEDATA, &result);
                if(handle.perform() != CURLE_OK) {
                    log_string("Failed to decrypt " + extension + " container: Couldn't contact decryption-server", LOG_ERR);
                    curl_formfree(post);
                    return false;
                }
                //log_string("Loadcontainer: result" + result, LOG_DEBUG);
                handle.cleanup();
                curl_formfree(post);
                //vector<string>
                links = split_string(result, "\n");
        }
        else
        {
            //log_string("Loadcontainer: result" + result, LOG_DEBUG);
            result = result.substr(result.find("[") + 2);
            result = result.substr(0, result.find("]") - 1);
            //vector<string>
            links = split_string(result, "\", \"");
            //debug
            if(extension == ".dlc" || extension == ".ccf")
            {
                links.erase(links.begin()); // the first link has nothing to do with the download
            }
        }
        for(size_t i = 0; i < links.size(); i++)
        {
            log_string("Loadcontainer: splitted-link =" + links[i],LOG_DEBUG);
            //Uploaded needs this because there are a forwarding from ul.to
            vector<string> splitted_url = split_string(links[i], "/");
            if (splitted_url[2].find("ul.to") != std::string::npos &&
                splitted_url[2].find("uploaded.to") == std::string::npos){
                splitted_url[2]= "uploaded.to/file";
                string temp = links[i];
                links[i].clear();
                for(size_t j = 0; j < splitted_url.size(); ++j) {
                    links[i] += splitted_url[j]+"/";}
                    links[i] = links[i].substr(0,links[i].size()-1);
                log_string("ul.to forward to: " + links[i],LOG_DEBUG);
                }
        }
        int pkg_id = -1;
        if(container == 0) {
            pkg_id = global_download_list.add_package("");
        }
        for(vector<string>::iterator it = links.begin(); it != links.end(); ++it) {
            if(pkg_id != -1) {
                download *dl = new download(*it);
                global_download_list.add_dl_to_pkg(dl, pkg_id);
            } else if(container) {
                container->add_download(*it, "");
            }
        }
        if(pkg_id != -1)
            global_download_list.start_next_downloadable();
        return true;
    }
    catch(std::exception &e) {
    log_string("Failed to decrypt " + extension + " container: " + string(e.what()), LOG_ERR);
    return false;
    }
}

std::string filename_from_path(const std::string &path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == string::npos) return path;
    return path.substr(pos + 1);
}


#ifdef BACKTRACE_ON_CRASH
#include <execinfo.h>
#include "../mgmt/global_management.h"
void print_backtrace(int sig) {
	void *array[50];
	size_t size;
	size = backtrace(array, 50);
	global_mgmt::backtrace = backtrace_symbols(array, size);

	//size = backtrace(_mgmt::backtrace, 50);
	global_mgmt::curr_sig = sig;
	global_mgmt::backtrace_size = size;
	//global_mgmt::backtrace_size = size;
	global_mgmt::sig_handle_cond.notify_one();
  // print out all the frames to stderr
  //backtrace_symbols_fd(array, size, 2);
  //exit(1);
}
#endif

#include "../plugins/captcha.cpp"
