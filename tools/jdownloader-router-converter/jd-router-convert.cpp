#include<iostream>
#include<fstream>
#include<string>
#include<sstream>
using namespace std;


void trim_string(std::string &str);
bool exec_next(std::istream &str, std::string& to_exec, std::string &result);
void step(std::istream &str, std::string& result);
void request(std::istream &str, std::string& result);
void wait(std::istream &str, std::string& result);

void trim_string(std::string &str) {
	while(str.length() > 0 && isspace(str[0])) {
		str.erase(str.begin());
	}
	while(str.length() > 0 && isspace(*(str.end() - 1))) {
		str.erase(str.end() -1);
	}
}

bool exec_next(std::istream &str, std::string& to_exec, std::string &result) {
	trim_string(to_exec);
	if(to_exec.find("[[[") == string::npos) {
		return false;
	}
	if(to_exec.find("[[[STEP]]]") == 0) {
		step(str, result);
	} else if(to_exec.find("[[[REQUEST]]]") == 0) {
		request(str, result);
	} else if(to_exec.find("[[[WAIT]]]") == 0) {
		wait(str, result);
	}
	return true;
}

void step(std::istream &str, std::string& result) {
	string to_exec;
	while(getline(str, to_exec)) {
		trim_string(to_exec);
		if(to_exec.empty()) {
			continue;
		}
		if(to_exec.find("[[[/STEP]]]") == 0) {
			return;
		} else if(!exec_next(str, to_exec, result)) {
			// we need to do stuff here that can stand in [[[STEP]]] and is NOT a command
		}

	}
}

void request(std::istream &str, std::string& result) {
	string to_exec;
	result += "\t{\n";
	result += "\t\tcurl_easy_reset(handle);\n";
	result += "\t\tcurl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);\n";
	result += "\t\tcurl_easy_setopt(handle, CURLOPT_WRITEDATA, &resultstr);\n";
	result += "\t\tstruct curl_slist *header = NULL;\n";
	while(getline(str, to_exec)) {
		trim_string(to_exec);
		if(to_exec.empty()) {
			continue;
		}
		if(to_exec.find("[[[/REQUEST]]]") == 0) {
			result += "\t\tcurl_easy_setopt(handle, CURLOPT_HTTPHEADER, header);\n";
			result += "\t\tcurl_easy_perform(handle);\n";
			result += "\t\tcurl_slist_free_all(header);\n";
			result += "\t}\n";
			return;
		} else if(!exec_next(str, to_exec, result)) {
			if(to_exec.find("%%%routerip%%%") != string::npos) {
				to_exec.replace(to_exec.find("%%%routerip%%%"), 14, "\" + host + \"");
			}

			if(to_exec.find("%%%user%%%") != string::npos) {
				to_exec.replace(to_exec.find("%%%user%%%"), 10, "\" + user + \"");
			}

			if(to_exec.find("%%%pass%%%") != string::npos) {
				to_exec.replace(to_exec.find("%%%pass%%%"), 10, "\" + password + \"");
			}

			if(to_exec.find("%%%basicauth%%%") != string::npos) {
				result += "\t\tcurl_easy_setopt(handle, CURLOPT_USERPWD, string(user + ':' + password).c_str());\n";
				continue;
			}

			if(to_exec.find("GET") == 0 || to_exec.find("POST") == 0) {
				to_exec = to_exec.substr(4);
				trim_string(to_exec);
				to_exec = to_exec.substr(0, to_exec.find_first_of(" \t\n"));
				result += "\t\tcurl_easy_setopt(handle, CURLOPT_URL, \"http://\" + host + \"";
				result += to_exec;
				result += "\");\n";
				continue;
			}

			if(to_exec.find("Host:") == 0) {
				continue;
			}

			if(to_exec.find(':') != string::npos) {
				result += "\t\theader = curl_slist_append_s(header, \"";
				result += to_exec;
				result += "\");\n";
			} else {
				result += "\t\tcurl_easy_setopt(handle, CURLOPT_POST, 1);\n";
				result += "\t\tcurl_easy_setopt(handle, CURLOPT_POSTFIELDS, \"" + to_exec + "\");\n";
			}
		}
	}
	result += "\t}\n";
}

void wait(std::istream &str, std::string& result) {
	//  sleep for the time specified in the wait tag

}

int main(int argc, char* argv[]) {
	//if(argc < 2) {
	//	cout << "Usage: " << argv[0] << " "
	//}

	string get;
	string total;
	while(get != "RUN!") {
		getline(cin, get);
		total += get;
		total += '\n';
	}

	total = total.substr(total.find("[[[HSRC]]]"), total.find("[[[/HSRC]]]"));

	stringstream file;
	file << total;

	std::string line;
	std::string cppresult;
	cppresult = "#include<iostream>\n"
				"#include<curl/curl.h>\n"
				"using namespace std;\n"
				"\n"
				"struct curl_slist *curl_slist_append_s(struct curl_slist *list, const std::string &string ) {\n"
				"\treturn curl_slist_append(list, string.c_str());\n"
				"}\n"
				"\n"
				"CURLcode curl_easy_setopt_s(CURL *handle, CURLoption option, string str) {\n"
				"\treturn curl_easy_setopt(handle, option, str.c_str());\n"
				"}\n"
				"\n"
				"size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {\n"
				"\tstd::string *blubb = (std::string*)userp;\n"
				"\tblubb->append((char*)buffer, nmemb);\n"
				"\treturn nmemb;\n"
				"}\n"
				"\n"
				"extern \"C\" void reconnect(std::string host, std::string user, std::string password) {\n"
				"\tCURL* handle = curl_easy_init();\n"
				"\tstd::string resultstr;\n"
				"\tcurl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);\n"
				"\tcurl_easy_setopt(handle, CURLOPT_WRITEDATA, &resultstr);\n";

	while(getline(file, line)) {
		exec_next(file, line, cppresult);
	}

	cppresult += "\tcurl_easy_cleanup(handle);\n"
				 "}";



	cout << cppresult;
/*	cout << total << endl;
	size_t last_step = 0, last_request = 0, step_end = 0, request_end = 0;
	while(last_step = total.find("[[[STEP]]]", last_step + 1)) {
		step_end = total.find("[[[/STEP]]]", last_step);






	}
*/
}
