#include "Siptransmit.h"
#include  <signal.h>   
#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#define VER_STRING "biwoo_Siptransmit v0.1"

Siptransmit::Siptransmit(void)
: m_threadEventInfo(NULL)
, m_threadSipEvent(NULL)
, m_bInitedSip(false), m_bIsRegistered(false)
//, m_osipmessage(0)

{
	m_regid = 0;
	m_EvenHandler = NULL;
}

Siptransmit::~Siptransmit(void)
{
	quitSip();
}

tstring Siptransmit::GetRemoteIp(int did)
{
	sdp_message_t *remote_sdp = NULL;//get server's rtp port
	remote_sdp =  eXosip_get_remote_sdp(did);
	if (remote_sdp == NULL)
		return _T("");

	char buffer[100];
	memset(buffer, 0, sizeof(buffer));
	strcpy(buffer, remote_sdp->o_addr);
	sdp_message_free (remote_sdp);
	return buffer;
}

tstring Siptransmit::GetCaller(int did)
{
	sdp_message_t *remote_sdp = NULL;//get server's rtp port
	remote_sdp =  eXosip_get_remote_sdp(did);
	if (remote_sdp == NULL)
		return _T("");

	char buffer[100];
	memset(buffer, 0, sizeof(buffer));
	strcpy(buffer, remote_sdp->o_username);
	sdp_message_free (remote_sdp);
	return buffer;
}

int Siptransmit::GetRemoteAudioPort(int did)
{
	sdp_message_t *remote_sdp = NULL;//get server's rtp port
	remote_sdp =  eXosip_get_remote_sdp(did);
	if (remote_sdp == NULL)
		return 0;
	sdp_media_t *remote_med = NULL;
	remote_med = eXosip_get_audio_media(remote_sdp);
	if (remote_med == NULL)
	{
		sdp_message_free (remote_sdp);
		return -1;
	}

	int result = atoi(remote_med->m_port);
	sdp_message_free (remote_sdp);
	return result;
}

int Siptransmit::GetRemoteVideoPort(int did)
{
	sdp_message_t *remote_sdp = NULL;//get server's rtp port
	remote_sdp =  eXosip_get_remote_sdp(did);
	if (remote_sdp == NULL)
		return 0;
	sdp_media_t *remote_med = NULL;
	remote_med = eXosip_get_video_media(remote_sdp);
	if (remote_med == NULL)
	{
		sdp_message_free (remote_sdp);
		return -1;
	}

	int result = atoi(remote_med->m_port);
	sdp_message_free (remote_sdp);
	return result;
}

void Siptransmit::GetRemoteMedia(int did)
{
	sdp_message_t *remote_sdp;
	sdp_media_t *remote_med;
	char *tmp = NULL;
	int pos;

	//char localip[128];


	remote_sdp = eXosip_get_remote_sdp (did);
	if (remote_sdp == NULL)
	{
		OSIP_TRACE (osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"No remote SDP body found for call\n"));
		return;                /* no existing body? */
	}

	pos = 0;
	while (!osip_list_eol(& (remote_sdp->m_medias), pos))
	{
		char payloads[128];
		int pos2;

		memset (payloads, '\0', sizeof (payloads));
		remote_med = (sdp_media_t *) osip_list_get (&(remote_sdp->m_medias), pos);

		if (0 == osip_strcasecmp (remote_med->m_media, "audio"))
		{
			pos2 = 0;
			while (!osip_list_eol (&(remote_med->m_payloads), pos2))
			{
				tmp = (char *) osip_list_get (&(remote_med->m_payloads), pos2);
				if (tmp != NULL &&
					(0 == osip_strcasecmp (tmp, "0")
					|| 0 == osip_strcasecmp (tmp, "8")))
				{
					strcat (payloads, tmp);
					strcat (payloads, " ");
				}
				pos2++;
			}
		}
		pos++;
	}

}

