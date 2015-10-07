/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CFGFILE_H_
#define CFGFILE_H_

#include <config.h>
#include <fstream>
#include <string>
#include <map>

#ifndef DDCLIENT_GUI
    #ifndef USE_STD_THREAD
	#include <boost/thread.hpp>
	namespace std {
	    using namespace boost;
	}
    #else
	#include <thread>
	#include <mutex>
    #endif
#endif


class cfgfile {
public:
	/** Default constructor: Constructs the Object with default values:
 	* Token for comments: #
 	* Read Only file
 	* assignment token: =
 	*/
	cfgfile();

	/** Constructs the object and opens a config file. The User can specify if the file should be
	*  opened with write-access. Comment token is #
	*	@param fp Path to the config-file
	*	@param open_writeable specifies rw/ro
	*/
	cfgfile(std::string &fp, bool open_writeable);

	/** The full constructor with every initializable element.
 	*	@param fp Path to the config-file
 	*	@param comm_token Token used for comments
 	*	@param eq_token Token used for assignemnts
 	*	@param open_writeable specifies rw/ro
 	*/
	cfgfile(std::string &fp, std::string &comm_token, char eq_token, bool open_writeable);

	/** Destructor - closes the file */
	~cfgfile();

	/** sets a default config file. If something isn't found in the normal config file
	*   we will use this to get the value and then save it in the normal config file if
	*   it is writeable
	*	@param defconf filename of the default config file
	*/
	void set_default_config(const std::string &defconf);

	/** read the filename of the default config file
	*	@returns the filename
	*/
	std::string get_default_config();


	/** Opens the cfgfile, if the object was constructed without specifying one.
 	*  Also, the file can be changed, if there is already one opened.
 	*	@param fp Path to the config-file
 	*	@param open_writeable specifies rw/ro
 	*/
	void open_cfg_file(const std::string &fp, bool open_writeable);


	/** Returns a configuration options found in a file as a std::string
 	*	@param cfg_identifier specifies the configruation option to search for
 	*	@returns Configuration value, or, if none found or no file is opened, an empty string
 	*/
	std::string get_cfg_value(const std::string &cfg_identifier);

	/** Returns the configuration option as a bool (default: false)
 	*	@param cfg_identifier specifies the configruation option to search for
 	*	@returns Configuration value, or, if none found or no file is opened, false
 	*/
 	bool get_bool_value(const std::string &cfg_identifier);

	/** Returns the configuration option as an int (default: 0)
 	*	@param cfg_identifier specifies the configruation option to search for
 	*	@returns Configuration value, or, if none found or no file is opened, 0
 	*/
 	long get_int_value(const std::string &cfg_identifier);

	/** Sets a configuration option. If the identifier already exists, the value is changed. Else, the option is appended.
 	*	@param cfg_identifier Identifier to search for/add
 	*	@param cfg_value the value that should be set
 	*  @returns true on success, else false
 	*/
	bool set_cfg_value(const std::string &cfg_identifier, const std::string &value);

	/** Get comment token
 	*	@returns current comment token
 	*/
	std::string get_comment_token() const;

	/** Sets the comment token that should be used
 	*	@param comment_token Token to set
 	*/
	void set_comment_token(const std::string &comment_token);

	/** reloads the file to make changes visible */
	void reload_file();

	/** closes the file */
	void close_cfg_file();

	/** Get writeable status
 	*	@returns True if writeable, false if RO
 	*/
	bool writeable() const;

	/** Get the filepath
	 *	@returns The Filepath to the config-file
	*/
	std::string get_filepath() const;

	/** Get a config variable list
 	*	@param resultstr A string in which the result is stored line by line
 	*	@returns true on success
 	*/
	bool list_config(std::string& resultstr);

	operator bool() { return file.good(); }

private:
	/** Internal use: removes whitespaces from the beginning and end of a string.
	*  @param str String to change
	*/
	void trim(std::string &str) const;

	std::fstream file;
	std::string filepath;
	std::string comment_token;
	bool is_writeable;
	char eqtoken;
	std::string default_config; // a config-file with default-values, if something isn't found in the main cfg
#ifndef DDCLIENT_GUI
	std::recursive_mutex mx;
#endif

    std::map<std::string, std::string> cfg_cache;
    typedef std::map<std::string, std::string>::iterator CfgIter;

};

#endif /*CFGFILE_H_*/
