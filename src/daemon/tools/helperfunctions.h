/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef HELPERFUNCTIONS_H_
#define HELPERFUNCTIONS_H_

#include <config.h>
#include <string>
#include <vector>
#include "../plugins/captcha.h"

#ifdef HAVE_SYSLOG_H
	#include <syslog.h>
#else
	enum { LOG_ERR, LOG_WARNING, LOG_DEBUG };
#endif

/** Convert a string to an int
* @param str string to convert
* @returns result
*/
int string_to_int(std::string str);

/** Conversion functions for a few types */
std::string int_to_string(int i);
std::string long_to_string(long i);
long string_to_long(std::string str);

/** Remove whitespaces from beginning and end of a string
* @param str string to process
* @returns str
*/
const std::string& trim_string(std::string &str);

/** Validation of a given URL to check if it's valid
* @param url Url to check
* @returns true if valid
*/
bool validate_url(std::string &url);

/** log a string with a specified log-level
* @param logstr String to write to the log
* @param level Log-level of the string - the higher, the more likely the user will see it (possible: LOG_ERR, LOG_WARNING, LOG_DEBUG)
*/
void log_string(const std::string logstr, int level);

/** replaces all occurences of a string with another string
* @param searchIn String in which stuff has to be replaced
* @param searchFor String with stuff to replace
* @param ReplaceWith String that should be inserted instead
*/
void replace_all(std::string& searchIn, std::string searchFor, std::string ReplaceWith);

/** Dumps the current internal download list (in vector global_download_list) to the download list file on the harddisk.
* @returns ture on success
*/
bool dump_list_to_file();

/** Checks if a given identifier is a valid configuration variable or not
* @param variable Identifier to check
* @returns True if it's valid, false if not
*/
bool variable_is_valid(std::string &variable);

/** When a variable changes, some actions might have to be done in order to apply that setting. this only affects very few variables.
* @param variable the config-variable that will be changed
* @param value The value that will be set
* @returns returns true if everything is okay, false to veto (do not set the variable)
*/
bool proceed_variable(const std::string &variable, std::string value);

/** Checks if a given idetifier is a valid router config variable or not
* @param variable Identifier to check
* @returns true if it's valid, false if not
*/
bool router_variable_is_valid(std::string &variable);

/** Checks if a path is relative or absolute and if it's relative, add the program_root in the beginning
* @param path Path to check/modify
*/
void correct_path(std::string &path);

/** Parses the value of an environment variable out of the char* env[] passed as the third argument to main()
*	@param env environment variable array from the third main() argument
*	@param var variable to search for
*	@returns the value of the variable
*/
std::string get_env_var(const std::string &var);

/** substitutes environment variables in string with the value of the variable. eg "$HOME/foo" will result in
*	"/home/uname/foo" or something like that. if the variable is not found, an empty string will be put in.
*	@param env environment variable array from the third main() argument
*	@param str string in which variables should be substituted
*/
void substitute_env_vars(std::string &str);

/** Recursively creates a directory path specified in dir
*	@param path to create
*	@returns if all directories were created successfully
*/
bool mkdir_recursive(std::string dir);

/** detects the name of a file if you give a URL or an empty string if it's a directory (url ending with /) ex.: http://blah.com/asdf?q=1 will return "asdf"
*	@param url URL from which to get the filename
*	@returns the filename (NOT path)
*/
std::string filename_from_url(const std::string &url);

/** sort() callback to compare strings case-insensitive
*	@param s1 first string
*	@param s2 second string
*	@returns true if s1 < s2
*/
bool CompareNoCase( const std::string& s1, const std::string& s2 );

/** creates a filename from a string by replacing special chars (ONLY pass a filename, not a path. / and \ will also be replaced!)
*	@param fn filename
*/
void make_valid_filename(std::string &fn);

/** create a binary string from ascii hex (e.g. FF will result ret.size() = 1, ret[0] = 255;)
*	@param ascii_hex string to unhexlify
*	@returns the binary representation
*/
std::string ascii_hex_to_bin(std::string ascii_hex);

/** Compares 2 doubles if they are almost equal/double equal
*   @param p1 first double
*   @param p2 second double
*   @returns true if they are almost equal
*/
bool fequal(double p1, double p2);

/** Splits a string into many strings, seperating them with the seperator
 *  @param inp_string string to split
 *  @param seperator Seperator to use for splitting
 *  @returns Vector of all the strings
 */
std::vector<std::string> split_string(const std::string& inp_string, const std::string& seperator, bool respect_escape = false);

/** Escapes characters in a string, see also unescape_string to revert this function
 *  @param s String in which to escape characters
 *  @param escape_chars a list of characters that should be escaped
 *  @param escape_sequence the sequence to escape the characters with (usually a \)
 *  @returns reference to s
 */
std::string escape_string(std::string s, std::string escape_chars = "|", const std::string& escape_sequence = "\\");

/** Unescapes characters in a string, reverse to escape_string
 *  @param s the string in which to revert escape_string
 *  @param escape_chars the characters that were escaped in the original string
 *  @param escape_sequence the escape-sequence that was used for escaping
 *  @returns reference to s
 */
std::string unescape_string(std::string s, std::string escape_chars = "|", const std::string& escape_sequence = "\\");

class download_container;
/** add a dlc-container to the download list
 * @param content the contents of the container file
 * @returns true on success
 */
bool decode_dlc(const std::string& content, download_container *container = NULL);

/** add a container (dlc,rsdf,ccf) to the download list
  * @param extension (.dlc,.rsdf,.ccf)
  * @param content the contents of the container file
  * @returns true on success
  */
bool loadcontainer(const std::string extension, const std::string& content, download_container* container=NULL);

/** filename when a full path is given
  * @param path full path
  * @returns the filename
  */
std::string filename_from_path(const std::string &path);

#ifdef BACKTRACE_ON_CRASH
void print_backtrace(int sig);
#endif

#endif /*HELPERFUNCTIONS_H_*/
