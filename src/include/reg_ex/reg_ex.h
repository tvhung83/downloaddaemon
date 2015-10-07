#ifndef REGEX_H
#define REGEX_H
#include <vector>
#include <string>
#include "trex.h"


/*! more advanced interface for regular expressions to individually compile and match regular expressions */
class reg_ex {
public:
	reg_ex();
	/*! compile a regular expression */
	bool compile(const std::string &regex);

	/*! match a previousely compiled regex to a text. the first version returns true if the regex matches.
		the second version also returns all matching results and the third version only returns the first match.
	*/

	/*! returns true if text is an exact match of the compiled expression */
	bool match(const std::string &text);

	/*! convenience function that does multiple regex searches. Every time the expression is extracted, it is
	    stored in the result vector and a new search is started from the end of the last match. This allows to simply
	    extract multiple matches (e.g. all URLs on a page */
	bool match(const std::string &text, std::vector<std::string> &result);

	/*! Normal regex search that finds a result and stores it */
	bool match(const std::string &text, std::string &result);

	~reg_ex();



	/*! 3 functions to simplify usage and compile/match in one go instead of seperate steps */
	static bool match(const std::string &text, const std::string &regex, std::vector<std::string> &result);
	static bool match(const std::string &text, const std::string &regex, std::string &result);
	static bool match(const std::string &text, const std::string &regex);
private:
	TRex *trex;
	
};

#endif
