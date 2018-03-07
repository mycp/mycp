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

#ifdef WIN32
#include <winsock2.h>
#endif
#include "SotpResponseImpl.h"
//#include "../../CGCBase/cgcString.h"
#include "SessionMgr.h"
#include <ThirdParty/stl/aes.h>
//#define USES_DATA_ENCODING
#ifdef USES_DATA_ENCODING
#include <ThirdParty/stl/zlib.h>
#endif

CSotpResponseImpl::CSotpResponseImpl(const cgcRemote::pointer& pcgcRemote, const cgcParserSotp::pointer& pcgcParser, CResponseHandler * pHandler, CParserSotpHandler* pParserSotpHandler)
: m_cgcRemote(pcgcRemote)
, m_cgcParser(pcgcParser)
, m_pResponseHandler(pHandler), m_pParserSotpHandler(pParserSotpHandler)
, m_bResponseSended(false)
, m_bNotResponse(false)
, m_nHoldSecond(-1)
, m_pSendLock(NULL)
, m_originalCallId(m_cgcParser->getCallid())
, m_originalCallSign(m_cgcParser->getSign())
{
	BOOST_ASSERT (m_cgcRemote.get() != 0);
	BOOST_ASSERT (m_cgcParser.get() != 0);
	m_bDisableZip = false;
}

CSotpResponseImpl::~CSotpResponseImpl(void)
{
	if (m_pParserSotpHandler!=NULL)
	{
		//printf("onReturnParserSotp\n");
		m_pParserSotpHandler->onReturnParserSotp(m_cgcParser);
	}
	m_cgcRemote.reset();
	m_cgcParser.reset();
	if (m_pSendLock)
		delete m_pSendLock;
}

