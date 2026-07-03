/******************************************************************************
 * Name:        common.cpp
 * Purpose:     common utils such as handling sound, i18n, string conversion
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef COMMON_H
#define COMMON_H

//#define wxUSE_UNICODE 1
// ============================================================================

#include "wx/wxprec.h"

#ifdef __BORLANDC__
	#pragma hdrstop
#endif

#ifndef WX_PRECOMP
	#include "wx/wx.h"
#endif

//#define BBMAN_NO_SSH

#include <wx/config.h>
//#include <wx/fileconf.h>
#define BBMAN_VERSION _T(" 3.2.0")


#ifdef __UNIX__
	#include <locale.h>
	#include <libintl.h>
#else
	#include <wx/intl.h>
	#define gettext(str) _(str)
#endif

enum { SOCK_UNKNOWN, SOCK_TELNET , SOCK_SSH };

// ----------------------------------------------------------------------------

WX_DEFINE_ARRAY(void*, wxPointerArray);
typedef enum { LINK_NONE, LINK_HTTP, LINK_HTTPS, LINK_SFTP, LINK_FTP, LINK_EMAIL, LINK_TELNET } LINK_TYPE;

enum { BBMAN_ICON_CLOSED = 0, BBMAN_ICON_CONNECTING, BBMAN_ICON_CONNECTED, BBMAN_ICON_MESSAGE,
	BBMAN_ICON_DIR, BBMAN_ICON_SITE, BBMAN_ICON_FILE, BBMAN_ICON_EDIT, BBMAN_ICON_BOOKMARK,
	BBMAN_ICON_PREV, BBMAN_ICON_NEXT, BBMAN_ICON_REMOVE,
	BBMAN_ICON_COPY, BBMAN_ICON_PASTE, BBMAN_ICON_WEB, BBMAN_ICON_EMAIL,
	BBMAN_ICON_UPLOAD, BBMAN_ICON_DOWNLOAD, BBMAN_ICON_FULLSCREEN, BBMAN_ICON_LOCK,
	BBMAN_ICON_ICON_16,
	BBMAN_ICON_END };

// ============================================================================
typedef int (*SCD_ThreadFunc)(void*);
void ExecFuncUsingThread(SCD_ThreadFunc func, void *clientdata);
// ----------------------------------------------------------------------------
void init_Icons();
wxBitmap& GetProgramIcon(int icon_id);
void MakeBitmapMask(wxBitmap& bmp);
void AppendMenuItemWithBitmap(wxMenu *parent, int id, const wxString& item, wxBitmap& bmp);
// ----------------------------------------------------------------------------
wxString GetResourcePath();
wxString GetThemePath();
wxString GetLocalePath();
void ShowThemeSelector(wxWindow *parent = NULL);
// ----------------------------------------------------------------------------
wxString CharPtrTowxString(const char *str);
wxCharBuffer wxStringToBig5Buffer(const wxString &str);
// ----------------------------------------------------------------------------
wxConfigBase* GetConfig();
// ----------------------------------------------------------------------------
void CopyToClipboard(wxString text , bool comm_with_other = false);
wxString GetTextFromClipboard( bool comm_with_other = false );
// ----------------------------------------------------------------------------
void ShowAbout();
wxString GetStatusBarText();
// ----------------------------------------------------------------------------
void SCD_PlaySound();
void init_sound();
// ----------------------------------------------------------------------------
void SetApp(wxApp *app);
wxApp* GetApp();
void AppProcessOtherEvents();
// ----------------------------------------------------------------------------
void init_Naws();
void EnableNaws(bool b);
bool isEnableNaws();
// ----------------------------------------------------------------------------
void init_PreventIdle();
void EnablePreventIdle(bool b);
bool isPreventIdle();
// ----------------------------------------------------------------------------
void ShowTelnet();
void ShowAnsiEditor();
void CloseAnsiEditor(wxFrame *win);
// ----------------------------------------------------------------------------
void LockScreen();
// ----------------------------------------------------------------------------
void init_font();
wxFont GetCurrentFont();
int GetVarFontSize();
void SetVarFontSize(int s);
void UserSetFont(wxFrame *frm);
// ----------------------------------------------------------------------------
void setLinkProgram(LINK_TYPE _t , wxString program_path);
wxString getLinkProgram(LINK_TYPE _t);
void OnLinkClicked(char *link, LINK_TYPE _t);
void OpenHyperlink(char *link);
// ----------------------------------------------------------------------------
void Init_LineWrapedLength();
void SetLineWrapedLength(int len);
// ----------------------------------------------------------------------------

// ============================================================================
#endif
