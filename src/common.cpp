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


#ifndef COMMON_CPP
#define COMMON_CPP
#include "common.h"

#include "login.h"

#include <wx/config.h>
static wxConfig *cfg = NULL;

// ============================================================================

#include <wx/thread.h>
class SCD_Thread : public wxThread
{
private:
	SCD_ThreadFunc m_thread_func;
	void *m_clientdata;

public:

	SCD_Thread() : wxThread()
	{
	}
	~SCD_Thread()
	{
	}

	static SCD_Thread* ExecFuncUsingThread(SCD_ThreadFunc func, void *clientdata)
	{
		SCD_Thread *t = new SCD_Thread();
		t->m_thread_func = func;
		t->m_clientdata = clientdata;
		t->Create();
		t->Run();
		return t;
	}

	ExitCode Entry()
	{
		m_thread_func(m_clientdata);
		return 0;
	}
};

void ExecFuncUsingThread(SCD_ThreadFunc func, void *clientdata)
{
	SCD_Thread::ExecFuncUsingThread(func, clientdata);
}

// ============================================================================

wxConfigBase* GetConfig()
{
	if( cfg == NULL ) cfg = new wxConfig(_T("BBMan"));
	return cfg;
}
// ----------------------------------------------------------------------------
wxString CharPtrTowxString(const char *str)
{
	static wxMBConvUTF8 mb;
	return wxString( mb.cMB2WC(str) );
}
// ----------------------------------------------------------------------------
char* wxStringToCharPtr(const wxString &str)
{
	static wxMBConvUTF8 mb;
	static wxCharBuffer buf;
	buf = str.mb_str(mb);
	return const_cast<char*>(buf.data());
}
// ----------------------------------------------------------------------------
wxCharBuffer wxStringToBig5Buffer(const wxString &str)
{
	static wxCSConv big5_conv("BIG5");
	return str.mb_str(big5_conv);
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#include <wx/imaglist.h>
wxBitmap icon_list[30];
// ----------------------------------------------------------------------------
void MakeBitmapMask(wxBitmap& bmp)
{	bmp.SetMask( new wxMask(bmp, *wxWHITE) );	}
// ----------------------------------------------------------------------------
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/dir.h>
void init_Icons()
{
	int i=0, k;
	int height;
	wxString theme_path = GetThemePath();

	//讀取連線標籤圖示
	if( wxFile::Exists( theme_path + _T("leds.bmp") ) )
	{
		wxBitmap tbimg( theme_path + _T("leds.bmp") , wxBITMAP_TYPE_BMP );
		height = tbimg.GetHeight();
		for(k=0;k<4;k++)
			MakeBitmapMask( icon_list[i++] = tbimg.GetSubBitmap( wxRect( k*height, 0, height, height ) ) );
	}


	//讀取一般圖示
	if( wxFile::Exists( theme_path + _T("icons.bmp") ) )
	{
		wxBitmap icon_img( theme_path + _T("icons.bmp") , wxBITMAP_TYPE_BMP );
		height = icon_img.GetHeight();
		for(k=0;k<16;k++)
			MakeBitmapMask( icon_list[i++] = icon_img.GetSubBitmap( wxRect( k*height, 0, height, height ) ) );
	}

	#include "icon/bbman_16.xpm"
	wxIcon bbman_icon( bbman_16_xpm );
	icon_list[i++] = bbman_icon;
}
// ----------------------------------------------------------------------------
wxBitmap& GetProgramIcon(int icon_id)
{
	static wxBitmap fallback_icon;
	if( ! fallback_icon.IsOk() )
	{
		fallback_icon.Create(16, 16);
		wxMemoryDC dc(fallback_icon);
		dc.SetBackground(*wxBLACK_BRUSH);
		dc.Clear();
		dc.SelectObject(wxNullBitmap);
	}

	if(icon_id >= BBMAN_ICON_END || ! icon_list[icon_id].IsOk())	return fallback_icon;
	return icon_list[icon_id];
}
// ----------------------------------------------------------------------------
void AppendMenuItemWithBitmap(wxMenu *parent, int id, const wxString& item, wxBitmap& bmp)
{
	wxMenuItem *mi;
	mi = new wxMenuItem(parent, id, item);
	mi->SetBitmap(bmp);
	parent->Append(mi);
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static wxString FindResourcePath(const wxString& baseDir)
{
	if( baseDir.IsEmpty() )
		return wxEmptyString;

	wxFileName base(baseDir, wxEmptyString);
	base.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
	wxString root = base.GetPathWithSep();

	if( wxFile::Exists(root + _T("theme/default/icons.bmp")) ||
		wxFileName::DirExists(root + _T("mo")) )
		return root;

	return wxEmptyString;
}
// ----------------------------------------------------------------------------
static wxString FindResourcePathFromParents(const wxString& baseDir)
{
	wxFileName dir(baseDir, wxEmptyString);
	dir.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);

	while( true )
	{
		wxString path = FindResourcePath(dir.GetPath());
		if( ! path.IsEmpty() )
			return path;

		if( dir.GetDirCount() == 0 )
			break;

		dir.RemoveLastDir();
	}

	return wxEmptyString;
}
// ----------------------------------------------------------------------------
wxString GetResourcePath()
{
	wxString resourceDir;
	if( wxGetEnv(_T("BBMAN_RESOURCE_DIR"), &resourceDir) )
	{
		wxString path = FindResourcePath(resourceDir);
		if( ! path.IsEmpty() )
			return path;
	}

	wxString path = FindResourcePathFromParents(
		wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath());
	if( ! path.IsEmpty() )
		return path;

	path = FindResourcePath(wxStandardPaths::Get().GetResourcesDir());
	if( ! path.IsEmpty() )
		return path;

	path = FindResourcePath(wxStandardPaths::Get().GetDataDir());
	if( ! path.IsEmpty() )
		return path;

	path = FindResourcePathFromParents(wxGetCwd());
	if( ! path.IsEmpty() )
		return path;

	return wxEmptyString;
}
// ----------------------------------------------------------------------------
static wxString FindThemePath(const wxString& resourceRoot, const wxString& theme)
{
	if( resourceRoot.IsEmpty() )
		return wxEmptyString;

	wxString path = resourceRoot + _T("theme/") + theme + _T("/");
	if( wxFile::Exists(path + _T("icons.bmp")) )
		return path;

	path = resourceRoot + _T("theme/default/");
	if( wxFile::Exists(path + _T("icons.bmp")) )
		return path;

	return wxEmptyString;
}
// ----------------------------------------------------------------------------
wxString GetThemePath()
{
	wxString theme;
	GetConfig()->Read( GetUserConfigPath(_T("/setting/theme")) , & theme , _T("default") );

	wxString path = FindThemePath(GetResourcePath(), theme);
	if( ! path.IsEmpty() )
		return path;

	return _T("theme/default/");
}
// ----------------------------------------------------------------------------
static bool IsLocaleRoot(const wxString& root)
{
	if( root.IsEmpty() || ! wxFileName::DirExists(root) )
		return false;

	wxDir dir(root);
	wxString lang;
	bool ok = dir.GetFirst(&lang, wxEmptyString, wxDIR_DIRS);
	while( ok )
	{
		if( wxFile::Exists(root + lang + _T("/LC_MESSAGES/bbman.mo")) )
			return true;
		ok = dir.GetNext(&lang);
	}

	return false;
}
// ----------------------------------------------------------------------------
static wxString FindLocalePath(const wxString& baseDir)
{
	if( baseDir.IsEmpty() )
		return wxEmptyString;

	wxFileName base(baseDir, wxEmptyString);
	base.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
	wxString root = base.GetPathWithSep();

	if( IsLocaleRoot(root) )
		return root;

	return wxEmptyString;
}
// ----------------------------------------------------------------------------
wxString GetLocalePath()
{
	wxString localeDir;
	if( wxGetEnv(_T("BBMAN_LOCALE_DIR"), &localeDir) )
	{
		wxString path = FindLocalePath(localeDir);
		if( ! path.IsEmpty() )
			return path;
	}

	wxString root = GetResourcePath();
	if( ! root.IsEmpty() )
	{
		wxString path = FindLocalePath(root + _T("mo"));
		if( ! path.IsEmpty() )
			return path;
	}

	wxFileName dataDir(wxStandardPaths::Get().GetDataDir(), wxEmptyString);
	dataDir.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
	if( dataDir.GetDirCount() > 0 )
	{
		dataDir.RemoveLastDir();
		wxString path = FindLocalePath(dataDir.GetPathWithSep() + _T("locale"));
		if( ! path.IsEmpty() )
			return path;
	}

	wxString path = FindLocalePath(_T("/usr/local/share/locale"));
	if( ! path.IsEmpty() )
		return path;

	path = FindLocalePath(_T("/usr/share/locale"));
	if( ! path.IsEmpty() )
		return path;

	return _T("./mo/");
}
// ----------------------------------------------------------------------------
#include <wx/choicdlg.h>
void ShowThemeSelector(wxWindow *parent)
{
	wxDir dir( GetResourcePath() + _T("theme") );
	wxString dirname;
	wxArrayString theme_list;
	int now_theme_id = -1;
	int c;

	wxString orig_theme;
	GetConfig()->Read( GetUserConfigPath(_T("/setting/theme")) , & orig_theme , _T("default") );

	//find all the sub dirs under "theme" directory
	bool b;
	b = dir.GetFirst( &dirname, wxEmptyString, wxDIR_DIRS );
	c = 0;
	while(b)
	{
		theme_list.Add( dirname );
		if(dirname == orig_theme)	now_theme_id = c;
		c++;
		b = dir.GetNext( &dirname );
	}

	int id = wxGetSingleChoiceIndex( wxString(gettext("Current theme : ")) + orig_theme ,
				wxString(gettext("Select a theme")), theme_list, parent );
	if(id==-1)	return;	//user cancel select a theme

	if( orig_theme == theme_list[id] )	return;	//user choose the original theme

	GetConfig()->Write( GetUserConfigPath(_T("/setting/theme")) , theme_list[id] );
	wxMessageBox( gettext("This setting will enabled at the next startup.") );
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#include <wx/clipbrd.h>
#include "scd_gtk_textctrl.h"	//ShowCopyDialog()
void CopyToClipboard(wxString text , bool comm_with_other)
{
	if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData( new wxTextDataObject(text) );
        wxTheClipboard->Close();
    }

/*
    if (wxTheClipboard->Open())
    {
//      wxTheClipboard->SetData( new wxTextDataObject( CharPtrTowxString(Big5ToUnicode(wxStringToCharPtr(text))) ) );
//      wxTheClipboard->SetData( new wxTextDataObject( text ) );
        wxTheClipboard->SetData( new wxTextDataObject(text) );
        wxTheClipboard->Close();
    }
    wxTheClipboard->Clear();
    static wxTextCtrl textctrl;
    wxString txt = GetSelectionContent(withANSI);
wxMessageBox(txt);
    textctrl.SetValue( txt );
    textctrl.SetSelection( 0, -1 );
    textctrl.Copy();
    textctrl.SetValue( wxEmptyString );
*/

#ifdef __WXGTK__
	/*if(comm_with_other)*/	ShowCopyDialog( text );
#endif

}
// ----------------------------------------------------------------------------
wxString GetTextFromClipboard(bool comm_with_other)
{
	wxString text;

    if (wxTheClipboard->Open())
    {
		if( wxTheClipboard->IsSupported( wxDF_TEXT ) )
        {
            wxTextDataObject data;
            wxTheClipboard->GetData( data );
            text = data.GetText();
        }
        wxTheClipboard->Close();
    }


//	text = UTF8_To_Big5( wxStringToCharPtr(text) );

#ifdef __WXGTK__
	/*if(comm_with_other && text.IsEmpty() )*/	//text = ShowPasteDialog();
#endif

//	text = UTF8_To_Big5( wxStringToCharPtr(text) );

	return text;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void OnLinkClicked( char *link, LINK_TYPE _t);

#include <wx/strconv.h>

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

// the application icon (under Windows and OS/2 it is in resources)
/*
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMAC__) || defined(__WXMGL__) || defined(__WXX11__)
	#include "mondrian.xpm"
#endif
*/
bool blAA = true;
wxFont global_default_fnt;
void ShowAbout()
{
	wxMessageBox( wxString( gettext("Welcome to BBMan")) + BBMAN_VERSION
		+ wxString( gettext(".\n\nBBMan is designed by Xie, Chun-Da, Dept. of Computer Science, National Chengchi University, Taiwan.\n\nBBMan web page: http://bbman.sf.net/\nMy email box: jakky1@pchome.com.tw\n\nThanks to :\nhttp://www.foood.net/ --- default toolbar Icon set designer\nchivalry@nccu.edu.tw --- BBMan Program Icon.") )
		, wxEmptyString, wxOK | wxICON_INFORMATION );

//	blAA = ! blAA;
//	global_default_fnt.SetNoAntiAliasing( !blAA );
}

wxString GetStatusBarText()
{
	return wxString(_T("BBMan ")) + BBMAN_VERSION
		+ wxString(gettext(". Designer : Xie, Chun-Da, Dept. of Computer Science, NCCU, Taiwan"));
}


#ifndef __WXGTK__
#include <wx/wave.h>	//wxWave 播放音樂
wxWave wav;
#endif
bool use_bell;
void SCD_PlaySound()
{
	if( use_bell )	wxBell();
	else
	{
#ifndef __WXGTK__
		wav.Play();
#else
		wxString player, file;
		if( GetConfig()->Read( GetUserConfigPath(_T("/setting/sound_file")) , & file ) && GetConfig()->Read( GetUserConfigPath(_T("/setting/sound_player")) , & player ) )
			wxExecute( player + _T(" ") + file );
#endif
	}
}

void init_sound()
{
	wxString sound_file;
	use_bell = false;
	GetConfig()->Read( GetUserConfigPath(_T("/setting/sound_file")) , & sound_file , wxEmptyString );
	if( sound_file.IsEmpty() )	use_bell = true;
#ifndef __WXGTK__
	else
	{
		if( wav.Create( sound_file ) )	use_bell = false;
		else
		{
			use_bell = true;
			wxMessageBox( gettext("Can't load sound file") );
		}
	}
#endif
}





//******************** 把目前正在開啟的視窗都記錄下來 ******************//
static bool blEnableNaws = false;
void init_Naws()
{
	blEnableNaws = GetConfig()->Exists( GetUserConfigPath(_T("/setting/enable_naws")) );
}
void EnableNaws(bool b)
{
	if( b == blEnableNaws )	return;
	blEnableNaws = b;
	if( b )	GetConfig()->Write( GetUserConfigPath(_T("/setting/enable_naws")) , true );
	else	GetConfig()->DeleteEntry( GetUserConfigPath(_T("/setting/enable_naws")) );
}
bool isEnableNaws()	{	return blEnableNaws;	}

static bool blPreventIdle = true;
void init_PreventIdle()
{
	GetConfig()->Read( GetUserConfigPath(_T("/setting/prevent_idle")) , & blPreventIdle , true );
}
void EnablePreventIdle(bool b)
{
	if( b == blPreventIdle )	return;
	blPreventIdle = b;
	GetConfig()->Write( GetUserConfigPath(_T("/setting/prevent_idle")) , b );
}
bool isPreventIdle()	{	return blPreventIdle;	}


#include "frm_telnet.h"
#include "frm_editterm.h"

BBS_Frame *telnet_frame;
wxPointerArray edit_win_list;

#if wxCHECK_VERSION(2, 9, 0)
static const int BBMAN_DEFAULT_WIN_WIDTH = 1205;
static const int BBMAN_DEFAULT_WIN_HEIGHT = 925;
#else
static const int BBMAN_DEFAULT_WIN_WIDTH = 750;
static const int BBMAN_DEFAULT_WIN_HEIGHT = 550;
#endif


void ShowTelnet()
{
	telnet_frame = new BBS_Frame(_T("BBMan"),
	                             wxPoint(50, 50),
	                             wxSize(BBMAN_DEFAULT_WIN_WIDTH, BBMAN_DEFAULT_WIN_HEIGHT));

	//還原成上次的視窗大小
	int w, h;
	if( GetConfig()->Read( GetUserConfigPath(_T("/setting/win_geo/width")), & w ) && GetConfig()->Read( GetUserConfigPath(_T("/setting/win_geo/height")), & h ) )
		telnet_frame->SetSize( w, h );
	if( GetConfig()->Read( GetUserConfigPath(_T("/setting/win_geo/left")), & w ) && GetConfig()->Read( GetUserConfigPath(_T("/setting/win_geo/top")), & h ) )
	{
//		wxSize ds = wxGetDisplaySize();
		if(w < 0)	w = 0;
//		else if(w + 300 > ds.GetWidth() )		w = ds.GetWidth() - 300;
		if(h < 0)	h = 0;
//		else if(h + 300 > ds.GetHeight() )	h = ds.GetHeight() - 300;
		telnet_frame->Move( w, h );
	}

	//開啟有設定自動開啟的 BBS
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
			si.Set(str);
			if( si.autoopen )	telnet_frame->connect(si);
		}
		ret = GetConfig()->GetNextEntry( name , id );
	}
	telnet_frame->Show(TRUE);
	telnet_frame->ShowSpecifiedTerminal(0);
	//

	GetConfig()->Read( GetUserConfigPath(_T("/setting/win_geo/fullscreen")) , & w , 0 );
	if(w==1)	telnet_frame->FullScreen();
	//
}

