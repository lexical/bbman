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


#ifndef EDITTERM_WIN_CPP
#define EDITTERM_WIN_CPP
#include "editterm_win.h"
#include "frm_editterm.h"

#include <wx/caret.h>
#include "scd_editterm.h"

// ============================================================================

BEGIN_EVENT_TABLE(EditTerm_win, wxScrolledWindow)
	EVT_PAINT(EditTerm_win::OnPaint)
	EVT_KEY_DOWN(EditTerm_win::OnKeyDown)
	EVT_CHAR(EditTerm_win::OnChar)
	EVT_LEFT_DOWN(EditTerm_win::OnMouseLeftDown)
	EVT_RIGHT_DOWN(EditTerm_win::OnMouseRightDown)
	EVT_MOTION(EditTerm_win::OnMouseMotion)
	EVT_LEFT_UP(EditTerm_win::OnMouseLeftUp)
	EVT_MIDDLE_DOWN(EditTerm_win::OnMouseMiddleDown)
	EVT_TIMER(0, EditTerm_win::OnTimer)
END_EVENT_TABLE()



EditTerm_win::EditTerm_win(wxWindow *parent) : wxScrolledWindow(parent, -1, wxDefaultPosition, wxDefaultSize, wxNO_FULL_REPAINT_ON_RESIZE)
{
	SetBackgroundColour(*wxBLACK);

	caret.Create(this, 8,2);
	this->SetCaret( &caret );
	caret.Show();

	this->SetFont( parent->GetFont() );

	editterm = new SCD_EditTerm(this);
	editterm->setScrolledWindow(this);
	editterm->SetOnLinkClickedFunc( OnLinkClicked );
    editterm->Show();

	wxSize cs = editterm->getCharSize();
    SetScrollRate( cs.GetWidth(), cs.GetHeight() );
    SetVirtualSize( editterm->getWindowSize() );

	//初始化 timer
	timer.SetOwner(this, 0);
	timer.Start(600);

}
// ----------------------------------------------------------------------------
EditTerm_win::~EditTerm_win()
{
	timer.Stop();
	if(editterm)	delete editterm;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void EditTerm_win::OnKeyDown(wxKeyEvent& event)
{	editterm->OnKeyDown(event);	}
// ----------------------------------------------------------------------------
void EditTerm_win::OnChar(wxKeyEvent& event)
{	editterm->OnChar(event);	}
// ----------------------------------------------------------------------------
void EditTerm_win::OnMouseLeftDown(wxMouseEvent& event)
{  	editterm->OnMouseLeftDown(event); 	}
// ----------------------------------------------------------------------------
void EditTerm_win::OnMouseMotion(wxMouseEvent& event)
{  	editterm->OnMouseMotion(event);	}
// ----------------------------------------------------------------------------
void EditTerm_win::OnMouseLeftUp(wxMouseEvent& event)
{
	editterm->OnMouseLeftUp(event);
	editterm->CopySelectionToClipboard(false);
}
// ----------------------------------------------------------------------------
void EditTerm_win::OnMouseRightDown(wxMouseEvent& event)
{
	frame_EditTerm *frame = static_cast<frame_EditTerm*>(GetParent());
	if( frame )
		frame->ShowCharPropContextMenu(wxPoint(event.GetX(), event.GetY()));
}
// ----------------------------------------------------------------------------
void EditTerm_win::OnMouseMiddleDown(wxMouseEvent& event)
{	editterm->PasteFromClipboard(true);	}
// ----------------------------------------------------------------------------
void EditTerm_win::OnTimer(wxTimerEvent& event)
{  	editterm->setBlinkCharVisibility( ! editterm->getBlinkCharVisibility() );	}
// ----------------------------------------------------------------------------
void EditTerm_win::OnPaint(wxPaintEvent& event)
{	editterm->OnPaint(event);	}

// ============================================================================
#endif
