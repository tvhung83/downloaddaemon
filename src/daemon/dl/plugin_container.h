/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef PLUGIN_CONTAINER_H_INCLUDED
#define PLUGIN_CONTAINER_H_INCLUDED

#include <config.h>
#include <string>
#include <vector>
#include <map>

#include "download.h"
#include <dlfcn.h>
#include "../tools/helperfunctions.h"

#ifndef USE_STD_THREAD
#include <boost/thread.hpp>
namespace std {
	using namespace boost;
}
#else
#include <thread>
#endif

struct plugin {
	std::string host;
	std::string rhost;

	bool allows_multiple;
	bool allows_resumption;
	bool offers_premium;
};


class plugin_container {
public:
	enum p_info { P_HOST, P_FILE };

	plugin_output get_info(const std::string& info, p_info kind);
	const std::vector<plugin>& get_all_infos();



	void clear_cache();

	void load_plugins();
	std::map<std::string, void*> handles;
	typedef std::map<std::string, void*>::iterator handleIter;

	std::string real_host(std::string host);

	void* operator[](std::string plg);

	template <typename func>
	bool load_function(const std::string &host, const std::string& sym, func& ret, bool log_error = true) {
		void* h = operator[](host);
		if(!h) return false;
		ret = (func)dlsym(h, sym.c_str());
		const char *dl_error;
		if ((dl_error = dlerror()) != NULL && log_error)  {
			log_string(std::string("Unable to execute plugin function: ") + dl_error, LOG_ERR);
			return false;
		}
		if (dl_error || !ret) return false;

		return true;
	}

	~plugin_container();

private:

	std::pair<bool, plugin> search_info_in_cache(const std::string& info);

	std::vector<plugin> plugin_cache;
	mutable std::recursive_mutex mx;


};

#endif // PLUGIN_CONTAINER_H_INCLUDED
