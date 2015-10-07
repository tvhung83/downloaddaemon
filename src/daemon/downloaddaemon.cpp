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
#include <cfgfile/cfgfile.h>
#include "mgmt/mgmt_thread.h"
#include "dl/download.h"
#include "dl/package_container.h"
#include "dl/plugin_container.h"
#include "mgmt/global_management.h"
#include "tools/helperfunctions.h"
#include "mgmt/connection_manager.h"

#ifndef USE_STD_THREAD
#include <boost/thread.hpp>
namespace std {
	using namespace boost;
}
#else
#include <thread>
#include <mutex>
#endif

#include <iostream>
#include <vector>
#include <string>
#include <climits>
#include <cstdlib>
#include <sstream>
#include <map>
#include <utility>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <curl/curl.h>

#ifndef HAVE_UINT64_T
	#warning You do not have the type uint64_t. this is pretty bad and you might get problems when you download big files.
	#warning This includes that DownloadDaemon may display wrong download sizes/progress. But downloading should work.. maybe...
#endif

#ifndef HAVE_INITGROUPS
	#warning "Your compiler doesn't offer the initgroups() function. This is a problem if you make downloaddaemon should download to a folder that"\
		 "is only writeable for a supplementary group of the downloadd user, but not to the downloadd user itsself."
#endif
		 
#define DAEMON_USER downloadd

using namespace std;

#ifndef DD_CONF_DIR
	#define DD_CONF_DIR /etc/downloaddaemon/
#endif

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

// GLOBAL VARIABLE DECLARATION:
// The downloadcontainer is just needed everywhere in the program, so let's make it global
package_container global_download_list;
plugin_container plugin_cache;

// configuration variables are also used a lot, so global too
cfgfile global_config;
cfgfile global_router_config;
cfgfile global_premium_config;
// same goes for the program root, which is needed for a lot of path-calculation
std::string program_root;
// the environment variables are also needed a lot for path calculations
char** env_vars;

