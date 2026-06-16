/******************************************************************************
 * Name:        login.cpp
 * Purpose:     handling log in BBMan authentication, and customized settings
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef LOGIN_CPP
#define LOGIN_CPP
#include "login.h"

#include "des.h"
#include "bookmark.h"

// ============================================================================

static wxString& LoginID()
{
	static wxString value;
	return value;
}
static wxString& LoginPassword()
{
	static wxString value;
	return value;
}
#define Login_ID LoginID()
#define Login_Password LoginPassword()
#define Default_Password _T("eU6cZpQw")

bool isAnonymousLogin()
{	return Login_ID.IsEmpty();	}

wxString GetLoginPassword()
{
	if( isAnonymousLogin() )	return Default_Password;
	else return Login_Password;
}
inline wxString GetLoginName()
{	return Login_ID;	}

wxString GetUserConfigPath(wxString _path)
{
	if( isAnonymousLogin() )	return _path;
	else return _T("/usr/") + Login_ID + _path;
}

bool auth(wxString _user , wxString _pass)	//驗證使用者帳號密碼
{
	wxString real_pass;
	if( _user.IsEmpty() || _pass.IsEmpty() )	return false;
	if( ! GetConfig()->Read( _T("/usr/") + _user + _T("/password") , & real_pass ) )	return false;
	if( SCD_des_decrypt( _pass , real_pass ).CompareTo(Default_Password) != 0 )	return false;
	return true;
}

void passwd(wxString _user , wxString _pass)	//修改密碼
{
	GetConfig()->Write( _T("/usr/") + _user + _T("/password") , SCD_des_encrypt( _pass , Default_Password ) );
}

void setLoginWhenStart(bool b)
{	GetConfig()->Write( _T("/setting/login_when_start") , b );	}
bool getLoginWhenStart()
{
	bool b;
	GetConfig()->Read( _T("/setting/login_when_start") , & b , false );
	return b;
}

bool isUserExist(wxString _user)
{	return GetConfig()->Exists( _T("/usr/") + _user );	}

void ShowNewAccountDialog()	//新增帳號
{
	wxString user, pass, tmp_pass;
 	while(true)
	{
		user = wxGetTextFromUser( gettext("New account name(Esc to cancel)") );
		user.Trim();
		if( user.IsEmpty() )	return;
		if( isUserExist(user) )
		{
			wxMessageBox( gettext("Account name exists.") );
			continue;
		}
		pass = wxGetPasswordFromUser( gettext("Please enter password (at least 3 letters)") );
		pass.Trim();
		if( pass.IsEmpty() || pass.Len() < 3 )
		{	wxMessageBox( gettext("Password Incorrect. Password has to be longer then 3 letters.") );	continue;	}
		tmp_pass = wxGetPasswordFromUser( gettext("Please enter password again to confirm.") );
		tmp_pass.Trim();
		if( tmp_pass != pass )
		{	wxMessageBox( gettext("Password confirming failed. Difference between twice password entering.") );	continue;	}

 		//建立新帳號
		passwd( user, pass );
		wxMessageBox( gettext("Adding new account finished.") );
		//
		break;
	}
}

void ShowPasswdDialog()	//修改密碼
{
	if( isAnonymousLogin() )
	{	wxMessageBox( gettext("You are anonymous login now, can't modify pasword.") );	return;	}

	wxString old_pass, new_pass, tmp_pass;
	while(true)
	{
		old_pass = wxGetPasswordFromUser( gettext("Please enter old password (Esc cancel)") , gettext("Step 1") );
		old_pass.Trim();	if( old_pass.IsEmpty() ) 	return;
		if( old_pass != GetLoginPassword() )	wxMessageBox( gettext("Old password not correct.") );
		else break;
	}
 	while(true)
	{
		new_pass = wxGetPasswordFromUser( gettext("Please enter new password (at least 3 letters) (Esc cancel)") , gettext("Step 2") );
		if( new_pass.IsEmpty() )	return;
		new_pass.Trim();
		if( new_pass.Len() < 3 )  	{	wxMessageBox( gettext("Password needs at least 3 letters, please enter again.") );	continue;	}
		tmp_pass = wxGetPasswordFromUser( gettext("Please enter new password again to confirm.") ,	gettext("Step 3") );
		tmp_pass.Trim();
		if( new_pass != tmp_pass )  	{	wxMessageBox( gettext("Difference between twice new password entering, please enter again.") );	continue;	}
		else break;
	}

	//修改密碼
	passwd( GetLoginName() , new_pass );
	//修改所有 bookmark 中的密碼
	SiteInfo si;
 	GetConfig()->SetPath( GetUserConfigPath(_T("/bookmark/")) );
	wxString name, str;
	long id;
	bool ret;
	ret = GetConfig()->GetFirstEntry( name , id );
	while( ret )
	{
		if( GetConfig()->Read( name , &str ) )
		{
			Login_Password = old_pass;
			si.Set(str);
			Login_Password = new_pass;
			GetConfig()->Write( name , si.Get() );
		}
		ret = GetConfig()->GetNextEntry( name , id );
	}
	//
	Login_Password = new_pass;
	wxMessageBox( _T("Changing password finished.") );
}

void ShowLoginDialog()	//登入
{
	wxString user, pass;
	while(true)
	{
		user = wxGetTextFromUser( gettext("Please enter login name.\n\nPress Esc to log in anonymously.\n\n Enter 'new' to add new account.\n\nIf you don't want this window shows every time at startup, please change it in menu [Connect] -> [Login Option].") , gettext("Log in BBMan") );
  		user.Trim();
		if( user.IsEmpty() )	break;
		if( user == _T("new") )
		{	ShowNewAccountDialog();	continue;	}
		pass = wxGetPasswordFromUser( gettext("Please enter password.") );
		pass.Trim();
		if( auth(user, pass) )
		{
			Login_ID = user;
			Login_Password = pass;
			break;
		}
		else
			wxMessageBox( gettext("Login name or password incorrect, please enter again.") );
	}
}


// ============================================================================
#endif