void ShowAnsiEditor()
{
	frame_EditTerm *editframe = new frame_EditTerm( gettext("BBMan - ANSI Editor"),
			wxPoint(50, 50), wxSize(750, 550) );
	editframe->Show(TRUE);

	edit_win_list.Add( editframe );
}
void CloseAnsiEditor(wxFrame *win)
{
	edit_win_list.Remove(win);
}
//**********************************************************************//

void LockScreen()
{
	wxString pass, line;
	if( isAnonymousLogin() )
	{
		while(true)
		{
			pass = wxGetPasswordFromUser( gettext("Please input a password. Later you will need this password to restore BBMan window.\n(Password need to be at least 3 letters)"), gettext("Hide BBMan") );
			if( pass.IsEmpty() )	{	wxMessageBox( gettext("Cancel Hiding BBMan") );	return;	}
			if( pass.Length() < 3 ) {	wxMessageBox( gettext("Password needs to be at last 3 letters please.") );	continue;	}
			line = wxGetPasswordFromUser( gettext("Please enter password again to make sure you remember what password you just typed."), gettext("Hide BBMan") );
			if( pass != line ) wxMessageBox( gettext("Password confirming failed, please input again.") );
			else	break;
		}
	}
	else pass = GetLoginPassword();

	int c = edit_win_list.GetCount();
	telnet_frame->Show(false);
	for(int i=0;i<c;i++)	((wxFrame*)(edit_win_list.Item(i)))->Show(false);

	while(true)
	{
		line = wxGetPasswordFromUser( gettext("Please enter password to restore BBMan window."), _T("Restore BBMan window") );
		if( pass == line ) 	break;
	}

	telnet_frame->Show(true);
	for(int i=0;i<c;i++)	((wxFrame*)(edit_win_list.Item(i)))->Show(true);
}


