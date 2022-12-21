#include "FileServer.hpp"

using namespace std;

FileServer::FileServer(int SockDescriptor, sockaddr_in clientAddress, sockaddr_in serverAddress)
: ClientConn(SockDescriptor, clientAddress, serverAddress), FSObj("/")
{
	StrBufLen = 3000;
	UpStrBuf.reset(new char[StrBufLen]);
	pStrBuf = UpStrBuf.get();
	
	DownBufLen = 6000;
	UpDownBuf.reset(new char[DownBufLen + 200]);
	pDownBuf = UpDownBuf.get();
}

FileServer::~FileServer()
{
	
}

void FileServer::InitRxVars()
{
	RecText = true;
	CrDet = false; CrLfEol = false;
	ResStr = ""; Boundary = ""; MethPath = ""; Proto = ""; UpFileName = "";
	
	MultiPartBegin = true;
	CurrMethod = 0; CurContentLength = 0;
	BoundOffset = 0; WriteProcess = false; WrCount = 0;
	
	ReqParams.clear();
	//BreakTransmission.store(true, memory_order_relaxed);
	
	cout << "Init Rx variables !!!. Client port = " << ntohs(ClientAddress.sin_port) << endl;
}

bool FileServer::ProcessRxBuffer(char *pInData, ssize_t Len)
{
	ssize_t k = 0;
	char EmptyStr[6] = "Empty"; size_t EmptySize = sizeof(EmptyStr);
	
	ssize_t PosBgn = 0, TxLn = Len;
	char *pWriteBuf = pInData;
	
	auto InsertPar = [this](const string & Param, const string & Value)
	{
		this->ReqParams.insert(pair <string, string> (Param, Value));
	};
	auto FindParam = [this](const string & Param)->bool
	{
		return (this->ReqParams.find(Param) != ReqParams.end());
	};
	
	if (!pInData)
	{
		// Initializing after accepting a connection
		InitRxVars(); return true;
	}
	
	// Rx buffer processing
	for (ssize_t i = 0; i < Len; i++)
	{
		// Text mode ---------------------------------------------------
		if (RecText)
		{
			if (pInData[i] == '\n')
			{
				CrLfEol = (CrDet) ? true : false;
				pStrBuf[k] = 0; ResStr.append(pStrBuf); ResStr = Trim(ResStr);
				
				cout << ResStr << endl;
				
				switch (CurrMethod)
				{
					case 0:
						CurrMethod = HTTPm.ParseMethod(ResStr, MethPath, Proto);
					break;
					case 1:
						// GET ------------------------------
						
						if (ResStr == "")
						{
							MethPath = UrlDecode(MethPath);
							InsertPar("Srv_Method_Path", MethPath);
							InsertPar("Srv_Method_Proto", Proto);
							InsertPar("Srv_Method", "Get");
							SendData(EmptyStr, EmptySize, TxCmds::SendDirContent, ReqParams);
							
							InitRxVars();
						}
						else HTTPm.ParseReqParams(ResStr, ReqParams);
					break;
					case 2:
						// POST ---------------------------
						
						if (ResStr == "")
						{
							string WrkStr;
							
							if (MultiPartBegin)
							{
								MethPath = UrlDecode(MethPath);
								InsertPar("Srv_Method_Path", MethPath);
								InsertPar("Srv_Method_Proto", Proto);
								InsertPar("Srv_Method", "Post");
								
								if (ReqParams.find("content-type") != ReqParams.end())
								{
									WrkStr = ReqParams["content-type"];
									map <string, string> CurCType(move(
									KwParse(WrkStr, ",|;", "=", "multipart/form-data; boundary")
									));
								
									if (CurCType.find("multipart/form-data") != CurCType.end() &&
										CurCType.find("boundary") != CurCType.end()
										)
									{
										Boundary = ""; BoundOffset = 0;
										MultiPartBegin = false;
									}
								}
								else
								{
									InsertPar("Srv_Error_Code", "400");
									SendData(EmptyStr, EmptySize, TxCmds::SendReqParams, ReqParams);
									InitRxVars();
								}
							}
							else
							{
								if (FindParam("content-disposition"))
								{
									//multipart/mixed
									WrkStr = ReqParams["content-disposition"];
									map <string, string> CurCDisp(move(
									KwParse(WrkStr, ",|;", "=", "form-data; name; filename")
									));
									
									UpFileName = CurCDisp["filename"];
									//cout << "filename = " << UpFileName << endl;
									
									if (Boundary != "")
									{
										// Start file uploading ---------------------------------
										
										// Adding CrLf at the beginning of the Boundary
										Boundary = (CrLfEol) ? "\r\n" + Boundary : "\n" + Boundary;
										
										RecText = false; BoundOffset = 0; WrCount = 0;
										
										cout << endl << "Redy to binary mode. Boundary = " << Boundary << endl;
										
										int Ret =
										FSObj.OpenFile(FSObj.CorrectPath(MethPath) + UpFileName, true);
										if (Ret == FSObject::RetCodes::Ok)
										{
											WriteProcess = true;
											
											if (i < Len - 1)
											{
												PosBgn = i + 1; TxLn = Len - PosBgn;
											}
											else
											{
												PosBgn = 0; TxLn = 0;
											}
											pWriteBuf = &pInData[PosBgn]; 
										}
										// --------------------------------------------------------
									}
									else
									{
										InsertPar("Srv_Error_Code", "400");
										SendData(EmptyStr, EmptySize, TxCmds::SendReqParams, ReqParams);
										InitRxVars();
										cout << endl << "Boundary is absent!" << endl;
									}
								}
								else
								{
									InsertPar("Srv_Error_Code", "400");
									SendData(EmptyStr, EmptySize, TxCmds::SendReqParams, ReqParams);
									InitRxVars();
								}
							}
						}
						else
						{
							if (!MultiPartBegin)
								if (Boundary == "")
								{
									Boundary = ResStr; cout << endl << "Getted Boundary = " << Boundary << endl;
								}
							
							HTTPm.ParseReqParams(ResStr, ReqParams);
						}
					break;
				}
				ResStr = ""; k = 0;
			}
			else
			{
				if (pInData[i] != '\r')
				{
					pStrBuf[k] = pInData[i];
					
					if (k < StrBufLen - 1) k++;
					else
					{
						pStrBuf[k] = 0; ResStr.append(pStrBuf); k = 0;
					}
					
					CrDet = false;
				}
				else CrDet = true;
			}
		}
		else
		{	// Binary Mode
			// Boundary checking -------------------------------------------------------------
			if (pInData[i] == Boundary[BoundOffset])
			{
				BoundOffset++;
				
				if (BoundOffset == Boundary.length())
				{
					// Finish write process   ---------------------
					//cout << endl << "Finish : PosBgn = " << PosBgn << endl;
					if (WriteProcess)
					{
						if (PosBgn < i + 1) TxLn = i + 1 - PosBgn;
						else TxLn = 0;
						
						WriteToFile(pWriteBuf, TxLn, true);
						
						// Sending File uploading answer ---------------
						InsertPar("Srv_Up_FileName", UpFileName);
						InsertPar("Srv_File_Uploaded", "");
						SendData(EmptyStr, EmptySize, TxCmds::SendDirContent, ReqParams);
					}
					
					InitRxVars();
				}
			}
			else
			{
				BoundOffset = (pInData[i] == Boundary[0]) ? 1 : 0;
			}
			// -------------------------------------------------------------------------------
		}
	}
	
	if (WriteProcess)
	{
		WriteToFile(pWriteBuf, TxLn, false);
	}
	if (RecText)
	{
		pStrBuf[k] = 0; ResStr.append(pStrBuf);
	}
	
	return false;
}

