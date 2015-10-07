/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DOWNLOADC_H
#define DOWNLOADC_H

#include <config.h>
#ifndef USE_STD_THREAD
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
namespace std {
    using namespace boost;
}
#else
#include <thread>
#include <mutex>
#endif

#ifdef HAVE_SYSLOG_H
        #include <syslog.h>
#else
        enum { LOG_ERR, LOG_WARNING, LOG_DEBUG };
#endif

#include <vector>
#include <map>
#include <utility>
#include <string>
#include <netpptk/netpptk.h>
#include <downloadc/client_exception.h>
#include <stdint.h>


/** Enum file_delete, manages the three options for deleting a file */
enum file_delete{ del_file = 0, dont_delete, dont_know };

/** Subcription Type, describes what to subscribe to. */
enum subs_type{ SUBS_NONE = 0, SUBS_DOWNLOADS, SUBS_CONFIG };

/** Reason Type, describes the reason for an update. */
enum reason_type{ R_UPDATE = 0, R_NEW, R_DELETE, R_MOVEUP, R_MOVEDOWN, R_MOVEBOTTOM, R_MOVETOP };

/** Update Content Struct, includes all the content of one update message */
struct update_content{

	subs_type sub;
	reason_type reason;

	// if sub == SUBS_CONFIG this is filled
	std::string var_name;
	std::string value;

	// if sub == SUBS_DOWNLOADS some of these are filled
	bool package; // package attributes are filled or not (contrary to download attributes)
	int id;

	// dl attributes
	int pkg_id;
	std::string date;
	std::string title;
	std::string url;
	std::string status;
	uint64_t downloaded;
	uint64_t size;
	int wait;
	std::string error;
	int speed;

	// package attributes
	std::string name;
	std::string password;
};

/** Download Struct, defines one single Download */
struct download{
    int id;
    std::string date;
    std::string title;
    std::string url;
    std::string status;
    uint64_t downloaded;
    uint64_t size;
    int wait;
    std::string error;
    int speed;

	download(){}

	download(const update_content &other){ // cast operator from update_content to download
		id = other.id;
		date = other.date;
		title = other.title;
		url = other.url;
		status = other.status;
		downloaded = other.downloaded;
		size = other.size;
		wait = other.wait;
		error = other.error;
		speed = other.speed;
	}
};

/** Package Struct, defines a Package containing an ID and Downloads */
struct package{
    int id;
    std::string name;
    std::vector<download> dls;
    std::string password;

	package(){}

	package(const update_content &other){ // cast operator from update_content to package
		id = other.id;
		name = other.name;
		password = other.password;
	}
};

/** DownloadClient Class, makes communication with DownloadDaemon easier */
class downloadc{
    public:

        /** Defaultconstructor */
        downloadc();

        /** Destructor */
        ~downloadc();

        /** Sets the bool term to value to signalize that get_updates() should terminate or not
        *   @param value value of term
        */
        void set_term(bool value);

        /** Connects to a DownloadDaemon, successful or not shown by exception
        *    @param host ip where the daemon runs
        *    @param port port where the daemon runs
        *    @param pass password of daemon
        *    @param encrypt force encryption for sending password or not
        */
        void connect(std::string host = "127.0.0.1", int port = 56789, std::string pass = "", bool encrypt = false);

        /** Waits for updates and receives them
        *   @returns updates as a string
        */
        std::vector<update_content> get_updates();


        // target DL
        /** Returns a list of all downloads
        *    @returns list of downloads
        */
        std::vector<package> get_list();

        /** Adds a download, successful or not shown by exception
        *    @param package package where the download should be in
        *    @param url URL of download
        *    @param title title of download
        */
        void add_download(int package, std::string url, std::string title="");

        /** Deletes a download, successful or not shown by exception
        *    @param id ID of the download
        *    @param delete_file shows how to handle downloaded files of the download to be deleted
        */
        void delete_download(int id, file_delete = dont_know);

        /** Stops a download, successful or not shown by exception
        *    @param id ID of the download to be stopped
        */
        void stop_download(int id);

        /** Sets priority of a download up, successful or not shown by exception
        *   @param id ID of the download
        */
        void priority_up(int id);

        /** Sets priority of a download down, successful or not shown by exception
        *   @param id ID of the download
        */
        void priority_down(int id);

        /** Set priority of a download to the top of the list, successful or not shown by exception
          * @param id ID of the download
        */
        void priority_top(int id);

        /** Set priority of a download to the bottom of the list, successful or not shown by exception
          * @param id ID of the download
        */
        void priority_bottom(int id);

        /** Activates a download, successful or not shown by exception
        *   @param id ID of the download
        */
        void activate_download(int id);

        /** Deactivates a download, successful or not shown by exception
        *   @param id ID of the download
        */
        void deactivate_download(int id);

        /** Setter for download variables, successful or not shown by exception
        *   @param id download ID
        *   @param var Name of the variable
        *   @param value Value to be set
        */
        void set_download_var(int id, std::string var, std::string value);

        /** Getter for download variables
        *   @param download package ID
        *   @param var variable which value should be returned
        *   @returns value or empty string
        */
        std::string get_download_var(int id, std::string var);


        // target PKG
        /** Returns a list of all packages without downloads
        *   @returns list of packages
        */
        std::vector<package> get_packages();

        /** Adds a package, successful or not shown by exception
        *   @param name optional name of the package
        *   @returns ID of the new package
        */
        int add_package(std::string name = "");

        /** Deletes a package
        *   @param id ID of the package
        */
        void delete_package(int id);

