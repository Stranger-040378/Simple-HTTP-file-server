#include "DirsAndFiles.hpp"

#include <iostream>

using namespace std;

FSObject::FSObject(string Dir)
: FsWatch(Dir), MonProcess(false)
{
	RootPath = Dir; CurPath = RootPath;
	
	WalkAnywhereAllowed = true;
	RootPathSetted = false;
	
	//SetRootPath(Dir);
}

FSObject::~FSObject() { StopMon(); }

vector <FSObject::FileDesc> FSObject::GetDirContent()
{
	lock_guard <mutex> Locker(MuDirLock);
	vector <FSObject::FileDesc> RetVal;
	
	if (!RootPathSetted) SetRootPath(RootPath);
	
	RetVal = CurDirContent;
	
	return RetVal;
}

int FSObject::SetRootPath(string Dir, bool Lock)
{
	int RetVal;
	string MemRootP = RootPath, MemCurP = CurPath;
	
	if (Lock) MuDirLock.lock();
	
	RootPath = Dir; CurPath = Dir;
	RetVal = ReadDirContent();
	
	if (RetVal != RetCodes::Ok)
	{
		RootPath = MemRootP; CurPath = MemCurP;
	}
	else RootPathSetted = true;
	
	MuDirLock.unlock();
	
	return RetVal;
}

void FSObject::BuildPath(vector <string> & InStrAr, string & Retstr,
							size_t End, size_t Begin, bool RemEndSep)
{
	size_t SepCheck = (RemEndSep) ? End - 1 : End;
	
	for (size_t i = Begin; i < End; i++)
	{
		Retstr += InStrAr[i]; if (i < SepCheck) Retstr += "/";
	}
}

int FSObject::ChDir(string Dir)
{
	lock_guard <mutex> Locker(MuDirLock);
	
	vector <string> SplDir, SplCurPath; size_t SplDirLn, SplCurPathLn;
	string TmpPath, MemPath;
	int RetVal = RetCodes::Ok;
	
	if (!RootPathSetted)
	{
		RetVal = SetRootPath(RootPath);
		if (!RootPathSetted) return RetVal;
	}
	
	Dir = Trim(Dir);
	
	if (Dir == "." || Dir == "/")
	{
		TmpPath = CorrectPath(RootPath);
		if (CorrectPath(CurPath) != TmpPath)
		{
			CurPath = RootPath; RetVal = ReadDirContent();
		}
		return RetVal;
	}
	else if (Dir == "") return RetVal;
	
	Dir = Gsub(Dir, "^/|/$", "");
	
	SplDir = RSplit(Dir, "/"); SplDirLn = SplDir.size();
	
	TmpPath = CurPath; TmpPath = Gsub(TmpPath, "^/|/$", "");
	SplCurPath = RSplit(TmpPath, "/"); SplCurPathLn = SplCurPath.size();
	
	if (Dir == "..")
	{
		TmpPath = CorrectPath(RootPath);
		if (CorrectPath(CurPath) != TmpPath)
		{
			TmpPath = "/";
			if (SplCurPathLn > 0) BuildPath(SplCurPath, TmpPath, SplCurPathLn - 1);
			CurPath = TmpPath; RetVal = ReadDirContent();
		}
	}
	else
	{
		size_t k = 0, StartPos = 0, Diff = 0;
		bool CompFlag = true;
		
		for (size_t i = 0; i < SplCurPathLn; i++)
		{
			if (k < SplDirLn)
			{
				if (SplCurPath[i] == SplDir[k])
				{
					if (k == 0) StartPos = i; k++;
				}
				else if(k) { CompFlag = false; break; }
			}
			else break;
		}
		Diff = SplDirLn - k;
		
		if (k == 0)
		{
			if (SplDirLn == 1)
			{
				cout << "Check Cache for Dir : " << Dir << endl;
				RetVal = CheckCache(Dir);
			}
			
			if (SplDirLn > 1 || RetVal == RetCodes::NotFound)
			{
				TmpPath = CorrectPath(RootPath) + Dir;
				MemPath = CurPath; CurPath = TmpPath;
				
				cout << "k = 0, CurPath = " << CurPath << endl;
				
				RetVal = ReadDirContent();
				if (RetVal != RetCodes::Ok) CurPath = MemPath;
			}
		}
		else
		{
			if (Diff == 1 && CompFlag)
			{
				cout << "Check Cache for : " << SplDir[SplDirLn - 1] << endl;
				RetVal = CheckCache(SplDir[SplDirLn - 1]);
			}
			else
			{
				TmpPath = "/";
				BuildPath(SplCurPath, TmpPath, StartPos + 1, 0, false);
				BuildPath(SplDir, TmpPath, SplDirLn, 1);
				
				cout << "Equiv: TmpPath = " << TmpPath << endl;
				
				if (CorrectPath(TmpPath) != CorrectPath(CurPath))
				{
					MemPath = CurPath; CurPath = TmpPath;
					RetVal = ReadDirContent();
					if (RetVal != RetCodes::Ok) CurPath = MemPath;
				}
			}
		}
	}
	
	return RetVal;
}

