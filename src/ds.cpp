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


#ifndef DS_CPP
#define DS_CPP
#include "ds.h"
// ============================================================================

void SiteInfo::Init()
{
	name = ip = username = message = password = wxEmptyString;
	port = 23;
	autoopen = false;
	protocol = SOCK_TELNET;
}
// ----------------------------------------------------------------------------
void SiteInfo::Set(wxString str)
{
	name = wxEmptyString;
	ip = wxEmptyString;
	port = 23;
	autoopen = false;
	username = wxEmptyString;
	password = wxEmptyString;

	wxString tmp;

	name = str.BeforeFirst(':');	str = str.AfterFirst(':');
	ip = str.BeforeFirst(':');	str = str.AfterFirst(':');
	tmp = str.BeforeFirst(':');	str = str.AfterFirst(':');
	if( ! tmp.ToLong( (long*) & port ) )	port = 0;
	tmp = str.BeforeFirst(':');	str = str.AfterFirst(':');
	if(tmp == _T("y") )	autoopen = true;
	username = str.BeforeFirst(':');	str = str.AfterFirst(':');
	password = str.BeforeFirst(':');	str = str.AfterFirst(':');
	message = str.BeforeFirst(':');		str = str.AfterFirst(':');
	tmp = str.BeforeFirst(':');	str = str.AfterFirst(':');
	tmp.ToLong( (long*) & protocol );

	if( protocol != SOCK_SSH )	protocol = SOCK_TELNET;
	if( port == 0 )	port = ( protocol == SOCK_SSH ) ? 22 : 23;

	//將密碼用 DES 解密
	if( ! password.IsEmpty() )	password = SCD_des_decrypt( GetLoginPassword() , password );
	if( ! message.IsEmpty() ) message = SCD_des_decrypt( GetLoginPassword() , message );
}
// ----------------------------------------------------------------------------
wxString SiteInfo::Get()
{
	wxString tmp_pass = wxEmptyString;
	wxString tmp_message = wxEmptyString;

	//用 DES 加密
	if( ! password.IsEmpty() ) tmp_pass = SCD_des_encrypt( GetLoginPassword() , password );
	if( ! message.IsEmpty() ) tmp_message = SCD_des_encrypt( GetLoginPassword() , message );

	return name + _T(":") + ip + _T(":") + wxString::Format( _T("%d") , port )
		+ _T(":") + (autoopen? _T("y") : _T("n") ) + _T(":") + username
		+ _T(":") + tmp_pass + _T(":") + tmp_message
		+ _T(":") + wxString::Format("%d", protocol) + _T(":::::");
}

// ============================================================================
#endif
