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

#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <sstream>
#include <ctime>
#include <cstring>
#include <cctype>
#include <algorithm>

#include <sys/socket.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>

#include "mgmt_thread.h"
#include "../dl/download.h"
#include <netpptk/netpptk.h>
#include <cfgfile/cfgfile.h>
#include <crypt/md5.h>
#include <crypt/AES/aes.h>
#include <crypt/base64.h>
#include "../tools/helperfunctions.h"
#include "../dl/download_container.h"
#include "../dl/curl_speeder.h"
#include "../global.h"
#include "global_management.h"
#include "connection_manager.h"
#ifndef USE_STD_THREAD
#include <boost/thread.hpp>
#include <boost/bind.hpp>
namespace std {
	using namespace boost;
}
#else
#include <thread>
#endif
using namespace std;


/** the main thread for the management-interface over tcp
 */
void mgmt_thread_main() {
	tkSock main_sock(global_config.get_int_value("mgmt_max_connections"), 1024);
	int mgmt_port = global_config.get_int_value("mgmt_port");
	if(mgmt_port == 0) {
		log_string("Unable to get management-socket-port from configuration file. defaulting to 56789", LOG_ERR);
		mgmt_port = 56789;
	}

	string bind_addr = global_config.get_cfg_value("bind_addr");

	if(!main_sock.bind(mgmt_port, bind_addr)) {
		log_string("bind failed for remote-management-socket. Maybe you specified a port wich is "
		           "already in use or you don't have read-permissions to the config-file.", LOG_ERR);
		exit(-1);
	}

	if(!main_sock.listen()) {
		log_string("Could not listen on management socket", LOG_ERR);
		exit(-1);
	}

	srand(time(NULL));

	connection_manager::create_instance();


	while(true) {
		try {
			client* connection = new client;
			main_sock.accept(*(connection->sock));
			connection_manager::instance()->add_client(connection);
			thread t(std::bind(connection_handler, connection));
			t.detach();
		} catch(...) {
			log_string("Failed to create socket or connection-thread", LOG_ERR);
		}
	}
}

/** connection handle for management connections (callback function for tkSock)
 * @param sock the socket we get for communication with the other side
 */
void connection_handler(client *connection) {
	tkSock *sock = connection->sock;
	std::string data;
	std::string passwd(global_config.get_cfg_value("mgmt_password"));
	if(*sock && passwd != "") {
		*sock << "102 AUTHENTICATION";
		*sock >> data;
		trim_string(data);
		bool auth_success = false;
		std::string auth_allowed = global_config.get_cfg_value("mgmt_accept_enc");
		if(auth_allowed != "plain" && auth_allowed != "encrypt") {
			auth_allowed = "both";
		}

		if(data == passwd && (auth_allowed == "plain" || auth_allowed == "both")) {
			auth_success = true;
		} else if(data == "ENCRYPT" && (auth_allowed == "both" || auth_allowed == "encrypt")) {
			std::string rnd;
			rnd.resize(128);
			for(int i = 0; i < 128; ++i) {
				rnd[i] = rand() % 255;
			}
			sock->send(rnd);
			std::string response_have;
			sock->recv(response_have);
			rnd += passwd;

			MD5_CTX md5;
			MD5_Init(&md5);
			unsigned char* enc_data = new unsigned char[rnd.length()];
			memset(enc_data, 0, 128);
			for(size_t i = 0; i < rnd.length(); ++i) {
				enc_data[i] = rnd[i];
			}

			MD5_Update(&md5, enc_data, rnd.length());
			unsigned char result[16];
			MD5_Final(result, &md5);
			std::string response_should((char*)result, 16);
			delete [] enc_data;

			if(response_should == response_have) {
				auth_success = true;
			}
		}

		if(!auth_success) {
			log_string("Authentication failed. " + sock->get_peer_name() + " entered a wrong password or used invalid encryption", LOG_WARNING);
			if(*sock) *sock << "102 AUTHENTICATION";
			return;
		} else {
			//log_string("User Authenticated", LOG_DEBUG);
			if(*sock) *sock << "100 SUCCESS";
		}
	} else if(*sock) {
		*sock << "100 SUCCESS";
	}
	while(*sock) {
		if(!connection->list().empty()) {
			// the client subscribed to something, so we have to check for commands and subscription updates at the same time
			while(true) {
				if(connection->messagecount() > 0) {
					string msg = connection->pop_message();
					*sock << msg;
					//log_string(msg, LOG_DEBUG);
				} else if(sock->select(50) || !*sock) {
					break;
				}
			}
		}
		if(!*sock) break;
		if(sock->recv(data) == 0) {
			*sock << "101 PROTOCOL";
			break;
		}
		trim_string(data);
		// the command before any processing took place
		string original_command = data;

		if(data.length() < 8 || data.find("DDP") != 0 || !isspace(data[3])) {
			if(*sock) *sock << "101 PROTOCOL";
			continue;
		}
		data = data.substr(4);
		trim_string(data);

		if(data.find("DL") == 0) {
			data = data.substr(2);
			if(data.length() == 0 || !isspace(data[0])) {
				*sock << "101 PROTOCOL";
				continue;
			}
			trim_string(data);
			target_dl(data, sock);
			global_download_list.start_next_downloadable();
		} else if(data.find("PKG") == 0) {
			data = data.substr(3);
			if(data.length() == 0 || !isspace(data[0])) {
				*sock << "101 PROTOCOL";
				continue;
			}
			trim_string(data);
			target_pkg(data, sock);
		} else if(data.find("VAR") == 0) {
			data = data.substr(3);
			if(data.length() == 0 || !isspace(data[0])) {
				*sock << "101 PROTOCOL";
				continue;
			}
			trim_string(data);
			target_var(data, sock);
			global_download_list.start_next_downloadable();
		} else if(data.find("FILE") == 0) {
			data = data.substr(4);
			if(data.length() == 0 || !isspace(data[0])) {
				*sock << "101 PROTOCOL";
				continue;
			}
			trim_string(data);
			target_file(data, sock);
		} else if(data.find("ROUTER") == 0) {
			data = data.substr(6);
			if(data.length() == 0 || !isspace(data[0])) {
				*sock << "101 PROTOCOL";
				continue;
			}
			trim_string(data);
			target_router(data, sock);
		} else if(data.find("PREMIUM") == 0) {
			data = data.substr(7);
			if(data.length() == 0 || !isspace(data[0])) {
				*sock << "101 PROTOCOL";
				continue;
			}
			trim_string(data);
			target_premium(data, sock);
			global_download_list.start_next_downloadable();
		} else if(data.find("SUBSCRIPTION") == 0) {
			data = data.substr(12);
			if(data.length() == 0 || !isspace(data[0])) {
				*sock << "101 PROTOCOL";
				continue;
			}
			trim_string(data);
			target_subscription(data, connection);
		} else if(data.find("CAPTCHA") == 0) {
			data.erase(0, 7);
			if(data.length() == 0 || !isspace(data[0])) {
				*sock << "101 PROTOCOL";
				continue;
			}
			trim_string(data);
			target_captcha(data, sock);
		} else {
			*sock << "101 PROTOCOL";
			continue;
		}

		// dump after each (valid) command except for DL LIST
		if(original_command.find("LIST") == string::npos ) {
			global_download_list.dump_to_file();
			global_download_list.start_next_downloadable();
			if(original_command.find("VAR") != string::npos) {
				// tell the global_mgmt thread that the times changes. This is needed that it doesn't access the HD more often than needed.
				global_mgmt::ns_mutex.lock();
				global_mgmt::curr_start_time = global_config.get_cfg_value("download_timing_start");
				global_mgmt::curr_end_time = global_config.get_cfg_value("download_timing_end");
				global_mgmt::downloading_active = global_config.get_bool_value("downloading_active");
				global_mgmt::ns_mutex.unlock();
			}
		}
	}
	connection_manager::instance()->del_client(connection->client_id);
}