bool Siptransmit::initSip(const CSipParameter & sipp, OnSipHandler * handler)
{
	if (!m_bInitedSip)
	{
		m_sipp = sipp;
		int ret = eXosip_init ();
		if (ret != 0)
			return false;

		if (eXosip_listen_addr (IPPROTO_UDP, NULL, m_sipp.sipport(), AF_INET, 0))
		{
			return false; ///´´½¨¼àÌý¶Ë¿ÚÊ§°Ü
		}

		eXosip_set_user_agent (VER_STRING);
		if (eXosip_add_authentication_info(m_sipp.ua().c_str(), m_sipp.ua().c_str(), m_sipp.pwd().c_str(), NULL, NULL))
		{
			return false;
		}

		m_EvenHandler = handler;
		m_bInitedSip = true;
		if (m_threadSipEvent == NULL)
			m_threadSipEvent = new boost::thread(boost::bind(do_proc_SipEvent, this));
		if (m_threadEventInfo == NULL)
			m_threadEventInfo = new boost::thread(boost::bind(do_proc_EventInfo, this));
	}
	return m_bInitedSip;
}

void Siptransmit::quitSip(void)
{
	sipUnRegister();

	if (m_bInitedSip)
	{
		m_bInitedSip = false;
		if (m_threadSipEvent != NULL)
		{
			m_threadSipEvent->join();
			delete m_threadSipEvent;
			m_threadSipEvent = NULL;
		}

		if (m_threadEventInfo != NULL)
		{
			m_threadEventInfo->join();
			delete m_threadEventInfo;
			m_threadEventInfo = NULL;
		}

		m_EvenHandler = NULL;
		m_mapEventInfo.clear();

		eXosip_quit();
	}
}

int Siptransmit::sipRegister(void)              ///×¢²á
{
	if (!m_bInitedSip) return -1;

	osip_message_t *reg = NULL;
	eXosip_lock ();
	m_regid = eXosip_register_build_initial_register (m_sipp.identity().c_str(), m_sipp.proxy().c_str(),
		NULL, 3600, &reg);
	if (m_regid < 0)
	{
		eXosip_unlock ();
		return -4;
	}

	osip_message_set_supported (reg, "100rel");
	osip_message_set_supported(reg, "path");

	int ret = eXosip_register_send_register (m_regid, reg);
	eXosip_unlock ();

	return ret;
}


void Siptransmit::sipUnRegister(void)                ///×¢Ïú
{
	if (!m_bInitedSip) return;

	m_bIsRegistered = false;
	osip_message_t *reg = NULL;

	try
	{
		AUTO_LOCK(m_mapCallInfo);
		CLockMap<int, SipCallInfo::pointer>::iterator iter;
		for (iter=m_mapCallInfo.begin(); iter!=m_mapCallInfo.end(); iter++)
		{
			sipCallTerminate(iter->second);
		}
		m_mapCallInfo.clear(false);
	}catch (...)
	{}

	eXosip_lock ();
	eXosip_register_remove(m_regid);
	eXosip_unlock ();
}

int Siptransmit::sipCallInvite(const tstring & callee_num) ///ºô½Ð
{
	if (!m_bInitedSip) return -1;

	osip_message_t *invite;
	int i;
	i = eXosip_call_build_initial_invite (&invite, callee_num.c_str(),
		m_sipp.identity().c_str(), NULL, "This is a call for a conversation");
	if (i != 0)
	{
		return -2;
	}
	osip_message_set_supported (invite, "100rel");
	{
		char tmp[4096];
		char localip[128];

		eXosip_guess_localip (AF_INET, localip, 128);
		snprintf (tmp, 4096,
			"v=0\r\n"
			"o=%s 0 0 IN IP4 %s\r\n"
			"s=conversation\r\n"
			"c=IN IP4 %s\r\n"
			"t=0 0\r\n"
			"m=audio %d RTP/AVP 0 8 101\r\n"
			"a=rtpmap:8 PCMA/8000\r\n"
			"a=rtpmap:0 PCMU/8000\r\n"
			"a=rtpmap:101 telephone-event/8000\r\n"
			"a=fmtp:101 0-11\r\n"

			"m=video %d RTP/AVP 99 98 34\r\n"
			"a=rtpmap:99 MP4V-ES/90000\r\n"
			"a=fmtp:99\r\n"
			"profile-level-id=3\r\n"
			"a=rtpmap:98 H263-1998/90000\r\n"
			"a=fmtp:98 CIF=1;QCIF=1\r\n"
			"a=rtpmap:34 H263/90000\r\n"
			"a=fmtp:34 QCIF=2\r\n"

			, this->m_sipp.ua().c_str(), localip, localip, m_sipp.localaudioport(), m_sipp.localvideoport());
		osip_message_set_body (invite, tmp, strlen (tmp));
		osip_message_set_content_type (invite, "application/sdp");
	}
	eXosip_lock ();
	i = eXosip_call_send_initial_invite (invite);
	if (i > 0)
	{
		eXosip_call_set_reference (i, NULL);
	}
	eXosip_unlock ();
	return i > 0 ? 0 : -3;
}