int FSObject::CheckCache(string Dir)
{
	int RetVal = RetCodes::Ok;
	bool DirFound = false;
	
	for (auto & Item: CurDirContent)
	{
		cout << "Item.Name = " << Item.Name << endl;
		
		if (Item.Name == Dir)
		{
			if (Item.IsDir)
			{
				CurPath = CorrectPath(CurPath) + Dir;
				RetVal = ReadDirContent();
				DirFound = true; break;
			}
			else
			{
				RetVal = RetCodes::ThisIsFileNotDir;
				DirFound = true; break;
			}
		}
	}
	if (!DirFound) RetVal = RetCodes::NotFound;
	
	return RetVal;
}

int FSObject::CheckDirOrFile(string Dir)
{
	struct stat FilAttrs;
	int RetVal = RetCodes::Ok;
	
	if(!stat(Dir.c_str(), &FilAttrs))
	{
		if(!S_ISDIR(FilAttrs.st_mode))
			RetVal = RetCodes::ThisIsFileNotDir;
	}
	else RetVal = RetCodes::NotFound;
	
	return RetVal;
}

string FSObject::CorrectPath(string Path)
{
	string TmpPath = Trim(Path);
	
	string RetVal =
		(TmpPath.substr(TmpPath.length() - 1) == "/") ? TmpPath : TmpPath + "/";
		
	return RetVal;
}

int FSObject::ReadDirContent(bool Lock, bool RunWatch)
{
	FSObject::FileDesc CurFile;
	multimap <string, FSObject::FileDesc> Dirs, Files;
	string FullName;
	
	DIR *dir;
	struct dirent *entry;
	struct stat FilAttrs, LnkAttrs;
	
	int RetVal = RetCodes::Ok;
	
	if (RunWatch) StopMon();
	
	if (Lock) MuDirLock.lock();
	
	RetVal = CheckDirOrFile(CurPath);
	if (RetVal != RetCodes::Ok)
	{
		if (Lock) MuDirLock.unlock();
		
		if (RetVal == RetCodes::NotFound)
			cout << "Not Found !" << endl;
		else
			cout << "This is file !" << endl;
		
		return RetVal;
	}
	
	dir = opendir(CurPath.c_str());
	
	if (!dir)
	{
		if (Lock) MuDirLock.unlock();
		
		RetVal = RetCodes::NotFound;
		perror("diropen");
		
		return RetVal;
	}
	
	while ( (entry = readdir(dir)) != NULL )
	{
		FullName = CorrectPath(CurPath) + entry->d_name;
		
		if(!stat(FullName.c_str(), &FilAttrs))
			if(!lstat(FullName.c_str(), &LnkAttrs))
			{
				CurFile.Name = entry->d_name;
				CurFile.IsLink = (S_ISLNK(LnkAttrs.st_mode)) ? true : false;
				CurFile.Size = FilAttrs.st_size;
				
				if(S_ISDIR(FilAttrs.st_mode))
				{
					CurFile.IsDir = true;
					Dirs.insert(pair <string, FSObject::FileDesc> {CurFile.Name, CurFile});
				}
				else
				{
					CurFile.IsDir = false;
					Files.insert(pair <string, FSObject::FileDesc> {CurFile.Name, CurFile});
				}
			}		
	};
	
	closedir(dir);
	
	CurDirContent.clear();
	for (auto & Item : Dirs) CurDirContent.push_back(Item.second);
	for (auto & Item : Files) CurDirContent.push_back(Item.second);
	
	if (Lock) MuDirLock.unlock();
	
	if (RunWatch)
	{
		FsWatch.InitMon(CurPath); StartMon();
	}
	
	return RetVal;
}

