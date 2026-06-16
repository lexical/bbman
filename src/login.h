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


#ifndef LOGIN_H
#define LOGIN_H

#include "common.h"

// ============================================================================

bool isAnonymousLogin();
wxString GetLoginPassword();
wxString GetLoginName();
wxString GetUserConfigPath(wxString _path);
void setLoginWhenStart(bool b);
bool getLoginWhenStart();
bool isUserExist(wxString _user);
void ShowNewAccountDialog();	//新增帳號
void ShowPasswdDialog();	//修改密碼
void ShowLoginDialog();	//登入


// ============================================================================
#endif
