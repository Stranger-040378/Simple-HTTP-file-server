#ifndef HTTP_Support_h
#define HTTP_Support_h

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>

#include "StringFunctions.hpp"

using namespace std;

class HTTPSupport
{
public:
	static unordered_map <int, string> SrvAnswer;
	static string ConnStr[];
	static string ContTypeStr[];
	static string DfTextStr, DfImageStr, DfAudioStr, DfVideoStr, DfAppStr;
	
	enum Conn { Close, Alive };
	enum CTypes { Text, Image, Audio, Video, App };
	enum HTypes { Answer, File };
	enum Answer
		{
		Ok = 200, BadRequest = 400, NotFound = 404, Conflict = 409, PayloadTooLarge = 413
		};
	
	int AnswerCode;
	string MessageStr;
	string FileName;
	
	string Host;
	
	int ContentTypeNum;
	string CurConType;
	string CurConSubType;
	string Charset;
	size_t ContentLength;
	
	int ConnType;
	
	int HeaderType;
	
	HTTPSupport();
	
	void ResetAnswer();
	string GetHTTPAnswer();
	
	int ParseMethod(string & InStr, string & What, string & Proto);
	void ParseReqParams(string & InStr, map <string, string> & ParamList);
	
	void MkContentType();
	void SetSubType();
	void DetectCType();
};

#endif //HTTP_Support_h
