/******************************************************************************
 * Name:        scd_socket.cpp
 * Purpose:     SSH / Telnet socket switcher
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/

#ifndef SCD_SOCKET_CPP
#define SCD_SOCKET_CPP
#include "scd_socket.h"

// ============================================================================

SCD_Socket::SCD_Socket()
{
	telnet_sock = NULL;
#ifndef BBMAN_NO_SSH
	ssh_sock = NULL;
#endif
	m_type = SOCK_UNKNOWN;
	evt_handler = NULL;
	m_clientdata = NULL;
	local_buf_len = 0;
}
// ----------------------------------------------------------------------------
SCD_Socket::~SCD_Socket()
{
	Destroy();

	//不能直接 delete socket , 必須執行 Destroy() 不然會 crash
//	if( m_type == SOCK_TELNET )	delete telnet_sock;
//	else if( m_type == SOCK_SSH )	delete ssh_sock;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#ifndef BBMAN_NO_SSH
SCD_wxSSH* SCD_Socket::GetSSH()
{
	if( m_type == SOCK_SSH )	return ssh_sock;
	else return NULL;
}
#endif
// ----------------------------------------------------------------------------
int SCD_Socket::GetType()
{	return m_type;	}
// ----------------------------------------------------------------------------
void SCD_Socket::SetType(int _t)
{
	if( _t != SOCK_TELNET && _t != SOCK_SSH )	return;

	if( ! IsDisconnected() )	return;
	if( m_type == _t )	return;

	if( m_type == SOCK_TELNET )
	{	delete telnet_sock;	telnet_sock = NULL;	}
#ifndef BBMAN_NO_SSH
	else if( m_type == SOCK_SSH )
	{	ssh_sock->Destroy();	ssh_sock = NULL;	}
#endif

	m_type = _t;

	if( m_type == SOCK_TELNET )
	{
		telnet_sock = new wxSocketClient();

		telnet_sock->SetTimeout(0);
		telnet_sock->SetFlags(wxSOCKET_NOWAIT);
		telnet_sock->SetClientData(m_clientdata);

		telnet_sock->SetNotify( wxSOCKET_INPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG );
		telnet_sock->SetEventHandler(*evt_handler, evt_id);
	}
#ifndef BBMAN_NO_SSH
	else if( m_type == SOCK_SSH )
	{
		ssh_sock = new SCD_wxSSH(wxSOCKET_NOWAIT);

		ssh_sock->SetEventHandler( *evt_handler, evt_id );
		ssh_sock->SetClientData( m_clientdata );
	}
#endif
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool SCD_Socket::Connect(wxIPV4address& address, bool wait, wxString _username)
{
	if( m_type == SOCK_TELNET )
	{
		local_buf_len = 0;
		telnet_sock->SetNotify( wxSOCKET_INPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG );
		telnet_sock->SetEventHandler(*evt_handler, evt_id);
		telnet_sock->Notify(true);
		bool b = telnet_sock->Connect(address, wait);
		return b;
	}
#ifndef BBMAN_NO_SSH
	else if( m_type == SOCK_SSH )	return ssh_sock->Connect(address, _username);
#endif
	else return false;
}
// ----------------------------------------------------------------------------
#ifndef BBMAN_NO_SSH
bool SCD_Socket::Login(wxString _username, wxString _password)	//for scd_wxssh
{
	if( m_type == SOCK_SSH )	return ssh_sock->Login( _username, _password );
	else return false;
}
#endif
// ----------------------------------------------------------------------------
#ifndef BBMAN_NO_SSH
bool SCD_Socket::isLogined()	//for scd_wxssh
{
	if( m_type == SOCK_SSH )	return ssh_sock->isLogined();
	else return false;
}
#endif
// ----------------------------------------------------------------------------
void SCD_Socket::Close()
{
	if( m_type == SOCK_TELNET )	telnet_sock->Close();
#ifndef BBMAN_NO_SSH
	else if( m_type == SOCK_SSH )	ssh_sock->Close();
#endif
}
// ----------------------------------------------------------------------------
bool SCD_Socket::Destroy()
{
//wxMessageBox("SCD_Socket::Destroy()");
	if( m_type == SOCK_TELNET )	return telnet_sock->Destroy();
#ifndef BBMAN_NO_SSH
	else if( m_type == SOCK_SSH )	return ssh_sock->Destroy();
#endif
	else return true;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool SCD_Socket::IsDisconnected()
{
	if( m_type == SOCK_TELNET )	return telnet_sock->IsDisconnected();
#ifndef BBMAN_NO_SSH
	else if( m_type == SOCK_SSH )	return ssh_sock->IsDisconnected();
#endif
	else return true;
}
// ----------------------------------------------------------------------------
bool SCD_Socket::IsConnected()
{
	if( m_type == SOCK_TELNET )	return telnet_sock->IsConnected();
#ifndef BBMAN_NO_SSH
	else if( m_type == SOCK_SSH )	return ssh_sock->IsConnected();
#endif
	else return false;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
int SCD_Socket::Read(void * buffer, wxUint32 nbytes)
{
	if( m_type == SOCK_TELNET )	//這是為了解決 wxSocketClient::Unread() 的問題而設
	{
		int c = 0;	//填入了多少資料進入 buf 了 ?
	
		if( local_buf_len >= nbytes )
		{
			telnet_sock->Read( buffer, nbytes );
			local_buf_len -= nbytes;
			c = nbytes;
		}
		else
 		{
 			telnet_sock->Read( buffer, local_buf_len );
 			nbytes -= local_buf_len;
 			telnet_sock->Read( (char*)buffer + local_buf_len , nbytes );
  			c = local_buf_len + telnet_sock->LastCount();
  			local_buf_len = 0;
		}

		return c;
	}
#ifndef BBMAN_NO_SSH
	else if( m_type == SOCK_SSH )
	{
		ssh_sock->Read(buffer, nbytes);
		return ssh_sock->LastCount();
	}
#endif
	else return 0;
}
// ----------------------------------------------------------------------------
void SCD_Socket::Write(const void * buffer, wxUint32 nbytes)
{
	if( m_type == SOCK_TELNET )	telnet_sock->Write(buffer, nbytes);
#ifndef BBMAN_NO_SSH
	else if( m_type == SOCK_SSH )	ssh_sock->Write(buffer, nbytes);
#endif
}
// ----------------------------------------------------------------------------
void SCD_Socket::Unread(const void * buffer, wxUint32 nbytes)
{
	if( m_type == SOCK_TELNET )	//這是為了解決 wxSocketClient::Unread() 的問題而設
	{
		telnet_sock->Unread(buffer, nbytes);
		local_buf_len += nbytes;
	}
#ifndef BBMAN_NO_SSH
	else if( m_type == SOCK_SSH )	ssh_sock->Unread(buffer, nbytes);
#endif
}
// ----------------------------------------------------------------------------
wxUint32 SCD_Socket::LastCount()
{
	if( m_type == SOCK_TELNET )	return telnet_sock->LastCount();
#ifndef BBMAN_NO_SSH
	else if( m_type == SOCK_SSH )	return ssh_sock->LastCount();
#endif
	else return 0;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_Socket::SetEventHandler(wxEvtHandler& handler, int id)
{
	evt_handler = & handler;
	evt_id = id;

	if( m_type == SOCK_TELNET )	telnet_sock->SetEventHandler(*evt_handler, id);
#ifndef BBMAN_NO_SSH
	else if( m_type == SOCK_SSH )	ssh_sock->SetEventHandler(*evt_handler, id);
#endif
}
// ----------------------------------------------------------------------------
void SCD_Socket::SetClientData(void *data)
{
	m_clientdata = data;
	if( m_type == SOCK_TELNET )	telnet_sock->SetClientData(m_clientdata);
#ifndef BBMAN_NO_SSH
	else if( m_type == SOCK_SSH )	ssh_sock->SetClientData(m_clientdata);
#endif
}
// ----------------------------------------------------------------------------
void* SCD_Socket::GetClientData()
{
	if( m_type == SOCK_TELNET )	return telnet_sock->GetClientData();
#ifndef BBMAN_NO_SSH
	else if( m_type == SOCK_SSH )	return ssh_sock->GetClientData();
#endif
	else return NULL;
}

// ============================================================================
#endif
