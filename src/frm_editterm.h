/******************************************************************************
 * Name:        frm_editterm.cpp
 * Purpose:     BBS ANSI editor (only useful if you are interested in BBS)
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef FRM_EDITTERM_H
#define FRM_EDITTERM_H

#include "common.h"
#include "editterm_win.h"

// ============================================================================

class frame_EditTerm : public wxFrame
{
private:
	SCD_EditTerm *editterm;
	EditTerm_win *editterm_win;
	wxMenu *menuCharProp;

	wxString filename;

public:
	frame_EditTerm(const wxString& title, const wxPoint& pos, const wxSize& size,
	long style = wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE );


	~frame_EditTerm()
	{
//     	delete editterm;
//	 	delete editterm_win;
	}



	bool SaveFile();
	bool SaveFile(wxString _filename, wxString content);
	bool SaveNewFile();

	void OnFile(wxCommandEvent& event);
	void OnEdit(wxCommandEvent& event);
	void OnCharProperty(wxCommandEvent& event);

	void UpdateTitle();
	void SetTerminalFont(wxFont fnt);

	void OnClose(wxCloseEvent& event);
	void OnAbout(wxCommandEvent& event);

	void OnActivate(wxActivateEvent& event)
	{	editterm_win->SetFocus();	}

private:
    DECLARE_EVENT_TABLE()
};

// ============================================================================
#endif
