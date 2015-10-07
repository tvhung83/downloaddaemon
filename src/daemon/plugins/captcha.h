/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CAPTCHA_H_INCLUDED
#define CAPTCHA_H_INCLUDED

#include <config.h>
#ifndef USE_STD_THREAD
	#include <boost/thread.hpp>
		namespace std {
			using namespace boost;
		}
#else
	#include <thread>
	#include <mutex>
#endif
#include <string>

#ifdef IS_PLUGIN
#include "plugin_helpers.h"
#endif

class captcha {
public:
	enum solve_type { SOLVE_AUTOMATIC, SOLVE_MANUAL };

	captcha() : dlid(-1), max_retrys(0), retry_count(0) {}

	/** do the actual decoding work
	*	@param img_data the image itsself
	*	@param img_type type of the image file ("png", "jpg", "gif", etc)
	*	@param gocr_options String which specifies the options to pass to gocr
	*	@param required_chars specify how many characters a captcha has, -1 if it's length is dynamic.
	*	@param use_db should a gocr-letter-database be used for decrypting?
	*	@param keep_whitespaces should whitespaces be stripped out of the resulting text?
	*	@param solve_type Set this to captcha::SOLVE_AUTOMATIC if captcha solving works with gocr, set it to captcha::SOLVE_MANUAL to tell a client to solve the captcha
	*	@returns the captcha as clear text
	*/
	std::string process_image(const std::string &img_data, const std::string &img_type, const std::string &gocr_options,
							  int required_chars, bool use_db = false, bool keep_whitespaces = false,
							  captcha::solve_type solver = SOLVE_AUTOMATIC);




	void set_image(const std::string &img);
	std::string get_image() const; // NO CONST REFERENCE -- THREAD SAFETY

	void set_imgtype(const std::string &type);
	std::string get_imgtype() const; // NO CONST REFERENCE -- THREAD SAFETY

	void set_result(const std::string &res);
	std::string get_result() const;

	void set_question(const std::string &q);
	std::string get_question() const;

	void setup(const std::string &gocr, int max_retrys, const std::string &data_dir, int dlid, const std::string &host);

private:
	std::string image;
	std::string img_type;
	std::string result;
	std::string question;
	std::string gocr;
	std::string share_directory;
	std::string host;
	int         dlid;
	int         max_retrys;
	int         retry_count;

	mutable std::recursive_mutex mx;

};


class captcha_exception {
public:
	enum captcha_problem { AUTOSOLVE_MAXTRIES, MANUALSOLVE_MAXTRIES, MANUALSOLVE_DISABLED, MANUALSOLVE_TIMEOUT };

	void set_problem(captcha_exception::captcha_problem prob) {
		problem = prob;
	}

	captcha_exception::captcha_problem get_problem() {
		return problem;
	}

private:
	captcha_exception::captcha_problem problem;
};

#endif // CAPTCHA_H_INCLUDED