int main(int argc, char* argv[], char* env[]) {
	env_vars = env;
	#ifdef BACKTRACE_ON_CRASH
	signal(SIGSEGV, print_backtrace);
	signal(SIGPIPE, SIG_IGN);
	#endif

	string conf_dir = STRINGIZE(DD_CONF_DIR);
	string daemon_user = STRINGIZE(DAEMON_USER);
	string daemon_group = STRINGIZE(DAEMON_USER);
	string pid_path = "/tmp/downloadd.pid";
	map<string, string> args;
	// typedef map<string, string>::iterator argIter;
	for(int i = 1; i < argc; ++i) {
		std::string arg = argv[i-1];
		std::string val = argv[i];
		if(val == "--help" || val == "-h") {
			cout << "Usage: downloaddaemon [options]" << endl << endl;
			cout << "Options:" << endl;
			cout << "   -d, --daemon   Start DownloadDaemon in Background" << endl;
			cout << "   --confdir      Use the configuration files in the specified directory" << endl;
			cout << "   --version      Print the version number of DownloadDaemon and exit" << endl;
			#ifndef __CYGWIN__
			cout << "   -u             Start DownloadDaemon as the specified user" << endl;
			cout << "   -g             Start DownloadDaemon with the permissions of a group" << endl;
			cout << "   -p             Specify the path of DownloadDaemons pid-file" << endl;
			#endif
			return 0;
		} else if(val == "-d" || val == "--daemon") {
			args.insert(pair<string, string>("--daemon", ""));
			continue;
		} else if(val == "--version" || val == "-v") {
			cout << "DownloadDaemon version " STRINGIZE(DOWNLOADDAEMON_VERSION) << endl;
			return 0;
		}
		if(i >= 2) {
			args.insert(pair<string, string>(arg, argv[i]));
		}
	}

	if(args.find("--confdir") != args.end()) conf_dir = args["--confdir"];
	if(args.find("-u") != args.end()) daemon_user = args["-u"];
	if(args.find("-g") != args.end()) daemon_group = args["-g"];
	if(args.find("-p") != args.end()) pid_path = args["-p"];

	correct_path(conf_dir);
	correct_path(pid_path);

	#ifndef __CYGWIN__
	if(getuid() == 0 || geteuid() == 0 || args.find("-u") != args.end() || args.find("-g") != args.end()) {
		struct passwd *pw = getpwnam(daemon_user.c_str());
		#ifdef HAVE_INITGROUPS
		if(pw && initgroups(daemon_user.c_str(), pw->pw_gid)) {
			std::cerr << "Setting the groups of the DownloadDaemon user failed. This is a problem if you make downloaddaemon should download to a folder that "
					  << " is only writeable for a supplementary group of DownloadDaemon, but not to the " STRINGIZE(DAEMON_USER) " user itsself." << endl;
		}
		#endif
		if((getuid() == 0 || geteuid() == 0) && !pw) {
			std::cerr << "Never run DownloadDaemon as root!" << endl;
			if(args.find("-u") == args.end()) cout << "Unable to find the specified user!" << endl;
			std::cerr << "In order to run DownloadDaemon, please run it as a normal user OR run it with the \"-u\" option" << endl;
			std::cerr << "OR execute these commands as root:" << endl;
			std::cerr << "   addgroup --system " + daemon_user << endl;
			std::cerr << "   adduser --system --ingroup " + daemon_user + " --home " + conf_dir + " " + daemon_user << endl;
			std::cerr << "   chown -R " + daemon_user + ":" + daemon_group + " " + conf_dir + " /var/downloads" << endl;
			std::cerr << "then rerun DownloadDaemon. It will automatically change its UID to the ones of " + daemon_user + "." << endl;
			exit(-1);
		} else if(getuid() == 0 || geteuid() == 0) {
			struct group* grpinfo = getgrnam(daemon_group.c_str());
			gid_t gid_to_use = 0;
			if(grpinfo) {
				gid_to_use = grpinfo->gr_gid;
			} else {
				gid_to_use = pw->pw_gid;
			}

			if(setgid(gid_to_use) != 0 || setuid(pw->pw_uid) != 0) {
				std::cerr << "Failed to set user-id or group-id. Please run DownloadDaemon as a user manually." << endl;
				exit(-1);
			}
		}
	} else {
		#ifdef HAVE_INITGROUPS
		struct passwd *pw = getpwuid(getuid());
		initgroups(pw->pw_name, getgid());
		#endif
	}

	#endif
	#ifndef __CYGWIN__
	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 1;
	int fdlock;

	if((fdlock = open(pid_path.c_str(), O_WRONLY|O_CREAT, 0777)) < 0 || fcntl(fdlock, F_SETLK, &fl) == -1) {
		std::cerr << "DownloadDaemon is already running. Exiting this instance" << endl;
		exit(0);
	} else {
		string pid = int_to_string(getpid());
		write(fdlock, pid.c_str(), pid.size());
	}

	// need to chmod seperately because the permissions set in open() are affected by the umask. chmod() isn't
	fchmod(fdlock, 0777);
	#else
	string p_tmp = getenv("PATH");
	p_tmp += ":/bin:.";
	setenv("PATH", p_tmp.c_str(), 1);
	#endif

	struct pstat st;

	// getting working dir
	std::string argv0 = argv[0];
	program_root = argv0;

	if(argv0[0] != '/' && argv0[0] != '\\' && (argv0.find('/') != string::npos || argv0.find('\\') != string::npos)) {
		// Relative path.. sux, but is okay.
		char* c_old_wd = getcwd(0, 0);
		std::string wd = c_old_wd;
		free(c_old_wd);
		wd += '/';
		wd += argv0;
		//
		program_root = wd;
		correct_path(program_root);
	} else if(argv0.find('/') == string::npos && argv0.find('\\') == string::npos && !argv0.empty()) {
		// It's in $PATH... let's go!
		std::string env_path(get_env_var("PATH"));

		std::string curr_path;
		bool found = false;
		
		vector<string> paths = split_string(env_path, ":");
		if(paths.size() == 1) paths = split_string(env_path, ";");
		for(vector<string>::iterator it = paths.begin(); it != paths.end(); ++it) {
			curr_path = *it + "/" + argv[0];
			if(pstat(curr_path.c_str(), &st) == 0) {
				found = true;
				break;
			}
		}

		if(found) {
			// successfully located the file..
			// resolve symlinks, etc
			program_root = curr_path;
			correct_path(program_root);
		} else if(!getenv("DD_DATA_DIR")){
			cerr << "Unable to locate executable!" << endl;
			exit(-1);
		}
	} else if(argv0.empty()) {

	}

	// else: it's an absolute path.. nothing to do - perfect!
	program_root = program_root.substr(0, program_root.find_last_of("/\\"));
	program_root = program_root.substr(0, program_root.find_last_of("/\\"));
	program_root.append("/share/downloaddaemon/");
	if(getenv("DD_DATA_DIR")) {
		program_root = getenv("DD_DATA_DIR");
		correct_path(program_root);
	}

	if(pstat(program_root.c_str(), &st) != 0) {
		#ifdef __CYGWIN__
			if(program_root.find("/usr/share/") != string::npos) {
				program_root = "/share/downloaddaemon/";
			}
		#else
			cerr << "Unable to locate program data (should be in bindir/../share/downloaddaemon)" << endl;
			cerr << "We were looking in: " << program_root << endl;
			exit(-1);
		#endif
	}
	chdir(program_root.c_str());
	{
		string dd_conf_path(conf_dir + "/downloaddaemon.conf");
		string premium_conf_path(conf_dir + "/premium_accounts.conf");
		string router_conf_path(conf_dir + "/routerinfo.conf");


		// check again - will fail if the conf file does not exist at all
		if(pstat(dd_conf_path.c_str(), &st) != 0) {
			cerr << "Could not locate configuration file!" << endl;
			exit(-1);
		}

		global_config.open_cfg_file(dd_conf_path.c_str(), true);
		global_router_config.open_cfg_file(router_conf_path.c_str(), true);
		global_premium_config.open_cfg_file(premium_conf_path.c_str(), true);
		if(!global_config) {
			uid_t uid = geteuid();
			struct passwd *pw = getpwuid(uid);
			std::string unam = "downloadd";
			if(pw) {
				unam = pw->pw_name;
			}

			cerr << "Unable to open config file!" << endl;
			cerr << "You probably don't have enough permissions to write the configuration file. executing \"chown -R "
				 << unam << ":downloadd " + conf_dir + " /var/downloads\" might help" << endl;
			exit(-1);
		}
		if(!global_router_config) {
			cerr << "Unable to open router config file" << endl;
		}
		if(!global_premium_config) {
			cerr << "Unable to open premium account config file" << endl;
		}

		global_config.set_default_config(program_root + "/dd_default.conf");
		global_router_config.set_default_config(program_root + "/router_default.conf");
		global_premium_config.set_default_config(program_root + "/premium_default.conf");


		std::string dlist_fn = global_config.get_cfg_value("dlist_file");
		correct_path(dlist_fn);

		// set the daemons umask
		std::stringstream umask_ss;
		umask_ss << std::oct;
		umask_ss << global_config.get_cfg_value("daemon_umask");
		int umask_i = 0;
		umask_ss >> umask_i;
		umask(umask_i);
		connection_manager::create_instance();

		// Create the needed folders

		string dl_folder = global_config.get_cfg_value("download_folder");
		//string log_file = global_config.get_cfg_value("log_file");
		string dlist = global_config.get_cfg_value("dlist_file");
		correct_path(dl_folder);
		mkdir_recursive(dl_folder);
		correct_path(dlist);
		dlist = dlist.substr(0, dlist.find_last_of('/'));
		mkdir_recursive(dlist);

		global_mgmt::ns_mutex.lock();
		global_mgmt::curr_start_time = global_config.get_cfg_value("download_timing_start");
		global_mgmt::curr_end_time = global_config.get_cfg_value("download_timing_end");
		global_mgmt::downloading_active = global_config.get_bool_value("downloading_active");
		global_mgmt::ns_mutex.unlock();
		
		if(args.find("--daemon") != args.end()) {
			int j = fork();
			if (j < 0) return 1; /* fork error */
			if (j > 0) return 0; /* parent exits */
			/* child (daemon) continues */
			setsid();	
		}
		plugin_cache.load_plugins();
		stringstream plglog;
		for(plugin_container::handleIter it = plugin_cache.handles.begin(); it != plugin_cache.handles.end(); ++it) {
			plglog << it->first << " ";
		}
		global_download_list.from_file(dlist_fn.c_str());
		curl_global_init(CURL_GLOBAL_SSL);

		thread mgmt_thread(mgmt_thread_main);
		mgmt_thread.detach();

		// tick download counters, start new downloads, etc each second
		thread once_per_sec_thread(do_once_per_second);
		once_per_sec_thread.detach();
		thread sync_sig_handler(sig_handle_thread);
		sync_sig_handler.detach();

		log_string("DownloadDaemon started successfully with these plugins: " + plglog.str(), LOG_DEBUG);

		global_download_list.start_next_downloadable();

	}
	while(true) {
		sleep(1);
		global_mgmt::once_per_sec_cond.notify_one();
	}
}
