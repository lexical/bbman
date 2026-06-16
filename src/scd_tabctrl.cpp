/******************************************************************************
 * Name:        scd_tabctrl.cpp
 * Purpose:     Tab control. wxWidgets only provide Notebook control
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef SCD_TABCTRL_CPP
#define SCD_TABCTRL_CPP

#include "scd_tabctrl.h"

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(SCD_TabCtrl, wxControl)
	EVT_LEFT_DOWN(SCD_TabCtrl::OnMouseLeftDown)
	EVT_LEFT_UP(SCD_TabCtrl::OnMouseLeftUp)
	EVT_MOTION(SCD_TabCtrl::OnMouseMove)
	EVT_RIGHT_DOWN(SCD_TabCtrl::OnMouseRightDown)
	EVT_PAINT(SCD_TabCtrl::OnPaint)

	EVT_SIZE(SCD_TabCtrl::OnResize)
	EVT_SPIN(666, SCD_TabCtrl::OnSpin)
END_EVENT_TABLE()


SCD_TabCtrl::~SCD_TabCtrl()
{
	int c = GetItemCount();
	for(int i=c-1;i>=0;i--)	DeleteItem(i);
}
// ----------------------------------------------------------------------------
void SCD_TabCtrl::Init()
{
	spin.Create(this, 666, wxDefaultPosition, wxDefaultSize, wxSP_HORIZONTAL);
	spin.SetRange(0,0);

	now_start_index = 0;
	now_selected_index = -1;
	now_right_index = 0;
	SetPadding( wxSize(6,6) );

	dragging = false;
	dragging_move_count = 0;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_TabCtrl::InsertItem(int item, const wxString& text, int imageId = -1, void* clientData = NULL)
{
	TabObject *tab = new TabObject;
	abs_SetText(tab, text);
	tab->imageId = imageId;
	tab->clientData = clientData;
	TabObjectList.Add(tab);
	recalcTabSize(tab);
	spin.SetRange(0, GetItemCount() - 1);

	Refresh();
}
// ----------------------------------------------------------------------------
bool SCD_TabCtrl::DeleteItem(int index)
{
	if( ! isTabExist(index) )	return false;

	TabObject *tab;
	tab = (TabObject*) ( TabObjectList.Item(index) );
	TabObjectList.RemoveAt(index);
	delete tab;

	if( GetItemCount() > 0 )
		spin.SetRange(0, GetItemCount() - 1);
	else
		spin.SetRange(0, 0);

	int old_index = GetSelection();

	if( old_index >= GetItemCount() )
		SetSelection( GetItemCount() - 1 );
	else if( old_index > index )
		SetSelection( old_index - 1 );
	else
		SetSelection( old_index );

	return true;
}
// ----------------------------------------------------------------------------
bool SCD_TabCtrl::DeleteItem(void *client_data)
{
	int i = TabObjectList.Index(client_data);
	if(i == wxNOT_FOUND)	return false;
	
	return DeleteItem(i);
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
int SCD_TabCtrl::SetSelection(int index)
{
	if( GetItemCount() <= 0 )
	{
		if( index == -1 );
		else return now_selected_index;
	}
	else if( ! isTabExist(index) )	return now_selected_index;
	

	if( index < now_start_index || index > now_right_index )
		now_start_index = index;

	int old_selected_index = now_selected_index;

	//產生 EVT_TAB_SEL_CHANGING 事件
	wxNotebookEvent eb(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING , GetId() , index , old_selected_index );
	GetParent()->GetEventHandler()->ProcessEvent(eb);	//這裡不能用 AddPenddingEvent() 不然會 crash

	//更新資料
	now_selected_index = index;
	Refresh();

	//產生 EVT_TAB_SEL_CHANGED 事件
	wxNotebookEvent ea(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED , GetId() , index , old_selected_index );
	GetParent()->GetEventHandler()->ProcessEvent(ea);	//這裡不能用 AddPenddingEvent() 不然會 crash

	return old_selected_index;
}
// ----------------------------------------------------------------------------
void SCD_TabCtrl::ShowPreviousTab()
{
	if( GetItemCount() <= 1 )	return;
//	else if( GetSelection() >= GetItemCount() )	return;
	else if( GetSelection() <= 0 )	SetSelection( GetItemCount() - 1 );
	else	SetSelection( GetSelection() - 1 );
}
// ----------------------------------------------------------------------------
void SCD_TabCtrl::ShowNextTab()
{
	if( GetItemCount() <= 1 )	return;
	else if( GetSelection() >= GetItemCount() - 1 )	SetSelection(0);
	else	SetSelection( GetSelection() + 1 );
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void* SCD_TabCtrl::GetItemData(int index)
{
	if( ! isTabExist(index ) ) return NULL;
	return ((TabObject*) (TabObjectList.Item( index )))->clientData;
}
// ----------------------------------------------------------------------------
int SCD_TabCtrl::GetItemIndex(void* p)
{
	int i, c;
	c = GetItemCount();
	for(i=0;i<c;i++)
  		if( p == GetItemData(i) )	return i;

	return -1;
}
// ----------------------------------------------------------------------------
int SCD_TabCtrl::GetItemImage(int item)
{
	if( ! isTabExist(item) ) return -1;
	TabObject *tab = (TabObject*) (TabObjectList.Item(item));
	return tab->imageId;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
wxString SCD_TabCtrl::GetItemText(int item)
{
	if( ! isTabExist(item) ) return wxEmptyString;
	TabObject *tab = (TabObject*) (TabObjectList.Item(item));
	return tab->text;
}
// ----------------------------------------------------------------------------
bool SCD_TabCtrl::SetItemText(int item, const wxString& text)
{
	if( ! isTabExist(item) ) return false;
	TabObject *tab = (TabObject*) (TabObjectList.Item(item));

	abs_SetText(tab, text);

	recalcTabSize(item);
	Refresh();
	return true;
}
// ----------------------------------------------------------------------------
void SCD_TabCtrl::abs_SetText(TabObject *tab, wxString text)
{
	tab->text = text;

	wxClientDC dc(this);	//*** need to optimization ***//
	dc.GetTextExtent( wxString( _T("00. ") ) + text , & tab->text_width , & tab->text_height );
}
// ----------------------------------------------------------------------------
bool SCD_TabCtrl::SetItemImage(int item, int image)
{
	if( ! isTabExist(item) ) return false;
	TabObject *tab = (TabObject*) (TabObjectList.Item(item));
	tab->imageId = image;
	recalcTabSize(item);
	Refresh();
	return true;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_TabCtrl::recalcTabSize(int index)
{
	if( ! isTabExist(index) )	return;
	recalcTabSize( (TabObject*)(TabObjectList.Item(index)) );
}
// ----------------------------------------------------------------------------
void SCD_TabCtrl::recalcTabSize(TabObject *tab)
{
	int img_w = 0 , img_h = 0;
	int _w = 0, _h = 0;

	if( image_list && tab->imageId >= 0 )
		image_list->GetSize( tab->imageId , img_w , img_h );

	_w = img_w + tab->text_width;
	_h = (tab->text_height > img_h) ? tab->text_height : img_h;

#ifdef __WXMSW__
	tab->width  = _w + padding.GetWidth();
#else
	tab->width  = _w + padding.GetWidth() * 2 + 6;
#endif
	tab->height = _h + padding.GetHeight() * 2;
}
// ----------------------------------------------------------------------------
inline bool SCD_TabCtrl::isTabVisible(int index)
{
	if( ! isTabExist(index) )	return false;
	return true;
}
// ----------------------------------------------------------------------------
int SCD_TabCtrl::getTabUnderMouse(int x, int y)	//取得位於座標 (x,y) 下的按鈕
{
	int button_right = 0;
	for(int i=now_start_index ; isTabVisible(i) ; i++)
	{
		TabObject *tab = (TabObject*)(TabObjectList.Item(i));
		button_right += tab->width;
		if( x < button_right )	return i;
	}
	return -1;
}
// ----------------------------------------------------------------------------
int SCD_TabCtrl::DrawTab(int index , wxDC *dc , int start_x)
{
	if( ! isTabVisible(index) )	return start_x;

	TabObject *tab = (TabObject*)(TabObjectList.Item(index));
	
	//畫上按鈕的底色
	if( index == now_selected_index )
 	{
	    dc->SetBrush( wxBrush( wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT) , wxSOLID ) );
		dc->SetPen( wxPen( wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW), 1, wxSOLID ) );
		dc->DrawRoundedRectangle( start_x + 1 , 2 , tab->width - 5, GetClientSize().GetHeight() - 4, 8 );
	}

