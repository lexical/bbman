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


#ifndef FRM_TELNET_H
#define FRM_TELNET_H
#include "common.h"

#include <wx/caret.h>

#include <wx/icon.h>
#include <wx/bitmap.h>
#include <wx/imaglist.h>

#include <wx/textctrl.h>
#include <wx/notebook.h>
#include <wx/scrolwin.h>
#include <wx/fontdlg.h>
#include <wx/utils.h>	//::wxGetHostName()
#include <wx/choicdlg.h>	//::wxGetMultipleChoice()

#include "scd_tabctrl.h"
#include "scd_telnet.h"
#include "telnet_panel.h"
#include "ds.h"

// ============================================================================

class BBS_Frame : public wxFrame
{
public:
	wxToolBar *tb;
 	Telnet_Panel *panel;
	wxTextCtrl *txtAddress;
	wxStaticText *stxtHostname;

private:
	SCD_Telnet *now_telnet;
	wxScrolledWindow scrollwin;

	wxTimer timer;
 	SCD_TabCtrl *tab;
 	wxImageList image_list;

	wxMenu *editMenu;
	wxMenu *menuBookmark;

	bool blFullScreen;

	bool isClosed;

	void CloseAllTerminals();
	void ReloadBookmark();	//讀取書籤列表
	void ReloadBookmark(wxMenu *parent, wxString config_path, int &id);

	void ShowPreviousTelnet();
	void ShowNextTelnet();

	int  getNowTelnetIndex();
	int  getTelnetCount();

public:
	// ctor(s)
	BBS_Frame(const wxString& title, const wxPoint& pos, const wxSize& size,
	long style = wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE );
	~BBS_Frame();

	void ShowSpecifiedTerminal(int id = -1);
	void DeleteTerminal(int id = -1 );
	void SetTerminalFont(wxFont fnt);
	void FitTerminalSize();	//在字形改變或切換連線時可呼叫此函式, 讓視窗大小隨著 terminal 設定而改變

	void connect( wxString addr, wxString name = wxEmptyString );
	void connect(int protocol, wxString ip, int port, wxString name);
	void connect(SiteInfo &si);

	void QuickConnect();
	void OnAddressTextEnterText(wxCommandEvent& event);
	void OnAddressTextSetFocus(wxFocusEvent& event);

	void SendCanedMessage(int id);

	// event handlers (these functions should _not_ be virtual)

	void OnSocketEvent( wxSocketEvent &e );

	void OnTimer(wxTimerEvent& event);

	void OnMouseRightDown(wxMouseEvent& event);
	void OnMouseRightUp(wxMouseEvent& event);
	void ShowEditContextMenu(const wxPoint& pos);

	void OnMouseLeftUp(wxMouseEvent& event);
	void OnMouseMiddleDown(wxMouseEvent& event);


	void OnConnection(wxCommandEvent& event);

	void OnCopy(wxCommandEvent& event);
	void OnCopy_withANSI(wxCommandEvent& event);
	void OnPaste(wxCommandEvent& event);
	void OnPaste_withANSI(wxCommandEvent& event);

	void OnSelectAll(wxCommandEvent& event);
	void OnOpenSelectionAsHyperlink(wxCommandEvent& event)
	{	if(now_telnet)	now_telnet->OpenSelectionAsHyperlink();	}
	void OnInputLine(wxCommandEvent& event);

	void OnAddBookmark(wxCommandEvent& event);
	void OnEditBookmark(wxCommandEvent& event);
	void OnBookmark(wxCommandEvent& event);

	void OnSetting(wxCommandEvent& event);
	void OnTool(wxCommandEvent& event);

	void OnGoTab(wxCommandEvent& event);
	void OnCanedMessage(wxCommandEvent& event);

	void FullScreen();

	void OnChar(wxKeyEvent& event);
	void OnKeyDown(wxKeyEvent& event);

	void OnMenuOpen(wxMenuEvent& event);
	void OnFocus(wxFocusEvent& event);

	void OnQuit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnActivate(wxActivateEvent& event);

	void OnTabChanged(wxNotebookEvent& event);

	void recalcVarFontSize();	//重新計算 panel 可接受最大字體大小
	void OnResize(wxSizeEvent& event);

	void OnOpenAnsiEditor(wxCommandEvent& event)
	{	ShowAnsiEditor();	}

	void OnUpdate_UI(wxUpdateUIEvent& event);
	void UpdateToolbarUI();

private:
	// any class wishing to process wxWindows events must use this macro
	DECLARE_EVENT_TABLE()
};

// ============================================================================
#endif
