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


#ifndef SCD_TELNET_CPP
#define SCD_TELNET_CPP
#include "scd_telnet.h"

#include "scd_terminal.h"
#include "main.h"
bool debugging = false;
// ============================================================================

BEGIN_EVENT_TABLE(SCD_Telnet, SCD_Telnet)
	EVT_SOCKET(0 , SCD_Telnet::OnSocketEvent)
END_EVENT_TABLE()

SCD_Telnet::SCD_Telnet(wxWindow *win) : SCD_Terminal(win)
{
//	local_buf_len = 0;
	connect_state = 0;
	bluserquery = 0;
	blMultibyteWordDecetionEnabled = true;
	blSupportNAWS = false;
	InitSocket();
}
// ----------------------------------------------------------------------------
SCD_Telnet::~SCD_Telnet()
{
//	close();

	//不可以直接 delete sock
 	//因為 sock.Destroy 可以防止 sock 在 destroy 之後又接收到 socket event
	DestroySocket();
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_Telnet::InitSocket()
{
/*
	sock.SetTimeout(0);
	sock.SetFlags(wxSOCKET_NOWAIT);
	sock.SetClientData(this);

	sock.SetNotify( wxSOCKET_INPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG );
	sock.SetEventHandler(*this, 0);
*/
	sock.SetClientData(this);
	sock.SetEventHandler(*this, 0);
}
// ----------------------------------------------------------------------------
bool SCD_Telnet::SocketConnect(wxIPV4address& address, bool wait, wxString _username)
{
//	sock.Notify(true);
	ResetTerminal();
	bluserquery = false;
	ssh_login_try_times = -1;
	return sock.Connect(address, wait, _username);
}
// ----------------------------------------------------------------------------
void SCD_Telnet::CloseSocket()
{
	sock.Close();
//	sock.Notify(false);

	//由於 sock.close() 並不會產生 SocketEvent, 所以要手動產生
	wxSocketEvent e(0);
//	e.SetEventObject( &sock );
	e.m_event = wxSOCKET_LOST;
	e.m_clientData = SocketGetClientData();
	OnSocketEvent(e);
}
// ----------------------------------------------------------------------------
inline void SCD_Telnet::DestroySocket()
{
	CloseSocket();
	//不可以直接 delete sock
 	//因為 sock.Destroy 可以防止 sock 在 destroy 之後又接收到 socket event
	sock.Destroy();
}
// ----------------------------------------------------------------------------
bool SCD_Telnet::SocketIsConnected()
{	return sock.IsConnected();	}
// ----------------------------------------------------------------------------
void* SCD_Telnet::SocketGetClientData()
{	return sock.GetClientData();	}
// ----------------------------------------------------------------------------
int SCD_Telnet::SocketRead(void *buf, int len)
{
/*
	int c = 0;	//填入了多少資料進入 buf 了 ?

	if( local_buf_len >= len )
	{
		sock.Read( buf, len );
		local_buf_len -= len;
		c = len;
	}
	else
 	{
 		sock.Read( buf, local_buf_len );
 		len -= local_buf_len;
 		sock.Read( (char*)buf + local_buf_len , len );
  		c = local_buf_len + sock.LastCount();
  		local_buf_len = 0;
	}

	return c;
*/
	return sock.Read(buf, len);
}
// ----------------------------------------------------------------------------
void SCD_Telnet::SocketUnread(void *buf, int len)
{
	sock.Unread( buf, len );
//	local_buf_len += len;
}
// ----------------------------------------------------------------------------
void SCD_Telnet::SocketWrite(void *buf, int len)
{
	sock.Write(buf, len);
	UnIdleServer();
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
int SCD_Telnet::getServerIdleTime()	//取得閒置的時間
{
	if( ! IsConnected() )	return -1;
	return wxGetLocalTime() - server_idle_time;
}
// ----------------------------------------------------------------------------
int SCD_Telnet::getUserIdleTime()	//取得閒置的時間
{
	if( ! IsConnected() )	return -1;
	return wxGetLocalTime() - user_idle_time;
}
// ----------------------------------------------------------------------------
void SCD_Telnet::UnIdleServer()	//解除閒置過久的狀態
{
	server_idle_time = wxGetLocalTime();	//紀錄開始閒置的時間
}
// ----------------------------------------------------------------------------
void SCD_Telnet::UnIdleUser()	//解除閒置過久的狀態
{
	user_idle_time = wxGetLocalTime();	//紀錄開始閒置的時間
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_Telnet::OnServerSupportNAWS()	//在得知 server 有支援長螢幕時
{	blSupportNAWS = true;	/*wxMessageBox( name );*/	OnResize();	}
// ----------------------------------------------------------------------------
void SCD_Telnet::OnLogined()	//在使用者剛登入時
{	if( ! blSupportNAWS )	setColumnRow(80,24);	}
// ----------------------------------------------------------------------------
bool SCD_Telnet::SetNAWS(int _c, int _r)	//改變 telnet 連線的行列數
{
//	if( ! blSupportNAWS )	return false;

	int col_count , row_count;
	getColumnRowCount( &col_count , &row_count );
	if( _c == col_count && _r == row_count )	return true;

	unsigned char buf[10];
	buf[0] = '\xff';	buf[1] = '\xfa';	buf[2] = '\x1f';
	buf[3] = '\x00';	buf[4] = (char)_c;	buf[5] = '\x00';
	buf[6] = (char)_r;	buf[7] = '\xff';	buf[8] = '\xf0';

	UserSend( (char*)buf, 9);

	setColumnRow( _c , _r );
	repaint(true);

	return true;
}
// ----------------------------------------------------------------------------
void SCD_Telnet::Show()
{
	OnResize();
	SCD_Terminal::Show();
}
// ----------------------------------------------------------------------------
void SCD_Telnet::OnResize()
{
	wxFont fnt = GetCurrentFont();

	if( blSupportNAWS && isEnableNaws() )
	{
		SetFont( fnt );

		wxSize cs = getCharSize();
		wxSize new_winsize = getParentWindow()->GetClientSize();
		int nc = new_winsize.GetWidth() / cs.GetWidth();
		int nr = new_winsize.GetHeight() / cs.GetHeight();
		if( nc < 80 )	nc = 80;
		if( nr < 24 )	nr = 24;

		SetNAWS(nc, nr);
	}
	else
	{
		fnt.SetPointSize( GetVarFontSize() );
		SetFont(fnt);

		SetNAWS(80, 24);
	}
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_Telnet::UserSend(char *buf, int len = -1)
{
	if( len < 0 )	len = strlen( (char*)buf);
	SocketWrite( buf, len );
}
// ----------------------------------------------------------------------------
void SCD_Telnet::UserSend(wxString buf)
{
    UserSend( wxStringToCharPtr(buf) );
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_Telnet::UserSend_spacial(wxString msg)	//送出格式化字串
{
	/*
		\n		enter
		\^R		ctrl+R
		\#{u,d,l,r}		上下左右
		\p1		pause
		\\		\
	*/

	wxString ret;
	ret.Empty();

	while(true)
	{
		ret += msg.BeforeFirst('\\');
		msg = msg.AfterFirst('\\');

		if( msg.IsEmpty() )  break;
		else //如果有控制字元
		{
	  		wxChar ch = msg.GetChar(0);
	  		switch(ch)
	  		{
         		case '#':
					{
                   		char b = (char) msg.GetChar(1);
						b = tolower(b);
                   		if( b == 'u' )
                   		{
							UserSend(ret);	ret.Empty();
							keyUp();
	    	           		msg = msg.Right( msg.Length() - 2 );
						}
                   		else if( b == 'd' )
                   		{
							UserSend(ret);	ret.Empty();
							keyDown();
	    	           		msg = msg.Right( msg.Length() - 2 );
						}
                   		else if( b == 'l' )
                   		{
							UserSend(ret);	ret.Empty();
							keyLeft();
	    	           		msg = msg.Right( msg.Length() - 2 );
						}
                   		else if( b == 'r' )
                   		{
							UserSend(ret);	ret.Empty();
							keyRight();
	    	           		msg = msg.Right( msg.Length() - 2 );
						}
						else
						{
          					ret += _T("\\");
						}
					}
               		break;
         		case 'n':
					UserSend(ret);	ret.Empty();
					keyEnter();
               		msg = msg.Right( msg.Length() - 1 );
               		break;
         		case '^':
					{
                   		char b = (char) msg.GetChar(1);
						b = tolower(b);
                   		if( b >= 'a' && b <= 'z' )
                   		{
							UserSend(ret);	ret.Empty();
							keyControl( b );
	    	           		msg = msg.Right( msg.Length() - 2 );
						}
						else
						{
		               		ret += _T("\\^");
        		       		msg = msg.Right( msg.Length() - 1 );
						}
					}
               		break;
         		case '\\':
               		ret += _T("\\");
               		msg = msg.Right( msg.Length() - 1 );
               		break;
         		case 'p':
               		{
                     	wxChar sleep_sec_ch = msg.GetChar(1);
                     	int sleep_sec = 0;
                     	if( sleep_sec_ch >= '0' && sleep_sec_ch <= '9' )
                     	{
                     		sleep_sec = sleep_sec_ch - '0';
		               		msg = msg.Right( msg.Length() - 2 );
						}
						else
						{
		               		msg = msg.Right( msg.Length() - 1 );
		               		ret += _T("\\p");
          					break;
						}

                     	UserSend(ret);	ret.Empty();

      					long et = wxGetLocalTime();
						while(true)
						{
          					if( wxGetLocalTime() - et >= sleep_sec )	break;
//							if( GetApp()->Pending() )	GetApp()->Dispatch();
//							GetApp()->Yield();
							AppProcessOtherEvents();
						}
					}
               		break;
			}
		}
	}

	UserSend(ret);
}
// ----------------------------------------------------------------------------
void SCD_Telnet::keyEnter()
{
    char ch = (char)13;
    UserSend( &ch, 1);
}
// ----------------------------------------------------------------------------
void SCD_Telnet::keyUp()
{	UserSend("\x1b\x5b\x41");	}
// ----------------------------------------------------------------------------
void SCD_Telnet::keyDown()
{	UserSend("\x1b\x5b\x42");	}
// ----------------------------------------------------------------------------
void SCD_Telnet::keyLeft()
{	UserSend("\x1b\x5b\x44");	}
// ----------------------------------------------------------------------------
void SCD_Telnet::keyRight()
{	UserSend("\x1b\x5b\x43");	}
// ----------------------------------------------------------------------------
void SCD_Telnet::keyPageUp()
{	UserSend("\x1b\x5b\x35\x7e");	}
// ----------------------------------------------------------------------------
void SCD_Telnet::keyPageDown()
{	UserSend("\x1b\x5b\x36\x7e");	}
// ----------------------------------------------------------------------------
void SCD_Telnet::keyHome()
{	UserSend("\x1b\x5b\x31\x7e");	}
// ----------------------------------------------------------------------------
void SCD_Telnet::keyEnd()
{	UserSend("\x1b\x5b\x34\x7e");	}
// ----------------------------------------------------------------------------
void SCD_Telnet::keyControl(char ch)
{
	ch = tolower(ch);
	if( ch >= 'a' && ch <= 'z' )
	{
	    ch = ch - 0x60;
    	UserSend( &ch , 1 );
	}
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_Telnet::Paste(char *txt, bool withANSI)
{
	txt = wxStringToCharPtr( GetLineWrapedString(txt) );
	char *w = txt;

	if( withANSI )	//如果要複製彩色 ANSI code
	{
		while( *w != '\0' )
		{
			if( *w == '\x1b' )	*w = '\x15';
			w++;
		}
	}
	else
	{
		char *r = txt;
		while( *r != '\0' )
		{
			if( *r == '\x1b' && r[1] == '[' )	//控制碼開頭
			{
				r++;
				while(1)
				{
					r++;
					if( isalpha(*r) )	{	r++;	break;	}
					else if( *r == '\0' )	break;
				}
			}
			else
			{
				*w = *r;
				w++;	r++;
			}
		}
		*w = '\0';
	}

	UserSend(txt);
}
// ----------------------------------------------------------------------------
void SCD_Telnet::PasteFromClipboard(bool withANSI)	//從剪貼簿上讀取字串, 並且傳送給 server
{
	if( ! IsConnected() )	return;

	wxString text = GetTextFromClipboard();
	if( text.IsEmpty() )	return;

	Paste( wxStringToCharPtr(text) , withANSI );
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_Telnet::StartUserQuery(char *prompt, bool _showchar)
{
//wxMessageBox("startuserquery " + wxString(prompt) );
	bluserquery = true;
	bluserquery_showchar = _showchar;
	userquery_len = 0;
	parse(prompt);
}
// ----------------------------------------------------------------------------
void SCD_Telnet::EndUserQuery()
{
	bluserquery = false;
	userquery_buf[ userquery_len ] = '\0';
	parse("\r\n", 2);
}
// ----------------------------------------------------------------------------
bool SCD_Telnet::IsUserQuerying()
{	return bluserquery;	}
// ----------------------------------------------------------------------------
void SCD_Telnet::UserQuery_OnChar(char ch)
{
	if( userquery_len < USERQUERY_BUF_LEN - 1 && isascii(ch) )
	{
		if(bluserquery_showchar)	parse( &ch , 1 );
		userquery_buf[ userquery_len ++ ] = ch;
	}
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_Telnet::OnChar(wxKeyEvent& event)
{
	char key = event.GetKeyCode();

	if(IsUserQuerying())
	{
		if( ! event.ShiftDown() ) key = tolower(key);
		UserQuery_OnChar(key);
		return;
	}

	UnIdleUser();

	if( event.AltDown() )	event.Skip();
	else
	{
//wxMessageBox( wxString::Format("OnChar : %%%02x -> %c", (unsigned int)key, (char)key) );
		UserSend( &key, 1 );
	}
}
// ----------------------------------------------------------------------------
void SCD_Telnet::OnKeyDown(wxKeyEvent& event)
{
	int key = event.GetKeyCode();

	if(key==WXK_F3 && ! event.ControlDown() ) debugging = ! debugging;	//除錯機制

	if( event.ControlDown() && (key < 'A' || key > 'Z') )
	{	/*event.Skip();*/	return;	}	//不能加上 event.Skip() 不然還是會被 OnChar 處理
	else if( event.AltDown() )
	{	event.Skip();	return;	}

	if(IsUserQuerying())
	{
		if( key == WXK_RETURN )
		{
			EndUserQuery();
#ifndef BBMAN_NO_SSH
			sock.Login( wxEmptyString , userquery_buf );
#endif
		}
		else if( key == WXK_BACK && userquery_len > 0 )
		{
			userquery_len --;
			if(bluserquery_showchar)	parse("\b \b", 3);
		}
		else if(isascii(key))	event.Skip();

		return;
	}

	if( ! IsConnected() ) return;	//如果沒有連線, 就不要傳資料
	UnIdleUser();

	char *output = NULL;
	char num_buf[] = "s";	//為 [鍵盤右邊的數字鍵] 保留緩衝區

	if( isascii(key) && ! iscntrl(key) )	//for 一般 ascii 文字
	{
#if defined(__WXGTK__)
		if( event.ControlDown() )
		{
//wxMessageBox( wxString::Format("OnChar : %%%02x -> %c", (unsigned int)key, (char)key) );
//			if( ! isalpha(key) )	return;
//			key = tolower(key) - 0x60;
			keyControl(key);
			return;
		}
		if( ! event.ShiftDown() )	key = tolower(key);
		UserSend( (char*)&key, 1 );
#else
		event.Skip();	//交給 OnChar event 處理
#endif
//wxMessageBox(wxString::Format("%d", key) );
	}
	else
	{
		if( key >= 326 && key <= 335 )	//如果是鍵盤右邊的數字鍵
		{
			num_buf[0] = (char)(key - 326) + '0';
			output = num_buf;
		}
		else
		{
			if( ! event.ControlDown() && ! event.AltDown() )
			switch(key)
			{
				case 372 :	//for NumPad [Enter] in Linux/FreeBSD
				case WXK_RETURN :	keyEnter();	break;
			    case WXK_DELETE :
					if( isMultibyteWordDecetionEnabled() && isCurrentAWord() )	output = "\x1b[3~\x1b[3~";
					else	output = "\x1b[3~";
					break;
				case WXK_BACK :	//back space
					if( isMultibyteWordDecetionEnabled() && isLeftAWord() )	output = "\x7f\x7f";
					else	output = "\x7f";
					break;
				case WXK_TAB :	output = "\t";	break;
				case WXK_HOME :
				case WXK_NUMPAD_HOME :	keyHome();	break;
				case WXK_END :
				case WXK_NUMPAD_END :	keyEnd();	break;
				case WXK_INSERT :	output = "\x1b\x40";	break;
				case WXK_NUMPAD_PRIOR :
				case WXK_NUMPAD_DECIMAL :	output = ".";	break;	//鍵盤右邊, 數字鍵附近的 [小數點] 鍵

				case WXK_UP :
				case WXK_NUMPAD_UP :		keyUp();		break;
				case WXK_DOWN :
				case WXK_NUMPAD_DOWN :		keyDown();		break;
				case WXK_LEFT :
				case WXK_NUMPAD_LEFT :
        			keyLeft();
					if( isMultibyteWordDecetionEnabled() && isLeftAWord() )	keyLeft();
        			break;
				case WXK_RIGHT :
				case WXK_NUMPAD_RIGHT :
        			keyRight();
					if( isMultibyteWordDecetionEnabled() && isCurrentAWord() )	keyRight();
        			break;

				case 312 :	keyPageUp();	break;	//page up
				case 313 :	keyPageDown();	break;	//page down
				case WXK_ESCAPE :	output = "\x1b";	break;

				default :
//wxMessageBox(wxString::Format("%d", key) );
					if( key < 0xff )	event.Skip();	//for 雙位元字
					break;
			}
		}
	}

	if( output != NULL )	UserSend( output );
}
// ----------------------------------------------------------------------------

void SCD_Telnet::OnSocketEvent( wxSocketEvent &event )
{
	if( event.GetSocketEvent() == wxSOCKET_INPUT )
	{
		unsigned char buf[30000];
//		int newline_count = 0;
		int scroll_count = 0;

		wxDC *dc = NULL;
		if( getVisible() )
		{
			dc = new wxClientDC( getParentWindow() );
			BeginDrawing(dc);
		}

//     	while(1)
		for(int kk=0;;kk++)
		{
			int buf_len = SocketRead(buf, 30000);
			if(buf_len == 0)	break;
if(debugging)
{
wxString tmp_str;
for(int i=0;i<buf_len;i++)
 tmp_str += wxString::Format("%02x(%c) ", buf[i] , isalnum(buf[i])?buf[i]:' ');
buf[buf_len] = '\0';
wxMessageBox(tmp_str + _T("\n\n") + wxString(buf) );
}

			unsigned char *e, *end;
			e = buf;
			end = buf + buf_len;
			while( e < end )	//先檢查看看 buf 裡面有沒有 IAC (0xff)
			{
				if( *e == 0xff )	//IAC ( need to negotiation )
   				{
					SocketUnread( e , end - e );
      				break;
				}
//				else if( *e == '\n' )	newline_count ++;
				e++;
			}

//			if( newline_count > 3 )	EnableDrawing(false);

			int unread_data_len;
			unread_data_len = parse( (char*)buf , e - buf , &scroll_count );
			if( unread_data_len > 0 )	//如果有資料尚未分析, 則 Unread
			{
				if( e < end && *e == 0xff );	//這行必須要加，不然遇到某些資料會進入無限迴圈 ( 例如資料 "\x1b[qqq\xff" )
				else SocketUnread( e - unread_data_len , unread_data_len );
			}

			if( scroll_count > 1 )
			{
				EnableDrawing(false);
			}
/*
wxString str = "";
for(unsigned char *r=buf;r<e;r++)	str += wxString::Format("%c", *r);
wxMessageBox(str);
*/

			if( e < end && *e == 0xff )	//如果 server 要求 negotiation
				negotiation();
			else
			{
				if( need_to_autologin	//自動登入
					&& sock.GetType() == SOCK_TELNET	//這是專為 telnet 設計的自動登入, ssh 不適用
					&& buf_len > 100 )	
				{
	    	  		need_to_autologin = false;
					if( ! username.IsEmpty() )
					{
						keyUp();	keyUp(); 	//兩次 [Up]
    	  				UserSend( (char*)( username.c_str()) ); keyEnter();
	      				UserSend( (char*)( password.c_str()) ); keyEnter();
					}
					if( ! message.IsEmpty() )	UserSend_spacial( message );

					OnLogined();
				}

//				break;
			}

		}

		if( getVisible() )	EndDrawing();
		if( scroll_count > 1 )	{	EnableDrawing(true);	repaint();	}
	}

	else if( event.GetSocketEvent() == wxSOCKET_CONNECTION
			|| event.GetSocketEvent() == wxSOCKET_LOST )
	{
		connect_state = ( SocketIsConnected() ? 2 : 0 );

		if( SocketIsConnected() && sock.GetType() == SOCK_SSH )	//如果是 ssh 連線
		{
			parse("done\r\nLogging in ...\r\n");
		}

//wxMessageBox( SocketIsConnected() ? "connected!!!" : "lost" );
	}

#ifndef BBMAN_NO_SSH
	else if( event.GetSocketEvent() == (wxSocketNotify)WXSSH_AUTH_FAIL )
	{
//		wxMessageBox("auth fail");

		if( sock.GetType() == SOCK_SSH )	//如果是 ssh 連線
		{
			if( ssh_login_try_times == -1 )
			{
				ssh_login_try_times = 0;
				if( ! site_info.password.IsEmpty() )
					sock.Login(wxEmptyString, site_info.password );
				else
					StartUserQuery("Password : ", false);
			}
			else
			{
				ssh_login_try_times ++;
				parse("Login Failed\r\n");
				StartUserQuery("Password : ", false);
			}
		}
		return;
	}
	else if( event.GetSocketEvent() == (wxSocketNotify)WXSSH_AUTH_SUCCESS )
	{
//		wxMessageBox("auth success");
		parse("Login Success\r\n");
		return;
	}
#endif
	else
	{
		wxMessageBox(_T("unknown socket event"));
		return;
 	}


	//也讓 parent window 處理這個 socket event
	event.m_clientData = this;	//讓 EvtHandler 可以透過 e.GetClientData() 取得 this
	getParentWindow()->ProcessEvent(event);
}

void SCD_Telnet::close()
{
	if( IsDisconnected() ) return;
	user_closed = true;
	CloseSocket();
}

bool SCD_Telnet::reconnect()
{
	if( ! IsDisconnected() ) 	return false;
	return	connect( sock.GetType(), getIP(), getPort(), getName() , username , password , message );
}
bool SCD_Telnet::connect(int _protocol, wxString _ip , int _port , wxString _name
	, wxString _user, wxString _passwd, wxString _message )
{
	SiteInfo _si;
	_si.ip = _ip;
	_si.port = _port;
	_si.name = _name;
	_si.username = _user;
	_si.password = _passwd;
	_si.message = _message;
	_si.protocol = _protocol;

	return connect(_si);
}

bool SCD_Telnet::connect(SiteInfo& _si)
{
	if( ! IsDisconnected() )	return false;
	UnIdleServer();
	UnIdleUser();

	//清除螢幕
	wxWindow *parent = getParentWindow();
	cleanScreen();
	if( parent && getVisible() )	repaint();
	setColumnRow( 80 , 24 );

	//設定自動登入
	site_info = _si;
 	username = _si.username;
 	password = _si.password;
	message = _si.message;
	need_to_autologin = true;
	blSupportNAWS = false;
	user_closed = false;

	//開始連線
	ip = _si.ip;
	port = _si.port;
	name = _si.name;

//	_si.protocol = SOCK_SSH;

#ifndef BBMAN_NO_SSH
	if( _si.protocol == SOCK_SSH )
	{
		sock.SetType( SOCK_SSH );
		parse("Connecting ... ");
	}
	else
		sock.SetType( SOCK_TELNET );
#else
	if( _si.protocol == SOCK_SSH )
	{
		parse("\x1b[1;33m");
		parse( wxStringToCharPtr(gettext("Sorry, this BBMan is a no-SSH version.\nIf you want to connect SSH server, Please download the full version of BBMan.")) );
		parse("\x1b[0m");
//		CloseSocket();
		return false;
	}
	else
		sock.SetType( SOCK_TELNET );
#endif


/*
	if( ! SocketConnect(addr, true) )
		wxMessageBox("無法連線");
	else
		start_time = wxGetLocalTime();
*/

#ifndef BBMAN_NO_SSH
	//如果是 ssh 連線必須先詢問登入帳號
	if( sock.GetType() == SOCK_SSH && _si.username.IsEmpty() )
	{
		wxString caption = gettext("Please Enter your account.");
		caption += _T("\n\nip : ") + ip;
		if( ! name.IsEmpty() )	caption += _T("\nname : ") + name;
		_si.username = wxGetTextFromUser( caption );
	}
#endif

	wxIPV4address addr;
	addr.Hostname(ip);
	addr.Service(port);
	connect_state = 1;
	SocketConnect(addr, false, _si.username);

	start_time = wxGetLocalTime();


	return true;
}


// **************************************************** //
// **************************************************** //
// *************** 實做 telnet protocol *************** //
// **************************************************** //
// **************************************************** //



bool SCD_Telnet::negotiation()
{

//定義在 telnet protocol 中會用到的指令

#define IAC 	255     // interpret as command:
#define DONT    254     // you are not to use option
#define DO  	253     // please, you use option
#define WONT    252     // I won't use option
#define WILL    251     // I will use option
#define SB  	250     // interpret as subnegotiation
#define SE  	240

#define TELOPT_ECHO 1   /* echo */
#define TELOPT_SGA  3   /* suppress go ahead */

#define TELOPT_TTYPE    24  // terminal type
#define TELQUAL_IS  	0   // option is...
#define TELQUAL_SEND    1   // send option
#define TELOPT_SGA  	3   // suppress go ahead
#define TELOPT_NAWS 	31	// window size

#define ANY 128

enum telnet_state
{	TSDATA, TSIAC, TSDO, TSDONT, TSWILL, TSWONT, TSSB_START, TSSBIAC, TSSB_END, SB_TERMTYPE	};

enum telnet_func
{	FUNC_NOOP, FUNC_PUTC, FUNC_DO, FUNC_WILL, FUNC_WONT, FUNC_DONT, FUNC_END, FUNC_SB_TERMTYPE , FUNC_WILL_NAWS };

//定義 state table
struct FSM_table
{
	telnet_state	now_state;		//目前狀態
	unsigned char	input;			//目前的 input 字元
	telnet_state	next_state;		//下一個狀態
	telnet_func		func;			//要執行的動作
};

static struct FSM_table neg_FSM[] =
{
	//	目前狀態	input	下一個狀態	要執行的動作

	{	TSDATA,		IAC,	TSIAC,		FUNC_NOOP	},	//start negotiation
	{	TSDATA,		ANY,	TSDATA,		FUNC_END	},	//stop  negotiation

	//DO, DONT, WILL, WONT, SB
	{	TSIAC,		DO,		TSDO,		FUNC_NOOP	},
	{	TSIAC,		DONT,	TSDONT,		FUNC_NOOP	},
	{	TSIAC,		WILL,	TSWILL,		FUNC_NOOP	},
	{	TSIAC,		WONT,	TSWONT,		FUNC_NOOP	},
	{	TSIAC,		SB,		TSSB_START,	FUNC_NOOP	},
	{	TSIAC,		IAC,	TSDATA,		FUNC_PUTC	},
	{	TSIAC,		ANY,	TSDATA,		FUNC_NOOP	},

	{	TSDO,		TELOPT_TTYPE,	TSDATA,		FUNC_WILL		},	//IAC WILL TERMTYPE
	{	TSDO,		TELOPT_NAWS,	TSDATA,		FUNC_WILL_NAWS		},	//開啟長螢幕機制
	{	TSDO,		ANY,	TSDATA,		FUNC_WONT		},
	{	TSDONT,		ANY,	TSDATA,		FUNC_NOOP		},

	{	TSWILL,  	TELOPT_SGA,		TSDATA,		FUNC_DO	},	//suppress go ahead
	{	TSWILL,  	TELOPT_ECHO,	TSDATA,		FUNC_DO	},	//echo
	{	TSWILL,  	ANY,	TSDATA,		FUNC_DONT	},
	{	TSWONT,		ANY,	TSDATA,		FUNC_NOOP	},

	//註冊 sub negotiation
	{	TSSB_START,	TELOPT_TTYPE,	SB_TERMTYPE,	FUNC_NOOP	},

	//sub negotiation common input
	{	TSSB_START,	IAC,	TSSBIAC,	FUNC_NOOP	},
	{	TSSB_START,	ANY,	TSSB_END,	FUNC_NOOP	},

	{	TSSB_END,	IAC,	TSSBIAC,	FUNC_NOOP	},
	{	TSSB_END,	ANY,	TSSB_END,	FUNC_NOOP	},

	{	TSSBIAC,	SE,		TSDATA,		FUNC_NOOP	},
	{	TSSBIAC,	ANY,	TSSB_END,	FUNC_NOOP	},

	//TERM_TYPE sub neg
	{	SB_TERMTYPE,	TELQUAL_SEND,	TSSB_END,	FUNC_SB_TERMTYPE	},
	{	SB_TERMTYPE,	ANY,			TSSB_END,	FUNC_NOOP	},

};



	//*** negotiation with server (telnet protocol) ***//




#ifdef DEBUG_NEGOTIATION

#include <stdio.h>
FILE *fp = NULL;

fp = fopen("log.txt", "w+");
fclose(fp);

fp = fopen("log.txt", "a+");
fprintf(fp, buf);
fclose(fp);

char map[256][30];
for(int i=0;i<256;i++)
	sprintf(map[i], "%02x", (unsigned char)i);
sprintf( map[IAC], "IAC" );
sprintf( map[DONT], "DONT" );
sprintf( map[DO], "DO  " );
sprintf( map[WONT], "WONT" );
sprintf( map[WILL], "WILL" );
sprintf( map[SB], "SB  " );
sprintf( map[SE], "SE  " );
sprintf( map[TELOPT_TTYPE], "TELOPT_TTYPE" );
sprintf( map[TELQUAL_IS], "TELQUAL_IS" );
sprintf( map[TELQUAL_SEND], "TELQUAL_SEND" );
sprintf( map[TELOPT_SGA], "TELOPT_SGA" );
sprintf( map[TELOPT_NAWS], "TELOPT_NAWS" );

char *state_name[] = {	"TSDATA", "TSIAC", "TSDO", "TSWILL", "TSSB_START", "TSSBIAC", "TSSB_END", "SB_TERMTYPE"	};
char *func_name[] = {	"FUNC_NOOP", "FUNC_PUTC", "FUNC_DO", "FUNC_WILL", "FUNC_WONT", "FUNC_DONT", "FUNC_END", "FUNC_SB_TERMTYPE"	};

wxLongLong  time_now = wxGetLocalTimeMillis(), time_diff;
fp = fopen("log.txt", "a+");
fclose(fp);

#endif

	//start negotiation

	enum telnet_state	now_state, next_state;
	enum telnet_func	func;
	unsigned char ch;

	now_state = TSDATA;


	while(1)
	{
		if( SocketRead( &ch, 1 ) != 1 )	//有可能是因為 non-blocking 而且目前沒有資料可讀取而造成的 error
    		return false;

		//從 neg_FSM 中尋找對應的資料
		for(int i=0;;i++)
		{
			if( neg_FSM[i].now_state == now_state )
				if( neg_FSM[i].input == ANY
    				|| neg_FSM[i].input == ch )
				{
     				next_state = neg_FSM[i].next_state;
     				func = neg_FSM[i].func;
					break;
				}
		}

#ifdef DEBUG_NEGOTIATION
fp = fopen("log.txt", "a+");

time_diff = wxGetLocalTimeMillis() - time_now;
fprintf(fp, "time=%-6s , ", time_diff.ToString().c_str() );
time_now += time_diff;

fprintf(fp, "now=%-13s , in=%-13s , next=%-13s , func=%-13s\n", state_name[now_state], map[ch], state_name[next_state], func_name[func] );
fclose(fp);
#endif
		now_state = next_state;

		//執行指定的動作

		if( func == FUNC_NOOP );
		else if( func == FUNC_WILL )
		{
			unsigned char tmp[3];
			tmp[0] = IAC;
			tmp[1] = WILL;
			tmp[2] = ch;
			SocketWrite(tmp, 3);
  		}
		else if( func == FUNC_DO )
		{
			unsigned char tmp[3];
			tmp[0] = IAC;
			tmp[1] = DO;
			tmp[2] = ch;
			SocketWrite(tmp, 3);
  		}
		else if( func == FUNC_WONT )
		{
			unsigned char tmp[3];
			tmp[0] = IAC;
			tmp[1] = WONT;
			tmp[2] = ch;
			SocketWrite(tmp, 3);
#ifdef DEBUG_NEGOTIATION
fp = fopen("log.txt", "a+");
fprintf(fp, "\nclient : ");
fprintf(fp, "%s ", map[ (unsigned char)tmp[0]]);
fprintf(fp, "%s ", map[ (unsigned char)tmp[1]]);
fprintf(fp, "%s\n\n", map[ (unsigned char)tmp[2]]);
fclose(fp);
#endif
		}
		else if( func == FUNC_DONT )
		{
			unsigned char tmp[3];
			tmp[0] = IAC;
			tmp[1] = DONT;
			tmp[2] = ch;
			SocketWrite(tmp, 3);
#ifdef DEBUG_NEGOTIATION
fp = fopen("log.txt", "a+");
fprintf(fp, "\nclient : ");
fprintf(fp, "%s ", map[ (unsigned char)tmp[0]]);
fprintf(fp, "%s ", map[ (unsigned char)tmp[1]]);
fprintf(fp, "%s\n\n", map[ (unsigned char)tmp[2]]);
fclose(fp);
#endif
		}
		else if( func == FUNC_SB_TERMTYPE )
		{
			unsigned char tmp[15];
			int i = 0;
			tmp[i++] = IAC;
			tmp[i++] = SB;
			tmp[i++] = TELOPT_TTYPE;
			tmp[i++] = TELQUAL_IS;
			tmp[i++] = 'v';
			tmp[i++] = 't';
			tmp[i++] = '1';
			tmp[i++] = '0';
			tmp[i++] = '0';
			tmp[i++] = IAC;
			tmp[i++] = SE;
			SocketWrite(tmp, i);
#ifdef DEBUG_NEGOTIATION
fp = fopen("log.txt", "a+");
fprintf(fp, "\nclient : ");
int t=0;
while(t<i)
	fprintf(fp, "%s ", map[ (unsigned char)tmp[t++]]);
fputs("\n", fp);
fclose(fp);
#endif
		}
		else if( func == FUNC_WILL_NAWS )
		{
			unsigned char tmp[3];
			tmp[0] = IAC;
			tmp[1] = WILL;
			tmp[2] = TELOPT_NAWS;
			SocketWrite(tmp, 3);
			OnServerSupportNAWS();
		}
		else if( func == FUNC_END )
  		{
  			SocketUnread( &ch , 1 );
    		break;
		}

	}
#ifdef DEBUG_NEGOTIATION
fp = fopen("log.txt", "a+");
fprintf(fp, "\ntelnet negotiation end\n");
fclose(fp);
#endif
	// ************************************************* //


#ifdef DEBUG_NEGOTIATION
while(1)
{
SocketRead( &ch , 1 );
fp = fopen("log.txt", "a+");
fprintf(fp, "%s ", map[ch]);
//fprintf(fp, "%c", ch);
fclose(fp);
}
#endif


	return true;
}

// ============================================================================

#endif
