/******************************************************************************
 * Name:        editterm_win.cpp
 * Purpose:     a wxScrolledWindow wrapper of scd_editterm.cpp
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef EDITTERM_WIN_H
#define EDITTERM_WIN_H

#include "common.h"
#include "scd_editterm.h"
#include <wx/caret.h>

// ============================================================================

class EditTerm_win : public wxScrolledWindow
{
private:
	wxCaret caret;
	wxTimer timer;
	wxMenu *popmenu;

public:
	SCD_EditTerm *editterm;

	EditTerm_win(wxWindow *parent);
	~EditTerm_win();

	void SetPopupMenu(wxMenu *mnu);
	void OnKeyDown(wxKeyEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnMouseLeftDown(wxMouseEvent& event);
	void OnMouseMotion(wxMouseEvent& event);
	void OnMouseLeftUp(wxMouseEvent& event);
	void OnMouseRightDown(wxMouseEvent& event);
	void OnMouseMiddleDown(wxMouseEvent& event);

	void OnTimer(wxTimerEvent& event);
	void OnPaint(wxPaintEvent& event);


private:
    DECLARE_EVENT_TABLE()
};

// ============================================================================
#endif
