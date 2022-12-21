#ifndef Client_Conn_h
#define Client_Conn_h

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <memory>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <cstring>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>

using namespace std;

struct Message
{
	static map<string, string> EmptyParams;
	
	unsigned long DataType;
	unique_ptr <map <string, string>> UpParams;
	
	ssize_t Len;
	unique_ptr <char[]> UpData;
	
	Message(const Message & InMsg);
	
	Message(Message && InMsg);
	
	Message(const char *pData = "Empty", ssize_t DtLen = 6, unsigned long DtType = 0
			, map<string, string> & Params = EmptyParams);
	
	Message & operator= (const Message & RightMsg);
	
	Message & operator= (Message && RightMsg);
};

class ClientConn
{
	thread ReceiveThread, TransmitThread;
	mutex MuTxQueueLock, MuTxLock;
	recursive_mutex MuIfLock;
	promise <void> TxLockPromise;
	future <void> TxLockFut;
	atomic<bool>
		StartRxTx, RxProcess, TxProcess,
		SockDescValid, ConnUsed, RepeatTransmission;
	
	static atomic<size_t> ConnCounter, ObjCounter;
	
public:
	
	struct TxData
	{
		const char *pData; ssize_t Len;
		unsigned long DataType;
		
		TxData() : DataType(0), Len(0), pData(NULL) {}
	};
	
	ClientConn(int SockDescriptor = -1, sockaddr_in clientAddress = {0}, sockaddr_in serverAddress = {0});
	virtual ~ClientConn();
	
	void SetSocket(int SockDescriptor, sockaddr_in clientAddress = {0}, sockaddr_in serverAddress = {0});
	
	void Start();
	void Stop();
	bool GetConnState();
	static size_t GetConnCount();
	static size_t GetObjCount();
	void SendData(char *pData, ssize_t Len, unsigned long DtType = 0
				, map<string, string> & Params = Message::EmptyParams);
private:
	
	int SockDesc;
	list <Message> TransmitQueue;
	
	void ReceiveProcess(); void TransmitProcess();
	void CloseConn();
	
	void SetTxLock();
	void UnlockTxThr();
	void StopTxThread();
	
protected:
	
	struct sockaddr_in ServerAddress, ClientAddress;
	
	virtual bool ProcessRxBuffer(char *pInData, ssize_t Len);
	virtual TxData ProcessTxData(Message & InMsg);
	void StartRepeatTx();
	void StopRepeatTx();
};

#endif // Client_Conn_h