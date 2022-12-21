#include <stdlib.h>

#include "./Modules/ClientConn.hpp"
#include "./Modules/FileServer.hpp"
#include "./Modules/SocketMain.hpp"

using namespace std;


int main(int argc, char const *argv[])
{
	IPSocket HTTPSrv;
	
	HTTPSrv.SetAddressPort("0.0.0.0", 3388);
	
	if (HTTPSrv.StartListen())
	{
		cout << "HTTP server was started." << endl;
		
		while(true)
		{
			this_thread::yield();
		};
	}
	else
	{
		cout << "Starting HTTP server failure." << endl;
	}
	
	return 0;
}