        /** Sets priority of a package up
        *   @param id ID of the package
        */
        void package_priority_up(int id);

        /** Sets priority of a package down
        *   @param id ID of the package
        */
        void package_priority_down(int id);

		/** Sets priority of a package to the top of the list
        *   @param id ID of the package
        */
        void package_priority_top(int id);

		/** Sets priority of a package to the bottom of the list
        *   @param id ID of the package
        */
        void package_priority_bottom(int id);

        /** Checks the existance of a package
        *   @param id ID of the package
        *   @param true if package exists
        */
        bool package_exists(int id);

        /** Setter for package variables, successful or not shown by exception
        *   @param id package ID
        *   @param var Name of the variable
        *   @param value Value to be set
        */
        void set_package_var(int id, std::string var, std::string value);

        /** Getter for package variables
        *   @param id package ID
        *   @param var variable which value should be returned
        *   @returns value or empty string
        */
        std::string get_package_var(int id, std::string var);

        /** Creates a packages and puts the contents of the container into it
        *   @param type container type
        *   @param content content of the container
        */
        void pkg_container(std::string type, std::string content);


        // target VAR
		/** Returns a map of all variables with values and an indicator if it's settable or not
		*	@returns map with value pair<string, bool>, where the first value is the value and the second indicates if it's settable.
		*/
		std::map<std::string, std::pair<std::string, bool> > get_var_list();

		/** Setter for variables, successful or not shown by exception
        *   @param var Name of the variable
        *   @param value Value to be set
        *   @param value Optional old value, needed if you set variable mgmt_password
        */
        void set_var(std::string var, std::string value, std::string old_value = "");

        /** Getter for variables
        *   @param var variable which value should be returned
        *   @returns value or empty string
        */
        std::string get_var(std::string var);


        // target FILE
        /** Deletes a downloaded file, successful or not shown by exception
        *   @param id ID of the file to be deleted
        */
        void delete_file(int id);

        /** Returns the file as binary data, NOT IMPLEMENTED BY DAEMON, DO NOT USE!
        *   @param id ID of the file to be sent
        */
        void get_file(int id);

        /** Returns local path of a file
        *   @param id ID of the file
        *   @returns path or empty string if it fails
        */
        std::string get_file_path(int id);

        /** Returns size of a file in byte
        *   @param id ID of the file
        *   @returns filesize in byte
        */
        uint64_t get_file_size(int id);


        // target ROUTER
        /** Returns the router list, successful or not shown by exception
        *   @returns router list in a vector
        */
        std::vector<std::string> get_router_list();

        /** Sets the router model, successful or not shown by exception
        *   @param model model to be set
        */
        void set_router_model(std::string model);

        /** Setter for router variables, successful or not shown by exception
        *   @param var Name of the variable
        *   @param value Value to be set
        */
        void set_router_var(std::string var, std::string value);

        /** Getter for router variables
        *   @param var variable which value should be returned
        *   @returns value or empty string
        */
        std::string get_router_var(std::string var);


        // target PREMIUM
        /** Returns the premium list, successful or not shown by exception
        *   @returns premium list in a vector
        */
        std::vector<std::string> get_premium_list();

        /** Setter for premium host, user and password, successful or not shown by exception
        *   @param host premiumhost
        *   @param user username
        *   @param password password
        */
        void set_premium_var(std::string host, std::string user, std::string password);

        /** Getter for premium username
        *   @param host host which the username is for
        *   @returns value or empty string
        */
        std::string get_premium_var(std::string host);


        // target SUBSCRIPTION
        /** Returns the subscription list, successful or not shown by exception
        *   @returns subscription list in a vector
        */
        std::vector<std::string> get_subscription_list();

        /** Adds a subscription, successful or not shown by exception
        *   @param type subscription type to add
        */
        void add_subscription(subs_type type);

        /** Removes a subscription, successful or not shown by exception
        *   @param type subscription type to delete
        */
        void remove_subscription(subs_type type);


		// target CAPTCHA
		/** Returns a captcha image for a specified download
		*	@param id ID of download of requested captcha
		*	@param type File Type of returned image
		*	@param question Question asked with captcha if available
		*	@returns Image information in binary form
		*/
		std::string get_captcha(int id, std::string &type, std::string &question);

		/** Resolve a captcha with the given answer
		*	@param id ID of download
		*	@param answer Captcha answer
		*/
		void captcha_resolve(int id, std::string answer);


        // helper functions
        /** Checks connection, successful or not shown by exception*/
        void check_connection();

        /** Splits a string into many strings, seperating them with the seperator
        *    @param inp_string string to split
        *    @param seperator Seperator to use for splitting
		*	 @param respect_escape Respect an escape sequence
		*	 @param max_elements Just split into a maximal number of elements
        *    @returns Vector of all the strings
        */
		static std::vector<std::string> split_string(const std::string& inp_string, const std::string& seperator, bool respect_escape = false, int max_elements = -1);

        /** Remove whitespaces from beginning and end of a string
        *   @param str string to process
        *   @returns the same string
        */
        static const std::string& trim_string(std::string &str);

    private:

        tkSock *mysock;
        std::recursive_mutex mx;
        bool skip_update;
        bool term;
        std::vector<std::string> old_updates;

        void split_special_string(std::vector<std::vector<std::string> > &new_content, std::string &answer);
        void check_error_code(std::string check_me);
        bool check_correct_answer(std::string check_me);
        void subs_to_string(subs_type t, std::string &ret);
};

#endif // DOWNLOADC_H
