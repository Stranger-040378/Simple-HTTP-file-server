#include "FSMonitoring.hpp"

#include <iostream>

using namespace std;

uint32_t FSMonObject::DfDirMask =
	IN_CREATE|IN_DELETE|IN_DELETE_SELF|IN_MOVE_SELF;
uint32_t FSMonObject::DfFileMask =
	IN_MODIFY|IN_CLOSE|IN_DELETE_SELF|IN_MOVE_SELF;

FSMonObject::FSMonObject(string WatchPath, uint32_t EvMask)
{
	EvBufLen = 0; MonFd = -1; Watchd = -1;
	MonPath = WatchPath; EventMask = EvMask;
	
	//InitMon(MonPath, EvMask);
}

FSMonObject::~FSMonObject()
{
	CloseMon();
}

int FSMonObject::InitMon(string MonPath, uint32_t EvMask)
{
	size_t EventSize = sizeof(struct inotify_event);
	size_t MaxEventsCount = 100, MaxNameSize = 2048;
	
	cout << "InitMon : MonPath = " << MonPath << endl;
	
	CloseMon();
	
	MonFd = inotify_init();
	if (MonFd < 0)
	{
		perror("InitMon : inotify_init");
		return MonFd;
	}
	
	Watchd = inotify_add_watch(MonFd, MonPath.c_str(), EvMask);
	if (Watchd < 0)
	{
		perror("InitMon : inotify_add_watch");
		close(MonFd); MonFd = -1; return Watchd;
	}
	
	EvBufLen = MaxEventsCount * ( EventSize + MaxNameSize );
	UpEvBuffer.reset(new char[EvBufLen]);
	
	EventMask = EvMask;
	
	return MonFd;
}

uint32_t FSMonObject::WatchEvents()
{
	int i, RdCount;
	struct inotify_event *pEvent;
	size_t EventSize = sizeof(struct inotify_event);
	char *pBuffer;
	uint32_t RetVal = 0;
	
	if (MonFd > -1 && EvBufLen)
	{
		pBuffer = UpEvBuffer.get();
		
		RdCount = read(MonFd, pBuffer, EvBufLen);
		if (RdCount < 0)
		{
			inotify_rm_watch(MonFd, Watchd);
			close(MonFd); MonFd = -1;
			
			perror("WatchEvents : read"); return 0;
		}
		
		while (i < RdCount)
		{
			pEvent = reinterpret_cast<struct inotify_event *>(&pBuffer[i]);
			
			RetVal = pEvent->mask & EventMask;
			if (RetVal) break;
			
			i += EventSize + pEvent->len;
		}
	}
	
	return RetVal;
}

void FSMonObject::CloseMon()
{
	inotify_rm_watch(MonFd, Watchd);
	close(MonFd); MonFd = -1;
}