int Siptransmit::sipCallAnswer(SipCallInfo::pointer callInfo, int localaudioport, int localvideoport)       ///Ó¦´ðºô½Ð
{
	BOOST_ASSERT (callInfo.get() != NULL);
	/*
	eXosip_lock ();
	eXosip_call_send_answer (tid, 100, NULL);//trying
	eXosip_call_send_answer (tid, 180, NULL);//ringing
	eXosip_unlock ();*/
	
	m_currentCallInfo = callInfo;
	m_sipp.localaudioport(localaudioport);
	m_sipp.localvideoport(localvideoport);

	int tid = callInfo->tranId();
	int did = callInfo->dialogId();

	osip_message_t *answer = NULL;
	int i;
	eXosip_lock ();
	i = eXosip_call_build_answer (tid, 200, &answer);
	if (i != 0)
	{
		eXosip_call_send_answer (tid, 400, NULL);
	}
	else
	{
		i = sdp_complete_200ok (did, answer, localaudioport, localvideoport);
		if (i != 0)
		{
			osip_message_free (answer);
			eXosip_call_send_answer (tid, 415, NULL);
		}
		else
			eXosip_call_send_answer (tid, 200, answer);
	}
	eXosip_unlock ();
	return 0;
}

int Siptransmit::sipCallAnswer(SipCallInfo::pointer callInfo)       ///Ó¦´ðºô½Ð
{
	return sipCallAnswer(callInfo, m_sipp.localaudioport(), m_sipp.localvideoport());
}

int Siptransmit::sipCallTerminate(SipCallInfo::pointer callInfo)                    ///¹Ò»ú
{
	BOOST_ASSERT (callInfo.get() != NULL);
	//m_osipmessage = 0;

	int status = 0;
	eXosip_lock();
	status = eXosip_call_terminate(callInfo->callId(), callInfo->dialogId());
	eXosip_unlock();
	return status;
}