#include <wx/settings.h>
#include <wx/fontutil.h>

int var_fnt_size;

void init_font()
{
	global_default_fnt = GetCurrentFont();
	var_fnt_size = global_default_fnt.GetPointSize();
}
wxFont GetCurrentFont()
{
	static bool isInited = false;
	if( ! isInited )
	{
		isInited = true;

		global_default_fnt = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
#if wxCHECK_VERSION(2, 9, 0)
		global_default_fnt = wxFont(12, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
		                            wxFONTWEIGHT_NORMAL, false,
		                            _T("AR PL UMing TW"));
#endif

		wxString fnt_name;

		if( GetConfig()->Read( GetUserConfigPath(_T("/setting/font")), & fnt_name ) )
		{
			wxNativeFontInfo fi;
			if( fi.FromString( fnt_name ) )	global_default_fnt.SetNativeFontInfo(fi);
		}
#if !wxCHECK_VERSION(2, 9, 0)
		global_default_fnt.SetNoAntiAliasing( !blAA );
#endif
//		global_default_fnt.SetDefaultEncoding(wxFONTENCODING_CP950);
	}
	return global_default_fnt;
}

int GetVarFontSize()
{	return var_fnt_size;	}
void SetVarFontSize(int s)
{	var_fnt_size = s;	}

