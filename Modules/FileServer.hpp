#ifndef File_Server_h
#define File_Server_h

#include <memory>
#include <atomic>
#include <map>
#include <string>

#include "ClientConn.hpp"
#include "DirsAndFiles.hpp"

#include "StringFunctions.hpp"
#include "HTTPSupport.hpp"
#include "HtmlWork.hpp"

using namespace std;

class FileServer : public ClientConn
{
public:
	
	FileServer(int SockDescriptor = -1,
			sockaddr_in clientAddress = {0}, sockaddr_in serverAddress = {0});
	
	virtual ~FileServer();
	
protected:
	
	enum TxCmds
	{
		TxBuffer = 0,
		SendDirContent,
		SendFileContent,
		SendReqParams
	};
	
	FSObject FSObj;
	
	virtual bool ProcessRxBuffer(char *pInData, ssize_t Len);
	virtual TxData ProcessTxData(Message & InMsg);
	
private:
	
	// Receive module variables
	int CurrMethod;
	string ResStr, Boundary, MethPath, Proto, UpFileName;
	map <string, string> ReqParams;
	bool RecText, MultiPartBegin, WriteProcess;
	bool CrDet, CrLfEol;
	
	size_t CurContentLength, WrCount, BoundOffset;
	
	unique_ptr <char[]> UpStrBuf;
	char *pStrBuf; size_t StrBufLen;
	
	// Transmit module variables
	string SrvAnswer;
	unique_ptr <char[]> UpDownBuf;
	char *pDownBuf; size_t DownBufLen;
	
	// HTTP Protocol support
	HTTPSupport HTTPm;
	
	// Service functions
	// Recieve
	void InitRxVars();
	void WriteToFile(char *pWriteBuf, ssize_t TxLn, bool CloseFile);
	
	// Transmit
	string MakeDirList(map <string, string> & InPars, const string & AdvText);
	string MakeNotFound(map <string, string> & InPars);
	
	// Common
	bool FindInMap(map <string, string> & InPars, const string & Search);
};

#endif // File_Server_h
