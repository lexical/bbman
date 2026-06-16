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


#ifndef SCD_WXSSH_H
#define SCD_WXSSH_H
#include "../common.h"
//#include "scd_wxsftp.h"

#include "libssh/libssh.h"
#include <wx/socket.h>
#include <wx/thread.h>

#define WXSSH_AUTH_SUCCESS 666	//SSH 詢問使用者密碼事件
#define WXSSH_AUTH_FAIL 667		//SSH 詢問使用者密碼事件
#define SCD_wxSSH_UNREAD_BUF_SIZE 1000

// ============================================================================

class SCD_wxSSH
{
public:
	SSH_SESSION *session;

public:
	wxString ip;
public:
	int port;
	wxString username, password;

	//請勿存取這些變數
	CHANNEL *channel;
	bool not_read_after_notify;
	wxEvtHandler *evt_handler;
	int evt_id;
	void SendEvent(wxSocketNotify v);

public:
	wxThread *working_thread;
private:
	friend void MonitorChannel( SCD_wxSSH *ssh );
	friend void UnMonitorChannel( SCD_wxSSH *ssh );
	friend int AutoSelectChannel();
	friend void ThreadFunc_Connect(SCD_wxSSH *ssh, wxIPV4address& address);
	friend void ThreadFunc_Login(SCD_wxSSH *ssh, wxString username, wxString password);
	//

	BUFFER *readbuf;

	char unread_buf[ SCD_wxSSH_UNREAD_BUF_SIZE ];
	unsigned int unread_len;

	void *m_clientdata;

	unsigned char connect_state;	//0(close) 1(connecting) 2(connected)
	bool blLogined;
	int intLastCount;

 	bool init_ssh_channel();
	void free_ssh_session();

public:
	SCD_wxSSH(wxSocketFlags /*flags*/);
	~SCD_wxSSH();

	//because the ssh must be closed after all of its sftp are closed,
	//but it may be a trouble for users if you cancel all the sftp immediately
	//when closing ssh, so if you want to delete ssh, try to call Destroy() instead.
	//*** DON'T delete SCD_wxSSH DIRECTLY ***
private:
	bool m_destroyed;
public:
	bool Destroy();

	//
	bool Connect(wxIPV4address& address, wxString _username);
	bool Login(wxString _username, wxString _password);
	bool isLogined();
	void Close();

	bool IsDisconnected();
	bool IsConnected();

	void Read(void * buffer, wxUint32 nbytes);
	void Write(const void * buffer, wxUint32 nbytes);
	void Unread(const void * buffer, wxUint32 nbytes);
	wxUint32 LastCount();

	void SetEventHandler(wxEvtHandler& handler, int id = -1);

	void SetClientData(void *data);
	void* GetClientData();

	//-------------------------------------------------
public:
	int sftp_count;
//	SCD_wxSFTP* CreateSFTP();
	void DeleteSFTP(void *_ftp);

};

// ============================================================================
#endif
