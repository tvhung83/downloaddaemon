/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef NETPPTK_H_
#define NETPPTK_H_

#include <config.h>
#include <string>

#ifdef _WIN32
	#include <winsock2.h>
#else
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h>
#endif


enum ERROR_CODE { SOCKET_CREATION_FAILED, UNKNOWN_HOST, CONNECTION_PROBLEM };

class tkSock {
public:
	/** Basic constructor which creates a socket and eventually calles Windows-init code */
	tkSock();

	/** Fullconstructor
	*	@param MaxConnections The max number of connections
	*	@param MaxReceive The maximal packet size
	*/
	tkSock(const unsigned int MaxConnections, const unsigned int MaxReceive);

	/** Normal destructor to close the socket */
	virtual ~tkSock();

	/** Bind the socket to a port and address
	*	@param port Port number
	*	@param addr Address to bind to (empty = bind to all)
	*	@returns True if we succeed to bind to at least one address
	*/
	bool bind(const int port, std::string addr = "");

	/** Listen on the specified port
	*	@returns True on success
	*/
	bool listen();

	/** Accept a queried connections
	*	@param new_socket A socket to handle the accepted connection
	*	@returns true on success
	*/
	bool accept(tkSock& new_socket);

	/** Connect to a remote host
	*	@param host Host to connect to
	*	@param port Port to use
	*	@returns True on success
	*/
	bool connect(const std::string &host, const int port);

	/** Disconnect from a connected socket
	*/
	void disconnect();

	/** Send data over the socket
	*	@param s Data to send
	*	@returns True on success
	*/
	bool send(const std::string &s);

	/** Receive data from a socket
	*	@param s String to store the received data
	*	@returns The number of received bytes, negative value on error
	*/
	int recv(std::string &s, int flags = 0);

	/** Convert the object to book to use easy if(..) constructs for validity-checks */
	operator bool() const;

	/** Read data from a string to the socket to send it */
	const tkSock& operator<< (const std::string&);

	/** Read data from a socket and save it in a string */
	const tkSock& operator>> (std::string&);

	/** Returns the socket descriptor for manual handling */
	int get_sockdesc() const {return m_sock;}

	/** Set a socket descriptor to pass management to the socket
	*	@param n Socket descriptor
	*/
	void set_sockdesc(int n) { m_sock = n;}

	/** Gets the ip-address of the "other side" of the connection
	*	@reutrns Peer ip-address, empty string if it fails
	*/
	std::string get_peer_name();

	/** Checks if there is data on the socket to be read
	*  @param msec Specifies how long to wait for input before returning in milliseconds (may be 0)
	*  @returns true if there is data
	*/
	bool select(long msec = 0);


private:
	int m_sock; // Socket descriptor
	bool valid;
	sockaddr_in m_addr;
	unsigned int m_maxconnections;
	unsigned int m_maxrecv;
	static int m_instanceCount;
	unsigned int m_open_connections;
	std::string append_header(std::string data);
	int remove_header(std::string &data);
};

std::ostream& operator<<(std::ostream& stream, tkSock &sock);
std::istream& operator>>(std::istream& stream, tkSock &sock);




// Exception class

class SocketError {
public:
	SocketError(ERROR_CODE e)
	: error(e) {}

	ERROR_CODE error;

	std::string what() {
		if(error == SOCKET_CREATION_FAILED)
			return std::string("Socket creation failed");;
		if(error == UNKNOWN_HOST)
			return std::string("Unknown Host");
		if(error == CONNECTION_PROBLEM)
			return std::string("A Problem occured while trying to handle a connection");
		return std::string("");
	}
};


#endif /*NETPPTK_H_*/
