/******************************************************************************
 * Name:        scd_wxssh.cpp
 * Purpose:     a wrapper of SSH in libssh library
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/

#ifndef SCD_WXSSH_CPP
#define SCD_WXSSH_CPP
#include "scd_wxssh.h"

#include <wx/thread.h>

wxPointerArray ChannelList;

int AutoSelectChannel();
void ThreadFunc_Connect(SCD_wxSSH *ssh, wxIPV4address& address);
void ThreadFunc_Login(SCD_wxSSH *ssh, wxString _username, wxString _password);

enum { WORK_AutoSelectChannel , WORK_CONNECT , WORK_LOGIN };
wxCriticalSection critical_section;
// ============================================================================

//用來定時檢查所有 channel 有沒有資料進來
class WorkThread : public wxThread
{
private:
public:
	bool toStop;
	int work_id;

	SCD_wxSSH *ssh;
	wxIPV4address address;
	wxString username, password;

public:

	WorkThread(int _workid) : wxThread()
	{
		work_id = _workid;
		toStop = false;
	}
	~WorkThread()
	{
		if(ssh)	ssh->working_thread = NULL;
//		wxMessageBox("wordthread destructor");
	}

	void ToStop()
	{
		toStop = true;
	}

	static WorkThread* CreateAutoSelectChannelThread()
	{
		WorkThread *thread;
		thread = new WorkThread(WORK_AutoSelectChannel);
		thread->Create();
		thread->SetPriority(0);
		thread->Run();
		return thread;
	}

	static WorkThread* CreateConnectThread(SCD_wxSSH *_ssh, wxIPV4address& _address)
	{
		WorkThread *thread;
		thread = new WorkThread(WORK_CONNECT);
		thread->ssh = _ssh;
		thread->address = _address;
		thread->Create();
		thread->Run();
		return thread;
	}

	static WorkThread* CreateLoginThread(SCD_wxSSH *_ssh, wxString _username, wxString _passwd)
	{
		WorkThread *thread;
		thread = new WorkThread(WORK_LOGIN);
		thread->ssh = _ssh;
		thread->username = _username;
		thread->password = _passwd;
		thread->Create();
		thread->Run();
		return thread;
	}

	ExitCode Entry()
	{
		switch(work_id)
		{
			case WORK_AutoSelectChannel :
			{
				int need_to_sleep_count = 0;
				while(true)
				{
					if( toStop )	break;
					int count = AutoSelectChannel();
					if(count==0)	//如果所有連線都沒有資料進來, 則應該要 sleep
					{
						//如果連續 6000 次以上應該要 sleep 才 sleep , 不然會造成反應遲鈍
						//6000 這數字也不能設的太小, 不然常常會反應遲鈍
						//6000 這數字設的太大也會造成 CPU loading 變大
						if(need_to_sleep_count >= 6300)	Sleep(100);
						else if(need_to_sleep_count >= 6000)	Sleep(30);
						else need_to_sleep_count ++;
					}
					else need_to_sleep_count = 0;
//					Yield();
				}
			}
				break;
			case WORK_CONNECT :
				ThreadFunc_Connect(ssh, address);
				break;
			case WORK_LOGIN :
				ThreadFunc_Login(ssh, username, password);
				break;
		}
		return 0;
	}

};

static WorkThread *cm_thread = NULL;

// ============================================================================

void MonitorChannel( SCD_wxSSH *ssh )
{
	critical_section.Enter();
	if( ChannelList.GetCount() == 0 )
  		if(!cm_thread)	cm_thread = WorkThread::CreateAutoSelectChannelThread();
	ChannelList.Add(ssh);

	critical_section.Leave();
}

void UnMonitorChannel( SCD_wxSSH *ssh )
{
	critical_section.Enter();

	ChannelList.Remove(ssh);

	if( ChannelList.GetCount() == 0 && cm_thread != NULL )
	{
		//下面兩行會 crash ...
//		cm_thread->ToStop();
//		cm_thread->Delete();
//		cm_thread = NULL;
	}

	critical_section.Leave();
}

int AutoSelectChannel()
{
	int count = 0;

	critical_section.Enter();

	SCD_wxSSH *ssh;
	for(int i=ChannelList.GetCount() - 1 ; i>=0 ; i-- )
	{
		ssh = (SCD_wxSSH*) ChannelList.Item(i);
		if( ssh->IsDisconnected() )	continue;

		if( ssh->channel->open == 0 )	//如果該連線已經斷線了
		{
			ssh->Close();
			continue;
		}

		if( ( ! ssh->not_read_after_notify ) && ssh->evt_handler )
		{
			if( channel_poll( ssh->channel , 0 ) > 0 )	//如果該連線有資料進來
			{
				ssh->not_read_after_notify = true;
				ssh->SendEvent(wxSOCKET_INPUT);
				count++;
			}
		}
	}

	critical_section.Leave();

	return count;
}

void ThreadFunc_Connect(SCD_wxSSH *ssh, wxIPV4address& address)
{	
	SSH_OPTIONS *options;
	int auth=0;
    char hash[MD5_DIGEST_LEN];
	char *banner;

	ssh->blLogined = false;
	ssh->connect_state = 0;
	ssh->intLastCount = 0;
	ssh->unread_len = 0;

	//建立連線資訊
	options = options_new();
    options_set_host(options, wxStringToCharPtr( address.Hostname() ) );
	options_set_port(options, address.Service() );
    options_set_username(options, wxStringToCharPtr(ssh->username) );
//wxMessageBox( wxString::Format("%s:%d", wxStringToCharPtr( address.Hostname() ) , address.Service() ) );

	ssh->connect_state = 1;
    ssh->session = ssh_connect(options);	//嘗試連到遠端 ssh server
    if(!ssh->session)	//連線失敗
	{
		ssh->connect_state = 0;
		ssh->SendEvent(wxSOCKET_LOST);
		return;
	}
	else
	{
		ssh->connect_state = 2;
		ssh->SendEvent(wxSOCKET_CONNECTION);
	}

    pubkey_get_hash(ssh->session,hash);	//取得遠端 ssh server 的 hash code

    auth = ssh_userauth_autopubkey(ssh->session);	//找出 localhost 上的 public key
    if(auth == AUTH_ERROR)	//如果 auth (非指登入) 失敗
	{
		ssh->Close();
		return;
	}

	banner = ssh_get_issue_banner(ssh->session);	//取得 banner (no use?)
	if(banner)	free(banner);

	if(auth == AUTH_SUCCESS)	//如果登入成功 (不須密碼) (anonymous login ?)
	{
		ssh->blLogined = true;
		ssh->SendEvent( (wxSocketNotify)WXSSH_AUTH_SUCCESS );
		ssh->init_ssh_channel();
	}
	else
	{
		ssh->blLogined = false;
		ssh->SendEvent( (wxSocketNotify)WXSSH_AUTH_FAIL );
	}

}

void ThreadFunc_Login(SCD_wxSSH *ssh, wxString _username, wxString _password)
{
	ssh->blLogined = (ssh_userauth_password(ssh->session,
		wxStringToCharPtr(_username),
		wxStringToCharPtr(_password) ) == AUTH_SUCCESS );

	if(ssh->blLogined)
	{
		ssh->password = _password;
		ssh->init_ssh_channel();
		ssh->SendEvent( (wxSocketNotify)WXSSH_AUTH_SUCCESS );
	}
	else
		ssh->SendEvent( (wxSocketNotify)WXSSH_AUTH_FAIL );
}

// ============================================================================

SCD_wxSSH::SCD_wxSSH(wxSocketFlags /*flags*/)
{
	session = NULL;
	channel = NULL;
	readbuf = buffer_new();
	blLogined = false;
	connect_state = 0;
	intLastCount = 0;
	unread_len = 0;
	not_read_after_notify = false;

	evt_handler = NULL;
	m_clientdata = NULL;

	working_thread = NULL;
	sftp_count = 0;
	m_destroyed = false;
}
// ----------------------------------------------------------------------------
SCD_wxSSH::~SCD_wxSSH()
{
	//因為要刪除此物件必須從外部呼叫 Destroy() 不能直接刪除 delete sock
	//因此如果這邊還呼叫 Destroy() 會造成無限回圈
//	Destroy();
//wxMessageBox( _T("~SCD_wxSSH") );
}
// ----------------------------------------------------------------------------
bool SCD_wxSSH::Destroy()
{
//wxMessageBox( _T("SCD_wxSSH::Destroy()") );
	if( !m_destroyed && ! IsDisconnected() )
	{
		Close();
		if(readbuf)
		{
			buffer_free(readbuf);
			readbuf = NULL;
		}

		if(working_thread)	working_thread->Delete();
	}

	m_destroyed = true;
	if(sftp_count==0)
	{
//wxMessageBox( _T("SCD_wxSSH::Destroy() sftp_count = 0 -> delete this") );
		delete this;	//delete ssh only if all its sftp are deleted
	}
	return true;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool SCD_wxSSH::Connect(wxIPV4address& address, wxString _username)
{
	if( ! IsDisconnected() )	return false;

	ip = address.Hostname();
	port = address.Service();
	username = _username;
	//因為 ssh_connect(), pubkey_get_hash(), ssh_userauth_autopubkey(), Login()
	//都是 blocking function, 因此放到 Thread 中執行
	working_thread = WorkThread::CreateConnectThread(this, address);
	return true;
}
// ----------------------------------------------------------------------------
bool SCD_wxSSH::isLogined()
{	return blLogined;	}
// ----------------------------------------------------------------------------
bool SCD_wxSSH::Login(wxString /*_username*/, wxString _password)
{
//wxMessageBox("login() start");
	if(blLogined)	return true;

	//因為 ssh_userauth_password() 是 blocking fucntion
	//因此放到 Thread 中執行
	working_thread = WorkThread::CreateLoginThread(this, username, _password);
/*
	blLogined = (ssh_userauth_password(session,
		wxStringToCharPtr(_username),
		wxStringToCharPtr(_password) ) == AUTH_SUCCESS );

	if(blLogined)	init_ssh_channel();
	if(blLogined)	wxMessageBox("logined");
	return blLogined;
*/
	return true;
}
// ----------------------------------------------------------------------------
bool SCD_wxSSH::init_ssh_channel()	//在登入後初始化 channel
{
	if( !session || !blLogined )	return false;

    channel = open_session_channel(session,1000,1000);
	if(!channel)	return false;

	MonitorChannel(this);
	channel_request_pty(channel);
    channel_request_shell(channel);	//啟動遠端的 shell
//wxMessageBox("init_ssh_channel() done");

	return true;
}
// ----------------------------------------------------------------------------
void SCD_wxSSH::Close()
{
	if( IsDisconnected() ) return;
	connect_state = 0;
	//這裡不可以產生 wxSOCKET_LOST 事件, 因為如果此一方法是在此物件被 delete 時產生時,
	//會造成 crash
//	SendEvent(wxSOCKET_LOST);

	if(channel)
	{
		UnMonitorChannel(this);
		channel_send_eof(channel);
		channel_free(channel);
		channel = NULL;
	}
	if(session)
	{
		ssh_disconnect(session);
		session = NULL;
	}

	blLogined = false;
	intLastCount = 0;
	unread_len = 0;
	not_read_after_notify = false;
}
// ----------------------------------------------------------------------------
void SCD_wxSSH::SendEvent(wxSocketNotify v)
{
/*	switch(v)
	{
		case wxSOCKET_INPUT : wxMessageBox("wxSOCKET_INPUT");	break;
		case wxSOCKET_CONNECTION : wxMessageBox("wxSOCKET_CONNECTION");	break;
		case wxSOCKET_LOST : wxMessageBox("wxSOCKET_LOST");	break;
		default : return;
	}*/
	if( evt_handler )
	{
		wxSocketEvent e( evt_id );
		e.m_event = v;
		evt_handler->AddPendingEvent(e);
	}
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool SCD_wxSSH::IsDisconnected()
{	return connect_state == 0;	}
// ----------------------------------------------------------------------------
bool SCD_wxSSH::IsConnected()
{	return connect_state == 2;	}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_wxSSH::Read(void * buffer, wxUint32 nbytes)
{
	if( ! IsConnected() || !channel )	return;

	char *strbuf = (char*)buffer;
	unsigned int poll_len;

	intLastCount = 0;
	int read_from_unread_len = ( unread_len > nbytes ) ? nbytes : unread_len;

	for(int i=0;i<read_from_unread_len;i++)
		strbuf[i] = unread_buf[ -- unread_len ];

	nbytes -= read_from_unread_len;
	if( nbytes > 0 && (poll_len = channel_poll( channel , 0 )) > 0 )
	{
		readbuf = buffer_new();
		intLastCount = (nbytes > poll_len) ? poll_len : nbytes;
		intLastCount = channel_read( channel , readbuf, intLastCount, 0);
		if( intLastCount == -1 )	Close();	//有錯誤發生
		memcpy( strbuf + read_from_unread_len , buffer_get(readbuf), intLastCount );	//複製資料到使用者緩衝區
	}

	intLastCount += read_from_unread_len;
	not_read_after_notify = false;
}
// ----------------------------------------------------------------------------
void SCD_wxSSH::Write(const void * buffer, wxUint32 nbytes)
{
	if( ! IsConnected() || !channel )	return;
	intLastCount = channel_write(channel, (void*)buffer, nbytes);
}
// ----------------------------------------------------------------------------
void SCD_wxSSH::Unread(const void * buffer, wxUint32 nbytes)
{
	if( ! IsConnected() )	return;

	char *strbuf = (char*)buffer;

	if( unread_len + nbytes > SCD_wxSSH_UNREAD_BUF_SIZE )
		nbytes = SCD_wxSSH_UNREAD_BUF_SIZE - unread_len;

	for(int i=nbytes-1;i>=0;i--)
		unread_buf[ unread_len ++ ] = strbuf[i];
}
// ----------------------------------------------------------------------------
wxUint32 SCD_wxSSH::LastCount()
{	return intLastCount;	}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_wxSSH::SetEventHandler(wxEvtHandler& handler, int id)
{
	evt_handler = & handler;
	evt_id = id;
}
// ----------------------------------------------------------------------------
void SCD_wxSSH::SetClientData(void *data)
{	m_clientdata = data;	}
// ----------------------------------------------------------------------------
void* SCD_wxSSH::GetClientData()
{	return m_clientdata;	}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/*
SCD_wxSFTP* SCD_wxSSH::CreateSFTP()
{
	if( ! IsConnected() || ! isLogined() )	return NULL;

	SFTP_SESSION *_sftp_session = sftp_new(session);
	if( ! _sftp_session )	return NULL;

	if( sftp_init(_sftp_session) != 0 )	//init failed
	{
		sftp_free(_sftp_session);
		return NULL;
	}

	SCD_wxSFTP *_ftp = new SCD_wxSFTP( / *this, * / _sftp_session);
	sftp_count ++;	//add sftp count
	return _ftp;
}
*/
// ----------------------------------------------------------------------------
void SCD_wxSSH::DeleteSFTP(void *_ftp)
{
	sftp_count --;	//decrease sftp count
	if(sftp_count==0 && m_destroyed)	delete this;	//delete ssh only if all its sftp are deleted
}

// ============================================================================
#endif
