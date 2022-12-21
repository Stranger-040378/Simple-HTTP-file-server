#ifndef Dirs_And_Files_h
#define Dirs_And_Files_h

#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mutex>
#include <atomic>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <fstream>

#include "StringFunctions.hpp"
#include "FSMonitoring.hpp"

using namespace std;

class FSObject
{
public:
	
	string RootPath, CurPath;
	
	enum FileTypes
	{
		Unknown = 0, File, Directory, SymLink
	};
	
	enum RetCodes
	{
		Ok = 0, NotFound, NotAllowed,
		ThisIsFileNotDir, ThisIsDirNotFile, CantOpenFile,
		OtherError
	};
	
	struct FileDesc
	{
		bool IsDir, IsLink;
		string Name; int Attrs;
		size_t Size;
	};
	
	FSMonObject FsWatch;
	
	FSObject(string Dir = "/"); ~FSObject();
	
	vector <FileDesc> GetDirContent();
	string CorrectPath(string Path);
	int SetRootPath(string Dir = "/", bool Lock = false);
	int ChDir(string Dir);
	int OpenFile(string Name, bool Write = false);
	int WriteBuffer(char *pBuff, size_t Len);
	int Truncate(string Name, size_t Size);
	void CloseFile();
	size_t GetFileSize(string Name);
	size_t ReadBuffer(char *pBuff, size_t Len);
	
private:
	
	thread MonThr; mutex MuDirLock, CurFileLock;
	atomic <bool> MonProcess;
	
	fstream CurFile;
	vector <FileDesc> CurDirContent;
	bool RootPathSetted, WalkAnywhereAllowed;
	
	
	int CheckCache(string Dir);
	int CheckDirOrFile(string Dir);
	void BuildPath(vector <string> & InStrAr, string & Retstr,
					size_t End, size_t Begin = 0, bool RemEndSep = true);
	int ReadDirContent(bool Lock = false, bool RunWatch = true);
	void StartMon(); void StopMon();
	
	void MonChanges();
};

#endif // Dirs_And_Files_h