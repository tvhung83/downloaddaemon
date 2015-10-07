/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CLIENT_EXCEPTION
#define CLIENT_EXCEPTION

#include <exception>
#include <string>


/** Exception Class for Downloadc. */
class client_exception : public std::exception{
	public:

		/** Constructor
		*	@param id ID
		*	@param message Message to be saved in the exception
		*/
		client_exception(int id, std::string message);

		/** Destructor */
		virtual ~client_exception() throw();

		/** Returns an explanation of the exception
		*	@returns explanation of the exception
		*/
		virtual const char* what() const throw();

		/** Returns exception ID
		*	@returns ID
		*/
		int get_id();

	private:
		int id;
		std::string message;
};

#endif // CLIENT_EXCEPTION