void FSObject::StartMon()
{
	MonProcess.store(true, std::memory_order_relaxed);
	
	thread LocThr( [this]() { this->MonChanges(); } );
	
	cout << "Dir monitoring started !" << endl;
	
	MonThr.swap(LocThr);
}

void FSObject::StopMon()
{
	MonProcess.store(false, std::memory_order_relaxed);
	FsWatch.CloseMon();
	
	cout << "StopMon : Wait for a thread stop !" << endl;
	
	if (MonThr.joinable()) MonThr.join();
	
	cout << "Mon thread was stopped !" << endl;
}

void FSObject::MonChanges()
{
	uint32_t CurEvent;
	
	while (!MonProcess.load(std::memory_order_relaxed));
	
	while (MonProcess.load(std::memory_order_relaxed))
	{
		CurEvent = FsWatch.WatchEvents();
		if (!CurEvent) break;
		else
		{
			cout << "Refrashing dir content !" << endl;
			if (ReadDirContent(true, false) != RetCodes::Ok) break;
		}
	}
	
	cout << "Dir monitoring finished !" << endl;
	MonProcess.store(false, std::memory_order_relaxed);
}

int FSObject::OpenFile(string Name, bool Write)
{
	int RetVal = RetCodes::Ok;
	lock_guard <mutex> Lock(CurFileLock);
	
	if (Write)
		CurFile.open(Name, ios_base::binary|ios_base::out);
	else
		CurFile.open(Name, ios_base::binary|ios_base::in);
	
	if (!CurFile.is_open())
	{
		RetVal = RetCodes::CantOpenFile;
		perror(("OpenFile : File : " + Name).c_str());
	}
	
	return RetVal;
}

int FSObject::WriteBuffer(char *pBuff, size_t Len)
{
	int RetVal = RetCodes::Ok;
	lock_guard <mutex> Lock(CurFileLock);
	
	if (CurFile.is_open())
	{
		CurFile.write(pBuff, Len);
	}
	else RetVal = RetCodes::OtherError;
	
	return RetVal;
}

int FSObject::Truncate(string Name, size_t Size)
{
	int RetVal = RetCodes::Ok;
	lock_guard <mutex> Lock(CurFileLock);
	
	if (truncate(Name.c_str(), Size) < 0)
		RetVal = RetCodes::OtherError;
	
	return RetVal;
}

void FSObject::CloseFile()
{
	lock_guard <mutex> Lock(CurFileLock);
	
	CurFile.close();
}

size_t FSObject::GetFileSize(string Name)
{
	lock_guard <mutex> Lock(CurFileLock);
	struct stat fi;
	
	stat(Name.c_str(), &fi);
	
	return fi.st_size;
}

size_t FSObject::ReadBuffer(char *pBuff, size_t Len)
{
	size_t RetVal;
	lock_guard <mutex> Lock(CurFileLock);
	
	if (CurFile.is_open() && !CurFile.eof())
	{
		CurFile.read(pBuff, Len); RetVal = CurFile.gcount();
	}
	else RetVal = 0;
	
	return RetVal;
}
