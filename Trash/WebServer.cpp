#include <stdlib.h>

#include "./Modules/ClientConn.hpp"
#include "./Modules/FileServer.hpp"

using namespace std;

void handleError(string msg)
{
  cerr << msg << " error code " << errno << " (" << strerror(errno) << ")\n";
  exit(1);
}

int main(int argc, char* argv[])
{
	int port = 28563;
	int listenSocket;
	struct sockaddr_in serverAddress, clientAddress;
	socklen_t AddrLn = sizeof(sockaddr_in);
	
	//создаем сокет для приема соединений всех клиентов
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//разрешаем повторно использовать тот же порт серверу после перезапуска (нужно, если сервер упал во время открытого соединения)
	int turnOn = 1;
	if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &turnOn, sizeof(turnOn)) == -1)
	handleError("setsockopt failed:");
	
	// Setup the TCP listening socket
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("0.0.0.0");
	serverAddress.sin_port = htons(port);
	
	if (bind( listenSocket, (sockaddr *) &serverAddress, sizeof(serverAddress)) == -1)
		handleError("bind failed:");
	
	if (listen(listenSocket, 1000) == -1) handleError("listen failed:");
	
	list <unique_ptr<ClientConn>> LUpCurrConns;
	list <unique_ptr<ClientConn>> :: iterator LIt;
	int clientSocket; bool SockSetted;
	int EmptyCount;
	
 	while (true)
 	{
		clientSocket = accept(listenSocket, (sockaddr *)&clientAddress, &AddrLn);
		if (clientSocket < 0) handleError("accept failed:");
		
		SockSetted = false;
		if (ClientConn::GetObjCount())
		{
			EmptyCount = 0;
			for (LIt = LUpCurrConns.begin(); LIt != LUpCurrConns.end(); LIt++)
			{
				if (!(*LIt)->GetConnState())
				{
					EmptyCount++;
					
					if (EmptyCount == 1)
					{
						(*LIt)->SetSocket(clientSocket, clientAddress, serverAddress);
						cout << endl << "Reusing an existing thread object!" << endl;
						SockSetted = true;
					}
					else if (EmptyCount > 2)
					{
						LUpCurrConns.erase(LIt);
						cout << "LUpCurrConns.size() = " << LUpCurrConns.size() << endl;
						break;
					}
				}
			}
		}
		if (!SockSetted)
		{
			LUpCurrConns.push_back(move(make_unique<FileServer>(
				clientSocket, clientAddress, serverAddress)));
			cout << endl << "A new thread object has been created!" << endl;
		}
	}
	
	return 0;
}
