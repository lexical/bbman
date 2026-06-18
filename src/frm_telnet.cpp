/******************************************************************************
 * Name:        frm_telnet.cpp
 * Purpose:     MainFrame, display the connections and tab
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef FRM_TELNET_CPP
#define FRM_TELNET_CPP
#include "frm_telnet.h"

#include "login.h"
#include "bookmark.h"
#ifndef BBMAN_NO_SSH
#include "frm_sftp.h"
#endif

enum { ICON_CLOSE, ICON_CONNECTING, ICON_CONNECTED, ICON_MESSAGE };


#include "frm_editterm.h"
#include "bookmark.h"
#include "scd_gtk_textctrl.h"

#include <wx/file.h>
#include <wx/numdlg.h>

// ============================================================================

enum
{
	Minimal_Quit = 1,

	MENU_ABOUT_BEGIN = wxID_ABOUT,
	MENU_ABOUT = wxID_ABOUT,	//Note NOT TO MODIFY THIS
	MENU_ABOUT_MESSAGE_CODE,
	MENU_ABOUT_GOWEB,	//瀏覽軟體網頁
	MENU_ABOUT_GOMAIL,	//寄信給 BBMan 軟體設計者
	MENU_ABOUT_END,

	MENU_ANSIEDITOR,

	MENU_BEGIN = 66,

	MENU_CONNECTION_BEGIN,
	MENU_OPENSITELIST,
	MENU_QUICKCONNECT,
	MENU_CLOSECONNECTION,
	MENU_DELETETELNET,
	MENU_RECONNECT,
	MENU_GOPREV,
	MENU_GONEXT,
	MENU_GOUSERSPECIFIEDTAB,
	MENU_FULLSCREEN,
	MENU_LOGINWHENSTART,
	MENU_PASSWD,
	MENU_CONNECTION_END,

	MENU_COPY,
	MENU_COPY_ANSI,
	MENU_PASTE,
	MENU_PASTE_ANSI,
	
	MENU_SELECTALL,
	MENU_OPEN_SELECTION_AS_LINK,
	MENU_INPUT_LINE,

	MENU_SETTING_BEGIN,
	MENU_SELECT_THEME,
	MENU_FONT,
	MENU_LINK_PROGRAM,
	MENU_AUTO_REPLY,
	MENU_EDIT_CANNED_MESSAGE,
	MENU_NAWS,
	MENU_LINE_WRAPED_LENGTH,
	MENU_MULTIBYTEWORDDETECTION,
	MENU_MESSAGE_SOUND,
	MENU_ALWAYS_HIGHLIGHT,
	MENU_AUTO_RECONNECT,
	MENU_PREVENT_DISCONNECT,
	MENU_SETTING_END,

	MENU_TOOL_BEGIN,
	MENU_LOCKSCREEN,
	MENU_SHOW_SFTP,
	MENU_EXPORT_SETTING,
	MENU_IMPORT_SETTING,
	MENU_TOOL_END,

	MENU_GOTAB_1,
	MENU_GOTAB_9 = MENU_GOTAB_1 + 8 ,

	MENU_CANNED_MESSAGE_1,
	MENU_CANNED_MESSAGE_12 = MENU_CANNED_MESSAGE_1 + 11 ,

	MENU_ADDBOOKMARK,
	MENU_EDITBOOKMARK,
	MENU_BOOKMARK_BEGIN,		//這一定要放在最後一個
	MENU_BOOKMARK_END = MENU_BOOKMARK_BEGIN + 1000	//最多容許 1000 個書籤
};

// ----------------------------------------------------------------------------
// event tables and other macros for wxWindows
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(BBS_Frame, wxFrame)

	EVT_NOTEBOOK_PAGE_CHANGED( 666, BBS_Frame::OnTabChanged )
	EVT_SIZE(BBS_Frame::OnResize)

	EVT_SOCKET(0 , BBS_Frame::OnSocketEvent)

	EVT_TIMER(0, BBS_Frame::OnTimer)

	EVT_LEFT_UP(BBS_Frame::OnMouseLeftUp)
	EVT_RIGHT_DOWN(BBS_Frame::OnMouseRightDown)
	EVT_RIGHT_UP(BBS_Frame::OnMouseRightUp)
	EVT_MIDDLE_DOWN(BBS_Frame::OnMouseMiddleDown)

	EVT_KEY_DOWN(BBS_Frame::OnKeyDown)
	EVT_CHAR(BBS_Frame::OnChar)

	EVT_SET_FOCUS(BBS_Frame::OnFocus)
	EVT_MENU_OPEN(BBS_Frame::OnMenuOpen)

	EVT_TEXT_ENTER(666, BBS_Frame::OnAddressTextEnterText)

	EVT_MENU_RANGE(MENU_CONNECTION_BEGIN, MENU_CONNECTION_END, BBS_Frame::OnConnection)

	EVT_MENU(MENU_COPY, BBS_Frame::OnCopy)
	EVT_MENU(MENU_COPY_ANSI, BBS_Frame::OnCopy_withANSI)
	EVT_MENU(MENU_PASTE, BBS_Frame::OnPaste)
	EVT_MENU(MENU_PASTE_ANSI, BBS_Frame::OnPaste_withANSI)

	EVT_MENU(MENU_SELECTALL, BBS_Frame::OnSelectAll)
	EVT_MENU(MENU_OPEN_SELECTION_AS_LINK, BBS_Frame::OnOpenSelectionAsHyperlink)
	EVT_MENU(MENU_INPUT_LINE, BBS_Frame::OnInputLine)

	EVT_MENU(MENU_ADDBOOKMARK,  BBS_Frame::OnAddBookmark)
	EVT_MENU(MENU_EDITBOOKMARK,  BBS_Frame::OnEditBookmark)
	EVT_MENU_RANGE(MENU_BOOKMARK_BEGIN, MENU_BOOKMARK_END, BBS_Frame::OnBookmark)

	EVT_MENU_RANGE(MENU_SETTING_BEGIN, MENU_SETTING_END, BBS_Frame::OnSetting)
	EVT_MENU_RANGE(MENU_TOOL_BEGIN, MENU_TOOL_END, BBS_Frame::OnTool)

	EVT_MENU(MENU_ANSIEDITOR, BBS_Frame::OnOpenAnsiEditor)

	EVT_MENU_RANGE(MENU_GOTAB_1, MENU_GOTAB_9, BBS_Frame::OnGoTab)
	EVT_MENU_RANGE(MENU_CANNED_MESSAGE_1, MENU_CANNED_MESSAGE_12, BBS_Frame::OnCanedMessage)

	EVT_MENU_RANGE(MENU_ABOUT_BEGIN, MENU_ABOUT_END, BBS_Frame::OnAbout)
	EVT_MENU(Minimal_Quit,  BBS_Frame::OnQuit)

	EVT_UPDATE_UI_RANGE( MENU_SETTING_BEGIN , MENU_SETTING_END , BBS_Frame::OnUpdate_UI)
	EVT_UPDATE_UI( MENU_LOGINWHENSTART , BBS_Frame::OnUpdate_UI)
	EVT_UPDATE_UI( MENU_FULLSCREEN , BBS_Frame::OnUpdate_UI)

	EVT_ACTIVATE(BBS_Frame::OnActivate)
	EVT_CLOSE(BBS_Frame::OnClose)
	
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BBS_Frame::~BBS_Frame()
{
	isClosed = true;
	timer.Stop();
	CloseAllTerminals();
#ifdef __WXGTK__
	destroy_CopyPasteDialog();
#endif
}

BBS_Frame::BBS_Frame(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
	   : wxFrame(NULL, -1, title, pos, size, style)
{
	// set the frame icon
	//you need to add both 16x16 & 32x32 icons
	//16x16 for frame title & taskbar icon
	//32x32 for alt-tab icon
	wxString iconPath = _T("icon/");
	if( ! wxFile::Exists(iconPath + _T("BBMan_48.png")) )
		iconPath = _T("../icon/");

	wxIconBundle icons;
	if( wxFile::Exists(iconPath + _T("BBMan_16.png")) )
		icons.AddIcon(wxIcon(iconPath + _T("BBMan_16.png"), wxBITMAP_TYPE_PNG));
	if( wxFile::Exists(iconPath + _T("BBMan_32.png")) )
		icons.AddIcon(wxIcon(iconPath + _T("BBMan_32.png"), wxBITMAP_TYPE_PNG));
	if( wxFile::Exists(iconPath + _T("BBMan_48.png")) )
		icons.AddIcon(wxIcon(iconPath + _T("BBMan_48.png"), wxBITMAP_TYPE_PNG));

	if( icons.IsEmpty() )
	{
		#include "icon/bbman_16.xpm"
		#include "icon/bbman_32.xpm"
		icons.AddIcon(wxIcon(bbman_16_xpm));
		icons.AddIcon(wxIcon(bbman_32_xpm));
	}
	SetIcons(icons);
	//

	tb = NULL;
	txtAddress = NULL;
	panel = new Telnet_Panel;
	tab = new SCD_TabCtrl;

#ifdef __WXMSW__
	SetBackgroundColour( wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE) );
#endif

#if wxUSE_MENUS
	// create a menu bar
	wxMenuBar *menuBar = new wxMenuBar();

	wxMenu *menuFile = new wxMenu;
	menuFile->Append(MENU_OPENSITELIST, gettext("Connection List") );
	menuFile->Append(MENU_QUICKCONNECT, gettext("&Quick Connect... \tAlt-Q") );
	menuFile->AppendSeparator();
//	menuFile->Append(MENU_CLOSECONNECTION, gettext("&Disconnect \tAlt-D") );
	AppendMenuItemWithBitmap(menuFile, MENU_CLOSECONNECTION, gettext("&Disconnect \tAlt-D"), GetProgramIcon(BBMAN_ICON_CLOSED) );
//	menuFile->Append(MENU_DELETETELNET, gettext("Close Connection \tAlt-W") );
	AppendMenuItemWithBitmap(menuFile, MENU_DELETETELNET, gettext("Close Connection \tAlt-W"), GetProgramIcon(BBMAN_ICON_REMOVE) );
//	menuFile->Append(MENU_RECONNECT, gettext("&Reconnect \tAlt-R") );
	AppendMenuItemWithBitmap(menuFile, MENU_RECONNECT, gettext("&Reconnect \tAlt-R"), GetProgramIcon(BBMAN_ICON_CONNECTED) );
	menuFile->AppendSeparator();
//	menuFile->Append(MENU_GOPREV, gettext("Previous Connection \tAlt-Z") );
	AppendMenuItemWithBitmap(menuFile, MENU_GOPREV, gettext("Previous Connection \tAlt-Z"), GetProgramIcon(BBMAN_ICON_PREV) );
//	menuFile->Append(MENU_GONEXT, gettext("Next Connection \tAlt-X") );
	AppendMenuItemWithBitmap(menuFile, MENU_GONEXT, gettext("Next Connection \tAlt-X"), GetProgramIcon(BBMAN_ICON_NEXT) );
	menuFile->Append(MENU_GOUSERSPECIFIEDTAB, gettext("Switch to assigning connection \tAlt+0") );
	menuFile->AppendSeparator();
//	menuFile->AppendCheckItem(MENU_FULLSCREEN, gettext("&Full screen \tAlt-Enter") );
	AppendMenuItemWithBitmap(menuFile, MENU_FULLSCREEN, gettext("&Full screen \tAlt-Enter"), GetProgramIcon(BBMAN_ICON_FULLSCREEN) );

	wxMenu *menuLogin = new wxMenu;
	menuLogin->AppendCheckItem(MENU_LOGINWHENSTART, gettext("Show &login dialog when startup") );
	menuLogin->Append(MENU_PASSWD, gettext("&Modify login password") );
	menuFile->Append(0, gettext("Login Option"), menuLogin);

	menuFile->AppendSeparator();
	menuFile->Append(Minimal_Quit, gettext("Exit &Program \tAlt-F4"));
	menuBar->Append(menuFile, gettext("Co&nnect"));

	editMenu = new wxMenu;
//	editMenu->Append(MENU_COPY , gettext("Copy (Plain Text)\tAlt-C") );
	AppendMenuItemWithBitmap(editMenu, MENU_COPY, gettext("Copy (Plain Text)\tAlt-C"), GetProgramIcon(BBMAN_ICON_COPY) );
	editMenu->Append(MENU_COPY_ANSI , gettext("Copy (with ANSI Color)\tCtrl-Alt-C") );
//	editMenu->Append(MENU_PASTE, gettext("Paste (Plain Text)\tAlt-V") );
	editMenu->AppendSeparator();
//	editMenu->Append(MENU_PASTE_ANSI, gettext("Paste\tCtrl-Alt-V") );
	AppendMenuItemWithBitmap(editMenu, MENU_PASTE_ANSI, gettext("Paste\tAlt-V"), GetProgramIcon(BBMAN_ICON_PASTE) );
	editMenu->AppendSeparator();
//	editMenu->Append(MENU_OPEN_SELECTION_AS_LINK , gettext("Open selected text as URL") );
	AppendMenuItemWithBitmap(editMenu, MENU_OPEN_SELECTION_AS_LINK, gettext("Open selected text as URL"), GetProgramIcon(BBMAN_ICON_WEB) );
	editMenu->AppendSeparator();
	editMenu->Append(MENU_SELECTALL , gettext("Select &All\tAlt-A") );
	editMenu->AppendSeparator();
	editMenu->Append(MENU_INPUT_LINE, gettext("Input &Line\tAlt-L") );
	menuBar->Append(editMenu, gettext("&Edit"));

	menuBookmark = new wxMenu;
	menuBookmark->Append(MENU_ADDBOOKMARK, gettext("Add favorite") );
	menuBookmark->Append(MENU_EDITBOOKMARK, gettext("Edit favorite") );
	menuBookmark->AppendSeparator();
	menuBar->Append(menuBookmark, gettext("&Favorite"));

	wxMenu *menuOption = new wxMenu;
	menuOption->Append(MENU_SELECT_THEME, gettext("Change Theme") );
	menuOption->Append(MENU_FONT, gettext("&Font...") );
	menuOption->Append(MENU_LINK_PROGRAM, gettext("Setup URL assicoation &program") );
	menuOption->Append(MENU_MESSAGE_SOUND, gettext("Play &Sound when receive hot call") );
	menuOption->Append(MENU_AUTO_REPLY, gettext("Auto replay hot call when idle") );
	menuOption->Append(MENU_EDIT_CANNED_MESSAGE, gettext("&Edit canned message") );
	menuOption->Append(MENU_LINE_WRAPED_LENGTH, gettext("Max line &width for pasting article") );
	menuOption->AppendCheckItem(MENU_NAWS, gettext("Enable &long screen") );
	menuOption->AppendCheckItem(MENU_MULTIBYTEWORDDETECTION, gettext("Enable double byte character &detect for current connection \tF12") );
	menuOption->AppendCheckItem(MENU_ALWAYS_HIGHLIGHT, gettext("Display with highlight text") );
	menuOption->AppendSeparator();
	menuOption->Append(MENU_AUTO_RECONNECT, gettext("&Auto reconnect") );
	menuOption->AppendCheckItem(MENU_PREVENT_DISCONNECT, gettext("Prevent I&dle by refreshing screen") );
	menuBar->Append(menuOption, gettext("&Options"));
	
	wxMenu *menuTool = new wxMenu;
#ifndef BBMAN_NO_SSH
	menuTool->Append(MENU_SHOW_SFTP, gettext("Show SFTP Window\tF6") );
#endif
//	menuTool->Append(MENU_ANSIEDITOR, gettext("ANSI &editor") );
	AppendMenuItemWithBitmap(menuTool, MENU_ANSIEDITOR, gettext("ANSI &editor"), GetProgramIcon(BBMAN_ICON_EDIT) );
//	menuTool->Append(MENU_EXPORT_SETTING, gettext("Export setting") );
//	menuTool->Append(MENU_IMPORT_SETTING, gettext("Import setting") );
	menuBar->Append(menuTool, gettext("&Tool"));

	wxMenu *helpMenu = new wxMenu;
//	helpMenu->Append(MENU_ABOUT, gettext("About BBMan \tF1") );
	AppendMenuItemWithBitmap(helpMenu, MENU_ABOUT, gettext("About BBMan \tF1"), GetProgramIcon(BBMAN_ICON_ICON_16) );
	helpMenu->Append(MENU_ABOUT_MESSAGE_CODE, gettext("About BBMan &format message") );
	helpMenu->AppendSeparator();
//	helpMenu->Append(MENU_ABOUT_GOWEB, gettext("Browse BBMan &web site") );
	AppendMenuItemWithBitmap(helpMenu, MENU_ABOUT_GOWEB, gettext("Browse BBMan &web site"), GetProgramIcon(BBMAN_ICON_WEB) );
//	helpMenu->Append(MENU_ABOUT_GOMAIL, gettext("&Mail to BBMan author") );
	AppendMenuItemWithBitmap(helpMenu, MENU_ABOUT_GOMAIL, gettext("&Mail to BBMan author"), GetProgramIcon(BBMAN_ICON_EMAIL) );
	menuBar->Append(helpMenu, gettext("&About"));


	SetMenuBar(menuBar);
	


#endif // wxUSE_MENUS


	#define FILENAME_TOOLBAR_ICON "toolbar.bmp"
	tb = NULL;
	wxString toolbarIconPath = GetThemePath() + _T(FILENAME_TOOLBAR_ICON);
	if( wxFile::Exists( toolbarIconPath ) )
	{
		int k;
		int height;
		wxBitmap toolbarIcons[8];
		wxBitmap tbimg( toolbarIconPath , wxBITMAP_TYPE_BMP );
		height = tbimg.GetHeight();
		for(k=0;k<8;k++)
			MakeBitmapMask( toolbarIcons[k] = tbimg.GetSubBitmap( wxRect( k*height, 0, height, height ) ) );
		
		tb = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxTB_FLAT | wxTB_HORIZONTAL | wxNO_BORDER );
		tb->SetToolBitmapSize(wxSize(height,height));
		tb->SetMargins(3, 3);
#ifdef __WXMSW__
		tb->SetBackgroundColour( wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE) );
#endif
		k = 0;
		tb->AddTool( MENU_OPENSITELIST , wxEmptyString , toolbarIcons[k++] , gettext("Connection List") );
		tb->AddTool( MENU_QUICKCONNECT , wxEmptyString , toolbarIcons[k++] , gettext("Quick Connect (Alt+Q)") );
		tb->AddSeparator();
		tb->AddTool( MENU_RECONNECT , wxEmptyString , toolbarIcons[k++] , gettext("Reconnect (Enter) (Alt+R)") );
		tb->AddTool( MENU_DELETETELNET , wxEmptyString , toolbarIcons[k++] , gettext("Close Connection (Ctrl+Del) (Alt+W)") );
		tb->AddSeparator();
		tb->AddTool( MENU_COPY , wxEmptyString , toolbarIcons[k++] , gettext("Copy plain text (Alt+C)") );
		tb->AddTool( MENU_PASTE_ANSI , wxEmptyString , toolbarIcons[k++] , gettext("Paste (Alt+V)") );
		tb->AddSeparator();
#ifndef BBMAN_NO_SSH
		tb->AddTool( MENU_SHOW_SFTP , wxEmptyString , toolbarIcons[k++] , gettext("Show SFTP Window (F6)") );
		tb->AddSeparator();
#else
		k++;
#endif
		tb->Realize();
	}

#ifndef __WXMSW__
	stxtHostname = new wxStaticText(this, -1, _T("Hostname :") );
	txtAddress = new wxTextCtrl(this, 666, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	txtAddress->Connect( 666 , wxEVT_SET_FOCUS , (wxObjectEventFunction) (wxEventFunction) (wxFocusEventFunction) &BBS_Frame::OnAddressTextSetFocus );
#endif

	if(txtAddress)
	{
		txtAddress->SetSizeHints( 360, txtAddress->GetSize().GetHeight() );
		txtAddress->SetSize( 360, -1 );
		txtAddress->SetToolTip( gettext("Enter the server address, and press [Enter] to connect") );
	}



#if wxUSE_STATUSBAR
	// create a status bar just for fun (by default with 1 pane only)
//#if ! defined(__WXGTK__)
	CreateStatusBar();
	SetStatusText( GetStatusBarText() );
	SetStatusBarPane(-1);
//#endif
#endif // wxUSE_STATUSBAR

	//設定 terminal 字型
//	SetFont( GetCurrentFont() );


	ReloadBookmark();


	//初始化 telnet 視窗
 	now_telnet = NULL;
	
	//初始化 timer
	timer.SetOwner(this, 0);
	timer.Start(600);
	

	image_list.Create(16, 16);
	image_list.Add( GetProgramIcon(BBMAN_ICON_CLOSED) );
	image_list.Add( GetProgramIcon(BBMAN_ICON_CONNECTING) );
	image_list.Add( GetProgramIcon(BBMAN_ICON_CONNECTED) );
	image_list.Add( GetProgramIcon(BBMAN_ICON_MESSAGE) );

	tab->Create(this, 666);
	tab->Init();
	tab->SetImageList( & image_list );


	panel->Create(this, -1, wxDefaultPosition, wxDefaultSize , wxNO_FULL_REPAINT_ON_RESIZE );
	panel->SetFont( GetFont() );


	


	//設定快速鍵
//#if defined(__WXMSW__) || defined(__WXMAC__)
	wxAcceleratorEntry entries[100];
	int ac = 0;
//	for(int i=0;i<12;i++)	entries[ac++].Set(wxACCEL_CTRL,  WXK_F1 + i , MENU_CANNED_MESSAGE_1 + i );
	for(int i=0;i<9;i++)	entries[ac++].Set(wxACCEL_ALT,  '1' + i , MENU_GOTAB_1 + i );
	entries[ac++].Set(wxACCEL_ALT,  '0' , MENU_GOUSERSPECIFIEDTAB );
//	entries[ac++].Set(wxACCEL_CTRL, WXK_LEFT, MENU_GOPREV );
//	entries[ac++].Set(wxACCEL_CTRL, WXK_RIGHT, MENU_GONEXT );

	entries[ac++].Set(wxACCEL_ALT,  'T' , MENU_QUICKCONNECT );


	wxAcceleratorTable accel( ac , entries);
	panel->SetAcceleratorTable(accel);
//#endif

	//


	blFullScreen = false;
	isClosed = false;

//	if(tb)	tb->SetSizeHints( tb->GetSize().GetWidth(), tb->GetSize().GetHeight() );
//	tab->SetSizeHints(1000,30,1000,30);
//	panel->SetSizeHints(1000,700,1000,1000);

	wxBoxSizer *main_sizer;
	wxBoxSizer *sub_sizer;
	main_sizer = new wxBoxSizer(wxVERTICAL);

	sub_sizer = new wxBoxSizer(wxHORIZONTAL);
	if(tb)
	{
		sub_sizer->Add( tb , 0, wxALIGN_CENTER_VERTICAL );
		sub_sizer->Add( 6, 0 );
	}
#ifndef __WXMSW__
	sub_sizer->Add( stxtHostname, 0, wxALIGN_CENTER_VERTICAL );
	sub_sizer->Add( txtAddress, 0, wxALIGN_CENTER_VERTICAL );
#endif
	main_sizer->Add( sub_sizer, 0, wxEXPAND );

	main_sizer->Add( tab, 0, wxEXPAND );
	main_sizer->Add( panel, 1, wxEXPAND );

	main_sizer->Layout();
	SetAutoLayout(true);
	SetSizer(main_sizer);
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void BBS_Frame::ShowPreviousTelnet()
{	tab->ShowPreviousTab();	}
// ----------------------------------------------------------------------------
void BBS_Frame::ShowNextTelnet()
{	tab->ShowNextTab();	}
// ----------------------------------------------------------------------------
int BBS_Frame::getNowTelnetIndex()
{	return tab->GetSelection();	}
// ----------------------------------------------------------------------------
int BBS_Frame::getTelnetCount()
{	return tab->GetItemCount();	}
// ----------------------------------------------------------------------------
void BBS_Frame::OnMouseRightDown(wxMouseEvent& event)
{
	if( event.GetId() == 666 )	//使用者在連線標籤上點選右鍵, 想關閉某個連線
	{
		int i = tab->getTabUnderMouse( event.GetX() , event.GetY() );
		if( i < 0 )	return;

		DeleteTerminal(i);
	}
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnMouseRightUp(wxMouseEvent& event)
{
	ShowEditContextMenu(wxPoint(event.GetX() + panel->GetPosition().x, event.GetY() + panel->GetPosition().y));
}
// ----------------------------------------------------------------------------
void BBS_Frame::ShowEditContextMenu(const wxPoint& pos)
{
	wxMenu contextMenu;
	AppendMenuItemWithBitmap(&contextMenu, MENU_COPY, gettext("Copy (Plain Text)"), GetProgramIcon(BBMAN_ICON_COPY) );
	contextMenu.Append(MENU_COPY_ANSI , gettext("Copy (with ANSI Color)") );
	contextMenu.AppendSeparator();
	AppendMenuItemWithBitmap(&contextMenu, MENU_PASTE_ANSI, gettext("Paste"), GetProgramIcon(BBMAN_ICON_PASTE) );
	contextMenu.AppendSeparator();
	AppendMenuItemWithBitmap(&contextMenu, MENU_OPEN_SELECTION_AS_LINK, gettext("Open selected text as URL"), GetProgramIcon(BBMAN_ICON_WEB) );
	contextMenu.AppendSeparator();
	contextMenu.Append(MENU_SELECTALL , gettext("Select All") );
	contextMenu.AppendSeparator();
	contextMenu.Append(MENU_INPUT_LINE, gettext("Input Line") );

	PopupMenu( &contextMenu , pos );
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnMouseLeftUp(wxMouseEvent& event)
{;}
// ----------------------------------------------------------------------------
void BBS_Frame::OnMouseMiddleDown(wxMouseEvent& event)
{;}
// ----------------------------------------------------------------------------
void BBS_Frame::OnMenuOpen(wxMenuEvent& event)
{
#if defined(__WXGTK__)
	wxFocusEvent e;
	panel->OnFocus(e);	//GTK 下用 alt 選取選單時會造成 telnet_panel::isAltDown 判斷錯誤，因此在這裡呼叫 telnet_panel::OnFocus 使之還原
#endif
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnFocus(wxFocusEvent& event)
{
#if defined(__WXGTK__)
	panel->OnFocus(event);	//GTK 下用 alt+tab 切換視窗時會造成 telnet_panel::isAltDown 判斷錯誤，因此在這裡呼叫 telnet_panel::OnFocus 使之還原
//fprintf(stderr, "SetFocus\n");
#endif
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void BBS_Frame::connect(int protocol, wxString ip, int port, wxString name = wxEmptyString )
{
	SiteInfo si;
	si.ip = ip;
	si.port = port;
	si.name = name;
	si.protocol = protocol;
	connect(si);
}
// ----------------------------------------------------------------------------
void BBS_Frame::connect( wxString addr, wxString name )
{
	// addr: protocol://user:pass@server-ip:port
	// protocol can be ignored (default is telnet)
	// user:pass can be ignored
	// pass can be ignored
	// port can be ignored (default 22 in ssh, 23 in telnet)

	SiteInfo si;
	si.Init();
	si.name = name;

	addr.Trim();

	//determine the protocol
	si.protocol = SOCK_TELNET;

	int pos = addr.First( _T("://") );
	if( pos >= 0 )	//if specified the protocol
	{
		wxString protocol_str = addr.Mid(0, pos);
		addr = addr.Mid(pos + 3);

		if( protocol_str == _T("ssh") )	si.protocol = SOCK_SSH;
		else if( protocol_str == _T("telnet") );
		else if( protocol_str == _T("bbs") );
		else
		{
			wxMessageBox( wxString(gettext("Unknown protocol")) + _T(" : ") + protocol_str );
			return;
		}
	}

	//check user:password info
	if( addr.Find('@') != -1 )	//if addr contains '@', it contains username
	{
		wxString username = addr.BeforeFirst('@');
		addr = addr.AfterFirst('@');

		if( username.Find(':') != -1 )	//if username contains ':', it contains password
		{
			si.password = username.AfterFirst(':');
			username = username.BeforeFirst(':');
		}
		si.username = username;
	}

	si.port = (si.protocol == SOCK_SSH) ? 22 : 23 ;
	if( addr.Find( ':' ) != -1 )
	{
		addr.AfterFirst(':').ToLong( (long*) &(si.port) );
		addr = addr.BeforeFirst(':');
	}

	si.ip = addr;
	connect(si);
}
// ----------------------------------------------------------------------------
void BBS_Frame::connect(SiteInfo &si)
{
	int next_tab_index = -1;

	wxString tab_title;
 	if( si.name == wxEmptyString )	tab_title = si.ip;
 	else	tab_title = si.name;

	if( now_telnet == NULL || ! now_telnet->IsDisconnected() )
	{	//如果目前的 telnet 數目 == 0 , 或者  目前的 telnet 連線不是關閉的
		//則新增一個連線
 		now_telnet = new SCD_Telnet(panel);
		now_telnet->SetOnLinkClickedFunc( OnLinkClicked );

 		tab->InsertItem(0, tab_title, ICON_CONNECTING, now_telnet);
 		next_tab_index = tab->GetItemCount() - 1;
	}
	else
	{
  		next_tab_index = tab->GetSelection();

		tab->SetItemText( next_tab_index , tab_title );
		tab->SetItemImage( next_tab_index, ICON_CONNECTING );
	}

	now_telnet->connect( si.protocol, si.ip, si.port , si.name , si.username , si.password , si.message );

	tab->SetSelection( next_tab_index );
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnTabChanged(wxNotebookEvent& event)
{
	if( isClosed )	return;

  	if( event.GetSelection() < 0 )
   	{
   		now_telnet = NULL;
   		panel->SetTelnet(NULL);
   		SetTitle(_T("BBMan"));
		if(txtAddress)	txtAddress->Clear();
   	}
   	else
   	{
		SCD_Telnet *t;
		t = (SCD_Telnet*) tab->GetItemData( tab->GetSelection() );
		now_telnet = t;

		panel->SetTelnet(t);

		wxString tmp_ip = t->getIP();
		int tmp_port = t->getPort();
		if( t->GetSocket()->GetType() == SOCK_SSH )
		{
			tmp_ip = _T("ssh://" + tmp_ip);
			if( tmp_port != 22 )	tmp_ip += _T(":") + wxString::Format(_T("%d"), tmp_port);
		}
		else
		{
			if( tmp_port != 23 )	tmp_ip += _T(":") + wxString::Format(_T("%d"), tmp_port);
		}

		if( t->getName().IsEmpty() )
			SetTitle( _T("BBMan - ") + tmp_ip );
		else
			SetTitle( _T("BBMan - ") + t->getName() + _T(" [ ") + tmp_ip + _T(" ]") );

		if(txtAddress)	txtAddress->SetValue(tmp_ip);

//		FitTerminalSize();
	}

	panel->SetFocus();

	UpdateToolbarUI();
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void BBS_Frame::OnCopy(wxCommandEvent& event)
{
	if(now_telnet == NULL)	return;
	now_telnet->CopySelectionToClipboard(false);
	now_telnet->CancelSelection();
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnCopy_withANSI(wxCommandEvent& event)
{
	if(now_telnet == NULL)	return;
	now_telnet->CopySelectionToClipboard(true);
	now_telnet->CancelSelection();
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnPaste(wxCommandEvent& event)
{
	if(now_telnet == NULL)	return;
	now_telnet->PasteFromClipboard(false);
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnPaste_withANSI(wxCommandEvent& event)
{
	if(now_telnet == NULL)	return;
	now_telnet->PasteFromClipboard(true);
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void BBS_Frame::OnSelectAll(wxCommandEvent& event)
{
	if(now_telnet == NULL)	return;
	now_telnet->SelectAll();
}
void BBS_Frame::OnInputLine(wxCommandEvent& event)
{
	if(now_telnet == NULL)	return;
#ifdef __WXGTK__
	wxString line = GetMultilineTextFromUser( gettext("Please enter message you want to send"), gettext("Input a line"), this );
#else
	wxString line = wxGetTextFromUser( gettext("Please enter message you want to send"), gettext("Input a line"), wxEmptyString );
#endif
	if( ! line.IsEmpty() )
		now_telnet->Paste( wxStringToCharPtr(line) , true );
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


// event handlers

void BBS_Frame::OnActivate(wxActivateEvent& event)
{	panel->SetFocus();	}
// ----------------------------------------------------------------------------
void BBS_Frame::OnQuit(wxCommandEvent& WXUNUSED(event))
{	Close();	}	//
// ----------------------------------------------------------------------------
void BBS_Frame::CloseAllTerminals()
{
	panel->DetachTelnet();
	now_telnet = NULL;

	while( tab->GetItemCount() > 0 )
	{
		SCD_Telnet *t = (SCD_Telnet*) tab->GetItemData(0);
		tab->DeleteItem(0);
		if( t )
		{
			t->close();
			delete t;
		}
	}
}
//這會產生 BBS_Frame 的 OnClose event
// ----------------------------------------------------------------------------
void BBS_Frame::OnClose(wxCloseEvent& event)
{
	bool hasConnection = false;
	for(int i=0;i<tab->GetItemCount();i++)
	{
		if( ((SCD_Telnet*)tab->GetItemData(i))->IsConnected() )
		{
			hasConnection = true;
			break;
		}
	}

	if( event.CanVeto() )	//如果不是系統強制關閉視窗
		if( hasConnection )	//如果目前有連線, 則先詢問是否要關閉目前連線
			if( wxNO == wxMessageBox( gettext("There are existing connection, are you sure to quit?") , wxEmptyString, wxYES_NO ) )
			{
				event.Veto();	//阻止系統關閉視窗 
				return;
			}

	isClosed = true;
	timer.Stop();
	CloseAllTerminals();

	//儲存視窗大小
	GetConfig()->Write( GetUserConfigPath(_T("/setting/win_geo/fullscreen")) , IsFullScreen()?1:0 );
 	if( ! IsFullScreen() )
	{
		wxSize  ws = GetSize();
		wxPoint wp = GetPosition();
		GetConfig()->Write( GetUserConfigPath(_T("/setting/win_geo/left")) , wp.x );
		GetConfig()->Write( GetUserConfigPath(_T("/setting/win_geo/top")) , wp.y );
		GetConfig()->Write( GetUserConfigPath(_T("/setting/win_geo/width")) , ws.GetWidth() );
		GetConfig()->Write( GetUserConfigPath(_T("/setting/win_geo/height")) , ws.GetHeight() );
	}
	//

	Destroy();	//
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnAbout(wxCommandEvent& event)
{
	int id = event.GetId();
	switch(id)
	{
		case MENU_ABOUT :	ShowAbout();	break;
		case MENU_ABOUT_MESSAGE_CODE : 	wxMessageBox( gettext("BBMan format message allow you to send special code at [auto send message after connect] or [send customer message] \n\nAll special code start with backlash \\ \n\\n:[Enter]\n\\^?:Ctrl+? (for example: \\^R means Ctrl+R) \n\\#{UDLR}:up down left right arrow key (\\#U: up arrow) (\\#L:left arrow) \n\\p3: pause for 3 secs (from 1 ~ 9)\n\\\\:backlash \\\n\nFor example when you login ptt, you can assign auto send string:\\p3username\\npassword\\n\\n\\#D\\#D\\n\\n \n(Pause 3 secs -> Send username -> [Enter] -> Send password -> [Enter]  login -> [Enter] -> [Down] twice -> [Enter] twice to User menu)") ,
											gettext("About BBMan format message") );
			break;
		case MENU_ABOUT_GOWEB :	OpenHyperlink("http://bbman.sf.net/");	break;
		case MENU_ABOUT_GOMAIL :	OpenHyperlink("mailto:jakky1@pchome.com.tw?subject=[BBMan] suggestion / bug report");	break;
	}
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void BBS_Frame::ReloadBookmark()
{
	wxMenuBar *mnubar = GetMenuBar();
	int id = MENU_BOOKMARK_BEGIN;
	menuBookmark = mnubar->Remove(2);
	delete menuBookmark;
	
	menuBookmark = new wxMenu;

//	menuBookmark->Append(MENU_ADDBOOKMARK, gettext("Add favorite") );
//	menuBookmark->Append(MENU_EDITBOOKMARK, gettext("Edit favorite") );
	AppendMenuItemWithBitmap(menuBookmark, MENU_ADDBOOKMARK, gettext("Add favorite"), GetProgramIcon(BBMAN_ICON_BOOKMARK) );
	AppendMenuItemWithBitmap(menuBookmark, MENU_EDITBOOKMARK, gettext("Edit favorite"), GetProgramIcon(BBMAN_ICON_EDIT) );
	menuBookmark->AppendSeparator();
	ReloadBookmark( menuBookmark, GetUserConfigPath( _T("/bookmark") ), id);

	mnubar->Insert(2, menuBookmark, gettext("&Favorite") );
}
// ----------------------------------------------------------------------------
void BBS_Frame::ReloadBookmark(wxMenu *parent, wxString config_path, int &id)
{
	wxString name, str;
	long cookie;
	bool ret;

	//讀取資料夾
 	GetConfig()->SetPath(config_path);
	ret = GetConfig()->GetFirstGroup( name , cookie );
	while( ret )
	{
		str = name.AfterFirst(' ');

		wxMenu *tmp_menu = new wxMenu;
//		parent->Append( id++ , str, tmp_menu );
		wxMenuItem *mi;
		mi = new wxMenuItem(parent, id++, str, wxEmptyString, wxITEM_NORMAL, tmp_menu);
		mi->SetBitmap( GetProgramIcon(BBMAN_ICON_DIR) );
		parent->Append(mi);

		ReloadBookmark( tmp_menu, config_path + _T("/") + name, id);

		GetConfig()->SetPath(config_path);
		ret = GetConfig()->GetNextGroup( name , cookie );
	}

	//讀取 site
 	GetConfig()->SetPath(config_path);
	ret = GetConfig()->GetFirstEntry( name , cookie );
	while( ret )
	{
		str = name.AfterFirst(' ');

		if( GetConfig()->Read( name , &str ) )
		{
			SiteInfo si;
			si.Set(str);
//			parent->Append( id++, si.name, str );
			wxMenuItem *mi;
			mi = new wxMenuItem(parent, id++, si.name, str);
			mi->SetBitmap( GetProgramIcon(BBMAN_ICON_SITE) );
			parent->Append(mi);
		}

		GetConfig()->SetPath(config_path);
		ret = GetConfig()->GetNextEntry( name , cookie );
	}

}
// ----------------------------------------------------------------------------
void BBS_Frame::OnEditBookmark(wxCommandEvent& event)
{
	if( ShowFavoriteEditor(this) )	//如果我的最愛已被修改
		ReloadBookmark();	//重新載入書籤資訊到選單
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnAddBookmark(wxCommandEvent& event)
{
	if(now_telnet == NULL)	return;

	SiteInfo si;
	si.ip = now_telnet->getIP();
	si.port = now_telnet->getPort();
	si.name = now_telnet->getName();
	if( si.name == wxEmptyString )	si.name = si.ip;

	if( ShowFavoriteEditor(this, &si) )	//如果我的最愛已被修改
		ReloadBookmark();	//重新載入書籤資訊到選單
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnBookmark(wxCommandEvent& event)
{
	wxMenuItem *item = menuBookmark->FindItem( event.GetId() );
	if( item )
	{
		SiteInfo si;
		si.Set( item->GetHelp() );
		connect(si);
	}
	else
		wxMessageBox(_T("bookmark not found"));
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void BBS_Frame::OnTimer(wxTimerEvent& event)
{
	if( isClosed )	return;

	if( isPreventIdle() )
	{
		char refresh = '\x0c';
		for(int i=0;i<tab->GetItemCount();i++)
		{
			SCD_Telnet *t = (SCD_Telnet*) tab->GetItemData(i);
			if( t->getUserIdleTime() > 60*3 )
			{
				t->UserSend(&refresh, 1);
				t->MarkKeepAliveSent();
			}
		}
	}

	if(now_telnet != NULL)
		now_telnet->setBlinkCharVisibility( ! now_telnet->getBlinkCharVisibility() );
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnSocketEvent( wxSocketEvent &e )
{
	if( isClosed )	return;

	//note : 在跟那些一連線馬上就斷線的 server 連線的情況下,
	//有可能 wxSOCKET_LOST 會在 wxSOCKET_CONNECTION 之前到
	//所以還是要看 socket::IsConnected() 比較正確

	SCD_Telnet *t = (SCD_Telnet*) e.GetClientData();
	if( t == NULL )	return;
	int i = tab->GetItemIndex(t);
	if( i < 0 )	return;	//如果 i < 0 , 代表使用者要求刪除該 tab

	if( e.GetSocketEvent() == wxSOCKET_CONNECTION
		|| e.GetSocketEvent() == wxSOCKET_LOST )
	{
		if( t->IsConnected() )	//如果已經連上線了
			tab->SetItemImage( i , ICON_CONNECTED );
		else	//如果連線中斷了
		{
			if( tab->GetItemImage(i) == ICON_CONNECTING )	//如果不是因為連不上而斷線 (因為如果是先連上才斷線的話, imageid 應該等於 ICON_CONNECTED , 如果 imageid == ICON_CONNECTING 則代表沒有連上過)
			{
				tab->SetItemImage( i , ICON_CLOSE );
				t->parse( wxStringToCharPtr( wxString( _T("\x1b[1;1H\x1b[1;5;33;43m ") ) + gettext("Connection fail ( maybe server out-of-service, or wrong address )") + wxString(_T(" \x1b[m")) ) );
			}
			else if( ! t->isUserClosed() && t->getConnectTime() < 10 )	//如果連線時間不到十秒鐘就被斷線, 則重新連線
			{
				tab->SetItemImage( i , ICON_CONNECTING );
				t->reconnect();
			}
			else
			{
				tab->SetItemImage( i , ICON_CLOSE );
				t->parse( wxStringToCharPtr( wxString( _T("\x1b[1;1H\x1b[1;5;33;43m ") ) + gettext("Disconnected") + wxString(_T(" \x1b[m")) ) );
			}
		}

		UpdateToolbarUI();
	}
	else if( e.GetSocketEvent() == wxSOCKET_INPUT )	//如果電腦接收到資料
	{
  		if( t->isBell() )	//看看經過 SCD_Telnet::parse() 之後, 有沒有 '\a' 這個字元 (也就是檢查有沒有其他人丟水球進來)
  		{
			if( ((SCD_Telnet*)tab->GetItemData(tab->GetSelection()))->getUserIdleTime() > 60 )	//如果使用者閒置超過一分鐘才自動切換至來訊連線
	  			tab->SetSelection(i);	//有人丟水球進來的話, 則切換到該 telnet 畫面

			tab->SetItemImage( i, ICON_MESSAGE );	//修改連線標籤的圖示提醒使用者有水球
//			wxBell();	
 			SCD_PlaySound();	//發出聲音提醒使用者有水球

			if( t->getUserIdleTime() > 60*3 )	//如果該連線閒置過久
			{
				wxString msg;
				if( GetConfig()->Read( GetUserConfigPath(_T("/setting/auto_reply")) , & msg ) )	//如果有設定自動回覆水球機制
				{
        			t->keyControl('R');
	 				t->UserSend( wxStringToCharPtr(msg) );
	 				t->keyEnter();
	 				t->UserSend( "Y" );
	 				t->keyEnter();
				}
			}
  		}
	}
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnChar(wxKeyEvent& event)
{
	int id = tab->GetSelection();
	if( tab->GetItemImage(id) == ICON_MESSAGE )
		tab->SetItemImage( id, ICON_CONNECTED );

#ifdef __WXGTK__
	if( event.AltDown() )
	{
		if( event.m_keyCode >= '1' && event.m_keyCode <= '9' )
			ShowSpecifiedTerminal( event.m_keyCode - '1' );
		else if( event.m_keyCode == '0' )
			ShowSpecifiedTerminal(-1);
//		else
//			event.Skip();	//避免 scd_telnet 處理這個字元
	}
//	else	event.Skip();
#else
	event.Skip();
#endif
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnKeyDown(wxKeyEvent& event)
{	
	int id = tab->GetSelection();
	if( tab->GetItemImage(id) == ICON_MESSAGE )
		tab->SetItemImage( id, ICON_CONNECTED );

	//自行處理快速鍵
	if( event.ControlDown() )
	{
		if( event.m_keyCode == WXK_LEFT )
			ShowPreviousTelnet();
		else if( event.m_keyCode == WXK_RIGHT )
			ShowNextTelnet();
		else if( event.m_keyCode == WXK_HOME )
			ShowSpecifiedTerminal(0);
		else if( event.m_keyCode == WXK_END )
			ShowSpecifiedTerminal( tab->GetItemCount() - 1 );
		else if( event.m_keyCode == WXK_INSERT )
			QuickConnect();

		else if( event.m_keyCode == WXK_DELETE )
		{
//			Command(MENU_DELETETELNET);
			DeleteTerminal(-1);	//error
/*
			wxCommandEvent e;
			e.SetId(MENU_DELETETELNET);
			this->AddPendingEvent(e);
*/
		}

		else if( event.m_keyCode >= WXK_F1 && event.m_keyCode <= WXK_F12 )
			SendCanedMessage( event.m_keyCode - WXK_F1 + 1 );
	}

	if( event.m_keyCode == 13 && now_telnet->IsDisconnected() )	//斷線時按 [Enter] 重新連線
	{
		tab->SetItemImage( tab->GetSelection(), ICON_CONNECTING );
		now_telnet->reconnect();
	}
}
// ----------------------------------------------------------------------------
void BBS_Frame::QuickConnect()
{
	wxString site = wxGetTextFromUser( wxString( gettext("Please enter site address\n\nIf not connects with default port, please add colon and port number\nFor example: use ptt.twbbs.org:3456 to connect ptt.twbbs.org with port 3456") )
		+ _T("\n\n") + gettext("If you want to make a SSH connection, Please add prefix ssh://\nfor example, enter ssh://your.server.ip") , gettext("Quick Connect") );
	if( ! site.IsEmpty() )	connect( site );
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnAddressTextEnterText(wxCommandEvent& event)
{	if(txtAddress)	connect( txtAddress->GetValue() );	}
// ----------------------------------------------------------------------------
void BBS_Frame::OnAddressTextSetFocus(wxFocusEvent& event)
{
//wxMessageBox("test");
//	if(txtAddress)	txtAddress->SetSelection(-1,-1);
}
// ----------------------------------------------------------------------------
#include <wx/textdlg.h>
void BBS_Frame::OnConnection(wxCommandEvent& event)
{
	int id = event.GetId();
	switch(id)
	{
		case MENU_OPENSITELIST :
			{
				wxString _addr, _name;
				if( ShowBBSListDialog( _addr, _name ) )
					connect( _addr, _name );
			}
			break;
		case MENU_QUICKCONNECT :
			QuickConnect();
			break;
		case MENU_CLOSECONNECTION :
			if(now_telnet)
			{
#ifndef BBMAN_NO_SSH
				if(now_telnet->GetSocket()->GetType() == SOCK_SSH)
					DeleteSFTP( now_telnet->GetSocket()->GetSSH() );
#endif
				now_telnet->close();
			}
			break;
		case MENU_DELETETELNET :
			DeleteTerminal(-1);
			break;
		case MENU_RECONNECT :
			if(now_telnet == NULL)	return;
			if( now_telnet->IsDisconnected() )
			{
				tab->SetItemImage( tab->GetSelection(), ICON_CONNECTING );
				now_telnet->reconnect();
			}
			break;
		case MENU_GOPREV :
   			ShowPreviousTelnet();
			break;
		case MENU_GONEXT :
			ShowNextTelnet();
			break;
		case MENU_GOUSERSPECIFIEDTAB :
			ShowSpecifiedTerminal(-1);
			break;
		case MENU_FULLSCREEN :
			FullScreen();
			break;
		case MENU_LOGINWHENSTART :
			setLoginWhenStart( ! getLoginWhenStart() );
			break;
		case MENU_PASSWD :
			ShowPasswdDialog();
			ReloadBookmark();	//一定要重新載入 bookmark 資訊, 因為舊的資訊還留有舊的密碼
			break;
	}
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnSetting(wxCommandEvent& event)
{
	int id = event.GetId();
	switch(id)
	{
		case MENU_SELECT_THEME :
			ShowThemeSelector(this);
			break;
		case MENU_FONT :
			UserSetFont(this);
			break;
		case MENU_LINK_PROGRAM :
			{
				wxArrayString list;
				list.Add( _T("http") );
				list.Add( _T("email") );
				list.Add( _T("ftp") );
				int id = wxGetSingleChoiceIndex( gettext("Please choose what hyperlinks associated program you want to edit?") , gettext("Edit hyperlinks associated program") , list );
				if( id == -1 )	break;

    			LINK_TYPE _t;
				switch(id)
				{
					case 0 : _t = LINK_HTTP;	break;
					case 1 : _t = LINK_EMAIL;	break;
					case 2 : _t = LINK_FTP;		break;
					default : return;
				}

				wxString link_program = getLinkProgram(_t);
#if defined(__WXGTK__)
				link_program = wxGetTextFromUser( gettext("What program to open this type of hyperlinks?") , gettext("Setup hyperlinks associated program") , link_program );
#else
				link_program = wxFileSelector( gettext("Setup hyperlinks associated program") , link_program, wxEmptyString, _T("exe") , _T("Program (*.exe)|*.exe|All (*.*)|*.*") , wxFD_OPEN | wxFD_FILE_MUST_EXIST , this );
#endif
				if( ! link_program.IsEmpty() )	setLinkProgram( _t , link_program );
			}
			break;
		case MENU_AUTO_REPLY :
			{
			wxString msg;
   			GetConfig()->Read( GetUserConfigPath(_T("/setting/auto_reply")) , & msg );
			msg = wxGetTextFromUser( gettext("Please enter message for auto replying hot call when idle.\n\nPress [Esc] or [Cancel] or [empty content] means you will disable this function\nPress [OK] to keep current setting\n(Not support BBMan format message)") , gettext("Auto hot call reply") , msg );
			if( ! msg.IsEmpty() )	GetConfig()->Write( GetUserConfigPath(_T("/setting/auto_reply")) , msg );
			else if( GetConfig()->Exists( GetUserConfigPath(_T("/setting/auto_reply")) ) )
			{
				if( wxYES == wxMessageBox( gettext("Disable auto hot call reply?") , wxEmptyString, wxYES_NO ) )
					GetConfig()->DeleteEntry( GetUserConfigPath(_T("/setting/auto_reply")) );
			}
			}
			break;
		case MENU_AUTO_RECONNECT :
      		wxMessageBox( gettext("This program has built in auto reconnect function.\nSome BBS (like KKCity) is hard to connect for sometime. This program will retry connecting till connected. ") );
      		break;
		case MENU_PREVENT_DISCONNECT :
			EnablePreventIdle( ! isPreventIdle() );
      		break;
		case MENU_EDIT_CANNED_MESSAGE :
			{
				wxArrayString msg_list;
				wxString key, msg;
				int id;
				GetConfig()->SetPath( GetUserConfigPath(_T("/canned_msg/")) );
				for(id=1;id<=12;id++)
				{
					key = wxString::Format( _T("%d") , id );
					GetConfig()->Read( key , & msg , wxEmptyString );
					msg = wxString::Format( _T("< Ctrl+F%d >: ") , id ) + msg;
					msg_list.Add( msg );
				}
				
				id = wxGetSingleChoiceIndex( gettext("Please choose which shortcut message you want to edit.\n\n(Support BBMAN format message. Please refer [Help] How to edit BBMan format message.)") , wxEmptyString , msg_list );
				if( id == -1 )	break;
				id ++;

				key = wxString::Format( _T("%d") , id );
				GetConfig()->Read( key , & msg , wxEmptyString );

				wxString new_msg;
				new_msg = wxGetTextFromUser( gettext("Edit shortcut message.\n\n(Empty or choose [Cancel] means cancel this shortcut message.)") , wxEmptyString , msg );
				if( new_msg == msg )	break;
				else if( new_msg.IsEmpty() )
				{
					if( wxYES == wxMessageBox( gettext("Are you sure to delete this canned message?") , wxEmptyString , wxYES_NO ) )
					{
						if( GetConfig()->Exists(key) )	GetConfig()->DeleteEntry(key);
					}
				}
				else
				{
					GetConfig()->Write( key , new_msg );
				}
			}
			break;
		case MENU_MESSAGE_SOUND :
			{

#ifdef __WXGTK__
				wxString old_player, new_player;
				GetConfig()->Read( GetUserConfigPath(_T("/setting/sound_player")) , & old_player , wxEmptyString );
				new_player = wxGetTextFromUser( gettext("Please assign sound player program\n\nFor example: esdplay / artsplay / xmms.") , wxEmptyString , old_player );
				if( new_player.IsEmpty() )
				{
					wxMessageBox( gettext("You canceled assigning sound player program.") );
					break;
				}
				else GetConfig()->Write( GetUserConfigPath(_T("/setting/sound_player")) , new_player );
#endif

				wxString old_file, new_file;
				GetConfig()->Read( GetUserConfigPath(_T("/setting/sound_file")) , & old_file , wxEmptyString );
#ifdef __WXGTK__
				new_file = wxFileSelector( gettext("Open sound file") , old_file.BeforeLast('/'), old_file.AfterLast('/'), wxEmptyString , wxEmptyString , wxFD_OPEN | wxFD_FILE_MUST_EXIST , this );
#else
				new_file = wxFileSelector( gettext("Open sound file. (Only accept .wav format)") , old_file.BeforeLast('\\'), old_file.AfterLast('\\'), wxEmptyString , _T("Sound (*.wav)|*.wav") , wxFD_OPEN | wxFD_FILE_MUST_EXIST , this );
#endif
				if( ! new_file.IsEmpty() )
				{
					GetConfig()->Write( GetUserConfigPath(_T("/setting/sound_file")) , new_file );
					init_sound();
					SCD_PlaySound();
				}
				else if( ! old_file.IsEmpty() )
				{
					if( wxYES == wxMessageBox( gettext("You just cancel choosing. Do you want to cancel playing sound file, instead of system beep when receiving hot call?") , wxEmptyString, wxYES_NO ) )
					{
						GetConfig()->DeleteEntry( GetUserConfigPath(_T("/setting/sound_file")) );
						init_sound();
					}
				}
			}
			break;
		case MENU_LINE_WRAPED_LENGTH :
			{
				long len = wxGetNumberFromUser( gettext("Please set max letters for pasting ( number has to be between 5 ~ 100 )\n\nIf some lines out of max number, system sill auto wrap.\nThis function will auto detect Chinese and English character. Will not cut in word."), wxEmptyString,
							gettext("Auto wrap when paste"), SCD_Terminal::GetLineWrapedLength(), 5, 100, this );
				if(len > 0)	SetLineWrapedLength( len );
			}
			break;
		case MENU_NAWS :
			EnableNaws( ! isEnableNaws() );
			if( now_telnet )	now_telnet->OnResize();
			break;
		case MENU_MULTIBYTEWORDDETECTION :
			now_telnet->EnableMultibyteWordDecetion( ! now_telnet->isMultibyteWordDecetionEnabled() );
			break;
		case MENU_ALWAYS_HIGHLIGHT :
			{
				bool b = ! TerminalChar::getAlwaysHighlight();
				TerminalChar::setAlwaysHighlight(b);
				GetConfig()->Write( GetUserConfigPath(_T("/setting/always_highlight")) , b );
				panel->Refresh();
			}
			break;
	}
}
// ----------------------------------------------------------------------------
/*
#if defined(__WXMSW__)
	#include <windows.h>
	#include <winreg.h>
#endif
*/

void BBS_Frame::OnTool(wxCommandEvent& event)
{
	int id = event.GetId();
	switch(id)
	{

#ifndef BBMAN_NO_SSH
		case MENU_SHOW_SFTP :
			if( now_telnet && now_telnet->GetSocket()->GetType() == SOCK_SSH )
				ShowSFTP( now_telnet->GetSocket()->GetSSH() );
			break;
#endif

	    case MENU_LOCKSCREEN :
			LockScreen();
			break;
		case MENU_EXPORT_SETTING :
			{
//#if defined(__WXMSW__)
/*
				HKEY reg_key;
				if( ! RegOpenKey( HKEY_CURRENT_USER , "\\Software\\BBMan", &reg_key ) == ERROR_SUCCESS )
    			{
					wxMessageBox( _T("目前 BBMan 沒有任何的設定值") );
					return;
				}
				wxString path = wxFileSelector( _T("匯出 BBMan 設定值") , wxEmptyString, _T("BBMan設定.reg"), _T("reg") , _T("Windows 登錄檔|*.reg") , wxFD_SAVE | wxFD_OVERWRITE_PROMPT , this );
				if( path.empty() )	return;
				RegSaveKey( HKEY_CURRENT_USER , wxStringToCharPtr(path), NULL );
*/
/*
#elif defined(__WXGTK__)
				wxString path = wxFileSelector( _T("匯出 BBMan 設定值") , wxEmptyString, _T("BBMan設定值.txt"), _T("reg") , _T("所有檔案 (*.*)|*.*") , wxFD_SAVE | wxFD_OVERWRITE_PROMPT , this );
				if( path.empty() )	return false;
				wxExecute( wxString( _T("cp ~/.BBMan ") ) + path );
#endif
*/
			}
			break;
	}
}
// ----------------------------------------------------------------------------
void BBS_Frame::SetTerminalFont(wxFont fnt)
{
	if(now_telnet)
	{
		now_telnet->OnResize();
		now_telnet->repaint(true);
	}
//	FitTerminalSize();
}
// ----------------------------------------------------------------------------
void BBS_Frame::FitTerminalSize()
{
/*
	now_telnet->repaint();
    wxSize size;
    if(now_telnet)	size = now_telnet->getWindowSize();
    else size = panel->GetClientSize();
    size.SetHeight( size.GetHeight() + tab->GetSize().GetHeight() );
	SetClientSize(size);
*/
}
// ----------------------------------------------------------------------------
void BBS_Frame::recalcVarFontSize()	//重新計算 panel 可接受最大字體大小
{
/*
#if defined(__WXGTK__)	//因為 GTK 下變換字型和大小會 crash 所以暫時取消此功能
	var_fnt_size = GetCurrentFont().GetPointSize();
	return;
#endif
*/

	wxSize new_winsize = panel->GetClientSize();

	wxFont fnt = GetCurrentFont();
	wxClientDC dc(this);
	int cw, ch;

	int delta_pt;
	int var_fnt_size = GetVarFontSize();
	if( var_fnt_size < 9 )	var_fnt_size = 9;
	else if( var_fnt_size > 25 )	var_fnt_size = 25;
	fnt.SetPointSize( var_fnt_size );
	dc.SetFont(fnt);

	cw = dc.GetCharWidth();
	ch = dc.GetCharHeight();

	if( cw*80 > new_winsize.GetWidth() || ch*24 > new_winsize.GetHeight() )
		delta_pt = -1;
	else	delta_pt = 1;

	while(true)
	{
		if( var_fnt_size < 9 ) 	break;
		else if( var_fnt_size > 25 ) break;

		var_fnt_size += delta_pt;
		fnt.SetPointSize( var_fnt_size );

		dc.SetFont(fnt);

		cw = dc.GetCharWidth();
		ch = dc.GetCharHeight();

		if( delta_pt == 1 )
		{
			if( cw*80 > new_winsize.GetWidth() || ch*24 > new_winsize.GetHeight() )
			{
				var_fnt_size --;
				break;
			}
		}
		else
		{
			if( cw*80 < new_winsize.GetWidth() && ch*24 < new_winsize.GetHeight() )
				break;
		}
	}

	SetVarFontSize( var_fnt_size );

}


void BBS_Frame::OnResize(wxSizeEvent& event)
{
	Layout();

	int t = GetVarFontSize();
	recalcVarFontSize();

	if(now_telnet && t != GetVarFontSize() )
	{
		now_telnet->OnResize();
		now_telnet->repaint(true);
	}
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnGoTab(wxCommandEvent& event)
{
	int id = event.GetId();
	id = id - MENU_GOTAB_1;
	if( tab->GetSelection() != id )	tab->SetSelection( id );
}
// ----------------------------------------------------------------------------
void BBS_Frame::SendCanedMessage(int id)
{
	if( ! now_telnet )	return;
	if( id < 1 || id > 12 )	return;

	wxString key = wxString::Format( _T("/canned_msg/%d") , id );
	wxString msg;

	if(  GetConfig()->Read( key , & msg ) && ( ! msg.IsEmpty() )  )
		now_telnet->UserSend_spacial(msg);
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnCanedMessage(wxCommandEvent& event)
{
	if( ! now_telnet )	return;

	int id = event.GetId();
	id = id - MENU_CANNED_MESSAGE_1 + 1;
	SendCanedMessage(id);
}
// ----------------------------------------------------------------------------
void BBS_Frame::DeleteTerminal(int id)
{
	if( id == -1 )	id = tab->GetSelection();
	if( id < 0 || id >= tab->GetItemCount() )	return;

	int img = tab->GetItemImage(id);

	if( img == ICON_CONNECTED || img == ICON_MESSAGE )
		if( wxNO == wxMessageBox( wxString::Format( gettext("Are you sure to close connection [ %d. %s ] ?") , id+1 , wxStringToCharPtr(tab->GetItemText(id)) ) , wxEmptyString, wxYES_NO ) )
			return;

	SCD_Telnet *t;
	t = (SCD_Telnet*) tab->GetItemData(id);
	now_telnet = NULL;
	t->close();
	tab->DeleteItem(id);
	delete t;
}
// ----------------------------------------------------------------------------
void BBS_Frame::ShowSpecifiedTerminal(int id)
{	//id==-1 代表要出現對話框讓使用者選擇

	if( id == -1 )
	{
		int c = tab->GetItemCount();
		if(c <= 1)	return;

		wxString line = wxGetTextFromUser( wxString::Format( gettext("Please enter connection number : %d to %d") , 1 , c) , wxEmptyString , wxEmptyString , this);
		if( line.IsEmpty() )	return;
		else if( ! line.IsNumber() )
		{
			wxMessageBox( gettext("The number you entered is Not a number") );
			return;
		}
		else
		{
			line.ToLong( (long*) &id );
			if( id < 1 || id > tab->GetItemCount() )
			{
				wxMessageBox( gettext("The number you entered is Out-of-bound") );
				return;
			}
			else	id = id - 1;
		}
	}

	if( id != tab->GetSelection() )	tab->SetSelection(id);
}
// ----------------------------------------------------------------------------
void BBS_Frame::FullScreen()
{
	blFullScreen = ! blFullScreen;
	//請不要把 wxFULLSCREEN_NOBORDER 這個設定值取消 :)
	ShowFullScreen( blFullScreen , wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION );

	if(tb)	tb->ToggleTool( MENU_FULLSCREEN , blFullScreen );
}
// ----------------------------------------------------------------------------
void BBS_Frame::OnUpdate_UI(wxUpdateUIEvent& event)
{
	int id = event.GetId();
	switch(id)
	{
		case MENU_NAWS :
			event.Check( isEnableNaws() );
			break;
		case MENU_MULTIBYTEWORDDETECTION :
			event.Check( now_telnet && now_telnet->isMultibyteWordDecetionEnabled() );
			break;
		case MENU_ALWAYS_HIGHLIGHT :
			event.Check( TerminalChar::getAlwaysHighlight() );
			break;
		case MENU_PREVENT_DISCONNECT :
			event.Check( isPreventIdle() );
			break;
		case MENU_LOGINWHENSTART :
			event.Check( getLoginWhenStart() );
			break;
		case MENU_FULLSCREEN :
			event.Check( blFullScreen );
			break;
	}
}
// ----------------------------------------------------------------------------
void BBS_Frame::UpdateToolbarUI()
{
	if(tb==NULL)	return;

	bool b, nb, c, s;
	if( now_telnet )
	{
		c = true;
		nb = now_telnet->IsDisconnected();
		b = ! nb;
		s = now_telnet->GetSocket()->GetType() == SOCK_SSH;
		tb->EnableTool( MENU_MULTIBYTEWORDDETECTION , now_telnet->isMultibyteWordDecetionEnabled() );
	}
	else
	{
		c = b = nb = s = false;
		tb->EnableTool( MENU_MULTIBYTEWORDDETECTION , false );
	}

	tb->EnableTool( MENU_CLOSECONNECTION , b );
	tb->EnableTool( MENU_RECONNECT , nb );
	tb->EnableTool( MENU_DELETETELNET , c );
	tb->EnableTool( MENU_ADDBOOKMARK , b );
	tb->EnableTool( MENU_COPY , b );
	tb->EnableTool( MENU_COPY_ANSI , b );
	tb->EnableTool( MENU_PASTE_ANSI , b );
	tb->EnableTool( MENU_MULTIBYTEWORDDETECTION , b );
	tb->EnableTool( MENU_COPY , b );
#ifndef BBMAN_NO_SSH
	tb->EnableTool( MENU_SHOW_SFTP , s );
#endif
}
// ============================================================================
#endif
