#include "connection_manager.h"
#include <algorithm>
using namespace std;

connection_manager* connection_manager::m_instance = 0;

connection_manager::connection_manager() {
}

connection_manager::~connection_manager() {
	while(connections.size() > 0) {
		delete connections[0];
	}
}

long connection_manager::add_client(client *c) {
	std::lock_guard<std::mutex> lock(mx);
	int id = 0;
	if(connections.size() > 0) {
		id = connections[connections.size() - 1]->client_id + 1;
	}
	c->client_id = id;
	connections.push_back(c);
	return id;
}

void connection_manager::del_client(long id) {
	std::lock_guard<std::mutex> lock(mx);
	for(vector<client*>::iterator it = connections.begin(); it != connections.end(); ++it) {
		if((*it)->client_id == id) {
				delete *it;
				connections.erase(it);
				break;
		}
	}
}

void connection_manager::push_message(subs_type type, const std::string &message) {
	std::lock_guard<std::mutex> lock(mx);
	for(vector<client*>::iterator it = connections.begin(); it != connections.end(); ++it) {
		(*it)->push_message(type, message);
	}
}

connection_manager::subs_type connection_manager::string_to_subs(const std::string &s) {
	if(s == "SUBS_DOWNLOADS") return SUBS_DOWNLOADS;
	if(s == "SUBS_CONFIG") return SUBS_CONFIG;
	return SUBS_NONE;
}

void connection_manager::subs_to_string(subs_type t, std::string &ret) {
	switch(t) {
	case SUBS_DOWNLOADS:
		ret = "SUBS_DOWNLOADS";
		return;
	case SUBS_CONFIG:
		ret = "SUBS_CONFIG";
		return;
	case SUBS_NONE:
		ret = "";
		return;
	}
}

void connection_manager::reason_to_string(reason_type t, std::string &ret) {
	switch(t) {
	case UPDATE:
		ret = "UPDATE";
		return;
	case NEW:
		ret = "NEW";
		return;
	case DELETE:
		ret = "DELETE";
		return;
	case MOVEUP:
		ret = "MOVEUP";
		return;
	case MOVEDOWN:
		ret = "MOVEDOWN";
		return;
	case MOVEBOTTOM:
		ret = "MOVEBOTTOM";
		return;
	case MOVETOP:
		ret = "MOVETOP";
		return;
	case UNRAR_START:
		ret = "UNRAR_START";
		return;
	case UNRAR_FINISHED:
		ret = "UNRAR_FINISHED";
		return;
	}
}

//////////////////////////////////////
//////// CLASS CLIENT ////////////////
//////////////////////////////////////

void client::push_message(connection_manager::subs_type type, const std::string &message) {
	std::lock_guard<std::mutex> lock(mx);
	string pmsg;
	connection_manager::subs_to_string(type, pmsg);
	pmsg += ":" + message;
	for(size_t i = 0; i < subscriptions.size(); ++i) {
		if(subscriptions[i] == type) {
				subs_messages.push_back(make_pair(type, pmsg));
		}
	}
}

std::string client::pop_message() {
	std::lock_guard<std::mutex> lock(mx);
	string ret(subs_messages.front().second);
	subs_messages.erase(subs_messages.begin());
	return ret;
}

size_t client::messagecount() {
	std::lock_guard<std::mutex> lock(mx);
	return subs_messages.size();
}

void client::subscribe(connection_manager::subs_type type) {
	std::lock_guard<std::mutex> lock(mx);
	if(find(subscriptions.begin(), subscriptions.end(), type) == subscriptions.end()) {
		subscriptions.push_back(type);
	}
}

void client::unsubscribe(connection_manager::subs_type type) {
	std::lock_guard<std::mutex> lock(mx);
	vector<connection_manager::subs_type>::iterator it = find(subscriptions.begin(), subscriptions.end(), type);
	if(it != subscriptions.end()) {
		subscriptions.erase(it);
	}
	for(size_t i = 0; i < subs_messages.size(); ++i) {
		if(subs_messages[i].first == type) {
				subs_messages.erase(subs_messages.begin() + i);
				--i;
		}
	}
}

std::vector<connection_manager::subs_type> client::list() {
	std::lock_guard<std::mutex> lock(mx);
	return subscriptions;
}

bool client::has_subscription(connection_manager::subs_type type) {
	if(find(subscriptions.begin(), subscriptions.end(), type) == subscriptions.end())
		return false;
	return true;
}
