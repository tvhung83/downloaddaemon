/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "reconnect_parser.h"
#include "../tools/helperfunctions.h"
#include "../global.h"
#include <cfgfile/cfgfile.h>
#include <cstdlib>
#include <cstring>
using namespace std;

extern cfgfile global_router_config;

reconnect::reconnect(const std::string &path_p, const std::string &host_p, const std::string &user_p, const std::string &pass_p)
	: path(path_p), host(host_p), user(user_p), pass(pass_p) {
	variables.insert(pair<string, string>("user", user));
	variables.insert(pair<string, string>("pass", pass));
	variables.insert(pair<string, string>("routerip", host));
	handle.setopt(CURLOPT_LOW_SPEED_LIMIT, (long)1024);
	handle.setopt(CURLOPT_LOW_SPEED_TIME, (long)20);
	handle.setopt(CURLOPT_CONNECTTIMEOUT, (long)1024);
	handle.setopt(CURLOPT_NOSIGNAL, 1);
	handle.setopt(CURLOPT_COOKIEFILE, "");
}

reconnect::~reconnect() {
	handle.cleanup();
}

std::string reconnect::get_current_ip() {
	ddcurl ip_handle;
	ip_handle.setopt(CURLOPT_LOW_SPEED_LIMIT, (long)10);
	ip_handle.setopt(CURLOPT_LOW_SPEED_TIME, (long)5);
	ip_handle.setopt(CURLOPT_CONNECTTIMEOUT, (long)5);
	ip_handle.setopt(CURLOPT_NOSIGNAL, 1);
	string ip_srv = global_router_config.get_cfg_value("ip_server");
	ip_handle.setopt(CURLOPT_URL, ip_srv.c_str());
	std::string resultstr;
	ip_handle.setopt(CURLOPT_WRITEFUNCTION, reconnect::write_data);
	ip_handle.setopt(CURLOPT_WRITEDATA, &resultstr);
	ip_handle.perform();
	ip_handle.cleanup();
	trim_string(resultstr);
	return resultstr;
}

bool reconnect::do_reconnect() {
	std::string old_ip = get_current_ip();
	int num_retries = global_router_config.get_int_value("reconnect_tries");
	int tried = 0;
	if(num_retries == 0) {
		log_string("reconnect_tries not specified, but reconnects are enabled. Defaulting to 3", LOG_WARNING);
		num_retries = 3;
	}
	trim_string(path);
	int ip_wait = global_router_config.get_int_value("new_ip_wait");
	int num_tries = global_router_config.get_int_value("reconnect_tries");
	string new_ip;
	while(tried < num_tries) {
		++tried; // try reconnectring num_tries times
		if(path.find("file:") != 0) {
			string script = program_root + "/reconnect/" + path;
			correct_path(script);
			file.open(script.c_str());
			if(!file.good() || !file.is_open()) {
				log_string("Unable to open reconnect script", LOG_WARNING);
				return false;
			}
			while(!curr_line.empty() || getline(file, curr_line)) {
				trim_string(curr_line);
				exec_next();
			}
			file.close();
		} else {
			// reconnectring with the file: syntax -- external script
			string script = path.substr(5);
			trim_string(script);
			if(script.size() > 255) {
				log_string("Path to reconnect-script too long (> 255 characters)", LOG_ERR);
				return false;
			}
			char buf[512];
			strncpy(buf, script.c_str(), 511);
			int j = fork();
			if(j < 0) {
				log_string("fork() failed when trying to execute a reconnect script", LOG_ERR);
				return false;
			}
			if(j == 0) {
				// child process
				execlp("/bin/sh", "/bin/sh", "-c", buf, (char *)NULL);
				exit(0);
			}
		}

		unsigned long start_time = time(NULL);
		while(start_time + ip_wait > (unsigned)time(NULL)) {
			// while we diddn't wait long enoguh, try to get the IP again
			long request_time = time(NULL);
			new_ip = get_current_ip();
			if(new_ip != old_ip && !new_ip.empty())
				return true;
			if(time(NULL) - request_time > 1) // request took more than a second.. try again
				continue;
			sleep(2); // request was very fast but we don't have a new IP (yet).. wait a bit
		}
	}
	if(new_ip.empty())
		log_string("An error occured when contacting the IP-Server. If this error doesn't disappear, check your routerinfo.conf", LOG_ERR);
	else
		log_string("Reconnecting failed. Please check your settings in routerinfo.conf", LOG_WARNING);
	return false;
}

