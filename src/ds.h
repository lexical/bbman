/******************************************************************************
 * Name:        ds.cpp
 * Purpose:     Site info (ip, port, username, password, etc...)
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef DS_H
#define DS_H

#include "common.h"
#include "des.h"
#include "login.h"

// ============================================================================

typedef struct SiteInfo
{

	// "name:ip:port:autoopen:username:password:message:protocol
public:
	wxString name, ip;
	int port;
	bool autoopen;	// auto-open when program starts
	wxString username;	//自動登入
	wxString message;	//連線後 (或自動登入後) 自動送出的 BBMan 格式化訊息
	wxString password;
	int protocol;	//telnet? ssh?
	wxString connection_username;	// URL/profile transport user, not persisted

public:

	void Init();
	void Set(wxString str);
	wxString Get();
	wxString GetPassword()
	{	return password;	}
	void SetPassword(wxString pass)
	{	password = pass;	}

	bool isAutoOpen()	{	return autoopen;	}
	bool isAutoLogin()	{	return ! username.IsEmpty();	}

};


// ============================================================================
#endif
