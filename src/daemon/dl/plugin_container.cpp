/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "plugin_container.h"
#include "../global.h"
#include "../tools/helperfunctions.h"
#include "download.h"

#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>

#include <string>
#include <vector>

using namespace std;

plugin_container::~plugin_container() {
	unique_lock<recursive_mutex> lock(mx);
	for(handleIter it = handles.begin(); it != handles.end(); ++it) {
		if(it->second) dlclose(it->second);
	}
	handles.clear();
}

plugin_output plugin_container::get_info(const std::string& info, p_info kind) {
	unique_lock<recursive_mutex> lock(mx);

	string host = info;
	if(kind == P_FILE) {
		try {
			size_t pos = info.rfind("lib") + 3;
			host = info.substr(pos, info.size() - 3 - pos);
		} catch (...) {
			log_string("plugin_container::get_info received invalid info: " + host + " which is of kind " + int_to_string((int)kind) + ". Please report this error.", LOG_ERR);
		}
	}

	std::pair<bool, plugin> ret = search_info_in_cache(host);
	plugin_output outp;
	if(ret.first) {
		// plugin is already in the cache - we just have to return the info.
		outp.allows_multiple = ret.second.allows_multiple;
		outp.allows_resumption = ret.second.allows_resumption;
		outp.offers_premium = ret.second.offers_premium;
		return outp;
	}

	// put the plugin in the cache and return info


	plugin_input inp;

	outp.allows_resumption = false;
	outp.allows_multiple = false;
	outp.offers_premium = false;

	if(handles.size() == 0) load_plugins();
	// Load the plugin function needed
	handleIter it = handles.find(host);

	if(host.empty() || it == handles.end()) {
		// no plugin found, using the default values
		outp.allows_resumption = true;
		outp.allows_multiple = true;
		return outp;
	}

	void (*plugin_getinfo)(plugin_input&, plugin_output&);
	plugin_getinfo = (void (*)(plugin_input&, plugin_output&))dlsym(it->second, "plugin_getinfo");

	try {
		string rhost = real_host(host);
		inp.premium_user = global_premium_config.get_cfg_value(rhost + "_user");
		inp.premium_password = global_premium_config.get_cfg_value(rhost + "_password");
		plugin_getinfo(inp, outp);

		plugin new_p;
		new_p.allows_multiple = outp.allows_multiple;
		new_p.allows_resumption = outp.allows_resumption;
		new_p.host = rhost;
		new_p.offers_premium = outp.offers_premium;
		plugin_cache.push_back(new_p);

	} catch(std::exception &e) { // nothing to do. It's just that the plugin won't be cached
	}
	return outp;
}


std::string plugin_container::real_host(std::string host) {
	unique_lock<recursive_mutex> lock(mx);
	if(host == "") {
		return "";
	}
	for(size_t i = 0; i < host.size(); ++i) host[i] = tolower(host[i]);

	if(handles.empty()) load_plugins();

	for(handleIter it = handles.begin(); it != handles.end(); ++it) {

		if(host.find(it->first) != string::npos) {
		   return it->first;
		}
	}
	return "";
}

void plugin_container::load_plugins() {
	unique_lock<recursive_mutex> lock(mx);
	string plugindir = program_root + "/plugins/";
	correct_path(plugindir);

	DIR *dp;
	struct dirent *ep;
	dp = opendir(plugindir.c_str());
	if (dp == NULL) {
		log_string("Could not open plugin directory!", LOG_ERR);
		return;
	}

	for(handleIter it = handles.begin(); it != handles.end(); ++it) {
		if(it->second) dlclose(it->second);
	}

	handles.clear();

	struct pstat st;

	while ((ep = readdir(dp))) {
		if(ep->d_name[0] == '.') {
			continue;
		}
		string current = plugindir + '/' + ep->d_name;

		if(pstat(current.c_str(), &st) != 0) continue;

		if(current.find("lib") == string::npos || current.rfind(".so") != current.size() - 3)
			continue;

		// Load the plugin needed
		void* l_handle = dlopen(current.c_str(), RTLD_NOW | RTLD_LOCAL);
		if (!l_handle) {
			log_string(std::string("Unable to open plugin file: ") + dlerror() + '/' + current, LOG_ERR);
			continue;
		}

		dlerror();	// Clear any existing error
		string host = ep->d_name;
		host = host.substr(3, host.size() - 6); // strip "lib" and ".so"
		for(size_t i = 0; i < host.size(); ++i) host[i] = tolower(host[i]);
		handles[host] = l_handle;


	}

	closedir(dp);

}

const std::vector<plugin>& plugin_container::get_all_infos() {
	DIR *dp;
	struct dirent *ep;
	std::string plugindir = program_root + "/plugins/";
	correct_path(plugindir);
	plugindir += '/';
	dp = opendir(plugindir.c_str());
	if(!dp) {
		log_string("Could not open plugin directory!", LOG_ERR);
		return plugin_cache;
	}
	while((ep = readdir(dp))) {
		if(ep->d_name[0] == '.') {
			continue;
		}
		string current = ep->d_name;
		if(current.find("lib") == string::npos || current.find(".so") == string::npos) continue;
		current = plugindir + current;
		get_info(current, P_FILE);
	}
	return plugin_cache;
}

std::pair<bool, plugin> plugin_container::search_info_in_cache(const std::string& info) {
	for(vector<plugin>::iterator it = plugin_cache.begin(); it != plugin_cache.end(); ++it) {
		if(info.find(it->host) != std::string::npos) {
			return pair<bool, plugin>(true, *it);
		}
	}
	plugin p;
	return pair<bool, plugin>(false, p);
}

void* plugin_container::operator[](std::string plg) {
	unique_lock<recursive_mutex> lock(mx);
	for(size_t i = 0; i < plg.size(); ++i) plg[i] = tolower(plg[i]);
	if(handles.empty()) load_plugins();
	for(handleIter it = handles.begin(); it != handles.end(); ++it) {
		if(plg.find(it->first) != string::npos) {
			return it->second;
		}
	}
	return NULL;
}

void plugin_container::clear_cache() {
	unique_lock<recursive_mutex> lock(mx);
	plugin_cache.clear();
}
