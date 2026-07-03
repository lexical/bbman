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


#ifndef TELNET_PANEL_CPP
#define TELNET_PANEL_CPP

#include "scd_telnet.h"
#include "frm_telnet.h"
#include "telnet_panel.h"
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(Telnet_Panel, wxWindow)
	EVT_SOCKET(0 , Telnet_Panel::OnSocketEvent)

	EVT_RIGHT_UP(Telnet_Panel::OnMouseRightUp)
	EVT_LEFT_DOWN(Telnet_Panel::OnMouseLeftDown)
	EVT_MOTION(Telnet_Panel::OnMouseMotion)
	EVT_LEFT_UP(Telnet_Panel::OnMouseLeftUp)
	EVT_MIDDLE_DOWN(Telnet_Panel::OnMouseMiddleDown)
	EVT_LEFT_DCLICK(Telnet_Panel::OnMouseLeftDoubleClick)

	EVT_PAINT(Telnet_Panel::OnPaint)
	EVT_KEY_DOWN(Telnet_Panel::OnKeyDown)
	EVT_KEY_UP(Telnet_Panel::OnKeyUp)
	EVT_CHAR(Telnet_Panel::OnChar)

	EVT_SET_FOCUS(Telnet_Panel::OnFocus)

END_EVENT_TABLE()


Telnet_Panel::Telnet_Panel()
{
	SetBackgroundColour(*wxBLACK);

	now_telnet = NULL;

	// wxWindow owns the caret after SetCaret().
	this->SetCaret( new wxCaret(this, 8, 2) );

	isAltDown = false;
}
// ----------------------------------------------------------------------------
void Telnet_Panel::DetachTelnet()
{
	now_telnet = NULL;
}
// ----------------------------------------------------------------------------
void Telnet_Panel::SetTelnet(SCD_Telnet *t)
{
	if( now_telnet )
	{	now_telnet->Hide();	}

	if( t == NULL )
	{
		now_telnet = NULL;
		return;
	}

//#ifdef __WXMSW__	//windows 下不加上這段時, 若 now_telnet 畫面比 t 大, 則會殘留 now_telnet 的畫面

	wxClientDC dc(this);
#if !wxCHECK_VERSION(2, 9, 0)
	dc.BeginDrawing();
#endif
	if( t && now_telnet )
	{
		wxSize old_ws, new_ws;
		old_ws = now_telnet->getWindowSize();
		new_ws = t->getWindowSize();

		int l = new_ws.GetWidth();
		int r = old_ws.GetWidth();
		int t = new_ws.GetHeight();
		int b = old_ws.GetHeight();
		dc.SetBrush(*wxBLACK_BRUSH);
		if(l<r)	dc.DrawRectangle(l, 0, r-l, (t>b)?t:b);
		if(t<b)	dc.DrawRectangle(0, t, (l>r)?l:r, b-t);
	}

#if !wxCHECK_VERSION(2, 9, 0)
	dc.EndDrawing();
#endif
//#endif

	now_telnet = t;

	if( now_telnet )
	{
//		now_telnet->EnableDrawing(false);
		now_telnet->Show();
//		now_telnet->EnableDrawing(true);
/*
//		wxSize new_ws = now_telnet->getWindowSize();
//		now_telnet->repaint( &dc, 0, 0, new_ws.GetWidth(), new_ws.GetHeight() );
		wxSize new_ws = GetClientSize();

		now_telnet->BeginDrawing(&dc);
		now_telnet->repaint( &dc, 0, 0, 1000, 1000 );
		now_telnet->EndDrawing();
*/
	}


#ifdef __WXGTK__	//wxGTK 下不加上這段時, console 右邊和下面會留下不少空白
	SetBackgroundColour(*wxBLACK);
#endif
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void Telnet_Panel::OnMouseRightUp(wxMouseEvent& event)
{
	BBS_Frame *frame = static_cast<BBS_Frame*>(GetParent());
	if( frame )
		frame->ShowEditContextMenu(wxPoint(event.GetX() + GetPosition().x, event.GetY() + GetPosition().y));
}
// ----------------------------------------------------------------------------
void Telnet_Panel::OnMouseLeftDown(wxMouseEvent& event)
{
	if(now_telnet == NULL)	return;
	now_telnet->OnMouseLeftDown(event);
}
// ----------------------------------------------------------------------------
void Telnet_Panel::OnMouseMotion(wxMouseEvent& event)
{
	if(now_telnet == NULL)	return;
	now_telnet->OnMouseMotion(event);
}
// ----------------------------------------------------------------------------
void Telnet_Panel::OnMouseLeftUp(wxMouseEvent& event)
{
	if(now_telnet)
	{
		now_telnet->OnMouseLeftUp(event);
		now_telnet->CopySelectionToClipboard(false);

		wxPostEvent(GetParent(), event);
	}
}
// ----------------------------------------------------------------------------
void Telnet_Panel::OnMouseMiddleDown(wxMouseEvent& event)
{
	if(now_telnet)
	{
		now_telnet->PasteFromClipboard(true);
		wxPostEvent(GetParent(), event);
	}
}
// ----------------------------------------------------------------------------
void Telnet_Panel::OnMouseLeftDoubleClick(wxMouseEvent& event)
{
	if(now_telnet)
	{
		now_telnet->OnMouseLeftDoubleClick(event);
		now_telnet->CopySelectionToClipboard(false);
	}
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void Telnet_Panel::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	if(now_telnet == NULL)	return;
	now_telnet->OnPaint(&dc);
}
// ----------------------------------------------------------------------------
void Telnet_Panel::OnKeyDown(wxKeyEvent& event)
{
#ifdef __WXGTK__	// wxGTK 下的 event.AltDown() 沒有作用，必須透過這種方式模擬
	if( event.GetKeyCode() == 307 )	isAltDown = true;
	event.m_altDown = isAltDown;
#endif

	//這必須放在前面，不然可能會被 now_telnet->OnKeyDown(event); 呼叫 event.Skip();
	GetParent()->GetEventHandler()->ProcessEvent(event);
	if(now_telnet)	now_telnet->OnKeyDown(event);
}
// ----------------------------------------------------------------------------
void Telnet_Panel::OnKeyUp(wxKeyEvent& event)
{
#ifdef __WXGTK__	// wxGTK 下的 event.AltDown() 沒有作用，必須透過這種方式模擬
	if( event.GetKeyCode() == 307 )	isAltDown = false;
#endif
}
// ----------------------------------------------------------------------------
void Telnet_Panel::OnChar(wxKeyEvent& event)
{
#ifdef __WXGTK__	// wxGTK 下的 event.AltDown() 沒有作用，必須透過這種方式模擬
	event.m_altDown = isAltDown;
#endif

	wxPostEvent(GetParent(), event);
	if(now_telnet)	now_telnet->OnChar(event);
}
// ----------------------------------------------------------------------------
void Telnet_Panel::OnSocketEvent( wxSocketEvent &e )
{ 	wxPostEvent(GetParent(), e);	}
// ----------------------------------------------------------------------------
void Telnet_Panel::OnFocus(wxFocusEvent& event)
{
#ifdef __WXGTK__	// wxGTK 下的 event.AltDown() 沒有作用，必須透過這種方式模擬
	isAltDown = false;
#endif
}

// ----------------------------------------------------------------------------
#endif