void UserSetFont(wxFrame *frm)
{
	wxFont fnt = wxGetFontFromUser( frm , global_default_fnt );
	if( fnt.Ok() )
	{
		global_default_fnt = fnt;
#if !wxCHECK_VERSION(2, 9, 0)
		global_default_fnt.SetNoAntiAliasing( !blAA );
#endif

		telnet_frame->SetTerminalFont(fnt);
		int c = edit_win_list.GetCount();
		for(int i=0;i<c;i++)
			((frame_EditTerm*)(edit_win_list.Item(i)))->SetTerminalFont(fnt);

		GetConfig()->Write( GetUserConfigPath(_T("/setting/font")), fnt.GetNativeFontInfoDesc() );
	}
	else
		wxMessageBox( gettext("The font you choosed is not correct, cancel change of font option.") );
}






void setLinkProgram(LINK_TYPE _t , wxString program_path)
{
	switch(_t)
	{
		case LINK_HTTP:
		case LINK_HTTPS:
			GetConfig()->Write( GetUserConfigPath(_T("/setting/link_program/http")), program_path );	break;
		case LINK_FTP:
		case LINK_SFTP:
			GetConfig()->Write( GetUserConfigPath(_T("/setting/link_program/ftp")), program_path );	break;
		case LINK_EMAIL:
			GetConfig()->Write( GetUserConfigPath(_T("/setting/link_program/email")), program_path );	break;
		default : break;
	}
}

