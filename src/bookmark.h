/******************************************************************************
 * Name:        bookmark.cpp
 * Purpose:     organize / edit bookmarks
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef BOOKMARK_H
#define BOOKMARK_H
#include "common.h"

#include "ds.h"
#include <wx/treectrl.h>
#include <wx/dialog.h>
#include <wx/radiobut.h>
#include <wx/hashmap.h>

// ----------------------------------------------------------------------------

bool ShowFavoriteEditor(wxWindow *parent = NULL, SiteInfo *si = NULL);
WX_DECLARE_HASH_MAP( int, wxString, wxIntegerHash, wxIntegerEqual, t_int_wxString_hash );

class frm_Favorite_Edit : public wxDialog
{
private:
	wxTreeCtrl tree;
	wxTreeItemId  list_root;
	wxTextCtrl *txtName, *txtIP, *txtPort, *txtProtocol, *txtUsername, *txtPassword, *txtMessage;
	wxCheckBox *chkAutoLogin, *chkAutoConnectAtStartup;
	wxRadioBox *rdbxProtocol;

	wxTreeItemId drag_src_item;

	bool bl_selected_item_setting_changed;
	bool bl_favorite_modified;
	void CopyTreeItem(wxTreeItemId src, wxTreeItemId tar);
	bool isParentNode(wxTreeItemId parent, wxTreeItemId child);

	//這些是為了解決站台資訊無法儲存在 node 中的問題
	t_int_wxString_hash node_name_map;
	wxString m_GetNodeText(wxTreeItemId node);
	void m_SetNodeText(wxTreeItemId node, wxString s);
	wxTreeItemId m_AppendItem(const wxTreeItemId& parent, const wxString& text, int image = -1);
	wxTreeItemId m_PrependItem(const wxTreeItemId& parent, const wxString& text, int image = -1);
	wxTreeItemId m_InsertItem(const wxTreeItemId& parent, const wxTreeItemId& previous, const wxString& text, int image = -1);

public:
	frm_Favorite_Edit(wxWindow *parent);
	~frm_Favorite_Edit();

	void LoadFavorites();
	void LoadFavorites(wxTreeItemId parent, wxString config_path);
	void SaveFavorites();
	void SaveFavorites(wxTreeItemId parent, wxString config_path);

	void AddFavorite(SiteInfo *si);

	bool save_site();
	bool IsFavoriteModified()	{	return bl_favorite_modified;	}

	void OnButton(wxCommandEvent& event);
	void OnDataChanged(wxCommandEvent& event);

	void OnItemSelecting(wxTreeEvent& event);
	void OnItemSelected(wxTreeEvent& event);
	void OnTreeBeginDrag(wxTreeEvent& event);
	void OnTreeEndDrag(wxTreeEvent& event);
	void OnTreeKeyDown(wxTreeEvent& event);

	void OnClose(wxCloseEvent& event);

private:
    DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------

bool ShowBBSListDialog(wxString &_addr, wxString &_name);

enum { MENU_OPENSITE };

class frm_BBSList : public wxDialog
{
public:
	frm_BBSList();

	wxString getName()	{	return name;	}
	wxString getIP()	{	return ip;	}
	int getPort()	{	return port;	}

private:
	wxTreeCtrl tree;
	wxTreeItemId  list_root;
	wxTextCtrl txtSearch;
	wxButton btnOK, btnCancel, btnSearch;
	

	wxString name, ip;
	int port;

	void OnSearch(wxCommandEvent& event);
	void OnMouseDoubleClick(wxMouseEvent& event);

	bool LoadSiteList(wxTreeItemId  rootId);
	void OnOpenSite(wxCommandEvent& event);

public:
	bool getSiteInfo(wxString &_addr, wxString &_name);

private:
    DECLARE_EVENT_TABLE()
};
// ----------------------------------------------------------------------------
#endif
