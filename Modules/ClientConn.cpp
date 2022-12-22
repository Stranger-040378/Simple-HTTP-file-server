#include "ClientConn.hpp"

using namespace std;

Message::Message(const char *pData, ssize_t DtLen, unsigned long DtType, map<string, string> & Params)
: Len(DtLen), UpData(new char[DtLen]), DataType(DtType)
{
	if (Len > 0) memcpy(UpData.get(), pData, Len);
	
	if (DataType)
		UpParams.reset(new map <string, string>(Params));
}

Message::Message(const Message & InMsg)
: Len(InMsg.Len), UpData(new char[InMsg.Len]), DataType(InMsg.DataType)
{
	if (Len > 0) memcpy(UpData.get(), InMsg.UpData.get(), Len);
	
	if (DataType && InMsg.UpParams != nullptr)
		UpParams.reset(new map <string, string>(*InMsg.UpParams));
}

Message::Message(Message && InMsg)
: Len(exchange(InMsg.Len, 6)), UpData(move(InMsg.UpData)),
		DataType(exchange(InMsg.DataType, 0)), UpParams(move(InMsg.UpParams))
{
	InMsg.UpData = make_unique <char[]>(6);
	strcpy(InMsg.UpData.get(), "Empty");
}

Message & Message::operator= (const Message & RightMsg)
{
	Len = RightMsg.Len;
	if (Len > 0)
	{
		UpData.reset(new char[Len]);
		memcpy(UpData.get(), RightMsg.UpData.get(), Len);
	}
	
	DataType = RightMsg.DataType;
	if (DataType && RightMsg.UpParams != nullptr)
		UpParams.reset(new map <string, string>(*RightMsg.UpParams));
	
	return *this;
}

Message && Message::operator= (Message && RightMsg)
{
	Len = exchange(RightMsg.Len, 6);
	UpData = move(RightMsg.UpData);
	
	DataType = exchange(RightMsg.DataType, 0);
	UpParams = move(RightMsg.UpParams);
	
	RightMsg.UpData = make_unique <char[]>(6);
	strcpy(RightMsg.UpData.get(), "Empty");
	
	return move(*this);
}

map <string, string> Message::EmptyParams = {};
atomic<size_t> ClientConn::ObjCounter = {0};
atomic<size_t> ClientConn::ConnCounter = {0};

ClientConn::ClientConn(int SockDescriptor, sockaddr_in clientAddress, sockaddr_in serverAddress)
:	SockDesc (-1), ServerAddress {0}, ClientAddress {0},
	StartRxTx( false ), RxProcess( false ), TxProcess { false }, TxPromValid { false },
	ConnUsed { false }, SockDescValid { false },
	RepeatTransmission { false }
{
	ObjCounter.fetch_add(1);
	
	SetTxLock();
	
	SetSocket(SockDescriptor, clientAddress, serverAddress);
}

ClientConn::~ClientConn()
{
	Stop();
	
	ObjCounter.fetch_sub(1);
	cout << "Destructor was called !!!" << endl;
}

size_t ClientConn::GetConnCount() { return ConnCounter.load(); }
size_t ClientConn::GetObjCount() { return ObjCounter.load(); }

bool ClientConn::GetConnState()
{
	return ( ConnUsed.load() || RxProcess.load() || TxProcess.load() );
}

bool ClientConn::ProcessRxBuffer(char *pInData, ssize_t Len)
{
	if (pInData) SendData(pInData, Len);
	else return true;
	
	return false;
}

ClientConn::TxData ClientConn::ProcessTxData(Message & InMsg)
{
	TxData OutMsg;
	
	OutMsg.pData = InMsg.UpData.get(); OutMsg.Len = InMsg.Len;
	
	return OutMsg;
}

void ClientConn::ReceiveProcess()
{
	ssize_t ReceivedCount;
	char RdBuffer[2049];
	
	if( !(SockDescValid.load() && SockDesc > -1) )
	{
		RxProcess.store(false); ConnUsed.store(false);
		return;
	}
	
	ConnCounter.fetch_add(1);
	cout << endl << "Current connections = " << ConnCounter.load() << endl;
	cout << "Server address = " << inet_ntoa(ServerAddress.sin_addr) << endl;
	cout << "Client address = " << inet_ntoa(ClientAddress.sin_addr) << endl;
	cout << "Client port = " << ntohs(ClientAddress.sin_port) << endl;
	cout << "Objects count = " << ObjCounter.load() << endl << endl;
	
	bool RxInit = this->ProcessRxBuffer(NULL, 0);
	if (!RxInit) { cout << "Init failure !!!"; return; }
	
	while (StartRxTx.load())
	{
		if ( SockDescValid.load() )
			ReceivedCount = recv(SockDesc, RdBuffer, sizeof(RdBuffer) - 1, 0);
		else
			ReceivedCount = -1;
		
		if (ReceivedCount > 0)
		{
			this->ProcessRxBuffer(RdBuffer, ReceivedCount);
		}
		else if (ReceivedCount == 0)
		{
			cout << endl << "client disconnected..." << endl;
			StartRxTx.store(false);
			break;
		}
		else
		{
			cout << endl << "Rx Error..." << endl;
			StartRxTx.store(false);
			break;
		}
	}
	CloseConn();
	StopTxThread();
	
	ConnCounter.fetch_sub(1);
	cout << endl << "Current connections = " << ConnCounter.load() << endl;
	cout << "Objects count = " << ObjCounter.load() << endl << endl;
	
	RxProcess.store(false); ConnUsed.store(false);
}