int Siptransmit::sdp_complete_200ok (int did, osip_message_t * answer, int audio_port, int video_port)
{
	sdp_message_t *remote_sdp;
	sdp_media_t *remote_med;
	char *tmp = NULL;
	char buf[4096];
	int pos;

	char localip[128];


	remote_sdp = eXosip_get_remote_sdp (did);
	if (remote_sdp == NULL)
	{
		OSIP_TRACE (osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"No remote SDP body found for call\n"));
		return -1;                /* no existing body? */
	}

	eXosip_guess_localip (AF_INET, localip, 128);
	snprintf (buf, 4096,
		"v=0\r\n"
		"o=%s 0 0 IN IP4 %s\r\n"
		"s=conversation\r\n" "c=IN IP4 %s\r\n" "t=0 0\r\n", this->m_sipp.ua().c_str(), localip, localip);

	pos = 0;
	while (!osip_list_eol(& (remote_sdp->m_medias), pos))
	{
		char payloads[128];
		int pos2;

		memset (payloads, '\0', sizeof (payloads));
		remote_med = (sdp_media_t *) osip_list_get (&(remote_sdp->m_medias), pos);

		if (0 == osip_strcasecmp (remote_med->m_media, "audio"))
		{
			pos2 = 0;
			while (!osip_list_eol (&(remote_med->m_payloads), pos2))
			{
				tmp = (char *) osip_list_get (&(remote_med->m_payloads), pos2);
				if (tmp != NULL &&
					(0 == osip_strcasecmp (tmp, "0")
					|| 0 == osip_strcasecmp (tmp, "8")))
				{
					strcat (payloads, tmp);
					strcat (payloads, " ");
				}
				pos2++;
			}
			strcat (buf, "m=");
			strcat (buf, remote_med->m_media);
			if (pos2 == 0 || payloads[0] == '\0')
			{
				strcat (buf, " 0 RTP/AVP \r\n");
				sdp_message_free (remote_sdp);
				return -1;        /* refuse anyway */
			} else
			{
				char tmp_port[10] = {0};
				sprintf(tmp_port, " %d", audio_port);
				strcat (buf, tmp_port);
				strcat (buf, " RTP/AVP ");
				strcat (buf, payloads);
				strcat (buf, "\r\n");

				if (NULL != strstr (payloads, " 0 ")
					|| (payloads[0] == '0' && payloads[1] == ' '))
					strcat (buf, "a=rtpmap:0 PCMU/8000\r\n");
				if (NULL != strstr (payloads, " 8 ")
					|| (payloads[0] == '8' && payloads[1] == ' '))
					strcat (buf, "a=rtpmap:8 PCMA/8000\r\n");

			}
		} else
		{
			char tmp_port[10];
			memset(tmp_port,0,10);
			strcat (buf, "m=");
			strcat (buf,remote_med->m_media);

			sprintf(tmp_port, " %d ", video_port);
			strcat (buf, tmp_port);

			strcat (buf, remote_med->m_proto);
			strcat (buf, " 99 98 34\r\n"
				"a=rtpmap:99 MP4V-ES/90000\r\n"
				"a=fmtp:99\r\n"
				"profile-level-id=3\r\n"
				"a=rtpmap:98 H263-1998/90000\r\n"
				"a=fmtp:98 CIF=1;QCIF=1\r\n"
				"a=rtpmap:34 H263/90000\r\n"
				"a=fmtp:34 QCIF=2\r\n");
		}
		pos++;
	}

	osip_message_set_body (answer, buf, strlen (buf));
	osip_message_set_content_type (answer, "application/sdp");
	sdp_message_free (remote_sdp);

	return 0;
}

int  Siptransmit::sipCallSendDtmf(SipCallInfo::pointer callInfo, char dtmf)
{
	BOOST_ASSERT (callInfo.get() != NULL);

	char dtmf_body[1000];
	char clen[10];
	osip_message_t *msg=NULL;
	/* Out of Band DTMF (use INFO method) */
	eXosip_lock();
	eXosip_call_build_info(callInfo->dialogId(),&msg);
	if (msg == 0)
	{
		eXosip_unlock();
		return -1;
	}

	snprintf(dtmf_body, 999, "Signal=%c\r\nDuration=250\r\n", dtmf);
	osip_message_set_body(msg,dtmf_body,strlen(dtmf_body));
	osip_message_set_content_type(msg,"application/dtmf-relay");
	snprintf(clen,sizeof(clen),"%lu",(unsigned long)strlen(dtmf_body));
	osip_message_set_content_length(msg,clen);

	eXosip_call_send_request(callInfo->dialogId(), msg);
	eXosip_unlock();
	return 0;
}

SipCallInfo::pointer Siptransmit::getCallInfo(int tranId, int callId, int dialogId)
{
	SipCallInfo::pointer result;
	if (!m_mapCallInfo.find(callId, result))
	{
		result = SipCallInfo::create(tranId, callId, dialogId);
		m_mapCallInfo.insert(callId, result);
	}else
	{
		result->dialogId(dialogId);
	}
	return result;
}

