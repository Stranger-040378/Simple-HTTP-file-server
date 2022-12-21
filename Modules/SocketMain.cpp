#include "SocketMain.hpp"

using namespace std;

IPSocket::IPSocket(string SrvIP, int SrvPort)
: Start( false ), AcceptProcess( false ), SockOpened ( false ),
	listenSocket( -1 )
{
	SetAddressPort(SrvIP, SrvPort);
}

IPSocket::~IPSocket()
{
	StopListen();
}

int IPSocket::SetAddressPort(string SrvIP, int SrvPort)
{
	lock_guard <recursive_mutex> Lock(IfLock);
	
	StrServerIP = SrvIP; Port = SrvPort;
	
	return 0;
}

int IPSocket::SetAdvParams(SockAdvParams & Params)
{
	lock_guard <recursive_mutex> Lock(IfLock);
	
	AdvPars = Params;
	
	return 0;
}

bool IPSocket::StartListen()
{
	lock_guard <recursive_mutex> Lock(IfLock);
	
	bool Retval = true;
	
	if (!GetListenState() && OpenSocket())
	{
		Start.store(true); AcceptProcess.store(true);
		
		thread LocThr( [this](){ this->AcceptMain(); } );
		
		SocketThr.swap(LocThr);
	}
	else Retval = false;
	
	return Retval;
}

int IPSocket::StopListen()
{
	lock_guard <recursive_mutex> Lock(IfLock);
	
	Start.store(false);
	
	CloseSocket();
	
	if (SocketThr.joinable()) SocketThr.join();
	
	return 0;
}

bool IPSocket::GetListenState()
{
	return AcceptProcess.load();
}

bool IPSocket::OpenSocket()
{
	if (SockOpened.load()) return false;
	
	// Создаем сокет для приема соединений от клиентов
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	// Разрешаем повторно использовать тот же порт серверу после перезапуска
	//(нужно, если сервер упал во время открытого соединения)
	int turnOn = 1;
	if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &turnOn, sizeof(turnOn)) == -1)
	{
		ErrorOccured("Setsockopt failed: "); return false;
	}
	
	// Setup the IP address / port
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(StrServerIP.c_str());
	serverAddress.sin_port = htons(Port);
	
	// Binding an IP address to a socket
	if (bind( listenSocket, (sockaddr *) &serverAddress, sizeof(serverAddress)) == -1)
	{
		ErrorOccured("Bind failed: "); return false;
	}
	
	// Starting IP / port listening
	if (listen(listenSocket, 1000) == -1)
	{
		ErrorOccured("Listen failed: "); return false;
	}
	
	SockOpened.store(true);
	
	return true;
}

void IPSocket::CloseSocket()
{
	if (SockOpened.load() && listenSocket > -1)
	{
		SockOpened.store(false); close(listenSocket);
	}
}

void IPSocket::ErrCloseSocket()
{
	if (listenSocket > -1)
	{
		SockOpened.store(false); close(listenSocket);
	}
}

void IPSocket::AcceptMain()
{
	list <unique_ptr<ClientConn>> :: iterator LUpIt;
	int clientSocket; bool SockSetted;
	int EmptyCount;
	socklen_t AddrLn = sizeof(sockaddr_in);
	
	if (!SockOpened.load())
	{
		cout << "Socket not opened !" << endl;
		
		Start.store(false); AcceptProcess.store(false);
		return;
	}
	
	cout << endl << "HTTP server started on" << endl
		<< "IP : " << inet_ntoa(serverAddress.sin_addr) << endl
		<< "Port : " << ntohs(serverAddress.sin_port) << endl << endl;
	
	while (Start.load(memory_order_relaxed))
 	{
		if (listenSocket > -1)
			clientSocket = accept(listenSocket, (sockaddr *)&clientAddress, &AddrLn);
		else
			clientSocket = -1;
		
		if (clientSocket < 0)
		{
			ErrorOccured("accept failed: ");
			break;
		}
		
		SockSetted = false;
		if (ClientConn::GetObjCount())
		{
			EmptyCount = 0;
			for (LUpIt = LUpCurrConns.begin(); LUpIt != LUpCurrConns.end(); LUpIt++)
			{
				if (!(*LUpIt)->GetConnState())
				{
					EmptyCount++;
					
					if (EmptyCount == 1)
					{
						cout << endl << "Reusing an existing thread object!" << endl;
						
						(*LUpIt)->SetSocket(clientSocket, clientAddress, serverAddress);
						SockSetted = true;
					}
					else if (EmptyCount > 2)
					{
						LUpCurrConns.erase(LUpIt);
						cout << "LUpCurrConns.size() = " << LUpCurrConns.size() << endl;
						break;
					}
				}
			}
		}
		if (!SockSetted)
		{
			cout << endl << "A new thread object has been created!" << endl;
			
			auto UpConn = make_unique<FileServer>();
			
			//sleep(1);
			
			UpConn->SetSocket(clientSocket, clientAddress, serverAddress);
			
			LUpCurrConns.push_back(move(UpConn));
			/*
			LUpCurrConns.push_back(move(make_unique<FileServer>(
				clientSocket, clientAddress, serverAddress)));
			*/
		}
	}
	CloseSocket();
	
	AcceptProcess.store(false);
}

void IPSocket::ErrorOccured(string Message)
{
	//cerr << Message.c_str() << " error code " << errno << " (" << strerror(errno) << ")\n";
	perror(Message.c_str());
	
	ErrCloseSocket(); Start.store(false);
}