int CSotpResponseImpl::sendSessionResult(long retCode, const tstring & sSessionId)
{
	if (isInvalidate() || m_cgcParser.get() == NULL) return -1;

	unsigned short seq = 0;
	if (m_pResponseHandler)
		seq = m_pResponseHandler->onGetNextSeq();
	//m_bResponseSended = true;

	try
	{
		tstring sSslPassword;
		if (m_session.get() != NULL)
		{
			CSessionImpl* pSessionImpl = (CSessionImpl*)m_session.get();
			sSslPassword = pSessionImpl->GetSslPassword();
		}
		const tstring responseData(m_cgcParser->getSessionResult(retCode, sSessionId, seq, true, m_sSslPublicKey, SOTP_DATA_ENCODING_GZIP|SOTP_DATA_ENCODING_DEFLATE));
		if (m_cgcParser->isSslRequest() && !sSslPassword.empty())
		{
			unsigned int nAttachSize = 0;
			unsigned char * pAttachData = m_cgcParser->getResSslString(sSslPassword, nAttachSize);
			if (pAttachData != NULL)
			{
				unsigned char * pSendData = new unsigned char[nAttachSize+responseData.size()+1];
				memcpy(pSendData, responseData.c_str(), responseData.size());
				memcpy(pSendData+responseData.size(), pAttachData, nAttachSize);
				pSendData[nAttachSize+responseData.size()] = '\0';

				int ret = -1;
				if (m_pResponseHandler != NULL)
					ret = m_pResponseHandler->onAddSeqInfo(pSendData, nAttachSize+responseData.size(), seq, m_cgcParser->getCallid(), m_cgcParser->getSign());

				const size_t sendSize = m_cgcRemote->sendData(pSendData, nAttachSize+responseData.size());
				delete[] pAttachData;
				if (ret != 0)
					delete[] pSendData;
				return sendSize;
			}
		}

		if (m_pResponseHandler)
			m_pResponseHandler->onAddSeqInfo((const unsigned char *)responseData.c_str(), responseData.size(), seq, m_cgcParser->getCallid(), m_cgcParser->getSign());

		//#ifdef _UNICODE
		//	std::string pOutTemp = cgcString::W2Char(responseData);
		//	return m_cgcRemote->sendData((const unsigned char*)pOutTemp.c_str(), pOutTemp.length());
		//#else
		return m_cgcRemote->sendData((const unsigned char*)responseData.c_str(), responseData.length());
		//#endif // _UNICODE
	}catch (const std::exception &)
	{
	}catch(...)
	{}
	return -1;
}
//#ifdef USES_DATA_ENCODING
//inline bool IsEnableZipAttach(const cgcAttachment::pointer& pAttach)
//{
//	if (pAttach.get()==NULL || pAttach->getName().size()<3) return true;
//	const size_t nSize = pAttach->getName().size();
//	tstring sExt(pAttach->getName().substr(nSize-3));
//	std::transform(sExt.begin(), sExt.end(), sExt.begin(), ::tolower);
//	return (sExt=="zip" || sExt=="rar" || sExt=="zip" || sExt.find("7z")!=std::string::npos)?false:true;
//}
//#endif
int CSotpResponseImpl::sendAppCallResult(long retCode, unsigned long sign, bool bNeedAck)
{
	if (isInvalidate() || m_cgcParser.get() == NULL)
	{
		if (m_pSendLock!=NULL)
		{
			boost::mutex::scoped_lock * pSendLockTemp = m_pSendLock;
			m_pSendLock = NULL;
			delete pSendLockTemp;
		}
		return -1;
	}
	if (m_bNotResponse)
	{
		if (m_pSendLock!=NULL)
		{
			boost::mutex::scoped_lock * pSendLockTemp = m_pSendLock;
			m_pSendLock = NULL;
			delete pSendLockTemp;
		}
		//if (m_session.get() != NULL)
		//{
		//	CSessionImpl* pSessionImpl = (CSessionImpl*)m_session.get();
		//	if (m_nHoldSecond > 0)
		//		pSessionImpl->HoldResponse(this->shared_from_this(), m_nHoldSecond);
		//	//else if (m_nHoldSecond == 0)
		//	//{
		//	//	m_nHoldSecond = -1;
		//	//	pSessionImpl->removeResponse(getRemoteId());
		//	//}
		//}
		return 0;
	}else if (m_nHoldSecond > 0)
	{
		if (m_session.get() != NULL)
		{
			CSessionImpl* pSessionImpl = (CSessionImpl*)m_session.get();
			m_nHoldSecond = -1;
			pSessionImpl->removeResponse(getRemoteId());
		}
	}

	if (sign == 0 && m_cgcParser->getSign() != m_originalCallSign)
	{
		//m_cgcParser->setCallid(m_originalCallId);
		m_cgcParser->setSign(m_originalCallSign);
	}else if (sign != 0 && sign != m_cgcParser->getSign())
	{
		//m_cgcParser->setCallid(0);
		m_cgcParser->setSign(sign);
	}
	unsigned short seq = 0;
	if (m_pResponseHandler)
		seq = m_pResponseHandler->onGetNextSeq();
	//printf("============ seq=%d, handler=%d, needack=%d\n", seq, (int)m_pResponseHandler, bNeedAck?1:0);

	boost::mutex::scoped_lock * pSendLockTemp = m_pSendLock;
	m_pSendLock = NULL;
	m_bResponseSended = true;

	tstring sSslPassword;
	if (m_session.get() != NULL)
	{
		CSessionImpl* pSessionImpl = (CSessionImpl*)m_session.get();
		sSslPassword = pSessionImpl->GetSslPassword();
	}
	if (!sSslPassword.empty())
	{
		const tstring sAppCallHead(m_cgcParser->getAppCallResultHead(retCode));
		const tstring sAppCallData(m_cgcParser->getAppCallResultData(seq, bNeedAck));

		// AES_BLOCK_SIZE

		unsigned int nAttachSize = 0;
		unsigned char * pAttachData = m_cgcParser->getResAttachString(nAttachSize);
		int nDataSize = ((sAppCallData.size()+nAttachSize+15)/16)*16;
		const int nSslSize = nDataSize;
		nDataSize += sAppCallHead.size();
		unsigned char * pSendData = new unsigned char[nDataSize+70];
		memcpy(pSendData, sAppCallHead.c_str(), sAppCallHead.size());
		memcpy(pSendData+sAppCallHead.size(), sAppCallData.c_str(), sAppCallData.size());
		if (pAttachData != NULL)
		{
			memcpy(pSendData+sAppCallHead.size()+sAppCallData.size(), pAttachData, nAttachSize);
		}
		unsigned char * pSendDataTemp = new unsigned char[nDataSize+50];
		memcpy(pSendDataTemp, sAppCallHead.c_str(), sAppCallHead.size());
		int n = 0;
		int nEncryptSize = sAppCallData.size()+nAttachSize;
		if (m_cgcParser->getSotpVersion()==SOTP_PROTO_VERSION_21)
		{
#ifdef USES_DATA_ENCODING
			if (nEncryptSize>128 && !m_bDisableZip)//IsEnableZipAttach(m_cgcParser->getResAttachment()))
			{
				unsigned long nAcceptEncoding = SOTP_DATA_ENCODING_UNKNOWN;
				m_cgcParser->getProtoItem(SOTP_PROTO_ITEM_TYPE_ACCEPT_ENCODING,&nAcceptEncoding);
				//printf("**** nEncryptSize＝%d, nAcceptEncoding=%d\n",nEncryptSize,(int)nAcceptEncoding);
				if ((nAcceptEncoding&SOTP_DATA_ENCODING_DEFLATE)==SOTP_DATA_ENCODING_DEFLATE)
				{
					uLong nZipDataSize = nDataSize+50-sAppCallHead.size();
					const int nZipRet = ZipData(pSendData+sAppCallHead.size(),(uLong)nEncryptSize,pSendDataTemp+sAppCallHead.size(),&nZipDataSize,Z_DEFAULT_COMPRESSION,0);
					//printf("**** ZipData nZipRet=%d,nZipDataSize=%d\n",(int)nZipRet, (int)nZipDataSize);
					if (nZipRet==Z_OK && (int)nZipDataSize<(nEncryptSize-30))
					{
						memmove(pSendData+sAppCallHead.size(),pSendDataTemp+sAppCallHead.size(),nZipDataSize);
						n = sprintf((char*)(pSendDataTemp+sAppCallHead.size()),"G2%d\n",(int)nEncryptSize);		// * deflate encoding
						nEncryptSize = nZipDataSize;
						nDataSize = ((nZipDataSize+15)/16)*16;
						nDataSize += sAppCallHead.size();
					}
				}else if ((nAcceptEncoding&SOTP_DATA_ENCODING_GZIP)==SOTP_DATA_ENCODING_GZIP)
				{
					uLong nZipDataSize = nDataSize+50-sAppCallHead.size();
					const int nZipRet = GZipData(pSendData+sAppCallHead.size(),(uLong)nEncryptSize,pSendDataTemp+sAppCallHead.size(),&nZipDataSize);
					//printf("**** GZipData nZipRet=%d,nZipDataSize=%d\n",(int)nZipRet, (int)nZipDataSize);
					if (nZipRet==Z_OK && (int)nZipDataSize<(nEncryptSize-30))
					{
						memmove(pSendData+sAppCallHead.size(),pSendDataTemp+sAppCallHead.size(),nZipDataSize);
						n = sprintf((char*)(pSendDataTemp+sAppCallHead.size()),"G1%d\n",(int)nEncryptSize);		// * gzip encoding
						nEncryptSize = nZipDataSize;
						nDataSize = ((nZipDataSize+15)/16)*16;
						nDataSize += sAppCallHead.size();
					}
				}
			}
#endif
			n += sprintf((char*)(pSendDataTemp+(sAppCallHead.size()+n)),"2%d\n",(int)(nDataSize-sAppCallHead.size()));
		}else
			n = sprintf((char*)(pSendDataTemp+sAppCallHead.size()),"Sd: %d\n",(int)(nDataSize-sAppCallHead.size()));
		if (aes_cbc_encrypt_full((const unsigned char*)sSslPassword.c_str(),(int)sSslPassword.size(),pSendData+sAppCallHead.size(),nEncryptSize,pSendDataTemp+(sAppCallHead.size()+n))!=0)
		//if (aes_cbc_encrypt_full((const unsigned char*)sSslPassword.c_str(),(int)sSslPassword.size(),pSendData+sAppCallHead.size(),sAppCallData.size()+nAttachSize,pSendDataTemp+(sAppCallHead.size()+n))!=0)
		{
			if (pSendLockTemp)
				delete pSendLockTemp;
			delete[] pSendData;
			delete[] pSendDataTemp;
			delete[] pAttachData;
			return false;
		}
		delete[] pSendData;
		pSendData = pSendDataTemp;
		nDataSize += n;
		pSendData[nDataSize] = '\n';
		nDataSize += 1;

		//unsigned char * pSotpData = new unsigned char[nSslSize+20];
		//memset(pSotpData,0,nSslSize+20);
		////pSotpData[0] = '\n';	// **
		////pSotpData[nSslSize+1] = '\0';		// **
		//aes_cbc_decrypt((const unsigned char*)sSslPassword.c_str(),(int)sSslPassword.size(),pSendDataTemp+(sAppCallHead.size()+n),nSslSize,pSotpData);	// nSslSize+16，也不行，前期会乱码
		////for (int i=-25;i<18;i++)
		////	printf("****b %d=%d [%c]\n",i,pSotpData[nSslSize+i],pSotpData[nSslSize+i]);	// 打印的数据是对的；
		//printf("****\n%s\n****\n",pSotpData);
		//delete[] pSotpData;

		// *避免重
		//m_cgcParser->getResAttachment()->clear();

		int ret = -1;
		if (bNeedAck && m_pResponseHandler != NULL)
			ret = m_pResponseHandler->onAddSeqInfo(pSendData, nDataSize, seq, m_cgcParser->getCallid(), m_cgcParser->getSign());

		const size_t sendSize = m_cgcRemote->sendData(pSendData, nDataSize);
		if (pSendLockTemp)
			delete pSendLockTemp;
		delete[] pAttachData;
		if (ret != 0)
			delete[] pSendData;
		return sendSize;
	}else
	{
		const std::string responseData = m_cgcParser->getAppCallResult(retCode, seq, bNeedAck);
		if (m_cgcParser->isResHasAttachInfo())
		{
			unsigned int nAttachSize = 0;
			unsigned char * pAttachData = m_cgcParser->getResAttachString(nAttachSize);
			//unsigned char * pAttachData = m_cgcParser->getAttachString(m_cgcParser->getResAttachment(), nAttachSize);
			if (pAttachData != NULL)
			{
				unsigned char * pSendData = new unsigned char[nAttachSize+responseData.size()+1];
				memcpy(pSendData, responseData.c_str(), responseData.size());
				memcpy(pSendData+responseData.size(), pAttachData, nAttachSize);
				pSendData[nAttachSize+responseData.size()] = '\0';

				// *避免重
				//m_cgcParser->getResAttachment()->clear();

				int ret = -1;
				if (bNeedAck && m_pResponseHandler != NULL)
					ret = m_pResponseHandler->onAddSeqInfo(pSendData, nAttachSize+responseData.size(), seq, m_cgcParser->getCallid(), m_cgcParser->getSign());

				const size_t sendSize = m_cgcRemote->sendData(pSendData, nAttachSize+responseData.size());
				if (pSendLockTemp)
					delete pSendLockTemp;
				delete[] pAttachData;
				if (ret != 0)
					delete[] pSendData;
				//return sendSize != nAttachSize+responseData.size() ? 0 : 1;
				return sendSize;
			}
		}

		if (bNeedAck && m_pResponseHandler != NULL)
			m_pResponseHandler->onAddSeqInfo((const unsigned char *)responseData.c_str(), responseData.size(), seq, m_cgcParser->getCallid(), m_cgcParser->getSign());
		const size_t sendSize = m_cgcRemote->sendData((const unsigned char*)responseData.c_str(), responseData.size());
		if (pSendLockTemp)
			delete pSendLockTemp;
		return sendSize;
	}
}
int CSotpResponseImpl::sendP2PTry(void)
{
	if (isInvalidate() || m_cgcParser.get() == NULL) return -1;
	const std::string responseData = m_cgcParser->getP2PTry();
	return m_cgcRemote->sendData((const unsigned char*)responseData.c_str(), responseData.size());
}