void Siptransmit::procOneSipEvent(void)
{
	SipEventInfo::pointer eventInfo;
	if (m_mapEventInfo.front(eventInfo))
	{
		BOOST_ASSERT (eventInfo.get() != NULL);
		switch (eventInfo->getEventType())
		{
		case SipEventInfo::ET_Register:
			{
				if (eventInfo->getEventData() == 2)
					m_bIsRegistered = true;
			}break;
		case SipEventInfo::ET_CallAnswered:
			{
				BOOST_ASSERT (eventInfo->getCallInfo().get() != NULL);

				m_currentCallInfo = eventInfo->getCallInfo();
			}break;
		default:
			break;
		}

		if (m_EvenHandler != NULL)
			m_EvenHandler->onSipEvent(eventInfo);

		switch (eventInfo->getEventType())
		{
		case SipEventInfo::ET_CallClosed:
			{
				BOOST_ASSERT (eventInfo->getCallInfo().get() != NULL);

				m_mapCallInfo.remove(eventInfo->getCallInfo()->callId());
				if (eventInfo->getCallInfo().get() == m_currentCallInfo.get())
					m_currentCallInfo.reset();
			}break;
		default:
			break;
		}
	}else
	{
#ifdef WIN32
		Sleep(100);
#else
		usleep(100000);
#endif
	}
}

void Siptransmit::addSipEvent(SipEventInfo::pointer eventInfo)
{
	BOOST_ASSERT (eventInfo.get() != NULL);

	if (m_EvenHandler != NULL)
		m_mapEventInfo.add(eventInfo);
}

SipCallInfo::pointer Siptransmit::getCallInfo(int callId)
{
	SipCallInfo::pointer result;
	m_mapCallInfo.find(callId, result);
	return result;
}

void Siptransmit::do_proc_EventInfo(Siptransmit * pSiptransmit)
{
	BOOST_ASSERT (pSiptransmit != NULL);

	while (pSiptransmit->isInitSip())
	{
		pSiptransmit->procOneSipEvent();
	}
}

