/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DOWNLOAD_CONTAINER_H_INCLUDED
#define DOWNLOAD_CONTAINER_H_INCLUDED

#include <config.h>
#include "download.h"
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

enum property { DL_ID = 0, DL_DOWNLOADED_BYTES, DL_SIZE, DL_WAIT_SECONDS, DL_PLUGIN_STATUS, DL_STATUS,
				DL_IS_RUNNING, DL_NEED_STOP, DL_SPEED, DL_CAN_RESUME };
enum string_property { DL_URL = 20, DL_COMMENT, DL_ADD_DATE, DL_OUTPUT_FILE };
enum pointer_property { DL_HANDLE = 40 };
enum return_status { LIST_SUCCESS = -20, LIST_PERMISSION, LIST_ID, LIST_PROPERTY };

class download_container {
	friend class package_container;

public:

	/** simple constructor
	*/
	download_container() {}

	download_container(int id, std::string container_name);

	/** Copy-constructor, needed because of package_container and the mutex
	*	@param cnt Container to copy from
	*/
	download_container(const download_container &cnt);

	~download_container();

	/** Check if download list is empty
	*	@returns True if empty
	*/
	bool empty();

	/** Returns the total amount of downloads in the list
	*	@returns download count
	*/
	int total_downloads();

	/** Moves a download upwards
	*	@param id download ID to move up
	*	@returns LIST_ID if it's already the top download, LIST_PERMISSION if the function fails because the dlist can't be written, LIST_SUCCESS on success
	*/
	int move_up(int id);

	/** Moves a download downwards
	*	@param id download ID to move down
	*	@returns LIST_ID if it's already the top download, LIST_PERMISSION if the function fails because the dlist can't be written, LIST_SUCCESS on success
	*/
	int move_down(int id);

	/** Moves a download to the top
	*	@param id download ID
	*	@returns LIST_ID if it's already the top download, LIST_PERMISSION if the function fails because the dlist can't be written, LIST_SUCCESS on success
	*/
	int move_top(int id);

	/** Moves a download to the bottom
	*	@param id download ID to move down
	*	@returns LIST_ID if it's already the bottom download, LIST_PERMISSION if the function fails because the dlist can't be written, LIST_SUCCESS on success
	*/
	int move_bottom(int id);

	/** Activates an inactive download
	*	@param id download ID to activate
	*	@returns LIST_ID if an invalid ID is given, LIST_PROPERTY if the download is already active, LIST_PERMISSION if the dlist file can't be written, LIST_SUCCESS
	*/
	int activate(int id);

	/** Deactivates a download
	*	@param id download ID to deactivate
	*	@returns LIST_ID if an invalid ID is given, LIST_PROPERTY if the download is already inactive, LIST_PERMISSION if the dlist file can't be written, LIST_SUCCESS
	*/
	int deactivate(int id);

	#ifndef IS_PLUGIN
	/** Gets the next downloadable item in the global download list (filters stuff like inactives, wrong time, etc)
 	*	@returns ID of the next download that can be downloaded or LIST_ID if there is none, LIST_SUCCESS
 	*/
	int get_next_downloadable(bool do_lock = true);
	#endif

	/** Adds a new download to the list
	*	@param dl Download object to add
	*	@param dl_id the ID of the new download
	*	@returns LIST_SUCCESS, LIST_PERMISSION
	*/
	int add_download(download *dl, int dl_id);

	/** Adds a download by strings
	*	@param url URL of the download
	*	@param title Title of the download
	*	@returns LIST_SUCCESS, LIST_PERMISSION
	*/
	int add_download(const std::string& url, const std::string& title = "");

	void set_url(int id, std::string url);
	std::string get_url(int id);

	void set_title(int id, std::string title);
	std::string get_title(int id);

	void set_add_date(int id, std::string add_date);
	std::string get_add_date(int id);

	void set_downloaded_bytes(int id, filesize_t bytes);
	filesize_t get_downloaded_bytes(int id);

	void set_size(int id, filesize_t size);
	filesize_t get_size(int id);

	void set_wait(int id, int seconds);
	int get_wait(int id);

