/******************************************************************************
 * Name:        telnet_panel.cpp
 * Purpose:     I can't type on wxFrame in FreeBSD + wxGTK + gtk1, so need this
                  to accept the keydown event
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef TELNET_PANEL_H
#define TELNET_PANEL_H

#include "common.h"

// ============================================================================

class Telnet_Panel : public wxWindow
{
private:
	bool isAltDown;

public:
	Telnet_Panel();
	void SetTelnet(SCD_Telnet *t);

	void OnMouseRightUp(wxMouseEvent& event);
	void OnMouseLeftDown(wxMouseEvent& event);
	void OnMouseMotion(wxMouseEvent& event);
	void OnMouseLeftUp(wxMouseEvent& event);
	void OnMouseMiddleDown(wxMouseEvent& event);
	void OnMouseLeftDoubleClick(wxMouseEvent& event);

	void OnPaint(wxPaintEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnChar(wxKeyEvent& event);

	void OnSocketEvent( wxSocketEvent &e );
	void OnFocus(wxFocusEvent& event);


private:

	SCD_Telnet *now_telnet;
	wxCaret caret;

    DECLARE_EVENT_TABLE()
};

// ============================================================================

#endif
