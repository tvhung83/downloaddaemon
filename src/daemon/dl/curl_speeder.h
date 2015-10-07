#include <string>
#include <vector>
#include <map>
#include <deque>
#include <curl/curl.h>
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

/** This class provides improved scheduling mechanisms for libcurl.
*   It allows speed settings based on the current (not just average) speed of a download
*   as well as setting a global speed limit for all transactions.
*   Also, it is completely thread-safe
*/
class curl_speeder {
public:
	typedef filesize_t ulong;
	/** Simple constructor, only initializing */
	curl_speeder() : glob_speed(0) {};
	
	/** Add a download to the speeder-management
	*   @param dl curl handle to add
	*/
	void add_dl(CURL* dl);

	/** Delete a download from the speeder-management. It's important do always do this with finished
	*   downloads!
	*   @param dl curl handle to delete
	*/	
	void del_dl(CURL* dl);

	/** Set the desired speed of a specific download
	*	@param dl curl handle of the download to set the speed for
	*	@param sp the desired speed
	*/
	void set_speed(CURL* dl, filesize_t sp);

	/** get the desired speed of a download (that was set with set_speed)
	*	@param dl curl-handle to get the speed from
	*	@returns the speed
	*/
	filesize_t get_speed(CURL* dl);

	/** get the REAL speed of a currently running download. This is different from libcurls speed.
	*   libcurl always returns the average speed of the whole download process. This function returns the
	*   speed at the current point in time (average over the last ~5 second). It only works correctly if
	*   the download is already running for a few seconds and has been in the scheduler for that time
	*   @param dl curl handle of the download
	*   @returns the current speed of the download
	*/
	filesize_t get_curr_speed(CURL* dl);

	/** set the speed that the whole speeder might generally use. The speed of all downloads should never be
	*   higher than this (the speeder will try to keep downloads[0] + downloads[1] + downloads[n] lower than
	*   this value).
	*   @param sp global speed to be set
	*/
	void set_glob_speed(ulong sp);

	/** get the global speed limit as set above
	*   @returns the speed
	*/
	filesize_t get_glob_speed();

	/** calculate the current overall-speed of all downloads in the scheduler (avg in the last ~5 seconds)
	*   @returns the overall speed
	*/
	filesize_t get_curr_glob_speed();

	/** This function does the real speeder-work. You should call it in curl's report_progress callback
	*   with the curl-handle as a parameter. This function will then call curl_easy_pause on the handle to
	*   either slow down or speed up the transfer. 
	*   @param dl handle to the running download
	*/
	void speed_me(CURL* dl);

	struct info {
		info() : desired_speed(0), curr_speed(0), handle(NULL) {}
		ulong recalc_speed() {
			if (times.size() > 3 && *times.begin() != times.back() && *sizes.begin() != sizes.back())
			{
				int third = sizes.size() / 3;
				curr_speed = (3  * avgFromTo(2 * third, sizes.size() - 1) +
							  2  * avgFromTo(third, 2 * third - 1) +
							  1 * avgFromTo(0, third - 1))
							 / 6;
				// curr_speed = ((sizes.back() - *sizes.begin()) * 1000000) / (times.back() - *times.begin());
			}
			else
				curr_speed = 0;
			curr_speed = curr_speed * 1;
			return curr_speed;
		}

		ulong get_curr_speed() {
			return curr_speed;
		}

		inline ulong avgFromTo(size_t from, size_t to) {
			if (from < 0 || to < 0 || times[from] == times[to]) return 0;
			return ((sizes[to] - sizes[from]) * 1000000) / (times[to] - times[from]);
		}

		std::deque <filesize_t> sizes;
		// using microseconds since 1.1.1970 for the times
		std::deque <filesize_t> times;
		filesize_t desired_speed;
		filesize_t curr_speed;
		CURL* handle;
	};

	static curl_speeder* instance() {
		if(!_instance) _instance = new curl_speeder;
		return _instance;
	}
	static void destroy() {
		if(_instance) delete _instance;
		_instance = NULL;
	}
private:
	std::map <CURL*, curl_speeder::info> handles;
	filesize_t glob_speed;
	void add_speed_to_list(CURL* dl);
	std::recursive_mutex mx;

	static curl_speeder* _instance;

};