void FileServer::WriteToFile(char *pWriteBuf, ssize_t TxLn, bool CloseFile)
{
		if (TxLn > 0)
		{
			//cout << endl << "Writing buffer PosBgn = " << PosBgn << " TxLn = " << TxLn << " CloseFile = " << CloseFile << endl;
			FSObj.WriteBuffer(pWriteBuf, TxLn); WrCount += TxLn;
		}
		
		if (CloseFile)
		{
			FSObj.CloseFile();
			
			if (WrCount >= Boundary.length())
				FSObj.Truncate(FSObj.CorrectPath(MethPath) + UpFileName, WrCount - Boundary.length());
			
			WriteProcess = false; WrCount = 0;
		}
		
		/*cout << endl << "++++++++++++++++++++++++++++++++++++++++++++++" << endl;
		cout << pWriteBuf;
		cout << endl << "++++++++++++++++++++++++++++++++++++++++++++++" << endl;*/
}

bool FileServer::FindInMap(map<string, string> & InPars, const string & Search)
{
	bool RetVal;
	
	RetVal = (InPars.find(Search) != InPars.end());
	
	return RetVal;
}

string FileServer::MakeDirList(map<string, string> & InPars, const string & AdvText)
{
	string RetVal, FullHRef, CurName, CurRef;
	vector <string> DirHeader;
	
	FullHRef = "http://" + InPars["host"]
			+ FSObj.CorrectPath(FSObj.CurPath);
	
	DirHeader.push_back("Content of: " + FSObj.CurPath);
	RetVal = GetHtmlHeaderT(DirHeader);
	
	for (auto & File : FSObj.GetDirContent())
	{
		CurName = File.Name;
		if (File.IsLink) CurName = "~" + CurName;
		if (File.IsDir) CurName = "/" + CurName;
		
		CurRef = (File.Name != ".") ? FullHRef + File.Name :
				"http://" + InPars["host"];
		
		RetVal += GetTableRow(vector <string> ({ GetHRef(CurRef, CurName) }));
	}
	RetVal += GetTableRow(vector <string> ({
		GetFileSubmitForm(FullHRef)
		}));
	
	if (FindInMap(InPars, "Srv_File_Uploaded"))
		RetVal += GetTableRow(vector <string> ({
			"File : " + InPars["Srv_Up_FileName"] + " sent."
			}));
	
	if (AdvText != "")
		RetVal += GetTableRow(vector <string> ( {AdvText} ));
	
	RetVal += GetHtmlFooterT();
	
	return RetVal;
}

