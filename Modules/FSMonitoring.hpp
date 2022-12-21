#ifndef FS_Monitoring_h
#define FS_Monitoring_h

#include <unistd.h>
#include <sys/inotify.h>

#include <string>
#include <memory>

using namespace std;

class FSMonObject
{
public:
	static uint32_t DfDirMask, DfFileMask;
	
	FSMonObject(string WatchPath = "/", uint32_t EvMask = DfDirMask);
	~FSMonObject();
	
	int InitMon(string WatchPath = "/", uint32_t EvMask = DfDirMask);
	uint32_t WatchEvents();
	void CloseMon();
	
private:
	
	string MonPath;
	int MonFd, Watchd; uint32_t EventMask;
	
	size_t EvBufLen;
	unique_ptr <char> UpEvBuffer;
};

#endif //FS_Monitoring_h