	void set_error(int id, plugin_status error);
	plugin_status get_error(int id);

	void set_output_file(int id, std::string output_file);
	std::string get_output_file(int id);

	void set_running(int id, bool running);
	bool get_running(int id);

	void set_need_stop(int id, bool need_stop);
	bool get_need_stop(int id);

	void set_status(int id, download_status status);
	download_status get_status(int id);

	void set_speed(int id, int speed);
	int get_speed(int id);

	void set_can_resume(int id, bool can_resume);
	bool get_can_resume(int id);

	void set_proxy(int id, std::string proxy);
	std::string get_proxy(int id);

	void set_password(const std::string& passwd);
	std::string get_password();

	void set_pkg_name(const std::string& pkg_name);
	std::string get_pkg_name();

	void set_captcha(int id, captcha *cap);
	captcha* get_captcha(int id);

	ddcurl* get_handle(int id);

	/** strip the host from the URL
	*	@param dl Download from which to get the host
	*	@returns the hostname
	*/
	std::string get_host(int dl);

	/** Every download with status DOWNLOAD_WAITING and wait seconds > 0 will decrease wait seconds by one. If 0 is reached, the status will be set to DOWNLOAD_PENDING
	*/
	void decrease_waits();

	/** Removes downloads with a DOWNLOAD_DELETED status from the list, if they can be deleted without danger
	*/
	void purge_deleted();

	/** Creates the list for the DL LIST command
	*	@param header just create header line
	*	@returns the list
	*/
	std::string create_client_list(bool header = false);

	/** Posts the package to all clients that subscribed to SUBS_DOWNLOAD
	*	@param reason reason why the message is sent
	*/
	void post_subscribers(connection_manager::reason_type reason = connection_manager::UPDATE);

	/** Gets the lowest unused ID that should be used for the next download
	*	@returns ID
	*/
	int get_next_id();

	/** Stops a download and sets its status
	*	@param id Download to stop
	*	@returns LIST_SUCCESS, LIST_ID, LIST_PERMISSION
	*/
	int stop_download(int id);

	/** Checks if the given link already exists in the list
	*	@param url The url to check for
	*	@returns true if the link already exists in the list
	*/
	bool url_is_in_list(std::string url);

	/** returns the position in the download-list for a given download ID
	*	@param id ID of the download to get the position for
	*	@returns the position
	*/
	int get_list_position(int id);

	/** inserts a container of downloads into this container
	*	@param pos position where to insert
	*	@param dl download list to insert
	*/
	void insert_downloads(int pos, download_container &dl);

	/** Sets the next proxy from list to the download
	*	@param id ID of the download
	*	@returns 1 If the next proxy has been set
	*		 	 2 if all proxys have been tried already
	*			 3 if there are no proxys at all
	*/
	int set_next_proxy(int id);

	void do_download(int id);

	/** Presets the file-status for all downloads in the list that don't have it yet. */
	bool preset_file_status();

	/** extracts this package */
	void extract_package();

private:
	typedef std::vector<download*>::iterator iterator;
	/** get an iterator to a download by giving an ID
	*	@param id download ID to search for
	*	@returns Iterator to this id
	*/
	download_container::iterator get_download_by_id(int id);

	/** Returns the amount of running download
	*	@returns download count
	*/
	int running_downloads();

	/** Erase a download from the list completely. Not for normal use. always set the status to DOWNLOAD_DELETED instead
	*	@param id ID of the download that should be deleted
	*	@returns success status
	*/
	int remove_download(int id);

	/** sets the status of a download without locking. For use in locking members
	*	@param it Iterator to the download
	*	@param st Status to set
	*/
	//void set_dl_status(download_container::iterator it, download_status st);




	std::vector<download*> download_list;
	std::recursive_mutex download_mutex;
	//std::mutex plugin_mutex; // makes sure that you don't call the same plugin multiple times at the same time
				   // because it would bring thread-safety problems
	int container_id;
	std::string name;
	std::string password; // password for rar-extraction
	std::string last_posted_message;
	bool subs_enabled;
};


#endif // DOWNLOAD_CONTAINER_H_INCLUDED