wxString getLinkProgram(LINK_TYPE _t)
{
	wxString program_path = wxEmptyString;

	switch(_t)
	{
		case LINK_HTTP:
		case LINK_HTTPS:
			if( GetConfig()->Read( GetUserConfigPath(_T("/setting/link_program/http")), & program_path ) );
			else
			{
#if defined(__WXGTK__)
				program_path = _T("xdg-open");
#else
				program_path = _T("C:\\Program Files\\Internet Explorer\\IEXPLORE.EXE");
#endif
			}
			break;
		case LINK_FTP:
		case LINK_SFTP:
			if( GetConfig()->Read( GetUserConfigPath(_T("/setting/link_program/ftp")), & program_path ) );
			else
			{
#if defined(__WXGTK__)
				program_path = _T("xdg-open");
#else
				program_path = _T("C:\\Program Files\\Internet Explorer\\IEXPLORE.EXE");
#endif
			}
			break;
		case LINK_EMAIL:
			if( GetConfig()->Read( GetUserConfigPath(_T("/setting/link_program/email")), & program_path ) );
			else
			{
#if defined(__WXGTK__)
				program_path = _T("xdg-open");
#else
				program_path = _T("C:\\Program Files\\Internet Explorer\\IEXPLORE.EXE");
#endif
			}
			break;
		default : break;
	}

	return program_path;
}

