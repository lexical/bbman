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


#ifndef SCD_SOCKET_H
#define SCD_SOCKET_H
#include "common.h"
#include <wx/socket.h>

#ifndef BBMAN_NO_SSH
	#include "scd_wxssh/scd_wxssh.h"
	typedef SCD_wxSSH SCD_SSH_Transport;
#else
	#include "scd_pty_ssh.h"
	typedef SCD_PtySSH SCD_SSH_Transport;
#endif

// ============================================================================

class SCD_Socket
{
private:
	wxSocketClient *telnet_sock;
SCD_SSH_Transport *ssh_sock;
	int m_type;	//telnet ? ssh ? unknown ?

	wxEvtHandler *evt_handler;
	int evt_id;
	void *m_clientdata;

	unsigned int local_buf_len;	//這是為了解決 wxSocketClient::Unread() 的問題而設

public:
	SCD_Socket();
	~SCD_Socket();

	int GetType();
	void SetType(int _t);
#ifndef BBMAN_NO_SSH
	SCD_wxSSH* GetSSH();
#endif

	bool Connect(wxIPV4address& address, bool wait = true, wxString _username = wxEmptyString);
#ifndef BBMAN_NO_SSH
	bool Login(wxString _username, wxString _password);	//for scd_wxssh
	bool isLogined();	//for scd_wxssh
#endif
	void Close();
	bool Destroy();

	bool IsDisconnected();
	bool IsConnected();

	int Read(void * buffer, wxUint32 nbytes);
	void Write(const void * buffer, wxUint32 nbytes);
	void Unread(const void * buffer, wxUint32 nbytes);
	wxUint32 LastCount();

	void SetWindowSize(int cols, int rows);
	void SetEventHandler(wxEvtHandler& handler, int id = -1);

	void SetClientData(void *data);
	void* GetClientData();
};

// ============================================================================
#endif

