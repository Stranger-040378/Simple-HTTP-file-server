#ifndef Html_Work_h
#define Html_Work_h

#include <string>
#include <sstream>
#include <vector>

#include "StringFunctions.hpp"

using namespace std;

extern string GetHtmlHeaderT(vector <string> Caption);
extern string GetHtmlFooterT();
extern string GetTableRow(vector <string> Data);
extern string GetFileSubmitForm(string InUrl);
extern string GetHRef(string InUrl, string Text);

#endif // Html_Work_h
