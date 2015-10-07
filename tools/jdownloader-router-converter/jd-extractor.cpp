#include <iostream>
#include <fstream>
#include <vector>
#include <string>
using namespace std;

void trim_string(std::string &str) {
	while(str.length() > 0 && isspace(str[0])) {
		str.erase(str.begin());
	}
	while(str.length() > 0 && isspace(*(str.end() - 1))) {
		str.erase(str.end() -1);
	}
}

void replace_all(std::string& str, const std::string& old, const std::string& new_s) {
	size_t n;
	while((n = str.find(old)) != string::npos) {
		str.replace(n, old.length(), new_s);	
	}
}

int main(int argc, char* argv[]) {
	if(argc == 1) {
		cout << "Usage: " << argv[0] << " filename" << endl;
		return 0;
	}

	ifstream xmldata(argv[1]);
	string curr_line;
	
	std::string file;

	while(getline(xmldata, curr_line)) {
		file.append(curr_line);
	}
	xmldata.close();

	
	std::string curr_router_name;
	std::string curr_script;


	size_t n = 0;
	while((n = file.find("<void index=\"0\">", n + 1)) != string::npos) {
		n = file.find("<string>", n + 1) + 8;
		curr_router_name = file.substr(n, file.find("</string>", n) - n);
		n = file.find("<void index=\"1\">", n);
		n = file.find("<string>", n + 1) + 8;
		curr_router_name += " - " + file.substr(n, file.find("</string>", n) - n);
		n = file.find("<void index=\"2\">", n);
		n = file.find("<string>", n) +  8;
		size_t end = file.find("</string>", n);
		curr_script = file.substr(n, end - n);
		replace_all(curr_script, "\r\n", "\n");
		replace_all(curr_script, "\r", "\n");
		replace_all(curr_script, "&#13;", "");
		replace_all(curr_script, "&quot;", "\"");
		replace_all(curr_script, "&lt;", "<");
		replace_all(curr_script, "&gt;", ">");
		replace_all(curr_script, "&apos;", "'");
		replace_all(curr_script, "&amp;", "&");
		replace_all(curr_router_name, "&#13;", "");
		replace_all(curr_router_name, "&quot;", "\"");
		replace_all(curr_router_name, "&lt;", "<");
		replace_all(curr_router_name, "&gt;", ">");
		replace_all(curr_router_name, "&apos;", "'");
		replace_all(curr_router_name, "&amp;", "&");
		replace_all(curr_router_name, "/", " ");
		replace_all(curr_router_name, "\\", " ");
		replace_all(curr_router_name, ":", " ");
		replace_all(curr_router_name, "*", " ");
		replace_all(curr_router_name, "?", " ");
		replace_all(curr_router_name, "\"", " ");
		replace_all(curr_router_name, "<", " ");
		replace_all(curr_router_name, ">", " ");
		replace_all(curr_router_name, "|", " ");
		replace_all(curr_router_name, "@", " ");
		replace_all(curr_router_name, "\n", " ");
		replace_all(curr_router_name, "\r", " ");
		replace_all(curr_router_name, "'", " ");
		replace_all(curr_router_name, "\"", " ");
		replace_all(curr_router_name, ";", " ");

		while((curr_router_name.size() > 0) && (curr_router_name[0] == '-')) {
			curr_router_name = curr_router_name.substr(1);
		}

		trim_string(curr_router_name);
		if(curr_router_name.empty()) {
			continue;
		}

		ofstream out(string("scripts/" + curr_router_name).c_str());
		out << curr_script;
		out.close();


	}











}