void ClientConn::TransmitProcess()
{
	Message InMsg;
	bool GotMessage = false;
	ssize_t TransmittedCount;
	TxData TxMsg;
	
	if( !(SockDescValid.load() && SockDesc > -1) )
	{
		TxProcess.store(false); ConnUsed.store(false);
		return;
	}
	
	while (StartRxTx.load())
	{
		if (!RepeatTransmission.load())
		{
			MuTxQueueLock.lock();
			
			if (!TransmitQueue.empty())
			{
				InMsg = move(TransmitQueue.front());
				TransmitQueue.pop_front();
				GotMessage = true;
			}
			else GotMessage = false;
			
			MuTxQueueLock.unlock();
		}
		else
		{
			//cout << "Use repeat transmission." << endl;
			InMsg.DataType = TxMsg.DataType;
			GotMessage = true;
		}
		
		if (GotMessage)
		{
			TxMsg = this->ProcessTxData(InMsg);
			//cout << "TxMsg.Len = " << TxMsg.Len << endl;
			
			if (TxMsg.Len > 0)
			{
				if ( SockDescValid.load() )
				{
					TransmittedCount = send(SockDesc, TxMsg.pData, TxMsg.Len, 0);
					//TransmittedCount = write(SockDesc, TxMsg.pData, TxMsg.Len);
				}
				else
					TransmittedCount = 0;
				
				if (TransmittedCount < 1)
				{
					StartRxTx.store(false, memory_order_relaxed);
					break;
				}
			}
		}
		else
		{
			if (StartRxTx.load(memory_order_relaxed))
			{
				//cout << endl << "Starting wait...." << endl;
				
				SetTxLock();
				TxLockFut.wait(); TxLockFut.get();
			}
		}
		
		//cout << endl << "Tx cycle step !" << endl;
	}
	
	cout << endl << "Tx process is dead!" << endl;
	
	TxProcess.store(false); ConnUsed.store(false);
}

void ClientConn::StartRepeatTx()
{
	cout << "StartRepeatTx" << endl;
	RepeatTransmission.store(true);
}

void ClientConn::StopRepeatTx()
{
	cout << "StopRepeatTx" << endl;
	RepeatTransmission.store(false);
}

void ClientConn::SendData(char *pData, ssize_t Len, unsigned long DtType, map<string, string> & Params)
{
	lock_guard <mutex> Locker(MuTxQueueLock);
	
	if (pData)
	{
		if (!DtType)
			TransmitQueue.push_back(move(Message(pData, Len, DtType)));
		else
			TransmitQueue.push_back(move(Message(pData, Len, DtType, Params)));
		
		UnlockTxThr();
	}
}

void ClientConn::SetSocket(int SockDescriptor, sockaddr_in clientAddress, sockaddr_in serverAddress)
{
	lock_guard <recursive_mutex>Lock(MuIfLock);
	
	ConnUsed.store(true);
	if (SockDescriptor < 0)
	{
		ConnUsed.store(false); return;
	}
	
	StopRepeatTx(); Stop();
	
	SockDesc = SockDescriptor;
	ClientAddress = clientAddress; ServerAddress = serverAddress;
	SockDescValid.store(true);
	
	Start();
}

void ClientConn::Stop()
{
	lock_guard <recursive_mutex>Lock(MuIfLock);
	
	StartRxTx.store(false);
	
	CloseConn();
	StopTxThread();
	
	if (ReceiveThread.joinable()) ReceiveThread.join();
	if (TransmitThread.joinable()) TransmitThread.join();
}

void ClientConn::CloseConn()
{
	lock_guard <recursive_mutex>Lock(MuIfLock);
	
	StopRepeatTx();
	
	if (SockDescValid.load())
	{
		SockDescValid.store(false);
		
		if (SockDesc > -1)
		{
			shutdown(SockDesc, SHUT_RDWR); close(SockDesc);
		}
	}
}

void ClientConn::Start()
{
	lock_guard <recursive_mutex>Lock(MuIfLock);
	
	bool ThNotWork = !( RxProcess.load() || TxProcess.load() );
	
	if (ThNotWork && SockDescValid.load() && SockDesc > -1)
	{
		RxProcess.store(true); TxProcess.store(true);
		StartRxTx.store(true);
		
		thread RecThr( [this]() { this->ReceiveProcess(); } );
		//ReceiveThread = move(LocalThr);
		ReceiveThread.swap(RecThr);
		
		thread TransThr( [this]() { this->TransmitProcess(); } );
		TransmitThread.swap(TransThr);
	}
}

void ClientConn::SetTxLock()
{
	lock_guard <mutex> Lock(MuTxLock);
	
	TxLockPromise = move(promise <void>());
	TxLockFut = TxLockPromise.get_future();
	TxPromValid.store(true);
}

void ClientConn::UnlockTxThr()
{
	lock_guard <mutex> Lock(MuTxLock);
	
	try
	{
		if (TxPromValid.load() && TxLockFut.valid())
		{
			TxPromValid.store(false);
			TxLockPromise.set_value();
		}
	}
	catch(const std::exception & e)
	{
		//cout << endl << "exception : " << e.what() << endl;
	}
}

void ClientConn::StopTxThread()
{
	StartRxTx.store(false);
	
	while (TxProcess.load())
	{
		UnlockTxThr(); this_thread::yield();
	}
}
