/*
    MYCP is a HTTP and C++ Web Application Server.
    Copyright (C) 2009-2010  Akee Yang <akee.yang@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "CgcTcpClient.h"

#define USES_OPENSSL
#ifdef USES_OPENSSL
#ifdef WIN32
//#pragma comment(lib, "libeay32.lib")  
//#pragma comment(lib, "ssleay32.lib") 
#endif // WIN32
#endif // USES_OPENSSL

#ifdef WIN32
#pragma warning(disable:4800)
#include <windows.h>
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#endif // WIN32


// cgc head
//#include <CGCBase/app.h>
#include <CGCBase/cgcService.h>
using namespace mycp;

tstring getipbyname(const char * name)
{
	struct   hostent   *   pHost;
	int   i;  
	pHost   =   gethostbyname(name);  
	for(   i   =   0;   pHost!=   NULL   &&   pHost-> h_addr_list[i]!=   NULL;   i++   )  
	{
		return inet_ntoa   (*(struct   in_addr   *)pHost-> h_addr_list[i]);
	}
	return _T("");
}


const tstring const_http_version = "HTTP/1.1";
//tstring theTempSavePath;

class CHttpService
	: public cgcServiceInterface
{
public:
	typedef std::shared_ptr<CHttpService> pointer;

	static CHttpService::pointer create(void)
	{
		return CHttpService::pointer(new CHttpService());
	}

protected:
	virtual tstring serviceName(void) const {return _T("HTTPSERVICE");}

	bool GetRequestInfo(const cgcValueInfo::pointer& inParam,tstring& pOutHost,tstring& pOutPort,tstring& pOutUrl,tstring& pOutHeader,tstring& pOutData, bool& pOutIsSSL, tstring& pOutSaveFile, int & pOutTimeoutSeconds) const
	{
		if (inParam.get() == NULL) return false;
		if (inParam->getType() == cgcValueInfo::TYPE_MAP)
		{
			cgcValueInfo::pointer varFind;
			if (!inParam->getMap().find("host", varFind))
				return false;
			pOutHost = varFind->toString();
			std::transform(pOutHost.begin(), pOutHost.end(), pOutHost.begin(), tolower);
			if (pOutHost.find("https:")!=std::string::npos)
				pOutIsSSL = true;
			if (inParam->getMap().find("port", varFind))
				pOutPort = varFind->toString();
			else
			{
				pOutPort = pOutIsSSL?"443":"80";
			}
			if (!inParam->getMap().find("url", varFind))
				return false;
			pOutUrl = varFind->toString();
			if (inParam->getMap().find("header", varFind))
				pOutHeader = varFind->getStr();
			if (inParam->getMap().find("data", varFind))
				pOutData = varFind->toString();
			if (inParam->getMap().find("save_file", varFind))
				pOutSaveFile = varFind->getStr();
			if (inParam->getMap().find("timeout_seconds", varFind))
				pOutTimeoutSeconds = varFind->getIntValue();
			return true;
		}else if (inParam->getType() == cgcValueInfo::TYPE_STRING)
		{
			// http://ip:port/aaa.html?xxx=aaa
			// http://www.entboost.com/abc.csp?xxx=bbb
			tstring sFullUrl = inParam->getStr();
			//printf("**** url=%s\n",sFullUrl.c_str());
			std::string::size_type find = sFullUrl.find("://");
			if (find != std::string::npos)	// remove before http://
			{
				tstring sHttpHead = sFullUrl.substr(0,find);
				std::transform(sHttpHead.begin(), sHttpHead.end(), sHttpHead.begin(), tolower);
				if (sHttpHead=="https")
					pOutIsSSL = true;
				sFullUrl = sFullUrl.substr(find+3);	// www.entboost.com/abc.csp?xxx=bbb
			}
			// find url
			find = sFullUrl.find("/");
			if (find != std::string::npos)
			{
				pOutUrl = sFullUrl.substr(find);		// "/abc.csp?xxx=bbb"
				sFullUrl = sFullUrl.substr(0,find);	// "www.entboost.com" OR "ip:port"
			}else
			{
				find = sFullUrl.find("?");
				if (find != std::string::npos)
				{
					pOutUrl = "/";
					sFullUrl = sFullUrl.substr(0,find);	// "www.entboost.com" OR "ip:port"
				}else
				{
					return false;
				}
			}
			// find port
			find = sFullUrl.find(":");
			if (find != std::string::npos)
			{
				pOutHost = sFullUrl.substr(0,find);	// "www.entboost.com" OR "ip"
				pOutPort = sFullUrl.substr(find+1);
			}else
			{
				pOutHost = sFullUrl;
				pOutPort = pOutIsSSL?"443":"80";
			}
			//printf("**** out-url=%s\n",pOutUrl.c_str());
			//printf("**** out-host=%s\n",pOutHost.c_str());
			//printf("**** out-port=%s\n",pOutPort.c_str());
			return true;
		}else if (inParam->getType() == cgcValueInfo::TYPE_VECTOR && !inParam->empty())
		{
			// 0=url	// http://ip:port/abc.html?xxx=xxx
			// 1=data	// for post data
			// 2=header
			const tstring sFullUrl = inParam->getVector()[0]->getStr();
			if (!GetRequestInfo(CGC_VALUEINFO(sFullUrl),pOutHost,pOutPort,pOutUrl,pOutHeader,pOutData,pOutIsSSL, pOutSaveFile,pOutTimeoutSeconds))
				return false;
			if (inParam->size()>=2)
			{
				pOutData = inParam->getVector()[1]->getStr();
			}
			//printf("**** out-data=%s\n",pOutData.c_str());
			if (inParam->size()>=3)
			{
				pOutHeader = inParam->getVector()[2]->getStr();
			}
			//printf("**** out-header=%s\n",pOutHeader.c_str());
			if (inParam->size()>=4)
				pOutSaveFile = inParam->getVector()[3]->getStr();
			if (inParam->size()>=5)
				pOutTimeoutSeconds = inParam->getVector()[4]->getIntValue();
			return true;
		}else
		{
			return false;
		}
	}
	static bool IsFileExist(const char* lpszFilePath)
	{
		FILE * f = fopen(lpszFilePath,"r");
		if (f==NULL) return false;
		fclose(f);
		return true;
	}
	static bool TestFileCreate(const char* lpszFilePath)
	{
		if (!IsFileExist(lpszFilePath)) return true;
		FILE * f = fopen(lpszFilePath,"a");
		if (f==NULL) return false;
		fclose(f);
		return true;
	}
	virtual bool callService(const tstring& function, const cgcValueInfo::pointer& inParam, cgcValueInfo::pointer outParam)
	{
		if (function == "GET")
		{
			if (inParam.get() == NULL || outParam.get() == NULL) return false;
			tstring sHost;		// www.abc.com
			tstring sPort;		// default 80
			tstring sUrl;		// "/abc.csp?p=v"
			tstring sHeader;
			tstring sData;
			bool bIsSSL = false;
			tstring sSaveFile;
			int nTimeoutSeconds = 30;
			if (!GetRequestInfo(inParam,sHost,sPort,sUrl,sHeader,sData,bIsSSL,sSaveFile,nTimeoutSeconds))
				return false;
			tstring sHostIp(sHost);
			//tstring sHostIp = getipbyname(sHost.c_str());
			//if (sHostIp.empty()) return false;

			tstring sHttpRequest;
			sHttpRequest.append(_T("GET "));
			sHttpRequest.append(sUrl);
			sHttpRequest.append(_T(" HTTP/1.1\r\n"));
			sHttpRequest.append(_T("Host: "));
			sHttpRequest.append(sHost);
			sHttpRequest.append(_T("\r\n"));
			sHttpRequest.append(_T("User-Agent: MYCP HttpService/5.0\r\n"));
			//sHttpRequest.append(_T("Accept: */*\r\n"));
			//sHttpRequest.append(_T("Accept-Language: zh-cn,zh;q=0.5\r\n"));
			//sHttpRequest.append(_T("Accept-Charset: GB2312,utf-8;q=0.7,*;q=0.7\r\n"));
			//sHttpRequest.append(_T("Keep-Alive: 115\r\n"));
			//sHttpRequest.append(_T("Connection: keep-alive\r\n"));
			sHttpRequest.append(_T("Connection: close\r\n"));
			if (!sHeader.empty())
				sHttpRequest.append(sHeader);
			sHttpRequest.append(_T("\r\n"));

			sHostIp.append(":");
			sHostIp.append(sPort);			// default 80
			try
			{
				return HttpRequest(nTimeoutSeconds,sSaveFile, bIsSSL, sHostIp, sHttpRequest, sUrl, outParam);
			}catch (const std::exception &)
			{
			}catch (const boost::exception &)
			{
			}catch(...)
			{}	
			return false;
		}else if (function == "POST")
		{
			if (inParam.get() == NULL || outParam.get() == NULL) return false;
			//if (inParam->getType() != cgcValueInfo::TYPE_MAP) return false;

			tstring sHost;		// www.abc.com
			tstring sPort;		// default 80
			tstring sUrl;		// "/abc.csp?p=v"
			tstring sHeader;
			tstring sData;		// for POST command
			bool bIsSSL = false;
			tstring sSaveFile;
			int nTimeoutSeconds = 30;
			if (!GetRequestInfo(inParam,sHost,sPort,sUrl,sHeader,sData,bIsSSL,sSaveFile,nTimeoutSeconds))
				return false;

			//cgcValueInfo::pointer varFind;
			//if (!inParam->getMap().find("host", varFind))
			//	return false;
			//sHost = varFind->toString();
			//if (!inParam->getMap().find("port", varFind))
			//	return false;
			//sPort = varFind->toString();
			//if (!inParam->getMap().find("url", varFind))
			//	return false;
			//sUrl = varFind->toString();
			//if (inParam->getMap().find("data", varFind))
			//	sData = varFind->toString();

			tstring sHostIp(sHost);
			//tstring sHostIp = getipbyname(sHost.c_str());
			//if (sHostIp.empty()) return false;

			tstring sHttpRequest;
			sHttpRequest.append(_T("POST "));
			sHttpRequest.append(sUrl);
			sHttpRequest.append(_T(" HTTP/1.1\r\n"));
			sHttpRequest.append(_T("Host: "));
			sHttpRequest.append(sHost);
			sHttpRequest.append(_T("\r\n"));
			sHttpRequest.append(_T("User-Agent: MYCP HttpService/5.0\r\n"));
			//sHttpRequest.append(_T("Accept: */*\r\n"));
			//sHttpRequest.append(_T("Accept-Language: zh-cn,zh;q=0.5\r\n"));
			//sHttpRequest.append(_T("Accept-Charset: GB2312,utf-8;q=0.7,*;q=0.7\r\n"));
			sHttpRequest.append(_T("Connection: close\r\n"));
			if (!sHeader.empty())
				sHttpRequest.append(sHeader);
			if (!sData.empty())
			{
				char sContentLength[20];
				sprintf(sContentLength, "%d", sData.size());
				sHttpRequest.append(_T("Content-Length: "));
				sHttpRequest.append(sContentLength);
				sHttpRequest.append(_T("\r\n\r\n"));
				sHttpRequest.append(sData.c_str());
			}else
			{
				sHttpRequest.append(_T("\r\n"));
			}
			sHostIp.append(":");
			sHostIp.append(sPort);			// default 80
			try
			{
				return HttpRequest(nTimeoutSeconds,sSaveFile, bIsSSL, sHostIp, sHttpRequest, sUrl, outParam);
			}catch (const std::exception &)
			{
			}catch (const boost::exception &)
			{
			}catch(...)
			{}	
			return false;
		}else
		{
			return false;
		}
		return true;
	}

	protected:
		bool HttpRequest(int nTimeoutSeconds,const tstring& sSaveFile, bool bIsSSL, const tstring & sHostIp, const tstring & sHttpRequest, const tstring& sUrl, cgcValueInfo::pointer outParam)
		{
			bool result = false;
			boost::asio::ssl::context * m_sslctx = NULL;
			if (bIsSSL)
			{
				mycp::asio::TcpClient::Test_To_SSL_library_init();
				namespace ssl = boost::asio::ssl;
				m_sslctx = new boost::asio::ssl::context (ssl::context::sslv23_client);	// OK
				m_sslctx->set_default_verify_paths();
				m_sslctx->set_options(ssl::context::verify_peer);
				m_sslctx->set_verify_mode(ssl::verify_peer);
				//m_sslctx->set_verify_callback(ssl::rfc2818_verification("smtp.entboost.com"));
			}

			mycp::httpservice::CgcTcpClient::pointer tcpClient = mycp::httpservice::CgcTcpClient::create(NULL);
			tstring sFilePathTemp(sSaveFile);
			if (!sSaveFile.empty())
			{
				int nIndex = 0;
				while (!TestFileCreate(sFilePathTemp))
				{
					char lpszBuffer[260];
					const std::string::size_type find = sSaveFile.rfind(".");
					if (find==std::string::npos)
						sprintf(lpszBuffer,"%s(%d)",sSaveFile.c_str(),(++nIndex));
					else
						sprintf(lpszBuffer,"%s(%d)%s",sSaveFile.substr(0,find).c_str(),(++nIndex),sSaveFile.substr(find).c_str());
					sFilePathTemp = lpszBuffer;
					//printf("**** FilePathTemp=%s\n",sFilePathTemp.c_str());
				}
				tcpClient->setResponseSaveFile(sFilePathTemp);
				tcpClient->setDeleteFile(false);
			}
			if (tcpClient->startClient(sHostIp, 0, m_sslctx) == 0)
			{
				tcpClient->sendData((const unsigned char *)sHttpRequest.c_str(), sHttpRequest.size());
				//const int nMaxSecond = sSaveFile.empty()?30:300;	// 300=5*60
				//int counter = 0;
				if (nTimeoutSeconds==0)
					nTimeoutSeconds = 30;
				do
				{
#ifdef WIN32
					Sleep(200);
#else
					usleep(200000);
#endif
#ifdef USES_PARSER_HTTP
				}while (!tcpClient->IsOvertime(nTimeoutSeconds) && !tcpClient->IsDisconnection() && !tcpClient->IsHttpResponseOk());	// 30S
				//}while ((++counter < 5*nMaxSecond) && !tcpClient->IsDisconnection() && !tcpClient->IsHttpResponseOk());	// 30S

				//printf("********** timeout_seconds=%d, isovertime=%d, disconnection=%d, isHttpResponseOk=%d\n",
				//	nTimeoutSeconds,
				//	(int)(tcpClient->IsOvertime(nTimeoutSeconds)?1:0),
				//	(int)(tcpClient->IsDisconnection()?1:0),
				//	(int)(tcpClient->IsHttpResponseOk()?1:0)
				//	);

				mycp::cgcParserHttp::pointer pParserHttp = tcpClient->GetParserHttp();
				if (tcpClient->IsHttpResponseOk())
				{
					outParam->totype(cgcValueInfo::TYPE_VECTOR);
					outParam->reset();
					//tstring sHttpState;
					outParam->addVector(CGC_VALUEINFO((int)pParserHttp->getStatusCode()));
					outParam->addVector(CGC_VALUEINFO(pParserHttp->getQueryString()));

					if (!sSaveFile.empty())
					{
						//std::vector<cgcUploadFile::pointer> pFiles;
						//pParserHttp->getUploadFile(pFiles);
						//if (pFiles.empty())
						//{
							const tstring & sContentType = pParserHttp->getReqContentType();
							//printf("**** Content-Type: %s\n",sContentType.c_str());
							// Accept-Ranges: bytes
							//outParam->addVector(CGC_VALUEINFO((int)1));
							cgcValueInfo::pointer pFileInfo = CGC_VALUEINFO(cgcValueInfo::TYPE_MAP);
							tstring sFileName = sUrl;
							const std::string::size_type find = sUrl.rfind("/");
							if (find!=std::string::npos)
							{
								sFileName = sUrl.substr(find+1);
							}
							//printf("**** file_ame=%s,Url=%s\n",sFileName.c_str(),sUrl.c_str());
//							if (theTempSavePath.empty())
//							{
//#ifdef WIN32
//								theTempSavePath = "c:\\";
//#else
//								theTempSavePath = "/";
//#endif
//							}
//							char lpszFilePath[260];
//							static unsigned int theIndex = 0;
//#ifdef WIN32
//							sprintf(lpszFilePath,"%s\\temp_%lld_%03d",theTempSavePath.c_str(),(mycp::bigint)time(0),((++theIndex)%1000));
//#else
//							sprintf(lpszFilePath,"%s/temp_%lld_%03d",theTempSavePath.c_str(),(mycp::bigint)time(0),((++theIndex)%1000));
//#endif
//							FILE * f = fopen(lpszFilePath,"wb");
							//FILE * f = fopen(sSaveFile.c_str(),"wb");
							//if (f!=NULL)
							//{
							//	fwrite(pParserHttp->getContentData(),1,pParserHttp->getContentLength(),f);
							//	fclose(f);
							//}
							pFileInfo->insertMap("name",CGC_VALUEINFO(sFileName));
							pFileInfo->insertMap("file_name",CGC_VALUEINFO(sFileName));
							pFileInfo->insertMap("file_path",CGC_VALUEINFO(sFilePathTemp));
							pFileInfo->insertMap("file_size",CGC_VALUEINFO((int)pParserHttp->getContentLength()));
							pFileInfo->insertMap("content_type",CGC_VALUEINFO(sContentType));
							outParam->addVector(pFileInfo);
						//}else
						//{
						//	//outParam->addVector(CGC_VALUEINFO((int)pFiles.size()));
						//	for (size_t i=0; i<pFiles.size(); i++)
						//	{
						//		cgcValueInfo::pointer pFileInfo = CGC_VALUEINFO(cgcValueInfo::TYPE_MAP);
						//		pFileInfo->insertMap("name",CGC_VALUEINFO(pFiles[i]->getName()));
						//		pFileInfo->insertMap("file_name",CGC_VALUEINFO(pFiles[i]->getFileName()));
						//		pFileInfo->insertMap("file_path",CGC_VALUEINFO(pFiles[i]->getFilePath()));
						//		pFileInfo->insertMap("file_size",CGC_VALUEINFO((int)pFiles[i]->getFileSize()));
						//		pFileInfo->insertMap("content_type",CGC_VALUEINFO(pFiles[i]->getContentType()));
						//		outParam->addVector(pFileInfo);
						//		break;
						//	}
						//}
					}
				}else
				{
					outParam->totype(cgcValueInfo::TYPE_STRING);
					if (pParserHttp.get()!=NULL)
						outParam->setStr(pParserHttp->getQueryString());
				}
				result = true;
#else
				}while ((++counter < 5*30) && !tcpClient->IsDisconnection() && tcpClient->GetReceiveData().empty());	// 30S
				std::string::size_type responseSize = tcpClient->GetReceiveData().size();
				counter = 0;
				while (++counter < 10 && responseSize > 0)	// wait more 5S
				{
#ifdef WIN32
					Sleep(500);
#else
					usleep(500000);
#endif
					if (tcpClient->GetReceiveData().size() == responseSize)
						break;
					responseSize = tcpClient->GetReceiveData().size();
				}
				const tstring & sResponse = tcpClient->GetReceiveData();
				//printf("**** Response=%s\n",sResponse.c_str());

				// Transfer-Encoding: chunked

				if (!sResponse.empty() && sResponse.size() > const_http_version.size())
				{
					outParam->totype(cgcValueInfo::TYPE_VECTOR);
					outParam->reset();
					std::string::size_type find = sResponse.find(const_http_version);
					if (find != std::string::npos)
					{
						std::string::size_type find2 = sResponse.find("\r\n", find+const_http_version.size());
						if (find2 != std::string::npos)
						{
							tstring sHttpState = sResponse.substr(find+const_http_version.size()+1, find2 - find - const_http_version.size()-1);
							outParam->addVector(CGC_VALUEINFO(sHttpState));
						}
					}
					if (outParam->empty())
					{
						outParam->addVector(CGC_VALUEINFO((tstring)"200"));
						//return false;
					}

//					// 特殊处理，用于淘宝应用
//					find = sResponse.find("top-bodylength: ", const_http_version.size());
//					if (find == std::string::npos)
//					{
						find = sResponse.find("Transfer-Encoding: chunked\r\n", const_http_version.size());
						if (find != std::string::npos)
						{
							// 先简单处理，直接取出二个回车后一回车开始位置，到0回车结束位置
//HTTP/1.1 200 OK
//Server: Apache-Coyote/1.1
//Content-Language: cn-ZH
//Transfer-Encoding: chunked
//Date: Wed, 29 Jun 2011 02:26:52 GMT
//
//10
//eyJzdGF0dXMiOjB9
//0

							find = sResponse.find("\r\n\r\n", find+10);
							// 后一回车开始位置
							find = sResponse.find("\r\n", find+5);
							const std::string::size_type findend = sResponse.find("\r\n0", find+2);
							if (find != std::string::npos && findend != std::string::npos)
							{
								const tstring sHttpData(sResponse.substr(find+2, findend-find-2));
								outParam->addVector(CGC_VALUEINFO(sHttpData));
							}else
							{
								//outParam->operator +=(CGC_VALUEINFO((tstring)""));
							}
						}else
						{
							find = sResponse.find("\r\n\r\n", const_http_version.size());
							if (find != std::string::npos)
							{
								const tstring sHttpData(sResponse.substr(find+4));
								outParam->addVector(CGC_VALUEINFO(sHttpData));
							}else
							{
								//outParam->operator +=(CGC_VALUEINFO((tstring)""));
							}
						}
//					}else
//					{
//						// 淘宝应用
//						find = sResponse.find("<?xml", const_http_version.size());
//						if (find != std::string::npos)
//						{
//							std::string::size_type find2 = sResponse.find("<!--top", find);
//							if (find2 == std::string::npos)
//							{
//								tstring sHttpData = sResponse.substr(find);
//								outParam->operator +=(CGC_VALUEINFO(sHttpData));
//							}else
//							{
//								tstring sHttpData = sResponse.substr(find, find2-find);
//								outParam->operator +=(CGC_VALUEINFO(sHttpData));
//							}
//						}
//					}
				}else
				{
					outParam->totype(cgcValueInfo::TYPE_STRING);
					outParam->setStr(sResponse);
				}

				result = true;
#endif
			}

			tcpClient->stopClient();
			tcpClient.reset();

			if (m_sslctx!=NULL)
				delete m_sslctx;
			return result;
		}

};

//#include <CGCBase/cgcServices.h>
//#include <CGCBase/cgcbase.h>
//cgcServiceInterface::pointer theFileSystemService;
CHttpService::pointer theServicePointer;

extern "C" bool CGC_API CGC_Module_Init(void)
{
	//theFileSystemService = theServiceManager->getService("FileSystemService");
	//theTempSavePath = theApplication->getAppConfPath();
	//if (theFileSystemService.get() != NULL)
	//{
	//	cgcValueInfo::pointer pOut = CGC_VALUEINFO(true);
	//	theFileSystemService->callService("exists", CGC_VALUEINFO(theTempSavePath),pOut);
	//	if (!pOut->getBoolean())
	//	{
	//		theFileSystemService->callService("create_directory", CGC_VALUEINFO(theTempSavePath));
	//	}
	//}
	return true;
}

extern "C" void CGC_API CGC_Module_Free(void)
{
	//theFileSystemService.reset();
	theServicePointer.reset();
}

extern "C" void CGC_API CGC_GetService(cgcServiceInterface::pointer & outService, const cgcValueInfo::pointer& parameter)
{
	if (theServicePointer.get() == NULL)
	{
		theServicePointer = CHttpService::create();
	}
	outService = theServicePointer;
}
