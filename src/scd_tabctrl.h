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


#ifndef SCD_TABCTRL_H
#define SCD_TABCTRL_H

#include "common.h"
#include "scd_terminal.h"
#include <wx/arrimpl.cpp>
#include <wx/notebook.h>
#include <wx/imaglist.h>
#include <wx/spinbutt.h>

// ============================================================================

class SCD_TabCtrl : public wxControl
{
public:
	~SCD_TabCtrl();

	void Init();

	void InsertItem(int item, const wxString& text, int imageId, void* clientData);
	bool DeleteItem(int index);
	bool DeleteItem(void *client_data);

	void ShowPreviousTab();
	void ShowNextTab();

	inline int GetSelection()	{	return now_selected_index;	}
	inline int GetItemCount()	{	return TabObjectList.Count();	}
	void* GetItemData(int index);
	int GetItemIndex(void* p);
	int GetItemImage(int item);

	int SetSelection(int index);
	wxString GetItemText(int item);
	bool SetItemText(int item, const wxString& text);
	bool SetItemImage(int item, int image);


	void SetImageList(wxImageList* list)	{	image_list = list;	}
	void SetPadding(const wxSize& p)
 	{
  		padding = p;
  		SetSize( GetClientSize().GetWidth(), GetCharHeight() + p.GetHeight() * 2 );
	}

	int getTabUnderMouse(int x, int y);	//取得位於座標 (x,y) 下的按鈕

private:

	typedef struct
	{
		wxString text;
		void *clientData;
		int imageId;

		//如果 text 或 image 有變動, 即 tab 的外表有變動的話, 則設為 true,
  		//並且強制重新計算 tab 的 width height
		bool outlook_changed;
		int width;
		int height;

		int text_width, text_height;

	} TabObject;

	wxPointerArray TabObjectList;
	int now_start_index, now_selected_index, now_right_index;
	wxImageList *image_list;
	wxSize padding;

	wxSpinButton spin;

	void abs_SetText(TabObject *tab, wxString text);

	inline bool isTabVisible(int index);
	inline int isTabExist(int index)	{	return index >= 0 && index < GetItemCount();	}
	int DrawTab(int index , wxDC *dc , int start_x);
	void recalcTabSize(int index);
	void recalcTabSize(TabObject *tab);
	void repaint(wxDC *dc);

	void OnResize(wxSizeEvent& event);
	void OnSpin(wxSpinEvent& event);

private:
	int drag_tab_index, dragging_move_count;
	bool dragging;
public:
	void OnMouseLeftDown(wxMouseEvent& event);
	void OnMouseLeftUp(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);

	void OnMouseRightDown(wxMouseEvent& event);
	void OnPaint(wxPaintEvent& event);

    DECLARE_EVENT_TABLE()
};

// ============================================================================
#endif
