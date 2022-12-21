#ifndef String_Functions_h
#define String_Functions_h

#include <algorithm>
#include <map>
#include <vector>
#include <string>
#include <regex>

using namespace std;

struct MatchRes
{
	size_t MrPos, MrLen;
};

extern string Trim(const string & InStr);
extern string LTrim(const string & InStr);
extern string RTrim(const string & InStr);
extern string Gsub(const string & InStr, string Rx, string Rep);
extern string SToLower(const string & InStr);
extern string SToUpper(const string & InStr);
extern string UrlDecode(const string & InStr);
bool RMatch(const string & InStr, string RExp, size_t Offest = 0, MatchRes *pMRes = NULL);
extern vector <string> RSplit(const string & InStr, string Div, bool UseFirst = false);
extern map <string, string> KwParse(const string & InStr, string FldDiv, string ArgDiv, string KeyWords, bool UseQuotes = true, bool UseFirst = false);

#endif // String_Functions_h