//	int _x = start_x + padding.GetWidth();
//	int _y = padding.GetHeight();

	//畫上 tab 的圖示 (如果有指定 ImageList 的話)
	int img_w = 0, img_h = 0;
	if( image_list && tab->imageId >= 0 )
	{
		image_list->GetSize( tab->imageId , img_w , img_h );
		int img_y = (tab->height - img_h) / 2;
		image_list->Draw( tab->imageId, *dc, start_x + padding.GetWidth(), img_y, wxIMAGELIST_DRAW_TRANSPARENT, true);
	}

	//畫上標題
	int text_y = (tab->height - tab->text_height) / 2;
	dc->SetTextForeground(*wxBLACK);
	dc->DrawText( wxString::Format( _T("%d. ") , index + 1 ) + tab->text, start_x + padding.GetWidth() + img_w + 6 , text_y );
	
	//畫上右邊界
	int right = start_x + tab->width;
	dc->SetPen( wxPen( wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW), 1, wxSOLID ) );
	dc->DrawLine(right-2, 0, right-2, tab->height);
	dc->SetPen( wxPen( wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT), 1, wxSOLID ) );
	dc->DrawLine(right-1, 0, right-1, tab->height);

	return right;	//傳回右邊界
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_TabCtrl::OnMouseLeftDown(wxMouseEvent& event)
{
	int m_x, m_y;
	m_x = event.GetX();
	m_y = event.GetY();

	int old_selection = GetSelection();
	int new_selection = getTabUnderMouse( m_x , m_y );
	if( new_selection >= 0 && new_selection != old_selection )
		SetSelection(new_selection);

	//拖曳 tab
	drag_tab_index = new_selection;
	dragging = true;
	dragging_move_count = 0;
}
// ----------------------------------------------------------------------------
void SCD_TabCtrl::OnMouseLeftUp(wxMouseEvent& event)
{
	//拖曳 tab

	if(!dragging)	return;
	dragging = false;
	if(dragging_move_count < 6)	return;
	SetCursor(wxNullCursor);
	dragging_move_count = 0;
	int drag_tar_tab = getTabUnderMouse( event.GetX(), event.GetY() );
	if( drag_tar_tab == drag_tab_index )	return;

	if( isTabExist(drag_tar_tab) && isTabExist(drag_tab_index) );
	else return;

	TabObject *tab;
	tab = (TabObject*) ( TabObjectList.Item(drag_tab_index) );
	TabObjectList.RemoveAt(drag_tab_index);
	TabObjectList.Insert(tab, drag_tar_tab);

	SetSelection(drag_tar_tab);
}
// ----------------------------------------------------------------------------
void SCD_TabCtrl::OnMouseMove(wxMouseEvent& event)
{
	if(!dragging)	return;
	if(dragging_move_count < 6)	dragging_move_count ++;
	else	SetCursor(wxCURSOR_SIZING);
}
// ----------------------------------------------------------------------------
void SCD_TabCtrl::OnMouseRightDown(wxMouseEvent& event)
{	wxPostEvent(GetParent(), event);	}
// ----------------------------------------------------------------------------
void SCD_TabCtrl::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	dc.SetFont( GetFont() );

	int win_width, win_height;
	GetClientSize( & win_width, & win_height );
	win_height = 30;

	if( now_start_index >= GetItemCount() && GetItemCount() > 0 )
 		now_start_index = GetItemCount() - 1;

#if !wxCHECK_VERSION(2, 9, 0)
	dc.BeginDrawing();
#endif

	int start_x, i;
	for(start_x = 0 , i = now_start_index ; start_x < win_width && isTabExist(i) ;i++)
		start_x = DrawTab(i, &dc , start_x);

#if !wxCHECK_VERSION(2, 9, 0)
	dc.EndDrawing();
#endif

	if( start_x < win_width )	now_right_index = i + 1;
	else	now_right_index = i - 2;
}
// ----------------------------------------------------------------------------
void SCD_TabCtrl::OnResize(wxSizeEvent& event)
{
	spin.Move( GetClientSize().GetWidth() - spin.GetSize().GetWidth() , 0 );
}
// ----------------------------------------------------------------------------
void SCD_TabCtrl::OnSpin(wxSpinEvent& event)
{
	if( now_start_index != event.GetPosition() )
	{
		now_start_index = event.GetPosition();
		Refresh();
	}
}
// ----------------------------------------------------------------------------
#endif