string FileServer::MakeNotFound(map <string, string> & InPars)
{
	string RetVal;
	
	HTTPm.ResetAnswer();
	HTTPm.Host = InPars["host"];
	HTTPm.AnswerCode = HTTPSupport::Answer::NotFound;
	HTTPm.MessageStr = "<html><head><title>Not found</title></head><body>404 Not Found.</body></html>";
	
	RetVal = HTTPm.GetHTTPAnswer();
	
	return RetVal;
}

ClientConn::TxData FileServer::ProcessTxData(Message & InMsg)
{
	TxData OutMsg;
	string OutPage, CurMethPath, AdvText;
	vector <string> TstHeader ({"Request parameter", "value"});
	int FsRet = 0, RetCode = 200; size_t FSize = 0;
	
	switch (InMsg.DataType)
	{
		case TxCmds::TxBuffer:
			OutMsg.pData = InMsg.UpData.get(); OutMsg.Len = InMsg.Len;
			break;
		case TxCmds::SendReqParams:
			// Sending request parameters for a requests with errors
			
			if (InMsg.UpParams == nullptr) return OutMsg;
			
			OutPage = GetHtmlHeaderT(TstHeader);
			
			for (auto & Item : *InMsg.UpParams)
			{
				OutPage += GetTableRow(vector <string> ({Item.first, Item.second}));
			}
			
			OutPage += GetHtmlFooterT();
			
			HTTPm.ResetAnswer();
			
			if(FindInMap(*InMsg.UpParams, "Srv_Error_Code"))
			{
				try
				{
					RetCode = stoi((*InMsg.UpParams)["Srv_Error_Code"]); HTTPm.AnswerCode = RetCode;
				}
				catch (...) {}
			}
			
			HTTPm.Host = (*InMsg.UpParams)["host"];
			HTTPm.MessageStr = OutPage;
			SrvAnswer = HTTPm.GetHTTPAnswer();
			
			//cout << SrvAnswer << endl;
			
			OutMsg.pData = SrvAnswer.c_str();
			OutMsg.Len = SrvAnswer.length();
		break;
		case TxCmds::SendDirContent:
			// Sending current folder content or begin file transmission
			
			if (InMsg.UpParams == nullptr) return OutMsg;
			
			CurMethPath = (*InMsg.UpParams)["Srv_Method_Path"];
			
			FsRet = FSObj.ChDir(CurMethPath);
			
			if (FsRet == FSObject::RetCodes::NotFound)
			{
				SrvAnswer = MakeNotFound(*InMsg.UpParams);
				OutMsg.pData = SrvAnswer.c_str();
				OutMsg.Len = SrvAnswer.length();
				
				cout << "SrvAnswer:" << endl << SrvAnswer << endl;
				
				return OutMsg;
			}
			
			if (FsRet != FSObject::RetCodes::ThisIsFileNotDir)
			{	// Sending current folder content
				if (FsRet == FSObject::RetCodes::NotFound)
				{
					AdvText = "404 Not Found";
					RetCode = 404;
				}
				
				OutPage = MakeDirList(*InMsg.UpParams, AdvText);
				
				HTTPm.ResetAnswer();
				HTTPm.Host = (*InMsg.UpParams)["host"];
				HTTPm.MessageStr = OutPage;
				//HTTPm.ConnType = HTTPSupport::Conn::Alive;
				HTTPm.AnswerCode = RetCode;
				SrvAnswer = HTTPm.GetHTTPAnswer();
				
				//cout << SrvAnswer << endl;
				
				OutMsg.pData = SrvAnswer.c_str();
				OutMsg.Len = SrvAnswer.length();
			}
			else
			{	// Begin file sending
				FSize = FSObj.GetFileSize(CurMethPath);
				
				if (FSObj.OpenFile(CurMethPath) == FSObject::RetCodes::Ok)
				{
					HTTPm.ResetAnswer();
					HTTPm.Host = (*InMsg.UpParams)["host"];
					HTTPm.FileName = CurMethPath;
					HTTPm.ContentLength = FSize;
					HTTPm.ConnType = HTTPSupport::Conn::Alive;
					
					SrvAnswer = HTTPm.GetHTTPAnswer();
					
					cout << endl << "Starting file transfer..." << endl << SrvAnswer << endl;
					
					OutMsg.pData = SrvAnswer.c_str();
					OutMsg.Len = SrvAnswer.length();
					
					OutMsg.DataType = TxCmds::SendFileContent;
					StartRepeatTx();
				}
				else
				{
					SrvAnswer = MakeNotFound(*InMsg.UpParams);
					OutMsg.pData = SrvAnswer.c_str();
					OutMsg.Len = SrvAnswer.length();
				}
			}
		break;
		case TxCmds::SendFileContent:
			
			size_t RdLen = FSObj.ReadBuffer(pDownBuf, DownBufLen);
			if (!RdLen)
			{
				FSObj.CloseFile(); StopRepeatTx();
			}
			
			OutMsg.DataType = TxCmds::SendFileContent;
			OutMsg.pData = pDownBuf; OutMsg.Len = RdLen;
			
			//cout << endl << "-----------------------------------" << endl;
			//cout << OutMsg.pData;
			//cout << "OutMsg.pData[0] = " << OutMsg.pData[0];
			//cout << endl << "-----------------------------------" << endl;
			
		break;
	}
	
	return OutMsg;
}
