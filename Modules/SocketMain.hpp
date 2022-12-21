#ifndef Socket_Main_hpp
#define Socket_Main_hpp

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

#include "ClientConn.hpp"
#include "FileServer.hpp"

using namespace std;

struct SockAdvParams
{
	string RootPath;
	size_t MaxFileSize;
};

class IPSocket
{
	struct sockaddr_in serverAddress, clientAddress;
	int listenSocket;
	
	list <unique_ptr<ClientConn>> LUpCurrConns;
	
	string StrServerIP;
	int Port;
	SockAdvParams AdvPars;
	
	thread SocketThr;
	atomic <bool> SockOpened, Start, AcceptProcess;
	recursive_mutex IfLock;
	
public:
	
	IPSocket(string SrvIP = "0.0.0.0", int SrvPort = 80);
	~IPSocket();
	
	int SetAddressPort(string SrvIP = "0.0.0.0", int SrvPort = 80);
	int SetAdvParams(SockAdvParams & Params);
	bool StartListen();
	int StopListen();
	bool GetListenState();
	
private:
	
	bool OpenSocket();
	void CloseSocket();
	void ErrCloseSocket();
	void AcceptMain();
	void ErrorOccured(string Message);
	
};

#endif //Socket_Main_hpp