void OnLinkClicked( char *link, LINK_TYPE _t)
{
	wxString url = CharPtrTowxString(link);

	switch(_t)
	{
		case LINK_TELNET:
			telnet_frame->connect( url );
			break;
		case LINK_HTTP:
		case LINK_HTTPS:
		case LINK_FTP:
		case LINK_SFTP:
		case LINK_EMAIL:
			if( _t == LINK_EMAIL && url.Find(_T(':')) == wxNOT_FOUND )
				url = _T("mailto:") + url;
			if( ! wxLaunchDefaultBrowser(url) )
				wxLogWarning(_T("Unable to open hyperlink: %s"), url);
			break;
		default : break;
	}
}

void OpenHyperlink(char *link)
{
	LINK_TYPE _t;
	if( strstr(link, "http:/") == link )	_t = LINK_HTTP;
	else if( strstr(link, "https:/") == link )	_t = LINK_HTTPS;
	else if( strstr(link, "telnet:/") == link )	_t = LINK_TELNET;
	else if( strstr(link, "bbs:/") == link )	_t = LINK_TELNET;
	else if( strstr(link, "ftp:/") == link )	_t = LINK_FTP;
	else if( strstr(link, "sftp:/") == link )	_t = LINK_SFTP;
	else if( strstr(link, "@") != NULL )	_t = LINK_EMAIL;
	else _t = LINK_HTTP;
	OnLinkClicked(link, _t);
}

// ----------------------------------------------------------------------------
wxApp *app = NULL;
void SetApp(wxApp *_app)
{	app = _app;	}
// ----------------------------------------------------------------------------
wxApp* GetApp()
{	return app;	}
// ----------------------------------------------------------------------------
void AppProcessOtherEvents()
{
	if( !app )	return;
	if( app->Pending() )	app->Dispatch();
	app->Yield();
}
// ----------------------------------------------------------------------------
void Init_LineWrapedLength()
{
	long len;
	if( GetConfig()->Read( GetUserConfigPath(_T("/setting/line_wraped_length")) , & len ) )
		SCD_Terminal::SetLineWrapedLength(len);
}
// ----------------------------------------------------------------------------
void SetLineWrapedLength(int len)
{
	SCD_Terminal::SetLineWrapedLength(len);
	GetConfig()->Write( GetUserConfigPath(_T("/setting/line_wraped_length")), SCD_Terminal::GetLineWrapedLength() );
}
// ============================================================================

#endif
