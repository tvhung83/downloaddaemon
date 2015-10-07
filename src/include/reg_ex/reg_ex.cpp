#include <vector>
#include <string>
#include <cstring>

#include "reg_ex.h"

using std::vector;
using std::string;
using std::size_t;

reg_ex::reg_ex() : trex(NULL) {
}

bool reg_ex::compile(const std::string &ex) {
	trex = trex_compile(ex.c_str(), NULL);
	if (!trex) return false;
	return true;
}

bool reg_ex::match(const std::string &text) {
	return trex_match(trex, text.c_str());
}

bool reg_ex::match(const std::string &text, vector<string> &result) {
	const char *in_begin = text.c_str(), *in_end = text.c_str() + text.size();
	const char *out_begin = NULL, *out_end = NULL;

	while (true) {
		bool ret = trex_searchrange(trex, in_begin, in_end, &out_begin, &out_end);
		if (!ret) return result.size() > 0;

		result.push_back(string(out_begin, out_end - out_begin));

		in_begin = out_end;
	}
		
}

bool reg_ex::match(const std::string &text, std::string &result) {
	result.clear();
	const char* out_begin, *out_end;
	bool ret = trex_search(trex, text.c_str(), &out_begin, &out_end);
	if (!ret) return false;
	result.assign(out_begin, out_end - out_begin);
	return true;
}

bool reg_ex::match(const std::string &text, const std::string &ex) {
	reg_ex r;
	if (!r.compile(ex)) return false;
	return r.match(text);
}

bool reg_ex::match(const std::string &text, const std::string &ex, std::vector<std::string> &result) {
	reg_ex r;
	result.clear();
	if (!r.compile(ex)) return false;
	return r.match(text, result);
}

bool reg_ex::match(const std::string &text, const std::string &ex, std::string &result) {
	result.clear();
	reg_ex r;
	if (!r.compile(ex)) return false;
	bool ret = r.match(text, result);
	return ret;
}

reg_ex::~reg_ex() {
	if (trex)
		trex_free(trex);
}

#ifdef REGEX_TESTING
#include <iostream>
#include <cstdlib>
#include <fstream>
int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " regex text" << std::endl;
		std::exit(1);
	}
	string text;
	if (argc == 2) {
		string tmp;
		while (getline(std::cin, tmp)) text +=tmp;
	} else
		text = argv[2];

	
	reg_ex r;
	if (!r.compile(argv[1])) {
		std::cout << "failed to compile regex" << std::endl;
		return 1;
	}
	std::vector<std::string> result;
	std::cout << "match: " << r.match(text, result) << std::endl;
	std::cout << "results: " << std::endl;
	for (unsigned int i = 0; i < result.size(); ++i) {
		std::cout << "match: " << result[i] << std::endl;
	}
}
#endif