//static int *SipEventProcess(void * pParam)
void Siptransmit::do_proc_SipEvent(Siptransmit * pSiptransmit)
{
	BOOST_ASSERT (pSiptransmit != NULL);
	while (pSiptransmit->isInitSip())
	{
		eXosip_lock ();
		eXosip_event_t * sipevent = eXosip_event_wait (0, 1000);
		eXosip_unlock ();
		if (sipevent == NULL)
		{
#ifdef WIN32
			Sleep(100);
#else
			usleep(100000);
#endif
			continue;
		}

		eXosip_automatic_action ();
		switch (sipevent->type)
		{
		case EXOSIP_REGISTRATION_NEW:				//reg start
			{
				//fprintf(stderr, "Received new registration");
				pSiptransmit->addSipEvent(SipEventInfo::create(SipEventInfo::ET_Register, 1));
			}break;
		case EXOSIP_REGISTRATION_SUCCESS:
			//print_error_message("Registration successful");
				pSiptransmit->addSipEvent(SipEventInfo::create(SipEventInfo::ET_Register, 2));
			break;
		case EXOSIP_REGISTRATION_FAILURE:
			{
				//print_error_message("Registration faild.");
				//regparam.auth = 1;
				pSiptransmit->addSipEvent(SipEventInfo::create(SipEventInfo::ET_Register, 3));
			}break;
		case EXOSIP_REGISTRATION_TERMINATED:		//reg end
			{
				//fprintf(stderr, "Registration terminated\n");
				pSiptransmit->addSipEvent(SipEventInfo::create(SipEventInfo::ET_Register, 4));
			}break;
		case EXOSIP_MESSAGE_NEW:  
			{
				//print_error_message("I've got the OPTIONS message from Asterisk ");	
				if (MSG_IS_MESSAGE (sipevent->request))
				{
					osip_message_t *answer = NULL;
					eXosip_lock ();
					if (eXosip_message_build_answer (sipevent->tid, 200,&answer) == 0)
					{
						eXosip_options_send_answer (sipevent->tid, 200, answer);
					}
					eXosip_unlock ();
				}
				// ??			pSiptransmit->GetHandler()->onDefaultEven(*sipevent);
				//status in trixbox, replace NULL with media message 
			}break;
		case EXOSIP_CALL_REINVITE:
		case EXOSIP_CALL_INVITE:					//invite start
			{
				//print_error_message("You've got a incoming call...");
				//pSiptransmit->GetRemoteAudioPort(sipevent->did);
				eXosip_call_send_answer (sipevent->tid, 180, NULL);

				SipCallInfo::pointer callInfo = pSiptransmit->getCallInfo(sipevent->tid, sipevent->cid, sipevent->did);
				BOOST_ASSERT (callInfo.get() != NULL);

				callInfo->callee(sipevent->request->req_uri->username);
				callInfo->caller(Siptransmit::GetCaller(callInfo->dialogId()));
				callInfo->remoteIp(Siptransmit::GetRemoteIp(callInfo->dialogId()));
				callInfo->audioPort(Siptransmit::GetRemoteAudioPort(callInfo->dialogId()));
				callInfo->videoPort(Siptransmit::GetRemoteVideoPort(callInfo->dialogId()));

				osip_content_type_t * contenttype = osip_message_get_content_type(sipevent->request);
				if (contenttype != 0)
				{
					osip_body_t * body;
					osip_message_get_body(sipevent->request, 0, &body);
					if (body != 0)
					{

					}
				}

				Siptransmit::GetRemoteMedia(callInfo->dialogId());

				pSiptransmit->addSipEvent(SipEventInfo::create(SipEventInfo::ET_CallInvite, callInfo));
			}break;
		case EXOSIP_CALL_NOANSWER:
			{

			}break;
		case EXOSIP_CALL_PROCEEDING:
			{
				//print_error_message("100 Trying...");
				SipCallInfo::pointer callInfo = pSiptransmit->getCallInfo(sipevent->tid, sipevent->cid, sipevent->did);
				BOOST_ASSERT (callInfo.get() != NULL);

				pSiptransmit->addSipEvent(SipEventInfo::create(SipEventInfo::ET_CallProceeding, callInfo));
			}break;
		case EXOSIP_CALL_RINGING:
			{
				SipCallInfo::pointer callInfo = pSiptransmit->getCallInfo(sipevent->tid, sipevent->cid, sipevent->did);
				BOOST_ASSERT (callInfo.get() != NULL);

				pSiptransmit->addSipEvent(SipEventInfo::create(SipEventInfo::ET_CallRinging, callInfo));
			}break;
		case EXOSIP_CALL_REQUESTFAILURE:
			{
				//print_error_message("EXOSIP_CALL_REQUESTFAILURE");//busy or declined
				SipCallInfo::pointer callInfo = pSiptransmit->getCallInfo(sipevent->tid, sipevent->cid, sipevent->did);
				BOOST_ASSERT (callInfo.get() != NULL);

				pSiptransmit->addSipEvent(SipEventInfo::create(SipEventInfo::ET_CallRequestFailure, callInfo));
			}break;
		case EXOSIP_CALL_ANSWERED:
			{
				//print_error_message("CALL ANSWERED.");
				//osip_message_t * ack = NULL;	
				osip_message_t * ack=NULL;
				eXosip_call_build_ack(sipevent->did, &ack);
				eXosip_call_send_ack(sipevent->did, ack);//send ACK when received 200OK, to complete the call

				SipCallInfo::pointer callInfo = pSiptransmit->getCallInfo(sipevent->tid, sipevent->cid, sipevent->did);
				BOOST_ASSERT (callInfo.get() != NULL);

				callInfo->caller(Siptransmit::GetCaller(callInfo->dialogId()));
				callInfo->remoteIp(Siptransmit::GetRemoteIp(callInfo->dialogId()));
				callInfo->audioPort(Siptransmit::GetRemoteAudioPort(callInfo->dialogId()));
				callInfo->videoPort(Siptransmit::GetRemoteVideoPort(callInfo->dialogId()));
				pSiptransmit->addSipEvent(SipEventInfo::create(SipEventInfo::ET_CallAnswered, callInfo));
			}break;
		case EXOSIP_CALL_ACK:						//invite end
			//print_error_message("ACK received!");
// ??			pSiptransmit->GetHandler()->onDefaultEven(*event);
			break;
		case EXOSIP_CALL_MESSAGE_NEW:            		 //passive hangup start
			{
			//print_error_message("EXOSIP_CALL_MESSAGE_NEW");//BYE was received 
// ??			pSiptransmit->GetHandler()->onDefaultEven(*event);
				SipCallInfo::pointer callInfo = pSiptransmit->getCallInfo(sipevent->tid, sipevent->cid, sipevent->did);
				BOOST_ASSERT (callInfo.get() != NULL);

				if (MSG_IS_INFO(sipevent->request))
				{
					osip_message_t *answer = NULL;
					eXosip_lock ();
					if (eXosip_message_build_answer (sipevent->tid, 200,&answer) == 0)
					{
						eXosip_options_send_answer (sipevent->tid, 200, answer);
					}
					eXosip_unlock ();

					osip_content_type_t * contenttype = osip_message_get_content_type(sipevent->request);
					if (contenttype != 0 && strcmp(contenttype->subtype, "dtmf-relay") == 0)
					{
						char dtmf = 0;
						osip_body_t * body;
						osip_message_get_body(sipevent->request, 0, &body);
						if (body != 0)
						{
							// Signal=
							char * findBegin = strstr(body->body, "Signal=");
							char * findEnd = strstr(findBegin, "\r\n");
							if (findBegin && findEnd)
							{
								dtmf = findBegin[7];
								callInfo->callMessage(SipCallInfo::MT_Dtmf, tstring(&dtmf, 1));
							}
						}
					}
				}

				//sdp_message_t * sdpMessage = eXosip_get_sdp_info (sipevent->request);

				pSiptransmit->addSipEvent(SipEventInfo::create(SipEventInfo::ET_CallMessageNew, callInfo));
			}break;
		case EXOSIP_CALL_CLOSED:					//passive hangup end
			{
				osip_message_t *answer = NULL;
				eXosip_lock ();
				if (eXosip_call_build_answer (sipevent->tid, 200, &answer) == 0)
				{
					eXosip_call_send_answer (sipevent->tid, 200, answer);
				}else
				{
					eXosip_call_send_answer (sipevent->tid, 400, NULL);
				}
				eXosip_unlock ();

				//print_error_message("Call Closed.");//BYE was received
				//200 OK sent automaticly by eXosip
				SipCallInfo::pointer callInfo = pSiptransmit->getCallInfo(sipevent->tid, sipevent->cid, sipevent->did);
				BOOST_ASSERT (callInfo.get() != NULL);

				pSiptransmit->addSipEvent(SipEventInfo::create(SipEventInfo::ET_CallClosed, callInfo));
			}break;
		case EXOSIP_CALL_MESSAGE_ANSWERED:		//active hangup start
			//print_error_message("EXOSIP_CALL_MESSAGE_ANSWERED 200 ok");
// ??			pSiptransmit->GetHandler()->onDefaultEven(*event);
			break;	
		case EXOSIP_CALL_RELEASED:					//active hangup end
			{
				//print_error_message("Call Released.");
				SipCallInfo::pointer callInfo = pSiptransmit->getCallInfo(sipevent->tid, sipevent->cid, sipevent->did);
				BOOST_ASSERT (callInfo.get() != NULL);

				pSiptransmit->addSipEvent(SipEventInfo::create(SipEventInfo::ET_CallClosed, callInfo));

			}break;			

		default:
// ??			pSiptransmit->GetHandler()->onDefaultEven(*event);
			//fprintf(stderr, "\n\n\nrecieved unknown eXosip event (type, did, cid) = (%d, %d, %d)\n\n\n",
			//	event->type, event->did, event->cid);
			break;
		}
		eXosip_event_free (sipevent);
	}
}



