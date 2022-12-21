#include "StringFunctions.hpp"

using namespace std;

string Trim(const string & InStr)
{
	return regex_replace(InStr, regex("^[[:space:]]+|[[:space:]]+$"), "");;
}

string LTrim(const string & InStr)
{
	return regex_replace(InStr, regex("^[[:space:]]+"), "");
}

string RTrim(const string & InStr)
{
	return regex_replace(InStr, regex("[[:space:]]+$"), "");
}

string Gsub(const string & InStr, string Rx, string Rep)
{
	return regex_replace(InStr, regex(Rx), Rep);
}

string SToLower(const string & InStr)
{
	string RetVal(InStr);
	
	transform(RetVal.begin(), RetVal.end(), RetVal.begin(), ::tolower);
	
	return RetVal;
}

string SToUpper(const string & InStr)
{
	string RetVal(InStr);
	
	transform(RetVal.begin(), RetVal.end(), RetVal.begin(), ::toupper);
	
	return RetVal;
}

vector <string> RSplit(const string & InStr, string Div, bool UseFirst)
{
	vector <string> RetVal;
	regex Rx(Div); smatch Mr; bool Found;
	size_t CurPos = 0, CurDiff;
	
	auto First = InStr.cbegin(); auto Last = InStr.cend();
	
	do
	{
		Found = regex_search(First, Last, Mr, Rx);
		if (Found)
		{
			RetVal.push_back(move(InStr.substr(CurPos, Mr.position())));
			
			CurDiff = Mr.position() + Mr.length();
			CurPos += CurDiff; First += CurDiff;
			
			if (UseFirst) break;
		}
	}
	while (Found);
	
	if (CurPos < InStr.length())
		RetVal.push_back(move(InStr.substr(CurPos)));
	
	return RetVal;
}

string UrlDecode(const string & InStr)
{
	string RetVal;
	regex Rx("%[[:digit:]a-fA-F]{2}");
	smatch Mr; bool Found;
	
	size_t CurPos = 0, MrPos, MrLn;
	string StrVal;
	
	int ChCode; unsigned char DecSym[] = {0, 0};
	char *pDecSym = reinterpret_cast<char *>(DecSym);
	
	auto First = InStr.cbegin(); auto Last = InStr.cend();
	
	do
	{
		Found = regex_search(First, Last, Mr, Rx);
		if (Found)
		{
			MrPos = Mr.position(); MrLn = Mr.length();
			if (MrPos) RetVal += InStr.substr(CurPos, MrPos);
			
			CurPos += MrPos;
			StrVal = InStr.substr(CurPos + 1, MrLn - 1);
			
			try { ChCode = stoi(StrVal, NULL, 16); }
			catch (...)
			{ ChCode = 32; }
			
			if (ChCode > 255) ChCode = 255;
			
			DecSym[0] = static_cast<unsigned char>(ChCode);
			RetVal.append(pDecSym);
			
			CurPos += MrLn; First += MrPos + MrLn;
		}
	}
	while (Found);
	
	if (CurPos < InStr.size()) RetVal += InStr.substr(CurPos);
	
	return RetVal;
}

bool RMatch(const string & InStr, string RExp, size_t Offest, MatchRes *pMRes)
{
	bool RetVal;
	regex Rx(RExp);
	smatch Mr; bool Found;
	
	auto First = InStr.cbegin(); auto Last = InStr.cend();
	
	if (Offest) First += Offest;
	
	RetVal = regex_search(First, Last, Mr, Rx);

	if (pMRes)
	{
		pMRes->MrPos = Mr.position();
		pMRes->MrLen = Mr.length();
	}
	
	return RetVal;
}

map <string, string>
	KwParse(const string & InStr, string FldDiv, string ArgDiv, string KeyWords, bool UseQuotes, bool UseFirst)
{
	map <string, string> RetVal;
	
	vector <string> KWrds(move(RSplit(KeyWords, ";")));
	vector <string> Fields(move(RSplit(InStr, FldDiv)));
	vector <string> KwVal;
	string ProcKw, ProcArg; size_t MaxIndex;
	
	for (auto & Str : KWrds)
		if (UseQuotes) Str = Trim(SToLower(Str));
		else Str = Gsub(SToLower(Str), "[[:space:]]", "");
	
	for (auto & Str : Fields)
	{
		KwVal = RSplit(Str, ArgDiv, UseFirst);
		MaxIndex = KwVal.size(); if (MaxIndex < 1) continue;
		MaxIndex--;
		
		if (UseQuotes)
		{
			ProcKw = Trim(SToLower(KwVal[0]));
			ProcKw = Gsub(ProcKw, "^\"|\"$", "");
		}
		else ProcKw = Gsub(SToLower(KwVal[0]), "[[:space:]]", "");
		
		for (auto & Kw : KWrds)
		{
			if (Kw == ProcKw)
			{ 
				if (MaxIndex > 0)
				{
					ProcArg = Trim(KwVal[MaxIndex]);
					if (UseQuotes) ProcArg = Gsub(ProcArg, "^\"|\"$", "");
				}
				else ProcArg = "";
				
				RetVal.insert(pair <string, string> (Kw, ProcArg));
			}
		}
	}
	
	return RetVal;
}