void CSotpResponseImpl::setCgcRemote(const cgcRemote::pointer& pcgcRemote)
{
	m_cgcRemote = pcgcRemote;
}

//void CSotpResponseImpl::setCgcParser(const cgcParserSotp::pointer& pcgcParser)
//{
//	m_cgcParser = pcgcParser;
//	BOOST_ASSERT (m_cgcParser.get() != NULL);
//
//	// ??
//	if (m_cgcParser.get() != 0)
//	{
//		m_originalCallId = m_cgcParser->getCallid();
//		m_originalCallSign = m_cgcParser->getSign();
//	}else
//	{
//		m_originalCallId = 0;
//		m_originalCallSign = 0;
//	}
//}

void CSotpResponseImpl::lockResponse(void)
{
	if (m_bNotResponse)
		return;
	boost::mutex::scoped_lock * pLockTemp = new boost::mutex::scoped_lock(m_sendMutex);
	m_pSendLock = pLockTemp;
}

int CSotpResponseImpl::sendResponse(long retCode, unsigned long sign, bool bNeedAck)
{
	try
	{
		return sendAppCallResult(retCode, sign, bNeedAck);
	}catch (const std::exception &)
	{
	}catch (...)
	{
	}

	return -105;
}

unsigned long CSotpResponseImpl::setNotResponse(int nHoldSecond)
{
	m_bNotResponse = true;
	if (m_nHoldSecond > 0  || nHoldSecond > 0)
	{
		m_nHoldSecond = nHoldSecond;
		if (m_session.get() != NULL)
		{
			CSessionImpl* pSessionImpl = (CSessionImpl*)m_session.get();
			if (m_nHoldSecond <= 0)
			{
				m_nHoldSecond = -1;
				pSessionImpl->removeResponse(getRemoteId());
			}else
			{
				pSessionImpl->HoldResponse(this->shared_from_this(), m_nHoldSecond);
			}
		}
	}
	//if (m_session.get() != NULL)
	//{
	//	CSessionImpl* pSessionImpl = (CSessionImpl*)m_session.get();
	//	pSessionImpl->HoldResponse(shared_from_this(), nHoldSecond);
	//}
	return getRemoteId();
}