bool reconnect::exec_next() {
	if(curr_line.find("[[[") != 0) {
		return false;
	}
	if(curr_line.find("[[[STEP") == 0) {
		curr_line = curr_line.substr(7);
		step();
	} else if(curr_line.find("[[[REQUEST") == 0) {
		curr_line = curr_line.substr(10);
		request();
	} else if(curr_line.find("[[[WAIT") == 0) {
		curr_line = curr_line.substr(7);
		wait();
	} else if(curr_line.find("[[[DEFINE") == 0) {
		curr_line = curr_line.substr(9);
		define();
	}else {
		if(curr_line.find("]]]") != string::npos) {
			curr_line = curr_line.substr(curr_line.find("]]]") + 3);
		} else {
			return false;
		}
	}

	return true;
}

void reconnect::step() {
	if(curr_line.find("]]]") == string::npos) {
		return;
	}

	curr_line = curr_line.substr(curr_line.find("]]]") + 3);

	while(!curr_line.empty() || getline(file, curr_line)) {
		trim_string(curr_line);
		if(curr_line.empty()) {
			continue;
		}

		std::string cmd = curr_line.substr(0, curr_line.find("[[["));
		trim_string(cmd);
		if(cmd.length() > 0) {
			// stuff that stands in [[[STEP]]] but not in [[[REQEST]]], saved in cmd
		}
		curr_line = curr_line.substr(curr_line.find("[[["));
		if(curr_line.find("[[[/STEP") == 0) {
			if(curr_line.find("]]]") != string::npos) {
				curr_line = curr_line.substr(curr_line.find("]]]") + 3);
				trim_string(curr_line);
			} else {
				curr_line.clear();
			}
			trim_string(curr_line);
			return;
		}
		trim_string(curr_line);
		exec_next();
	}
}

void reconnect::request() {
	if(curr_line.find("]]]") != 0) {
		return;
	}
	curr_line = curr_line.substr(curr_line.find("]]]") + 3);
	trim_string(curr_line);
	handle.reset();
	std::string resultstr;

	handle.setopt(CURLOPT_WRITEFUNCTION, reconnect::write_data);
	handle.setopt(CURLOPT_WRITEDATA, &resultstr);
	handle.setopt(CURLOPT_URL, string("http://" + host).c_str());
	struct curl_slist *header = NULL;

	bool this_is_data = false;
	while(!curr_line.empty() || getline(file, curr_line)) {
		trim_string(curr_line);
		if(curr_line.empty()) {
			this_is_data = true;
			continue;
		}
		std::string cmd;
		cmd = curr_line.substr(0, curr_line.find("[[["));
		if(curr_line.find("[[[") != string::npos) {
			curr_line = curr_line.substr(curr_line.find("[[["));
		} else {
			curr_line.clear();
		}

		if(cmd.empty() && curr_line.find("[[[/REQUEST]]]") == 0) {
			if(!curr_post_data.empty()) {
				handle.setopt(CURLOPT_COPYPOSTFIELDS, curr_post_data.c_str());
			}
//			header = curl_slist_append(header, "User-Agent: Mozilla/5.0 (X11; U; Linux x86_64; de; rv:1.9.2.14pre) Gecko/20101216 Ubuntu/10.10 (maverick) Namoroka/3.6.14pre");
//			header = curl_slist_append(header, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
//			header = curl_slist_append(header, "Accept-Language: de, en-gb;q=0.9, en;q=0.8");
//			header = curl_slist_append(header, "Accept-Encoding: gzip");
//			header = curl_slist_append(header, "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7");
//			header = curl_slist_append(header, "Cache-Control: no-cache");
//			header = curl_slist_append(header, "Pragma: no-cache");
//			header = curl_slist_append(header, "Connection: close");
			if(header != NULL)
				handle.setopt(CURLOPT_HTTPHEADER, header);
			handle.perform();
			if(header != NULL) {
				curl_slist_free_all(header);
				header = 0;
			}
			curr_post_data.clear();
			curr_line = curr_line.substr(14);
			return;
		}
		else if(cmd.empty()) {
			exec_next();
		} else {
			substitute_vars(cmd);
			if(cmd.empty()) continue;
			if(cmd.find("%basicauth%") != string::npos) {
				handle.setopt(CURLOPT_USERPWD, string(user + ':' + pass).c_str());
				cmd = "";
				continue;
			}
			bool do_post = (cmd.find("POST") == 0);
			if(cmd.find("GET") == 0 || do_post) {
				cmd = cmd.substr(4);
				trim_string(cmd);
				cmd = cmd.substr(0, cmd.find_first_of(" \t\n"));
				cmd = "http://" + host + cmd;
				handle.setopt(CURLOPT_URL, cmd.c_str());
				if(do_post)
					handle.setopt(CURLOPT_POST, 1);
				continue;
			}

			if(cmd.find(':') == string::npos || this_is_data) {
				curr_post_data += cmd;
				continue;
			}
			struct curl_slist *tmp_slist = curl_slist_append(header, cmd.c_str());
			if(tmp_slist == 0) {
				curr_post_data = cmd;
				this_is_data = true;
			} else {
				header = tmp_slist;
			}
		}
		trim_string(curr_line);
	}
}

