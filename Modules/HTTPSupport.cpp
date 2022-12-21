#include "HTTPSupport.hpp"

using namespace std;

unordered_map <int, string> HTTPSupport::SrvAnswer =
	{
	{200, "OK"}, {400, "Bad Request"},
	{404, "Not Found"}, {409, "Conflict"},
	{413, "Payload Too Large"}
	};

string HTTPSupport::ConnStr[] = {"close", "keep-alive"};
string HTTPSupport::ContTypeStr[] = {"text", "image", "audio", "video", "application"};

string HTTPSupport::DfTextStr = "plain";
string HTTPSupport::DfImageStr = "jpeg";
string HTTPSupport::DfAudioStr = "basic";
string HTTPSupport::DfVideoStr = "mpeg";
string HTTPSupport::DfAppStr = "octet-stream";


HTTPSupport::HTTPSupport()
{
	ResetAnswer();
}

void HTTPSupport::ResetAnswer()
{
	HeaderType = HTypes::Answer;
	ContentTypeNum = CTypes::Text;
	Charset = "UTF-8";
	ConnType = Conn::Close;
	
	FileName = "";
	CurConType = "";
	CurConSubType = "";
	Host = "";
	
	AnswerCode = Answer::Ok;
	ContentLength = 0;
}

void HTTPSupport::DetectCType()
{
	string FNameWoPath, FNameWoExt, FExt;
	
	FNameWoPath = Gsub(FileName, "^/|/$", "");
	
	if (FNameWoPath != "")
	{
		vector <string> PathParts(move(RSplit(FNameWoPath, "/")));
		FNameWoPath = PathParts[PathParts.size() - 1];
		
		vector <string> FileParts(move(RSplit(FNameWoPath, "\\.")));
		FNameWoExt = Trim(SToLower(FileParts[0]));
		
		if (FileParts.size() > 1 && FNameWoExt != "")
		{
			FExt = Trim(SToLower(FileParts[FileParts.size() - 1]));
			
			if (FExt == "jpg" || FExt == "jpeg")
			{
				ContentTypeNum = CTypes::Image; CurConSubType = "jpeg";
			}
			else if (FExt == "gif")
			{
				ContentTypeNum = CTypes::Image; CurConSubType = "gif";
			}
			else if (FExt == "ico")
			{
				ContentTypeNum = CTypes::Image; CurConSubType = "x-icon";
			}
			else if (FExt == "avi")
			{
				ContentTypeNum = CTypes::Video; CurConSubType = "vnd.avi";
			}
			else if (FExt == "wmv")
			{
				ContentTypeNum = CTypes::Video; CurConSubType = "x-ms-wmv";
			}
			else if (FExt == "mp4")
			{
				ContentTypeNum = CTypes::Video; CurConSubType = "mp4";
			}
			else if (FExt == "wav")
			{
				ContentTypeNum = CTypes::Audio; CurConSubType = DfAudioStr;
			}
			else if (RMatch(FExt, "^txt$|^cpp$|^hpp$|^c$|^h$|^sh$|^ini$|^py$|^cmd$|^bat$|^sql$|^json$"))
			{
				ContentTypeNum = CTypes::Text; CurConSubType = DfTextStr;
			}
			else if (FExt == "htm" || FExt == "html")
			{
				ContentTypeNum = CTypes::Text; CurConSubType = "html";
			}
			else if (FExt == "zip")
			{
				ContentTypeNum = CTypes::App; CurConSubType = "zip";
			}
			else
			{
				ContentTypeNum = CTypes::App; CurConSubType = DfAppStr;
			}
		}
		else
		{
			if (RMatch(Trim(SToLower(FNameWoPath)),
				"^makefile$|^\\.bash_history$|^\\.bash_profile$|^\\.bash_logout$|^\\.bashrc$")
				)
			{
				ContentTypeNum = CTypes::Text; CurConSubType = DfTextStr;
			}
			else
			{
				ContentTypeNum = CTypes::App; CurConSubType = DfAppStr;
			}
		}
	}
}

void HTTPSupport::SetSubType()
{
	if (CurConSubType == "")
		switch (ContentTypeNum)
		{
		case CTypes::Text:
			CurConSubType = (HeaderType == HTypes::Answer) ?
				"html" : DfTextStr;
			break;
		case CTypes::Image:
			CurConSubType = DfImageStr;
			break;
		case CTypes::Audio:
			CurConSubType = DfAudioStr;
			break;
		case CTypes::Video:
			CurConSubType = DfVideoStr;
			break;
		case CTypes::App:
			CurConSubType = DfAppStr;
			break;
		}
}

void HTTPSupport::MkContentType()
{
	CurConType = ContTypeStr[ContentTypeNum] + "/" + CurConSubType;

	if (ContentTypeNum == CTypes::Text)
		CurConType += "; charset=" + Charset;
}

string HTTPSupport::GetHTTPAnswer()
{
	string RetVal, StrAns;
	
	HeaderType = (FileName == "") ?
		HTypes::Answer : HTypes::File;
		
	if (HeaderType == HTypes::File)
		DetectCType();
	else
		ContentLength = MessageStr.length();
	
	SetSubType(); MkContentType();
	
	if (SrvAnswer.find(AnswerCode) == SrvAnswer.end())
		AnswerCode = 200;
	
	StrAns = to_string(AnswerCode) + " " + SrvAnswer[AnswerCode];
	
	RetVal =
	"HTTP/1.1 " + StrAns + "\r\n" +
	"Host: " + Host + "\r\n" +
	"Content-Type: " + CurConType + "\r\n" +
	"Connection: " + ConnStr[ConnType] + "\r\n" +
	"Content-Length: " + to_string(ContentLength) + "\r\n\r\n";
	
	if (HeaderType == HTypes::Answer) RetVal += MessageStr;
	
	return RetVal;
}

int HTTPSupport::ParseMethod(string & InStr, string & What, string & Proto)
{
	int RetVal = 0;
	vector <string> Fields(move(RSplit(InStr, "[[:space:]]")));
	size_t Sz = Fields.size();
	string TstStr;
	
	if(Sz > 1)
	{
		TstStr = SToUpper(Fields[0]);
		
		RetVal = (TstStr == "GET") ? 1 : (TstStr == "POST") ? 2 : 0;
		
		if (RetVal)
		{
			What = Fields[1]; if(Sz > 2) Proto = Fields[2];
		}
	}
	
	return RetVal;
}

void HTTPSupport::ParseReqParams(string & InStr, map <string, string> & ParamList)
{
	map <string, string> CurFound(move(
		KwParse(InStr, "\n", ":", "Host; Connection; Cache-Control; Upgrade-Insecure-Requests;"
									"User-Agent; Accept; Accept-Encoding; Accept-Language;"
									"Origin; Content-Type; Referer; Content-Length;"
									"Content-Disposition; Content-Transfer-Encoding"
									, true, true)
	));
	
	ParamList.insert(CurFound.begin(), CurFound.end());
}