void CSotpResponseImpl::invalidate(void)
{
	if (m_cgcRemote.get() != NULL )
		m_cgcRemote->invalidate();
}

/*
int CSotpResponseImpl::sendString(const tstring & sString)
{
	if (isInvalidate()) return -1;
	if (sString.empty()) return 0;

	m_bResponseSended = true;
	tstring responseData = sString;

	try
	{
#ifdef _UNICODE
		std::string pOutTemp = cgcString::W2Char(responseData);
		return m_cgcRemote->sendData((const unsigned char*)pOutTemp.c_str(), pOutTemp.length());
#else
		return m_cgcRemote->sendData((const unsigned char*)responseData.c_str(), responseData.length());
#endif // _UNICODE
	}catch (const std::exception &)
	{
	}catch (...)
	{
	}

	return -105;
}
*/

/*
tstring CSotpResponseImpl::GetResponseClusters(const ClusterSvrList & clusters)
{
	//boost::wformat fRV(L"<svr n=\"%s\" h=\"%s\" p=\"%d\" c=\"%s\" r=\"%d\" />");
	boost::wformat fRV(L"<svr n=\"%s\" h=\"%s\" c=\"%s\" r=\"%d\" />");
	tstring result;
	int count = 0;

	ClusterSvrList::const_iterator pIter;
	for (pIter=clusters.begin(); pIter!=clusters.end(); pIter++)
	{
		ClusterSvr * pCluster = *pIter;

		tstring svrName;
		if (m_sXmlEncoding.compare(L"UTF-8") == 0)
		{
#ifdef WIN32
// ???			svrName = cgcString::Convert(pCluster->getName(), CP_ACP, CP_UTF8);
			int i=0;
#else
			cgcString::GB2312ToUTF_8(svrName, pCluster->getName().c_str(), pCluster->getName().length());
#endif
		}else
		{
			svrName = pCluster->getName();
		}
		tstring sCluster((fRV%svrName.c_str()%pCluster->getHost().c_str()%pCluster->getCode().c_str()%pCluster->getRank()).str());

		result.append(sCluster);

		//
		// 只返回三个足够实现功能
		//
		if (++count == 3)
			break;
	}

	return result;
}
*/
#define MYCP_RESPONSE_CONFIG_DISABLE_ZIP 1
bool CSotpResponseImpl::setConfigValue(int nConfigItem, unsigned int nConfigValue)
{
	switch(nConfigItem)
	{
	case MYCP_RESPONSE_CONFIG_DISABLE_ZIP:
		m_bDisableZip = nConfigValue==1?true:false;
		return true;
	default:
		break;
	}
	return false;
}
