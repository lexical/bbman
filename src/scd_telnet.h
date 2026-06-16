/******************************************************************************
 * Name:        scd_telnet.cpp
 * Purpose:     add SSH / telnet ability to scd_terminal.cpp
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef SCD_TELNET_H
#define SCD_TELNET_H

#include "common.h"
#include "scd_terminal.h"
#include "scd_socket.h"
#include "ds.h"
#include <wx/socket.h>
#include <wx/clipbrd.h>
#include <wx/timer.h>	//wxGetLocalTime()
#include <ctype.h>		//tolower()

// ============================================================================

//為了讓 SCD_Telnet 能接收 SocketEvent , 所以必須繼承 wxEvtHandler
class SCD_Telnet : public SCD_Terminal , protected wxEvtHandler
{
private:
	wxString ip, name;
	int port;
	SiteInfo site_info;

	wxString username , password , message;	//自動登入
	bool need_to_autologin;
	bool user_closed;	//使用者關閉連線？server 斷線？

	bool negotiation();

	//abstract
//	SCD_SSH sock;
	SCD_Socket sock;
	bool SocketConnect(wxIPV4address& address, bool wait = true, wxString _username = wxEmptyString);
 	int SocketRead(void *buf, int len);	//這是為了解決 Socket::Unread 的問題而設的
	inline void SocketUnread(void *buf, int len);	//這是為了解決 Socket::Unread 的問題而設的
	inline void SocketWrite(void *buf, int len);
	void InitSocket();
	void CloseSocket();
	void DestroySocket();
	bool SocketIsConnected();
	void *SocketGetClientData();
 	//

	//這些是為了 SSH 連線時讓使用者能自行輸入帳號密碼而設
	bool bluserquery;
	int userquery_len;
	bool bluserquery_showchar;
	void StartUserQuery(char *prompt, bool _showchar = true);
	void EndUserQuery();
	bool IsUserQuerying();
	void UserQuery_OnChar(char ch);
#define USERQUERY_BUF_LEN 100
	char userquery_buf[USERQUERY_BUF_LEN];
	int ssh_login_try_times;
	//

public:
	SCD_Telnet(wxWindow *win);
	~SCD_Telnet();

	wxString getIP()	{	return ip;		}
	int getPort()		{	return port;	}
	wxString getName()	{	return name;		}

	bool connect(int _protocol, wxString _ip , int _port , wxString _name
				, wxString _user = wxEmptyString , wxString _passwd = wxEmptyString , wxString _message = wxEmptyString );
	bool connect(SiteInfo& si);
	bool reconnect();
	void close();
	inline bool isUserClosed()	{	return user_closed;	}

	void UserSend(char *buf, int len);
	void UserSend(wxString buf);
	void UserSend_spacial(wxString msg);	//送出格式化字串

	void keyEnter();
	void keyUp();
	void keyDown();
	void keyLeft();
	void keyRight();
	void keyPageUp();
	void keyPageDown();
	void keyHome();
	void keyEnd();
	void keyControl(char ch);

	void Paste(char *txt, bool withANSI);
 	void PasteFromClipboard(bool withANSI);	//從剪貼簿上讀取字串, 並且傳送給 server

	SCD_Socket* GetSocket()	{	return &sock;	}




	bool blSupportNAWS;	//該 server 是否有支援長螢幕
	void OnServerSupportNAWS();	//在得知 server 有支援長螢幕時
	void OnLogined();	//在使用者剛登入時
	bool SetNAWS(int _c, int _r);	//改變 telnet 連線的行列數
	void Show();
	void OnResize();


private:
	unsigned char connect_state;	//0(close) 1(connecting) 2(connected)
public:
	bool IsDisconnected()	{	return connect_state == 0;	}
	bool IsConnecting()		{	return connect_state == 1;	}
	bool IsConnected()		{	return connect_state == 2;	}



	//**************** 記錄連線時間 ****************//

private:	long start_time;
public:
	int getConnectTime()	{	return wxGetLocalTime() - start_time;	}

	//**************** 防呆 ****************//

private:
	long server_idle_time;
	long user_idle_time;	//使用者最後 OnKeyEvent 的時間
	void UnIdleServer();	//解除閒置過久的狀態
	void UnIdleUser();		//解除閒置過久的狀態
public:
	int getServerIdleTime();	//取得閒置的時間
	int getUserIdleTime();	//取得閒置的時間


	//**************** 事件 ****************//

	void OnSocketEvent( wxSocketEvent &e );

	void OnKeyDown(wxKeyEvent& event);
	void OnChar(wxKeyEvent& event);

private:
	bool blMultibyteWordDecetionEnabled;
public:
	inline void EnableMultibyteWordDecetion(bool b)
	{	blMultibyteWordDecetionEnabled = b;	}
	inline bool isMultibyteWordDecetionEnabled()
	{	return blMultibyteWordDecetionEnabled;	}

private:
    DECLARE_EVENT_TABLE()
};

// ============================================================================
#endif
