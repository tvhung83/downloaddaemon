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
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>

#include <curl/curl.h>

#include "../dl/download.h"
#include "../dl/download_container.h"
#include "../tools/helperfunctions.h"
#include <cfgfile/cfgfile.h>
#include "global_management.h"
#include "../global.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
using namespace std;

namespace global_mgmt {
	std::recursive_mutex ns_mutex;
	std::mutex once_per_sec_mutex;
	std::condition_variable once_per_sec_cond;
	std::string curr_start_time;
	std::string curr_end_time;
	bool downloading_active;

	bool presetter_running = false;
	// for sync signal handling
	std::condition_variable sig_handle_cond;
	int curr_sig = -1;
	char **backtrace;
	int backtrace_size = 0;
}


void do_once_per_second() {
	bool was_in_dltime_last = global_download_list.in_dl_time_and_dl_active();
	unique_lock<mutex> lock(global_mgmt::once_per_sec_mutex);
	while(true) {
		global_mgmt::once_per_sec_cond.wait(lock);
		global_download_list.purge_deleted();

		global_mgmt::ns_mutex.lock();

		if(global_download_list.total_downloads() > 0) {
			global_download_list.decrease_waits();
			//if (global_mgmt::start_presetter && global_config.get_bool_value("precheck_links")) {
			//	thread t(bind(&package_container::preset_file_status, &global_download_list));
			//	t.detach();
			//}
		}
		global_mgmt::ns_mutex.unlock();

		bool in_dl_time = global_download_list.in_dl_time_and_dl_active();
		if(!was_in_dltime_last && in_dl_time) {
			// we just reached dl_start time
			global_download_list.start_next_downloadable();
			was_in_dltime_last = true;
		}

		if(was_in_dltime_last && !in_dl_time) {
			// we just left dl_end time
			was_in_dltime_last = false;
		}

	}
}

void sig_handle_thread() {
	mutex sig_handle_mutex;
	unique_lock<mutex> lock(sig_handle_mutex);
	while(true) {
		global_mgmt::sig_handle_cond.wait(lock);
		switch(global_mgmt::curr_sig) {
			case SIGSEGV:
				#ifdef BACKTRACE_ON_CRASH
				log_string("DOWNLOADDAEMON CRASH DETECTED. BACKTRACE:", LOG_ERR);
				for (int i = 0; i < global_mgmt::backtrace_size; ++i) {
					log_string(global_mgmt::backtrace[i], LOG_ERR);
				}
				global_mgmt::curr_sig = -1;
				#endif
				exit(-1);
			break;

		}

	}
}
