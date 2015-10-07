/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef PACKAGE_EXTRACTOR_H_INCLUDED
#define PACKAGE_EXTRACTOR_H_INCLUDED
#include <config.h>
#include <string>
#include <vector>
#include "../mgmt/connection_manager.h"

#ifndef USE_STD_THREAD
#include <boost/thread.hpp>
namespace std {
	using namespace boost;
}
#else
#include <thread>
#endif

class pkg_extractor {
public:
	enum extract_status { PKG_SUCCESS, PKG_PASSWORD, PKG_INVALID, PKG_ERROR };
	enum tool { GNU_UNRAR, RARLAB_UNRAR, TAR, TAR_BZ2, TAR_GZ, ZIP, NONE, HJSPLIT, SEVENZ };


	static extract_status extract_package(const std::string& filename, const int& id, const std::string& password = "");
	static tool required_tool(std::string filename);
	static tool get_unrar_type();

	static extract_status extract_rar(const std::string& filename, const std::string& password, tool t);
	static extract_status extract_tar(const std::string& filename, tool t);
	static extract_status extract_zip(const std::string& filename, const std::string& password, tool t);
        static extract_status merge_hjsplit(const std::string& filename, tool t);
	static extract_status extract_7z(const std::string& filename, const std::string& password, tool t);
	static extract_status deep_extract(const std::string& targetDir, pkg_extractor::extract_status extract_status, const std::string& password);
        static std::string getTargetDir(const std::string& filename, tool t);
        static std::vector<std::string> getDir(std::string dir);
        
	static void post_subscribers(connection_manager::reason_type reason = connection_manager::UPDATE,std::string temp="");
	static void setLastPostedMessage(std::string message);
	static std::string getLastPostMessage();
private:
	static std::string last_posted_message;
	static int container_id;
	static std::recursive_mutex mx;
};

#endif // PACKAGE_EXTRACTOR_H_INCLUDED