void reconnect::wait() {
	std::string cmd = curr_line.substr(0, curr_line.find("]]]"));
	trim_string(cmd);
	if(cmd.empty()) return;
	size_t n;
	if((n = cmd.find("seconds")) != string::npos) {
		n = cmd.find('=', n);
		std::string num_secs;
		bool num_found = false;
		for(size_t i = n; i < cmd.length() && !num_found; ++i) {
			if(isdigit(cmd[i])) {
				num_found = true;
				num_secs += cmd[i];
			} else if(num_found) {
				break;
			}
		}
		int secs = atoi(num_secs.c_str());
		sleep(secs);
	}
	if(curr_line.find("]]]") != string::npos) {
		curr_line = curr_line.substr(curr_line.find("]]]") + 3);
		trim_string(curr_line);
	} else {
		curr_line.clear();
	}
}

void reconnect::define() {
	string cmd = curr_line.substr(0, curr_line.find("]]]"));
	trim_string(cmd);
	if(cmd.empty()) return;
	cmd += "="; // so we don't get off-by-one error
	for(size_t i = cmd.find('='); cmd.find('=', i + 1) != string::npos;) {
		size_t j = i;
		while(isspace(cmd[--j]));
		while(!isspace(cmd[j--]) && j != 0);
		string varname = cmd.substr(j, i);
		trim_string(varname);

		string value;
		size_t start = cmd.find('"', i) + 1;
		j = start + 1;
		while(cmd.length() > j++) {
			if(cmd[j] == '\"' && cmd[j-1] != '\\') {
				break;
			}
		}
		value = cmd.substr(start, j - start);
		variables.insert(pair<string, string>(varname, value));
	}

	if(curr_line.find("]]]") != string::npos) {
		curr_line.substr(curr_line.find("]]]") + 3);
	} else {
		curr_line.clear();
	}
}


size_t reconnect::write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *result = (std::string*)userp;
	result->append((char*)buffer, nmemb);
	return nmemb;
}

void reconnect::substitute_vars(std::string& str) {
	for(std::map<std::string, std::string>::iterator it = variables.begin(); it != variables.end(); ++it) {
		replace_all(str, "%%%" + it->first + "%%%", it->second);
		replace_all(str, "%%" + it->first + "%%", it->second);
		replace_all(str, "%" + it->first + "%", it->second);
	}
	if(str.find("%Set-Cookie%") != string::npos) {
		str.clear();
	}

}
