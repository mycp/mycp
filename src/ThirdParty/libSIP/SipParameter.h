// SipParameter.h file here
#ifndef __SipParameter_h__
#define __SipParameter_h__

#include "../stl/stldef.h"

class CSipParameter
{
public:
	CSipParameter(const CSipParameter & sipp)
	{
		equal(sipp);
	}
	const CSipParameter & operator = (const CSipParameter & sipp)
	{
		equal(sipp);
		return *this;
	}

	void proxy(const tstring & newv) {m_proxy = newv;}
	const tstring & proxy(void) const {return m_proxy;}

	void identity(const tstring & newv) {m_identity = newv;}
	const tstring & identity(void) const {return m_identity;}
	void ua(const tstring & newv) {m_useraccount = newv;}
	const tstring & ua() const {return m_useraccount;}
	void pwd(const tstring & newv) {m_passwd = newv;}
	const tstring & pwd(void) const {return m_passwd;}

	void sipport(int newv) {m_sipPort = newv;}
	int sipport(void) const {return m_sipPort;}
	void localaudioport(int newv) {m_localAudioPort = newv;}
	int localaudioport(void) const {return m_localAudioPort;}
	void localvideoport(int newv) {m_localVideoPort = newv;}
	int localvideoport(void) const {return m_localVideoPort;}

protected:
	void equal(const CSipParameter & sipp)
	{
		this->m_proxy = sipp.proxy();
		this->m_identity = sipp.identity();
		this->m_useraccount = sipp.ua();
		this->m_passwd = sipp.pwd();

		this->m_sipPort = sipp.sipport();
		this->m_localAudioPort = sipp.localaudioport();
		this->m_localVideoPort = sipp.localvideoport();
	}
public:
	CSipParameter(void)
		: m_proxy(_T(""))
		, m_identity(_T("")), m_useraccount(_T("")), m_passwd(_T(""))
		, m_sipPort(0), m_localAudioPort(0), m_localVideoPort(0)
	{}
	~CSipParameter(void)
	{}
private:
	tstring m_proxy;
	tstring m_identity;
	tstring m_useraccount;
	tstring m_passwd;

	int		m_sipPort;
	int		m_localAudioPort;
	int		m_localVideoPort;

};

#endif // __SipParameter_h__
