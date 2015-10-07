#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

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
#include <vector>
#include <queue>
#include <netpptk/netpptk.h>

class client; // forward declaration

class connection_manager
{
public:
	enum subs_type { SUBS_NONE = 0, SUBS_DOWNLOADS, SUBS_CONFIG };

	/** Describes the reason why a message is sent to a subscriber.
	*/
	enum reason_type { UPDATE = 0, NEW, DELETE, MOVEUP, MOVEDOWN, MOVETOP, MOVEBOTTOM, UNRAR_START, UNRAR_FINISHED };

	connection_manager();
	~connection_manager();

	long add_client(client *c);
	void del_client(long id);
	void push_message(subs_type type, const std::string &message);

	static void create_instance()         { if (!m_instance) m_instance = new connection_manager; }
	static void destroy_instance()        { if(m_instance) delete m_instance; }
	static connection_manager* instance() { return m_instance; }

	static subs_type string_to_subs(const std::string &s);
	static void subs_to_string(subs_type t, std::string &ret);

	/** Fills a string with the reason why a message is sent to a subscriber.
	*	@param t reason type
	*	@param ret return String
	*/
	static void reason_to_string(reason_type t, std::string &ret);
private:
	static connection_manager *m_instance;
	std::vector<client *>  connections;
	std::mutex mx;
};


class client {
public:
	client() : sock(new tkSock), client_id(-1) {}
	~client()   { if(sock) delete sock; }

	void        push_message(connection_manager::subs_type type, const std::string &message);
	std::string pop_message ();
	size_t      messagecount();
	void        subscribe   (connection_manager::subs_type type);
	void        unsubscribe (connection_manager::subs_type type);
	bool        has_subscription(connection_manager::subs_type type);

	std::vector<connection_manager::subs_type> list();

	tkSock      *sock;
	long        client_id;
private:

	std::vector<connection_manager::subs_type>  subscriptions;
	std::vector<std::pair<connection_manager::subs_type, std::string> > subs_messages;

	std::mutex mx;
};

#endif // CONNECTION_MANAGER_H
