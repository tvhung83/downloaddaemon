#include <cmath>
#include <ctime>
#include <cstdlib>
#include <unistd.h>


#include "curl_speeder.h"
using namespace std;

typedef std::map <CURL*, curl_speeder::info> speedmap;

curl_speeder* curl_speeder::_instance = NULL;

void curl_speeder::add_dl(CURL* dl) {
	unique_lock<recursive_mutex> lock(mx);
	curl_speeder::info info;
	info.handle = dl;
	handles.insert(pair<CURL*, curl_speeder::info>(dl, info));
}

void curl_speeder::del_dl(CURL* dl) {
	unique_lock<recursive_mutex> lock(mx);
	speedmap::iterator it = handles.find(dl);
	if(it != handles.end())
		handles.erase(it);
}

void curl_speeder::set_speed(CURL* dl, filesize_t sp) {
	unique_lock<recursive_mutex> lock(mx);
	speedmap::iterator it = handles.find(dl);
	if(it != handles.end())
		it->second.desired_speed = sp;
}

filesize_t curl_speeder::get_speed(CURL* dl) {
	unique_lock<recursive_mutex> lock(mx);
	speedmap::iterator it = handles.find(dl);
	if(it != handles.end())
		return it->second.desired_speed;
	return 0;
}

filesize_t curl_speeder::get_curr_speed(CURL* dl) {
	unique_lock<recursive_mutex> lock(mx);
	speedmap::iterator it = handles.find(dl);
	if(it != handles.end())
		return it->second.get_curr_speed();
	return 0;
}

filesize_t curl_speeder::get_glob_speed() {
	unique_lock<recursive_mutex> lock(mx);
	return glob_speed;
}

void curl_speeder::set_glob_speed(filesize_t sp) {
	unique_lock<recursive_mutex> lock(mx);
	glob_speed = sp;
}

filesize_t curl_speeder::get_curr_glob_speed() {
	unique_lock<recursive_mutex> lock(mx);
	filesize_t speed = 0;
	for(speedmap::iterator it = handles.begin(); it != handles.end(); ++it) {
		speed += it->second.get_curr_speed();
	}
	return speed;
}

void curl_speeder::speed_me(CURL* dl) {
	unique_lock<recursive_mutex> lock(mx);
	speedmap::iterator it = handles.find(dl);
	add_speed_to_list(dl);
	if(it == handles.end())
		return;

    // bool speeded = false;
	while(it->second.desired_speed > 0 && it->second.get_curr_speed() > it->second.desired_speed) {
		lock.unlock();
                usleep(1000);
		lock.lock();
		add_speed_to_list(dl);
        // speeded = true;
	}
	while(glob_speed > 0 && get_curr_glob_speed() > glob_speed && it->second.get_curr_speed() > 100) { // make sure we are never slower than 100 bytes/s because of timeouts
		lock.unlock(); // these unlock makes sure that multiple downloads can reach this point at the same time and can all be slowed down in parallel
                usleep(1000);
		lock.lock();
		add_speed_to_list(dl);
        // speeded = true;
	}
}

void curl_speeder::add_speed_to_list(CURL* dl) {
	unique_lock<recursive_mutex> lock(mx);
	speedmap::iterator it = handles.find(dl);
	if(it == handles.end()) return;
	double size;
	curl_easy_getinfo(dl, CURLINFO_SIZE_DOWNLOAD, &size);
	struct timespec t;
	#ifdef CLOCK_MONOTONC_RAW
		clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	#elif defined(CLOCK_MONOTONIC)
		clock_gettime(CLOCK_MONOTONIC, &t);
	#elif defined(CLOCK_REALTIME)
		clock_gettime(CLOCK_REALTIME, &t);
	#else
		#warning No realtime/high resolution clock was found on your machine. Download speed settings will not work correctly.
	#endif

	filesize_t mcsecs = (t.tv_sec * 1000000) + (t.tv_nsec / 1000);
	it->second.sizes.push_back(floor(size+0.5));
	it->second.times.push_back(mcsecs);
        while(it->second.times.size() > 1 && mcsecs > it->second.times[0] + 3000000) {
		it->second.times.pop_front();
		it->second.sizes.pop_front();
	}

	it->second.recalc_speed();
}