//////////////////////////////////////////////
////////// DL TARGET START ///////////////////
//////////////////////////////////////////////

void target_dl(std::string &data, tkSock *sock) {
	if(data.find("LIST") == 0) {
		data = data.substr(4);
		trim_string(data);
		target_dl_list(data, sock);

	} else if(data.find("ADD") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_dl_add(data, sock);

	} else if(data.find("DEL") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		target_dl_del(data, sock);

	} else if(data.find("STOP") == 0) {
		data = data.substr(4);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_dl_stop(data, sock);

	} else if(data.find("UP") == 0) {
		data = data.substr(2);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		log_string("up",LOG_DEBUG);
		target_dl_up(data, sock);

	} else if(data.find("DOWN") == 0) {
		data = data.substr(4);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		log_string("down",LOG_DEBUG);
		target_dl_down(data, sock);

	} else if(data.find("TOP") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		log_string("top",LOG_DEBUG);
		target_dl_top(data, sock);

	} else if(data.find("BOTTOM") == 0) {
		data = data.substr(6);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		log_string("bottom",LOG_DEBUG);
		target_dl_bottom(data, sock);

	} else if(data.find("ACTIVATE") == 0) {
		data = data.substr(8);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_dl_activate(data, sock);

	} else if(data.find("DEACTIVATE") == 0) {
		data = data.substr(10);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_dl_deactivate(data, sock);
	} else if(data.find("SET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_dl_set(data, sock);
	} else if(data.find("GET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_dl_get(data, sock);
	} else {
		*sock << "101 PROTOCOL";
	}

}

void target_dl_list(std::string &data, tkSock *sock) {
	//log_string("Dumping download list to client", LOG_DEBUG);
	*sock << global_download_list.create_client_list();
}

void target_dl_add(std::string &data, tkSock *sock) {
	int package;
	std::string url;
	std::string comment;
	if(data.find_first_of("\n\r") != string::npos) {
		*sock << "108 VARIABLE";
		return;
	} else {
		size_t n;
		package = atoi(data.substr(0, n = data.find_first_of(" \t")).c_str());
		if(n == string::npos) {
			*sock << "108 VARIABLE";
			return;
		}
		data = data.substr(n);
		trim_string(data);
		n = data.find(' ');
		url = data.substr(0, n);
		if(n != string::npos)
			comment = data.substr(n);
		trim_string(data);
		trim_string(url);
		trim_string(comment);
	}
	if(validate_url(url)) {
		if(global_download_list.url_is_in_list(url)) {
			if(global_config.get_bool_value("refuse_existing_links")) {
				*sock << "108 VARIABLE";
				return;
			}

		}
		download* dl = new download(url);
		dl->set_title(comment);
		std::string logstr("Adding download: ");
		logstr += dl->serialize();
		logstr.erase(logstr.length() - 1);
		log_string(logstr, LOG_DEBUG);

		if(global_download_list.add_dl_to_pkg(dl, package) != LIST_SUCCESS) {
			log_string("Tried to add a download to a non-existant package", LOG_WARNING);
			*sock << "104 ID";
		} else {
			*sock << "100 SUCCESS";
		}
		return;
	}
	log_string("Could not add a download", LOG_WARNING);
	*sock << "103 URL";
}

void target_dl_del(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	vector <string> ids = split_string(data, ",");
	bool bret = true;
	for (vector<string>::iterator it = ids.begin(); it != ids.end(); ++it) {
		trim_string(*it);
		if (it->size() > 0 && isalnum(it->at(0))) {
			dlindex id;
			id.second = atoi(it->c_str());
			id.first = global_download_list.pkg_that_contains_download(id.second);
			if(id.first == LIST_ID) {
				bret = false;
			} else {
				global_download_list.set_status(id, DOWNLOAD_DELETED);
			}
		} else {
			bret = false;
		}
	}

	if(!bret)
		*sock << "104 ID";
	else
		*sock << "100 SUCCESS";
}

void target_dl_stop(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	vector<string> ids = split_string(data, ",");
	dlindex id;
	bool fail = false;
	for(vector<string>::iterator it = ids.begin(); it != ids.end(); ++it) {
		id.second = atoi(it->c_str());
		id.first = global_download_list.pkg_that_contains_download(id.second);
		if(id.first == LIST_ID)
			fail = true;
		else
			global_download_list.set_need_stop(id, true);
	}
	if(!fail)
		*sock << "100 SUCCESS";
	else
		*sock << "104 ID";
}

void target_dl_up(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	bool fail = false;
	vector<string> ids = split_string(data, ",");
	for(vector<string>::iterator it = ids.begin(); it != ids.end(); ++it) {
		id.second = atoi(it->c_str());
		id.first = global_download_list.pkg_that_contains_download(id.second);
		if(id.first == LIST_ID)
			fail = true;
		else
		{
			global_download_list.move_dl(id, package_container::DIRECTION_UP);
			log_string("download_up",LOG_DEBUG);
		}
	}
	if(!fail)
		*sock << "100 SUCCESS";
	else
		*sock << "104 ID";
}

void target_dl_down(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	bool fail = false;
	vector<string> ids = split_string(data, ",");
	for(vector<string>::iterator it = ids.begin(); it != ids.end(); ++it) {
		id.second = atoi(it->c_str());
		id.first = global_download_list.pkg_that_contains_download(id.second);
		if(id.first == LIST_ID)
			fail = true;
		else
		{
			global_download_list.move_dl(id, package_container::DIRECTION_DOWN);
			log_string("download_down",LOG_DEBUG);
		}

	}
	if(!fail)
		*sock << "100 SUCCESS";
	else
		*sock << "104 ID";
}

void target_dl_top(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	bool fail = false;
	vector<string> ids = split_string(data, ",");
	for(vector<string>::iterator it = ids.begin(); it != ids.end(); ++it) {
		id.second = atoi(it->c_str());
		id.first = global_download_list.pkg_that_contains_download(id.second);
		if(id.first == LIST_ID)
			fail = true;
		else
		{
			global_download_list.move_dl(id, package_container::DIRECTION_TOP);
			log_string("download_top",LOG_DEBUG);
		}
	}
	if(!fail)
		*sock << "100 SUCCESS";
	else
		*sock << "104 ID";
}

void target_dl_bottom(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	bool fail = false;
	vector<string> ids = split_string(data, ",");
	for(vector<string>::iterator it = ids.begin(); it != ids.end(); ++it) {
		id.second = atoi(it->c_str());
		id.first = global_download_list.pkg_that_contains_download(id.second);
		if(id.first == LIST_ID)
			fail = true;
		else
		{
			global_download_list.move_dl(id, package_container::DIRECTION_BOTTOM);
			log_string("package_bottom",LOG_DEBUG);
		}
	}
	if(!fail)
		*sock << "100 SUCCESS";
	else
		*sock << "104 ID";
}

void target_dl_activate(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	id.second = atoi(data.c_str());
	id.first = global_download_list.pkg_that_contains_download(id.second);
	if(id.first == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	if(global_download_list.get_status(id) == DOWNLOAD_INACTIVE) {
		global_download_list.set_status(id, DOWNLOAD_PENDING);
		*sock << "100 SUCCESS";
	} else {
		*sock << "106 ACTIVATE";
	}
}

void target_dl_deactivate(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	id.second = atoi(data.c_str());
	id.first = global_download_list.pkg_that_contains_download(id.second);
	if(id.first == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	if(global_download_list.get_status(id) == DOWNLOAD_INACTIVE) {
		*sock << "107 DEACTIVATE";
	} else {
		global_download_list.set_status(id, DOWNLOAD_INACTIVE);
		global_download_list.set_wait(id, 0);
		*sock << "100 SUCCESS";
	}
}

void target_dl_set(std::string &data, tkSock *sock) {
	string option;
	string value;
	size_t n;
	int id = atoi(data.substr(0, (n = data.find_first_of("\n\r\t "))).c_str());
	int pkg_id = global_download_list.pkg_that_contains_download(id);
	if(pkg_id == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	download_status st = global_download_list.get_status(pair<int, int>(pkg_id, id));
	if(st == DOWNLOAD_RUNNING || st == DOWNLOAD_FINISHED || global_download_list.get_running(pair<int, int>(pkg_id, id))) {
		*sock << "108 VARIABLE";
		return;
	}

	data = data.substr(n);
	trim_string(data);
	if((n = data.find('=')) == string::npos || n + 1 > data.size()) {
		*sock << "101 PROTOCOL";
		return;
	}
	option = data.substr(0, n);
	value = data.substr(n + 1);
	if(data.find_first_of("\n\r|") != string::npos) {
		*sock << "101 PROTOCOL";
		return;
	}
	trim_string(option);
	trim_string(value);
	dlindex index = pair<int, int>(pkg_id, id);
	if(option == "DL_URL") {
		if(validate_url(value) && (!global_download_list.get_running(index) || global_download_list.get_url(index) == value)) {
			global_download_list.set_url(index, value);
			*sock << "100 SUCCESS";
		} else {
			*sock << "103 URL";
		}
	} else if(option == "DL_TITLE") {
		global_download_list.set_title(index, value);
		*sock << "100 SUCCESS";
	} else {
		*sock << "101 PROTOCOL";
	}
}

void target_dl_get(std::string &data, tkSock *sock) {
	size_t n;
	int id = atoi(data.substr(0, (n = data.find_first_of("\n\r "))).c_str());
	int pkg_id = global_download_list.pkg_that_contains_download(id);
	if(pkg_id == LIST_ID || n == string::npos) {
		*sock << "";
		return;
	}
	data = data.substr(n);
	trim_string(data);
	dlindex idx(pkg_id, id);
	if(data == "DL_URL") {
		*sock << global_download_list.get_url(idx);
	} else if(data == "DL_TITLE") {
		*sock << global_download_list.get_title(idx);
	} else if(data == "DL_ADD_DATE") {
		*sock << global_download_list.get_add_date(idx);
	} else {
		*sock << "";
	}
}

////////////////////////////////////////////////
/////////////// PKG TARGET START ///////////////
////////////////////////////////////////////////

void target_pkg(std::string &data, tkSock *sock) {
	if(data.find("ADD") == 0) {
		data = data.substr(3);
		trim_string(data);
		target_pkg_add(data, sock);
	} else if(data.find("DEL") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_del(data, sock);
	} else if(data.find("UP") == 0) {
		data = data.substr(2);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_up(data, sock);
	} else if(data.find("DOWN") == 0) {
		data = data.substr(4);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_down(data, sock);
	} else if(data.find("TOP") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_top(data, sock);
	} else if(data.find("BOTTOM") == 0) {
		data = data.substr(6);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_bottom(data, sock);
	} else if(data.find("EXISTS") == 0) {
		data = data.substr(6);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_exists(data, sock);
	} else if(data.find("SET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_set(data, sock);
	} else if(data.find("GET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_get(data, sock);
	} else if(data.find("CONTAINER") == 0) {
		data = data.substr(9);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_container(data, sock);
	} else if(data.find("ACTIVATE") == 0) {
		data = data.substr(8);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_activate(data, sock);
	} else if(data.find("DEACTIVATE") == 0) {
		data = data.substr(10);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_deactivate(data, sock);
	} else {
		*sock << "101 PROTOCOL";
	}
}

void target_pkg_add(std::string &data, tkSock *sock) {
	if(data.find_first_of("|\n\r") != string::npos) {
		*sock << "-1";
		return;
	}
	*sock << int_to_string(global_download_list.add_package(data));
}

void target_pkg_del(std::string &data, tkSock *sock) {
	global_download_list.del_package(atoi(data.c_str()));
	*sock << "100 SUCCESS";
}

void target_pkg_up(std::string &data, tkSock *sock) {
	global_download_list.move_pkg(atoi(data.c_str()), package_container::DIRECTION_UP);
	*sock << "100 SUCCESS";
}

void target_pkg_top(std::string &data, tkSock *sock) {
	log_string("package_top",LOG_DEBUG);
	global_download_list.move_pkg(atoi(data.c_str()), package_container::DIRECTION_TOP);
	*sock << "100 SUCCESS";
}

void target_pkg_bottom(std::string &data, tkSock *sock) {
	log_string("package_bottom",LOG_DEBUG);
	global_download_list.move_pkg(atoi(data.c_str()), package_container::DIRECTION_BOTTOM);
	*sock << "100 SUCCESS";
}

void target_pkg_down(std::string &data, tkSock *sock) {
	global_download_list.move_pkg(atoi(data.c_str()), package_container::DIRECTION_DOWN);
	*sock << "100 SUCCESS";
}

void target_pkg_exists(std::string &data, tkSock *sock) {
	*sock << int_to_string(global_download_list.pkg_exists(atoi(data.c_str())));
}

void target_pkg_set(std::string &data, tkSock *sock) {
	string option;
	string value;
	size_t n;
	int id = atoi(data.substr(0, (n = data.find_first_of("\n\r\t "))).c_str());
	if(!global_download_list.pkg_exists(id)) {
		*sock << "104 ID";
		return;
	}
	data = data.substr(n);
	trim_string(data);
	if((n = data.find('=')) == string::npos || n + 1 > data.size()) {
		*sock << "101 PROTOCOL";
		return;
	}
	option = data.substr(0, n);
	value = data.substr(n + 1);
	if(data.find_first_of("\n\r|") != string::npos) {
		*sock << "101 PROTOCOL";
		return;
	}
	trim_string(option);
	trim_string(value);
	if(option == "PKG_NAME") {
		global_download_list.set_pkg_name(id, value);
		*sock << "100 SUCCESS";
	} else if(option == "PKG_PASSWORD") {
		global_download_list.set_password(id, value);
		*sock << "100 SUCCESS";
	} else {
		*sock << "101 PROTOCOL";
	}
}

void target_pkg_get(std::string &data, tkSock *sock) {
	size_t n;
	int pkg_id = atoi(data.substr(0, (n = data.find_first_of("\n\r "))).c_str());
	if(n == string::npos) {
		*sock << "";
		return;
	}
	// if the package doesn't exists, "" is returned automatically

	data = data.substr(n);
	trim_string(data);
	if(data == "PKG_NAME") {
		*sock << global_download_list.get_pkg_name(pkg_id);
	} else if(data == "PKG_PASSWORD") {
		*sock << global_download_list.get_password(pkg_id);
	} else {
		*sock << "";
	}
}

void target_pkg_container(std::string &data, tkSock *sock) {
	// format:
	// container <TYPE>:<data>
	size_t n;

	string type = data.substr(0, (n = data.find(":")));
	trim_string(type);
	if(n == string::npos) {
		*sock << "101 PROTOCOL";
		return;
	}
	data = data.substr(data.find(":") + 1);
	if(type == "RSDF") {
//		replace_all(data, "\r\n", "");
//		replace_all(data, "\n", "");
//		replace_all(data, "\r", "");
//
//		std::string key = ascii_hex_to_bin("8C35192D964DC3182C6F84F3252239EB4A320D2500000000");
//		string iv = ascii_hex_to_bin("a3d5a33cb95ac1f5cbdb1ad25cb0a7aa");
//
//		std::basic_stringstream<unsigned char> container;
//		data = ascii_hex_to_bin(data);
//		replace_all(data, "\r\n", "");
//		replace_all(data, "\n", "");
//		replace_all(data, "\r", "");
//
//		vector<string> linkvec = split_string(data, "==", false);
//		data.clear();
//		for(size_t i = 0; i < linkvec.size(); ++i) {
//			data += base64_decode(linkvec[i]);
//		}
//
//		AES_KEY aes_key;
//		AES_set_encrypt_key((const unsigned char*)key.c_str(), 192, &aes_key);
//		char iv_c[16];
//		memcpy(iv_c, iv.c_str(), 16);
//		char* plaintext = new char[data.size() + 17];
//		memset(plaintext, 0, data.size() + 17);
//		AES_cfb8_encrypt((const unsigned char*)data.c_str(), (unsigned char*)plaintext, data.size() + 17, &aes_key, (unsigned char*)iv_c, 0, AES_DECRYPT);
//                string result(plaintext, data.size());
//		delete [] plaintext;
//
//		bool first = true;
//		int pkg_id = global_download_list.add_package("");
//		replace_all(result, "CCF: ", "\r\n");
//		trim_string(result);
//                vector<string> final = split_string(result, "\r\n");
//                if(final.size() == 1) { // sometimes the CCF: is missing
//                    final = split_string(result, "http://");
//                    final.erase(final.begin());
//                    for(vector<string>::iterator it = final.begin(); it != final.end(); ++it) *it = "http://" + *it;
//                }
//		for(vector<string>::iterator it = final.begin(); it != final.end(); ++it) {
//			trim_string(*it);
//                        if(it->empty()) continue;
//			download* dl = new download(*it);
//			global_download_list.add_dl_to_pkg(dl, pkg_id);
//			if(first) {
//				string pkg_name = filename_from_url(*it);
//				size_t last_dot = pkg_name.find_last_of(".");
//				if(last_dot != 0)
//					pkg_name = pkg_name.substr(0, last_dot);
//				global_download_list.set_pkg_name(pkg_id, pkg_name);
//				first = false;
//			}
//		}
//		*sock << "100 SUCCESS";
		log_string("Received RSDF container",LOG_DEBUG);
		thread d(std::bind(loadcontainer, ".rsdf",data, (download_container*)0));
		d.detach();
		*sock << "100 SUCCESS";
		return;
	} else if(type == "DLC") {
		log_string("Received DLC Container",LOG_DEBUG);
		thread d(std::bind(decode_dlc, data, (download_container*)0));
		d.detach();
		*sock << "100 SUCCESS";
		return;
	} else if(type == "CCF")
	{
		log_string("Received CCF Container",LOG_DEBUG);
		thread d(std::bind(loadcontainer, ".ccf",data, (download_container*)0));
		d.detach();
		*sock << "100 SUCCESS";
		return;
	} else {
		*sock << "112 UNSUPPORTED";
		return;
	}
}

void target_pkg_activate(std::string &data, tkSock *sock) {
	vector<string> pkgs = split_string(data, ",");
	for(vector<string>::iterator pkg = pkgs.begin(); pkg != pkgs.end(); ++pkg) {
		vector<int> dls = global_download_list.get_download_list(atoi(pkg->c_str()));

		for(vector<int>::iterator dl =  dls.begin(); dl != dls.end(); ++dl) {
			dlindex idx(atoi(pkg->c_str()), *dl);
			if(global_download_list.get_status(idx) == DOWNLOAD_INACTIVE) {
				global_download_list.set_status(idx, DOWNLOAD_PENDING);
			}
		}

	}
	*sock << "100 SUCCESS";
}

void target_pkg_deactivate(std::string &data, tkSock *sock) {
	vector<string> pkgs = split_string(data, ",");
	for(vector<string>::iterator pkg = pkgs.begin(); pkg != pkgs.end(); ++pkg) {
		vector<int> dls = global_download_list.get_download_list(atoi(pkg->c_str()));

		for(vector<int>::iterator dl =  dls.begin(); dl != dls.end(); ++dl) {
			dlindex idx(atoi(pkg->c_str()), *dl);
			if(global_download_list.get_status(idx) != DOWNLOAD_FINISHED)
				global_download_list.set_status(idx, DOWNLOAD_INACTIVE);
		}
	}
	*sock << "100 SUCCESS";
}

////////////////////////////////////////////////
////////////////// VAR TARGET START ////////////
////////////////////////////////////////////////

void target_var(std::string &data, tkSock *sock) {
	if(data.find("GET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_var_get(data, sock);
	} else if(data.find("SET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_var_set(data, sock);
	} else if(data.find("LIST") == 0) {
		data = data.substr(4);
		trim_string(data);
		target_var_list(data, sock);
	}
	else {
		*sock << "101 PROTOCOL";
	}
}

void target_var_get(std::string &data, tkSock *sock) {
	if(data == "mgmt_password") {
		*sock << "";
	} else {
		*sock << global_config.get_cfg_value(data);
	}
}

void target_var_set(std::string &data, tkSock *sock) {
	if(data.find('=') == string::npos || data.find_first_of("\n\r") != string::npos) {
		*sock << "101 PROTOCOL";
		return;
	}
	if(data.find("mgmt_password") == 0) {
		if(data.find(';') == std::string::npos) {
			*sock << "101 PROTOCOL";
			return;
		}
		size_t pos1 = data.find('=') + 1;
		size_t pos2 = data.find(';', pos1);
		std::string old_pw;
		if(pos1 == pos2) {
			old_pw = "";
		} else {
			old_pw = data.substr(pos1, pos2 - pos1);
		}
		trim_string(old_pw);
		if(old_pw == global_config.get_cfg_value("mgmt_password")) {
			if(global_config.set_cfg_value("mgmt_password", data.substr(data.find(';') + 1))) {
				log_string("Changed management password", LOG_WARNING);
				connection_manager::instance()->push_message(connection_manager::SUBS_CONFIG, "mgmt_password");
				*sock << "100 SUCCESS";
			} else {
				log_string("Unable to write configuration file", LOG_ERR);
				*sock << "110 PERMISSION";
			}

		} else {
			log_string("Unable to change management password", LOG_ERR);
			*sock << "102 AUTHENTICATION";
		}
	} else {
		std::string identifier(data.substr(0, data.find('=')));
		trim_string(identifier);
		std::string value(data.substr(data.find('=') + 1));
		trim_string(value);
		if(!proceed_variable(identifier, value)) {
			*sock << "111 VALUE";
			return;
		}

		if(variable_is_valid(identifier)) {
			if(global_config.set_cfg_value(identifier, value)) {
				*sock << "100 SUCCESS";
				connection_manager::instance()->push_message(connection_manager::SUBS_CONFIG, identifier + " = " + value);
			} else {
				*sock << "110 PERMISSION";
			}
		} else {
			*sock << "108 VARIABLE";
		}
	}

}

void target_var_list(std::string &data, tkSock *sock) {
	extern std::string possible_vars, insecure_vars;
	vector<string> vars = split_string(possible_vars, ",");
	if(global_config.get_bool_value("insecure_mode")) {
		vector<string> ivars = split_string(insecure_vars, ",");
		vars.insert(vars.end(), ivars.begin(), ivars.end());
	}
	sort(vars.begin(), vars.end(), CompareNoCase);
	ostringstream ss;
	bool first = true;
	for(vector<string>::iterator it = vars.begin(); it != vars.end(); ++it) {
		if(!it->empty() && *it != "mgmt_password") {
			if(!first) ss << "\n";
			ss << *it << " = " << global_config.get_cfg_value(*it);
			first = false;
		}
	}
	*sock << ss.str();
}

/////////////////////////////
////// Target File //////////
/////////////////////////////

void target_file(std::string &data, tkSock *sock) {
	if(data.find("DEL") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_file_del(data, sock);
	} else if(data.find("GETPATH") == 0) {
		data = data.substr(7);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_file_getpath(data, sock);
	} else if(data.find("GETSIZE") == 0) {
		data = data.substr(7);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_file_getsize(data, sock);
	} else {
		*sock << "101 PROTOCOL";
	}
}

void target_file_del(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	id.second = atoi(data.c_str());
	id.first = global_download_list.pkg_that_contains_download(id.second);
	if(id.first == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	std::string fn = global_download_list.get_output_file(id);
	struct pstat st;
	if(fn.empty() || pstat(fn.c_str(), &st) != 0) {
		*sock << "109 FILE";
		return;
	}

	if(remove(fn.c_str()) != 0) {
		log_string(std::string("Failed to delete file ID: ") + data, LOG_WARNING);
		*sock << "110 PERMISSION";
	} else {
		log_string(std::string("Deleted file ID: ") + data, LOG_DEBUG);
		string folder = fn.substr(0, fn.find_last_of("/\\"));
		correct_path(folder);
		string dl_folder = global_config.get_cfg_value("download_folder");
		correct_path(dl_folder);
		if(folder != dl_folder) {

			bool del_folder = true;
			DIR *dp;
			struct dirent *ep;
			dp = opendir(folder.c_str());
			if (dp == NULL) {
				del_folder = false;
			} else {
				while ((ep = readdir (dp))) {
					string curr = ep->d_name;
					if(!curr.empty() && curr != "." && curr != "..") {
						del_folder = false;
						break;
					}
				}
				closedir (dp);
			}
			if(del_folder) {
				log_string("The deleted file was downloaded to a subfolder and there were no other files in it. Deleting it, too.", LOG_DEBUG);
				rmdir(folder.c_str());
			}

		}

		*sock << "100 SUCCESS";
		global_download_list.set_output_file(id, "");
	}
}

void target_file_getpath(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	id.second = atoi(data.c_str());
	id.first = global_download_list.pkg_that_contains_download(id.second);
	if(id.first == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	std::string output_file = global_download_list.get_output_file(id);
	//struct pstat st; // nice to have, but this slows it down a LOT for larger download lists (40+ downloads) when called after each dl list.
	//if(pstat(output_file.c_str(), &st) != 0) {
	//	*sock << "";
	//	return;
	//}
	*sock << output_file;
}

void target_file_getsize(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	id.second = atoi(data.c_str());
	id.first = global_download_list.pkg_that_contains_download(id.second);
	if(id.first == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	std::string output_file = global_download_list.get_output_file(id);

	struct pstat st;
	if(pstat(output_file.c_str(), &st) != 0) {
		log_string(std::string("Failed to get size of file ID: ") + data, LOG_WARNING);
		*sock << "-1";
		return;
	}
	*sock << int_to_string(st.st_size);
}

////////////////////////////////////////////////////
///////////// ROUTER TARGET START //////////////////
////////////////////////////////////////////////////

void target_router(std::string &data, tkSock *sock) {
	if(data.find("LIST") == 0) {
		data = data.substr(4);
		trim_string(data);
		target_router_list(data, sock);
	} else if(data.find("SETMODEL") == 0) {
		data = data.substr(8);
		trim_string(data);
		target_router_setmodel(data, sock);
	} else if(data.find("SET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_router_set(data, sock);
	} else if(data.find("GET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_router_get(data, sock);
	} else {
		*sock << "101 PROTOCOL";
	}
	if(global_download_list.reconnect_needed()) {
		try {
			thread t(bind(&package_container::do_reconnect, &global_download_list));
			t.detach();
		} catch(...) {
			log_string("Failed to start the reconnect-thread. There are probably too many running threads.", LOG_ERR);
		}
	}
}

void target_router_list(std::string &data, tkSock *sock) {
	DIR *dp;
	struct dirent *ep;
	vector<std::string> content;
	std::string current;
	std::string path = program_root + "/reconnect/";
	dp = opendir (path.c_str());
	if (dp == NULL) {
		log_string("Could not open reconnect script directory", LOG_ERR);
		*sock << "";
		return;
	}

	while ((ep = readdir (dp))) {
		if(ep->d_name[0] == '.') {
			continue;
		}
		current = ep->d_name;
		content.push_back(current);
	}

	closedir (dp);
	sort(content.begin(), content.end(), CompareNoCase);
	std::string to_send;
	for(vector<std::string>::iterator it = content.begin(); it != content.end(); ++it) {
		to_send.append(*it);
		to_send.append("\n");
	}
	if(!to_send.empty()) {
		to_send.erase(to_send.end() - 1);
	}

	*sock << to_send;
}

void target_router_setmodel(std::string &data, tkSock *sock) {
	struct dirent *de = NULL;
	DIR *d = NULL;
	std::string dir = program_root + "/reconnect/";
	d = opendir(dir.c_str());

	if(d == NULL) {
		log_string("Could not open reconnect plugin directory", LOG_ERR);
		*sock << "109 FILE";
		return;
	}

	bool plugin_found = true;
	if(!data.empty()) {
		plugin_found = false;
		while((de = readdir(d))) {
			if(data == de->d_name) {
				plugin_found = true;
			}
		}
	closedir(d);
	}

	if(!plugin_found) {
		log_string("Selected reconnect plugin not found", LOG_WARNING);
		*sock << "109 FILE";
		return;
	}

	if(global_router_config.set_cfg_value("router_model", data)) {
		log_string("Changed router model to " + data, LOG_DEBUG);
		*sock << "100 SUCCESS";
	} else {
		log_string("Unable to set router model", LOG_ERR);
		*sock << "110 PERMISSION";
	}
}

void target_router_set(std::string &data, tkSock *sock) {
	size_t eqpos = data.find('=');
	if(eqpos == std::string::npos) {
		*sock << "101 PROTOCOL";
		return;
	}
	std::string identifier = data.substr(0, eqpos);
	std::string variable = data.substr(eqpos + 1);
	trim_string(identifier);
	trim_string(variable);

	if(!router_variable_is_valid(identifier)) {
		log_string("Tried to change an invalid router variable", LOG_WARNING);
		*sock << "108 VARIABLE";
		return;
	}

	if(global_router_config.set_cfg_value(identifier, variable)) {
		log_string("Changed router variable", LOG_DEBUG);
		*sock << "100 SUCCESS";
	} else {
		log_string("Unable to set router variable!", LOG_ERR);
		*sock << "110 PERMISSION";
	}
}

void target_router_get(std::string &data, tkSock *sock) {
	if(data == "router_password") {
		*sock << "";
		return;
	} else {
		*sock << global_router_config.get_cfg_value(data);
	}
}

/////////////////////////////////////////////////////
//////////////// PREMIUM TARGET START ///////////////
/////////////////////////////////////////////////////

void target_premium(std::string &data, tkSock *sock) {
	if(data.find("LIST") == 0) {
		data = data.substr(4);
		trim_string(data);
		if(!data.empty()) {
			*sock << "";
			return;
		}
		target_premium_list(data, sock);
	} else if(data.find("GET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_premium_get(data, sock);
	} else if(data.find("SET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_premium_set(data, sock);
		plugin_cache.clear_cache();
	}
}

void target_premium_list(std::string &data, tkSock *sock) {
	vector<plugin> plugs = plugin_cache.get_all_infos();
	string to_send;
	for(vector<plugin>::const_iterator it = plugs.begin(); it != plugs.end(); ++it) {
		if (!it->offers_premium) continue;
		to_send.append(it->host);
		to_send.append("\n");
	}
	if(!to_send.empty()) {
		to_send.erase(to_send.end() - 1);
	}
	*sock << to_send;
}

void target_premium_get(std::string &data, tkSock *sock) {
	// that's really all. if an error occurs, get_cfg_value's return value is an empty string - perfect
	*sock << global_premium_config.get_cfg_value(data + "_user");
}

void target_premium_set(std::string &data, tkSock *sock) {
	std::string user;
	std::string password;
	std::string host;
	if(data.find_first_of(" \n\t\r") == string::npos) {
		*sock << "101 PROTOCOL";
		return;
	}
	host = data.substr(0, data.find_first_of(" \n\t\r"));
	data = data.substr(host.length());
	trim_string(data);
	if(data.find(';') == string::npos) {
		*sock << "101 PROTOCOL";
		return;
	}
	user = data.substr(0, data.find(';'));
	data = data.substr(data.find(';'));
	if(data.length() > 1) {
		password = data.substr(data.find(';') + 1);
	}
	if(global_premium_config.set_cfg_value(host + "_user", user) && global_premium_config.set_cfg_value(host + "_password", password)) {
		*sock << "100 SUCCESS";
	} else {
		*sock << "110 PERMISSION";
	}

}

////////////////////////////////////////////
////// CAPTCHA TARGET START ////////////////
////////////////////////////////////////////

void target_captcha(std::string &data, tkSock *sock) {
	if(data.find("REQUEST") == 0) {
		data.erase(0, 7);
		if(data.size() < 2 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_captcha_request(data, sock);
	} else if(data.find("SOLVE") == 0) {
		data.erase(0, 5);
		if(data.size() < 2 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_captcha_solve(data, sock);
	}
}

void target_captcha_request(std::string &data, tkSock *sock) {
	if(data.size() < 1 || !isdigit(data[0])) {
		*sock << "101 PROTOCOL";
		return;
	}
	int id = atoi(data.c_str());
	dlindex idx(global_download_list.pkg_that_contains_download(id), id);
	plugin_status ps = global_download_list.get_error(idx);
	if(ps == PLUGIN_CAPTCHA) {
		captcha *c = global_download_list.get_captcha(idx);
		if(c) {
			string to_send = c->get_imgtype() + "|" + c->get_question() + "|" + c->get_image();
			*sock << to_send;
		}
	}
	*sock << "";
}

void target_captcha_solve(std::string &data, tkSock *sock) {
	size_t spos = data.find_first_of("\r\n\t ");
	if(data.size() < 1 || !isdigit(data[0]) || spos == string::npos) {
		*sock << "101 PROTOCOL";
		return;
	}

	string ids = data.substr(0, spos);
	string answer = data.substr(spos);
	trim_string(answer);
	int id = atoi(ids.c_str());
	dlindex idx(global_download_list.pkg_that_contains_download(id), id);
	plugin_status ps = global_download_list.get_error(idx);
	if(ps == PLUGIN_CAPTCHA) {
		captcha *c = global_download_list.get_captcha(idx);
		if(c) {
			c->set_result(answer);
		}
	}
	*sock << "100 SUCCESS";
}

///////////////////////////////////////////////////////
///////// SUBSCRIPTION TARGET START ///////////////////
///////////////////////////////////////////////////////

void target_subscription(std::string &data, client *cl) {
	tkSock *sock = cl->sock;
	if(data.find("ADD") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_subscription_add(data, cl);
	} else if(data.find("DEL") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_subscription_del(data, cl);
	} else if(data.find("LIST") == 0) {
		data = data.substr(4);
		trim_string(data);
		target_subscription_list(data, cl);
	} else {
		*sock << "101 PROTOCOL";
	}

}

void target_subscription_add(std::string &data, client *cl) {
	tkSock *sock = cl->sock;
	trim_string(data);
	connection_manager::subs_type type = connection_manager::string_to_subs(data);
	if(type == connection_manager::SUBS_NONE) {
		*sock << "108 VARIABLE";
		return;
	}
	cl->subscribe(type);
	*sock << "100 SUCCESS";
}

void target_subscription_del(std::string &data, client *cl) {
	tkSock *sock = cl->sock;
	trim_string(data);
	connection_manager::subs_type type = connection_manager::string_to_subs(data);
	if(type == connection_manager::SUBS_NONE) {
		*sock << "108 VARIABLE";
		return;
	}
	cl->unsubscribe(type);
	*sock << "100 SUCCESS";
}

void target_subscription_list(std::string &data, client *cl) {
	tkSock *sock = cl->sock;
	std::vector<connection_manager::subs_type> list = cl->list();
	std::stringstream ss;
	for(size_t i = 0; i < list.size(); ++i) {
		std::string tmp;
		connection_manager::subs_to_string(list[i], tmp);
		ss << tmp;
		if(i != list.size() - 1) ss << "\n";
	}
	*sock << ss.str();
}


