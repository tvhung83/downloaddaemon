/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CURL_CALLBACKS_H_
#define CURL_CALLBACKS_H_

/** normal callback to download to a file */
size_t write_file	(void *buffer, size_t size, size_t nmemb, void *userp);

/** normal callback for progress report when doing normal downloads */
int report_progress	(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);

/** callback to parse the header of a download */
size_t parse_header	(void *ptr, size_t size, size_t nmemb, void *clientp);

/** header-callback to get the size of a download (Content-Length: header) */
int get_size_progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);

/** write-callback that does nothing if we just want to simulate downloading */
size_t pretend_write_file(void *buffer, size_t size, size_t nmemb, void *userp);

/** write-callback to write downloaded data to an std::string passed as userp */
size_t write_to_string(void *buffer, size_t size, size_t nmemb, void *userp);

#endif /*CURL_CALLBACKS_H_*/
