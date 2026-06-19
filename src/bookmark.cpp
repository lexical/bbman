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


#ifndef BOOKMARK_CPP
#define BOOKMARK_CPP
#include "bookmark.h"

#include <wx/imaglist.h>
#include <wx/file.h>
#include <wx/statline.h>
#include "login.h"

enum { ICON_DIR, ICON_SITE };
enum { MENU_BEGIN, RDBX_PROTOCOL, CHK_AUTOLOGIN, CHK_AUTOCONNECT,
		MENU_MOVEUP, MENU_MOVEDOWN, MENU_NEWSITE, MENU_MKDIR, MENU_COPY, MENU_DELETE,
		MENU_SAVE_EXIT, MENU_END };
enum { ID_TEXT_BEGIN, ID_TEXT_NAME, ID_TEXT_IP, ID_TEXT_PORT, ID_TEXT_USERNAME, ID_TEXT_PASSWORD, ID_TEXT_MESSAGE, ID_TEXT_END };
// ============================================================================

static wxString SiteListBig5ToWxString(const char *text)
{
	static wxCSConv big5_conv(_T("BIG5"));
	wxString value(text, big5_conv);
	if( value.IsEmpty() && text[0] != '\0' )
		value = wxString::FromUTF8(text);
	value.Trim();
	value.Trim(false);
	return value;
}

bool ShowFavoriteEditor(wxWindow *parent, SiteInfo *si)
{
	frm_Favorite_Edit frm(parent);
	frm.LoadFavorites();
	if(si)	frm.AddFavorite(si);
	frm.ShowModal();
	bool ret = frm.IsFavoriteModified();

	return ret;
}

// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(frm_Favorite_Edit, wxDialog)
	EVT_COMMAND_RANGE(ID_TEXT_BEGIN, ID_TEXT_END, wxEVT_COMMAND_TEXT_UPDATED, frm_Favorite_Edit::OnDataChanged)
	EVT_CHECKBOX(CHK_AUTOLOGIN, frm_Favorite_Edit::OnDataChanged)
	EVT_CHECKBOX(CHK_AUTOCONNECT, frm_Favorite_Edit::OnDataChanged)
	EVT_RADIOBOX(RDBX_PROTOCOL, frm_Favorite_Edit::OnDataChanged)

	EVT_COMMAND_RANGE(MENU_BEGIN, MENU_END, wxEVT_COMMAND_BUTTON_CLICKED, frm_Favorite_Edit::OnButton)

	EVT_TREE_SEL_CHANGING( 0, frm_Favorite_Edit::OnItemSelecting )
	EVT_TREE_SEL_CHANGED( 0, frm_Favorite_Edit::OnItemSelected )
	EVT_TREE_BEGIN_DRAG( 0, frm_Favorite_Edit::OnTreeBeginDrag )
	EVT_TREE_END_DRAG( 0, frm_Favorite_Edit::OnTreeEndDrag )
	EVT_TREE_KEY_DOWN( 0, frm_Favorite_Edit::OnTreeKeyDown )

	EVT_CLOSE(frm_Favorite_Edit::OnClose)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
wxString frm_Favorite_Edit::m_GetNodeText(wxTreeItemId node)
{	return node_name_map[node.GetID()];	}
// ----------------------------------------------------------------------------
void frm_Favorite_Edit::m_SetNodeText(wxTreeItemId node, wxString s)
{
	node_name_map[node.GetID()] = s;
	if( tree.GetItemImage(node) == ICON_DIR )
		tree.SetItemText(node, s);
	else
	{
		SiteInfo si;
		si.Set(s);
		tree.SetItemText(node, si.name);
	}
}
// ----------------------------------------------------------------------------
wxTreeItemId frm_Favorite_Edit::m_AppendItem(const wxTreeItemId& parent, const wxString& text, int image)
{
	wxTreeItemId new_node = tree.AppendItem(parent, text, image);
	m_SetNodeText( new_node, text );
	return new_node;
}
// ----------------------------------------------------------------------------
wxTreeItemId frm_Favorite_Edit::m_PrependItem(const wxTreeItemId& parent, const wxString& text, int image)
{
	wxTreeItemId new_node = tree.PrependItem(parent, text, image);
	m_SetNodeText( new_node, text );
	return new_node;
}
// ----------------------------------------------------------------------------
wxTreeItemId frm_Favorite_Edit::m_InsertItem(const wxTreeItemId& parent, const wxTreeItemId& previous, const wxString& text, int image)
{
	wxTreeItemId new_node = tree.InsertItem(parent, previous, text, image);
	m_SetNodeText( new_node, text );
	return new_node;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void frm_Favorite_Edit::OnClose(wxCloseEvent& event)
{
	if( IsModal() )
		EndModal(wxID_CANCEL);
	else
		event.Skip();
}
// ----------------------------------------------------------------------------
void frm_Favorite_Edit::OnTreeBeginDrag(wxTreeEvent& event)
{
//	wxMessageBox( m_GetNodeText( event.GetItem() ) );
	drag_src_item = event.GetItem();
	event.Allow();
}
// ----------------------------------------------------------------------------
void frm_Favorite_Edit::OnTreeEndDrag(wxTreeEvent& event)
{
//	wxMessageBox( m_GetNodeText( event.GetItem() ) );
	wxTreeItemId drag_tar_item = event.GetItem();
	if( isParentNode( drag_src_item , drag_tar_item ) )
	{
		wxMessageBox( gettext("Can't move item to its child node!") );
		return;
	}

	wxTreeItemId new_item, prev_tar_item;
	int src_item_img = tree.GetItemImage(drag_src_item);

	prev_tar_item = tree.GetPrevSibling(drag_tar_item);
	if( !prev_tar_item.IsOk() )	prev_tar_item = drag_tar_item;

	new_item = m_InsertItem( tree.GetItemParent(prev_tar_item), prev_tar_item,
		m_GetNodeText(drag_src_item), src_item_img );
	if( src_item_img == ICON_DIR )	CopyTreeItem(drag_src_item, new_item);
	tree.Delete(drag_src_item);
	tree.SelectItem(new_item);
}
// ----------------------------------------------------------------------------
bool frm_Favorite_Edit::isParentNode(wxTreeItemId parent, wxTreeItemId child)
{
	while(true)
	{
		if(!child.IsOk())	return false;
		else if(child == parent)	return true;
		child = tree.GetItemParent(child);
	}
}
// ----------------------------------------------------------------------------
void frm_Favorite_Edit::OnTreeKeyDown(wxTreeEvent& event)
{
	int key = event.GetKeyCode();
	if( key == WXK_DELETE )
	{
		wxCommandEvent e;
		e.SetId(MENU_DELETE);
		OnButton(e);
	}
}
// ----------------------------------------------------------------------------
void frm_Favorite_Edit::OnDataChanged(wxCommandEvent& event)
{
//	wxMessageBox("Text Changed");
	bl_selected_item_setting_changed = true;
//	save_site();

	int id = event.GetId();
	switch(id)
	{
		case RDBX_PROTOCOL :
			{
				int default_port;
				if( rdbxProtocol->GetStringSelection().CompareTo( _T("SSH") ) == 0 )
					default_port = 22;
				else
					default_port = 23;

				txtPort->SetValue( wxString::Format("%d", default_port) );
			}
			break;
		case CHK_AUTOLOGIN :
			{
				bool b = chkAutoLogin->GetValue();
				txtUsername->Enable(b);
				txtPassword->Enable(b);
			}
			break;
			
	}

}
// ----------------------------------------------------------------------------
void frm_Favorite_Edit::LoadFavorites()
{
    wxString root_path = GetUserConfigPath( _T("/bookmark") );
	LoadFavorites(list_root, root_path);
}
// ----------------------------------------------------------------------------
void frm_Favorite_Edit::LoadFavorites(wxTreeItemId parent, wxString config_path)
{
	wxString name, str;
	long id;
	bool ret;
	wxTreeItemId child;

	//讀取資料夾
 	GetConfig()->SetPath(config_path);
	ret = GetConfig()->GetFirstGroup( name , id );
	while( ret )
	{
		str = name.AfterFirst(' ');
		child = m_AppendItem(parent, str, ICON_DIR);
		LoadFavorites( child, config_path + _T("/") + name );

		GetConfig()->SetPath(config_path);
		ret = GetConfig()->GetNextGroup( name , id );
	}

	//讀取 site
 	GetConfig()->SetPath(config_path);
	ret = GetConfig()->GetFirstEntry( name , id );
	while( ret )
	{
		if( GetConfig()->Read( name , &str ) )
			m_AppendItem(parent, str, ICON_SITE);
		GetConfig()->SetPath(config_path);
		ret = GetConfig()->GetNextEntry( name , id );
	}

	bl_favorite_modified = false;
}
// ----------------------------------------------------------------------------
void frm_Favorite_Edit::SaveFavorites()
{
    wxString root_path = GetUserConfigPath( _T("/bookmark") );
	GetConfig()->DeleteGroup(root_path);
	SaveFavorites(list_root, root_path);
	if( ! GetConfig()->Flush() )
		wxLogWarning(_T("Unable to save favorites."));

	bl_favorite_modified = true;
}
// ----------------------------------------------------------------------------
void frm_Favorite_Edit::SaveFavorites(wxTreeItemId parent, wxString config_path)
{
 	GetConfig()->SetPath( config_path );

	wxTreeItemIdValue cookie;
	wxTreeItemId child;
	wxString child_text, child_name, child_path;
	int index = 0;
//	m_SetNodeText(tar, m_GetNodeText(src) );
	child = tree.GetFirstChild(parent, cookie);
	while(child.IsOk())
	{
		child_text = m_GetNodeText(child);
     	int child_img = tree.GetItemImage(child);

		if(child_img == ICON_DIR)
			child_name = wxString::Format("%03d ", index) + child_text;
		else //if(child_img == ICON_SITE)
		{
			SiteInfo si;
			si.Set(child_text);
			child_name = wxString::Format("%03d ", index) + si.name;
		}

     	child_path = config_path + _T("/") + child_name;
		if( child_img == ICON_DIR )
  		{
			if( tree.ItemHasChildren(child) )
				SaveFavorites(child, child_path);
		}
		else
			GetConfig()->Write( child_path, child_text );
			
		child = tree.GetNextChild(parent, cookie);

		index ++;
	}

}
// ----------------------------------------------------------------------------
void frm_Favorite_Edit::AddFavorite(SiteInfo *si)
{
	wxTreeItemId new_item = m_AppendItem( list_root, si->Get(), ICON_SITE );
	tree.SelectItem(new_item);
	txtName->SetSelection(-1,-1);
	if( si->protocol == SOCK_SSH )
		rdbxProtocol->SetStringSelection(_T("SSH"));	//這行會使 focus 跑到 rdbxProtocol...
	txtName->SetFocus();
	bl_selected_item_setting_changed = true;
}
// ----------------------------------------------------------------------------
bool frm_Favorite_Edit::save_site()
{
	if( ! bl_selected_item_setting_changed )	return true;

	wxTreeItemId sel_item;
	sel_item = tree.GetSelection();
	if(!sel_item.IsOk())	return false;

	wxString site_desc = m_GetNodeText( sel_item );
	int sel_item_img = tree.GetItemImage(sel_item);

	if(sel_item_img == ICON_DIR)
		m_SetNodeText( sel_item, txtName->GetValue() );
	else //if(sel_item_img == ICON_SITE)
	{
		SiteInfo si;
		si.Init();

		si.name = txtName->GetValue();
		si.ip = txtIP->GetValue();
		if( ! txtPort->GetValue().ToLong( (long*)&si.port ) )
			si.port = 23;
		if( rdbxProtocol->GetStringSelection().CompareTo( _T("SSH") ) == 0 )
			si.protocol = SOCK_SSH;
		else si.protocol = SOCK_TELNET;
		si.autoopen = chkAutoConnectAtStartup->GetValue();
		if( chkAutoLogin->GetValue() )
		{
			si.username = txtUsername->GetValue();
			si.password = txtPassword->GetValue();
		}
		si.message = txtMessage->GetValue();

		m_SetNodeText( sel_item, si.Get() );
	}

	bl_selected_item_setting_changed = false;
	return true;
}
// ----------------------------------------------------------------------------
void frm_Favorite_Edit::OnButton(wxCommandEvent& event)
{
	int id = event.GetId();
	switch(id)
	{
		case MENU_MOVEDOWN :
		case MENU_MOVEUP :
			{
				wxTreeItemId sel_item, next_item, new_item;
				sel_item = tree.GetSelection();
				if( !sel_item.IsOk() )	break;
				int sel_item_img = tree.GetItemImage(sel_item);
				wxString site_desc = m_GetNodeText( sel_item );

				if(sel_item_img == ICON_DIR)	tree.Collapse(sel_item);

				next_item = sel_item;
				if(id == MENU_MOVEDOWN )
				{
					next_item = tree.GetNextVisible(next_item);
					if( !next_item.IsOk() )	return;
					if( tree.GetItemImage(next_item) == ICON_DIR )	//如果 next_item 是資料夾, 則新增至 next_item 底下成為第一個 child
						new_item = m_PrependItem(next_item, site_desc, sel_item_img);
					else	//新增至 next_item 之後
						new_item = m_InsertItem( tree.GetItemParent(next_item), next_item, site_desc, sel_item_img);
				}
				else
				{
					next_item = tree.GetPrevVisible(next_item);
					if( !next_item.IsOk() )	return;
					next_item = tree.GetPrevVisible(next_item);
					if( !next_item.IsOk() )	return;
					if( tree.GetItemImage(next_item) == ICON_DIR )	//如果 next_item 是資料夾, 則新增至 next_item 底下成為第一個 child
						new_item = m_AppendItem(next_item, site_desc, sel_item_img);
					else	//新增至 next_item 之後
					{
						new_item = m_InsertItem( tree.GetItemParent(next_item), next_item, site_desc, sel_item_img);
					}
				}


//				tmp_item = m_InsertItem( tree.GetItemParent(next_item) , next_item, site_desc, sel_item_img );
				if( sel_item_img == ICON_DIR ) CopyTreeItem(sel_item, new_item); //如果是資料夾則複製整個資料夾結構
				tree.Delete(sel_item);
				tree.SelectItem(new_item);
			}
			break;
		case MENU_NEWSITE :
			{
				SiteInfo si;
				si.Init();
				si.name = _T("New Site");

				wxTreeItemId sel_item;
				sel_item = tree.GetSelection();
				sel_item = m_InsertItem( tree.GetItemParent(sel_item) , sel_item, si.Get(), ICON_SITE );

				tree.SelectItem(sel_item);
				txtName->SetSelection(-1,-1);
				txtName->SetFocus();
				bl_selected_item_setting_changed = true;
			}
			break;
		case MENU_MKDIR :
			{
				wxTreeItemId sel_item;
				sel_item = tree.GetSelection();
				sel_item = m_InsertItem( tree.GetItemParent(sel_item) , sel_item, _T("New Folder"), ICON_DIR );

				tree.SelectItem(sel_item);
				txtName->SetSelection(-1,-1);
				txtName->SetFocus();
				bl_selected_item_setting_changed = true;
			}
			break;
		case MENU_COPY :
			{
				wxTreeItemId sel_item, new_item;
				sel_item = tree.GetSelection();
				if(!sel_item.IsOk())	return;
				int sel_item_img = tree.GetItemImage(sel_item);
				new_item = m_InsertItem( tree.GetItemParent(sel_item), sel_item, m_GetNodeText(sel_item), sel_item_img );
				if(sel_item_img == ICON_DIR)	CopyTreeItem(sel_item, new_item);
				tree.SelectItem(new_item);
				txtName->SetSelection(-1,-1);
				txtName->SetFocus();
			}
			break;
		case MENU_DELETE :
			{
				wxTreeItemId sel_item;
				sel_item = tree.GetSelection();
				wxString site_desc = m_GetNodeText( sel_item );
				int sel_item_img = tree.GetItemImage(sel_item);

				int ret;
				if( sel_item_img == ICON_DIR )
					ret = wxMessageBox( site_desc + _T("\n") + gettext("Delete this folder, including all of its children ?") , wxEmptyString, wxYES_NO );
				else
				{
					SiteInfo si;
					si.Init();
					si.Set(site_desc);
					ret = wxMessageBox( si.name + _T("\n") + gettext("Delete this site?") , wxEmptyString, wxYES_NO );
				}
				if(ret == wxYES)
				{
					wxTreeItemId next_item;
					next_item = tree.GetNextVisible(sel_item);
					tree.Delete(sel_item);
					tree.SelectItem(next_item);
				}
			}
			break;
		case MENU_SAVE_EXIT :
			save_site();
			SaveFavorites();
			EndModal(wxID_CANCEL);
			break;
	}
}

void frm_Favorite_Edit::CopyTreeItem(wxTreeItemId src, wxTreeItemId tar)
{
	wxTreeItemIdValue cookie;
	wxTreeItemId src_child, tar_child;
//	m_SetNodeText(tar, m_GetNodeText(src) );
	src_child = tree.GetFirstChild(src, cookie);
	while(src_child.IsOk())
	{
     	int src_child_img = tree.GetItemImage(src_child);
		tar_child = m_AppendItem( tar, m_GetNodeText(src_child), src_child_img );
		if( tree.ItemHasChildren(src_child) )	CopyTreeItem(src_child, tar_child);
		src_child = tree.GetNextChild(src, cookie);
	}
}

void frm_Favorite_Edit::OnItemSelecting(wxTreeEvent& event)
{
	save_site();
}

void frm_Favorite_Edit::OnItemSelected(wxTreeEvent& event)
{
	wxTreeItemId sel_item = event.GetItem();
	int sel_item_img = tree.GetItemImage(sel_item);
	wxString site_desc = m_GetNodeText( sel_item );

	if(sel_item_img == ICON_SITE)	//如果選取的是站台物件
	{
		SiteInfo si;
		si.Init();
		si.Set(site_desc);

		txtName->SetValue( si.name );
		txtIP->Enable(true);	txtIP->SetValue( si.ip );
		txtPort->Enable(true);	txtPort->SetValue( wxString::Format("%d", si.port) );
//		int i = (si.protocol == SOCK_SSH) ? 1 : 0;
//		rdbxProtocol->SetSelection(i);	//SetSelection 會造成 wxMSW 2.4.2 crash
		wxString t = (si.protocol == SOCK_SSH) ? _T("SSH") : _T("BBS / Telnet");
		rdbxProtocol->Enable(true);	rdbxProtocol->SetStringSelection(t);	//這行會使 focus 跑到 rdbxProtocol...
		tree.SetFocus();
		chkAutoConnectAtStartup->Enable(true);	chkAutoConnectAtStartup->SetValue( si.autoopen );
		txtUsername->Enable(true);	txtUsername->SetValue( si.username );
		txtPassword->Enable(true);	txtPassword->SetValue( si.password );
		txtMessage->Enable(true);	txtMessage->SetValue( si.message );
		chkAutoLogin->Enable(true);	chkAutoLogin->SetValue( ! si.username.IsEmpty() );
	}
	else	//如果選取的是資料夾
	{
		txtName->SetValue(site_desc);

		txtIP->Enable(false);	txtIP->SetValue( wxEmptyString );
		txtPort->Enable(false);	txtPort->SetValue( wxEmptyString );
//		int i = (si.protocol == SOCK_SSH) ? 1 : 0;
//		rdbxProtocol->SetSelection(i);	//SetSelection 會造成 wxMSW 2.4.2 crash
		rdbxProtocol->Enable(false);	rdbxProtocol->SetStringSelection(_T("BBS / Telnet"));	//這行會使 focus 跑到 rdbxProtocol...
		tree.SetFocus();
		chkAutoLogin->Enable(false);	chkAutoLogin->SetValue(false);
		chkAutoConnectAtStartup->Enable(false);	chkAutoConnectAtStartup->SetValue(false);
		txtUsername->Enable(false);	txtUsername->SetValue(wxEmptyString);
		txtPassword->Enable(false);	txtPassword->SetValue(wxEmptyString);
		txtMessage->Enable(false);	txtMessage->SetValue(wxEmptyString);
	}

	bl_selected_item_setting_changed = false;
}

frm_Favorite_Edit::frm_Favorite_Edit(wxWindow *parent)
	: wxDialog(parent, 0, wxString(_T("Edit favorite")), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
	wxButton btnNew, btnMkdir, btnDelete, btnConnect, btnApply, btnCancel;

	tree.Create(this, 0, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT );
	list_root = tree.AddRoot(wxEmptyString);
	tree.Bind(wxEVT_TREE_SEL_CHANGING, &frm_Favorite_Edit::OnItemSelecting, this);
	tree.Bind(wxEVT_TREE_SEL_CHANGED, &frm_Favorite_Edit::OnItemSelected, this);
	tree.Bind(wxEVT_TREE_BEGIN_DRAG, &frm_Favorite_Edit::OnTreeBeginDrag, this);
	tree.Bind(wxEVT_TREE_END_DRAG, &frm_Favorite_Edit::OnTreeEndDrag, this);
	tree.Bind(wxEVT_TREE_KEY_DOWN, &frm_Favorite_Edit::OnTreeKeyDown, this);

	//載入圖示
	wxImageList *img_list = new wxImageList(16,16, true, 2);
	img_list->Add( GetProgramIcon(BBMAN_ICON_DIR) );
	img_list->Add( GetProgramIcon(BBMAN_ICON_SITE) );
	tree.AssignImageList(img_list);

	wxGridSizer *btn_sizer = new wxGridSizer(3, 6, 6);	//配置所有按鈕位置
	btn_sizer->Add( new wxButton(this, MENU_MOVEUP, gettext("Move Up"))  , 0 , wxEXPAND | wxALL );
	btn_sizer->Add( new wxButton(this, MENU_MKDIR, gettext("New Folder")), 0 , wxEXPAND | wxALL );
	btn_sizer->Add( new wxButton(this, MENU_COPY, gettext("Copy"))  , 0 , wxEXPAND | wxALL );
	btn_sizer->Add( new wxButton(this, MENU_MOVEDOWN, gettext("Move Down"))  , 0 , wxEXPAND | wxALL );
	btn_sizer->Add( new wxButton(this, MENU_NEWSITE, gettext("New Site"))  , 0 , wxEXPAND | wxALL );
	btn_sizer->Add( new wxButton(this, MENU_DELETE, gettext("Delete"))    , 0 , wxEXPAND | wxALL );

	wxBoxSizer *main_btn_sizer = new wxBoxSizer(wxHORIZONTAL);
	main_btn_sizer->Add( new wxButton(this, MENU_SAVE_EXIT, gettext("Save and Exit")) );
	main_btn_sizer->Add( 6 , 0 );
	main_btn_sizer->Add( new wxButton(this, wxID_CANCEL, gettext("Cancel")) );

	wxBoxSizer *tree_btn_sizer = new wxBoxSizer( wxVERTICAL );
	tree_btn_sizer->Add( &tree , 1, wxGROW | wxALL );
	tree_btn_sizer->Add( btn_sizer , 0, wxGROW | wxALL , 6 );

	wxFlexGridSizer *edit_sizer = new wxFlexGridSizer(2, 10, 6);	//配置所有 wxTextCtrl 和 wxStaticText
	edit_sizer->AddGrowableCol(1);
	edit_sizer->Add( new wxStaticText(this, -1, gettext("Site name"))  , 0 , wxALIGN_CENTER_VERTICAL );
	edit_sizer->Add( txtName = new wxTextCtrl(this, ID_TEXT_NAME)  , 1 , wxEXPAND | wxALL );
	edit_sizer->Add( new wxStaticText(this, -1, gettext("Hostname"))  , 0 , wxALIGN_CENTER_VERTICAL );
	edit_sizer->Add( txtIP = new wxTextCtrl(this, ID_TEXT_IP)  , 1 , wxEXPAND | wxALL );
	edit_sizer->Add( new wxStaticText(this, -1, gettext("Port"))  , 0 , wxALIGN_CENTER_VERTICAL );
	edit_sizer->Add( txtPort = new wxTextCtrl(this, ID_TEXT_PORT)  , 1 , wxEXPAND | wxALL );
	edit_sizer->Add( new wxStaticText(this, -1, gettext("Protocol"))  , 0 , wxALIGN_CENTER_VERTICAL );
	wxString protocol_opts[] = { _T("BBS / Telnet"), _T("SSH") };
	edit_sizer->Add( (rdbxProtocol = new wxRadioBox(this, RDBX_PROTOCOL, wxEmptyString, wxDefaultPosition, wxDefaultSize, 2, protocol_opts ))  , 0 );
	edit_sizer->Add( 0 , 0 );
	edit_sizer->Add( chkAutoConnectAtStartup = new wxCheckBox(this, CHK_AUTOCONNECT, gettext("Auto Connect When Startup"))  , 0 , wxALIGN_CENTER_VERTICAL );
	edit_sizer->Add( 0 , 0 );
	edit_sizer->Add( chkAutoLogin = new wxCheckBox(this, CHK_AUTOLOGIN, gettext("Auto Login"))  , 0 , wxALIGN_CENTER_VERTICAL );
	edit_sizer->Add( new wxStaticText(this, -1, gettext("Username"))  , 0 , wxALIGN_CENTER_VERTICAL );
	edit_sizer->Add( txtUsername = new wxTextCtrl(this, ID_TEXT_USERNAME)  , 1 , wxEXPAND | wxALL );
	edit_sizer->Add( new wxStaticText(this, -1, gettext("Password") )  , 0 , wxALIGN_CENTER_VERTICAL );
	edit_sizer->Add( txtPassword = new wxTextCtrl(this, ID_TEXT_PASSWORD, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD)  , 1 , wxEXPAND | wxALL );
	edit_sizer->Add( 0 , 0 );
	edit_sizer->Add( new wxStaticText(this, -1, gettext("Auto Send Message When Connect"))  , 0 , wxALIGN_CENTER_VERTICAL );
	edit_sizer->Add( 0 , 0 );
	edit_sizer->Add( txtMessage = new wxTextCtrl(this, ID_TEXT_MESSAGE, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD)  , 1 , wxEXPAND | wxALL );
	edit_sizer->Add( 0 , 0 );
	edit_sizer->Add( new wxButton(this, -1, gettext("About Message"))  , 1 , wxEXPAND | wxALL );

	wxBoxSizer *tree_edit_sizer = new wxBoxSizer( wxHORIZONTAL );
	tree_edit_sizer->Add( 6 , 0 );
	tree_edit_sizer->Add( tree_btn_sizer , 1 , wxEXPAND | wxALL );
	tree_edit_sizer->Add( 6 , 0 );
	tree_edit_sizer->Add( edit_sizer , 1, wxEXPAND | wxALL );
	tree_edit_sizer->Add( 6 , 0 );

	wxBoxSizer *main_sizer = new wxBoxSizer( wxVERTICAL );
	main_sizer->Add( 0 , 6 );
	main_sizer->Add( tree_edit_sizer , 1 , wxEXPAND | wxALL );
	main_sizer->Add( 0 , 6 );
	main_sizer->Add( new wxStaticLine(this, -1) , 0 , wxEXPAND | wxALL );
	main_sizer->Add( 0 , 6 );
	main_sizer->Add( main_btn_sizer , 0, wxALIGN_CENTER_HORIZONTAL );
	main_sizer->Add( 0 , 6 );


	SetSizer(main_sizer);
	main_sizer->SetSizeHints( this );
	SetAutoLayout( TRUE );

	CenterOnScreen();

	bl_selected_item_setting_changed = false;
	bl_favorite_modified = false;
}

frm_Favorite_Edit::~frm_Favorite_Edit()
{
//	wxMessageBox("~frm_Favorite_Edit()");
}
// ============================================================================

enum { ITEM_FOLDER , ITEM_SITE };
enum { ID_BTN_SEARCH = 600 };

BEGIN_EVENT_TABLE(frm_BBSList, wxDialog)
	EVT_MENU(MENU_OPENSITE, frm_BBSList::OnOpenSite)
	EVT_LEFT_DCLICK( frm_BBSList::OnMouseDoubleClick )
	EVT_BUTTON( ID_BTN_SEARCH, frm_BBSList::OnSearch )
END_EVENT_TABLE()


bool frm_BBSList::getSiteInfo(wxString &_addr, wxString &_name)
{
	wxTreeItemId nodeId = tree.GetSelection();
	if( nodeId.IsOk()
 		&& tree.GetItemImage( nodeId ) == ITEM_SITE )	//而且該 item 是一個 site
	{
		wxString txt = tree.GetItemText(nodeId);

		_name = txt.BeforeFirst('\t').BeforeFirst(' ');
		_addr = txt.AfterLast('\t').Trim();

		return true;
	}
	else return false;
}

// ----------------------------------------------------------------------------

frm_BBSList::frm_BBSList()
	: wxDialog( NULL, -1, wxString( gettext("Connection List") ), wxDefaultPosition, wxSize(500, 500) )
{

	tree.Create(this, 666, wxDefaultPosition, this->GetClientSize(), wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT );
	tree.Bind(wxEVT_LEFT_DCLICK, &frm_BBSList::OnMouseDoubleClick, this);


	wxBoxSizer *sub_sizer, *sizer = new wxBoxSizer( wxVERTICAL );

	sub_sizer = new wxBoxSizer( wxHORIZONTAL );
	btnOK.Create(this, wxID_OK , gettext("Connect") );
	btnCancel.Create(this, wxID_CANCEL, gettext("Cancel") );
	txtSearch.Create(this, 0, wxEmptyString);
	btnSearch.Create(this, ID_BTN_SEARCH, gettext("Search") );
	sub_sizer->Add( & btnOK , 0 , wxEXPAND | wxALL , 6 );
	sub_sizer->Add( & btnCancel , 0 , wxEXPAND | wxALL , 6 );
	sub_sizer->Add( & txtSearch , 0 , wxEXPAND | wxALL , 6 );
	sub_sizer->Add( & btnSearch , 0 , wxEXPAND | wxALL , 6 );
	sizer->Add( sub_sizer );

	sizer->Add( &tree );

	SetSizer(sizer);
	sizer->SetSizeHints( this );
	SetAutoLayout( TRUE );


	list_root = tree.AddRoot(wxEmptyString);
	LoadSiteList(list_root);
}
// ----------------------------------------------------------------------------
void frm_BBSList::OnSearch(wxCommandEvent& event)
{
	wxString tmp, name = txtSearch.GetValue();
	name.Trim();
	if( name.IsEmpty() )	return;
	
	wxTreeItemId next, now, orig = tree.GetSelection();
	now = orig;
	wxTreeItemIdValue cookie;
	if(!now.IsOk())	return;

	while(true)
	{
		if( tree.ItemHasChildren(now) )
			now = tree.GetFirstChild(now, cookie);
		else
		{
			next = tree.GetNextSibling(now);
			if( next.IsOk() )	now = next;
			else
			{
				while(true)
				{
					now = tree.GetItemParent(now);
					if( !now.IsOk() )	//遇到 root node
					{
						now = tree.GetRootItem();
						now = tree.GetFirstChild(now, cookie);
						break;
					}
					next = tree.GetNextSibling(now);
					if( next.IsOk() )
					{	now = next;	break;	}
				}
			}
		}

		tmp = tree.GetItemText(now);
		if( tmp.First(name) >= 0 )	//如果找到了
		{
			tree.SelectItem(now);
			return;
		}

		if( now == orig )	//找過所有節點, 但沒有符合的
		{
			wxMessageBox( gettext("not found") );
			return;
		}

	}
}
// ----------------------------------------------------------------------------
void frm_BBSList::OnOpenSite(wxCommandEvent& event)
{
	wxMouseEvent e;
	OnMouseDoubleClick(e);
}
// ----------------------------------------------------------------------------
void frm_BBSList::OnMouseDoubleClick(wxMouseEvent& event)
{
	wxTreeItemId nodeId = tree.GetSelection();
	if( nodeId.IsOk()
 		&& tree.GetItemImage( nodeId ) == ITEM_SITE )	//而且該 item 是一個 site
	{
		EndModal(wxID_OK);
	}
}
// ----------------------------------------------------------------------------
bool frm_BBSList::LoadSiteList(wxTreeItemId  rootId)
{
	wxString site_list = GetResourcePath() + _T("sites.dat");
	if( ! wxFile::Exists(site_list) )
		site_list = _T("sites.dat");

	FILE *fp = wxFopen( site_list , _T("r") );
	if(fp==NULL)	return false;


	wxImageList *img_list = new wxImageList(16,16, true, 2);
	img_list->Add( GetProgramIcon(BBMAN_ICON_DIR) );
	img_list->Add( GetProgramIcon(BBMAN_ICON_SITE) );
	tree.AssignImageList(img_list);


	wxTreeItemId  now_root_Id , parent_node_Id[1000];
	int deep = 0;
	now_root_Id = rootId;
	
	char buf[1024];
	while( fgets( buf, sizeof(buf), fp ) != NULL )
	{
		switch( buf[0] )
		{
			case 's':	//define a site
				tree.AppendItem( now_root_Id , SiteListBig5ToWxString(buf+1) , ITEM_SITE );
				break;
			case 'd':	//mkdir
				parent_node_Id[ deep++ ] = now_root_Id;
				now_root_Id = tree.AppendItem( now_root_Id , SiteListBig5ToWxString(buf+1), ITEM_FOLDER );
				break;
			case '\n':
			case '\r':
			case '\0':	//cdup
				if( deep > 0 )
					now_root_Id = parent_node_Id[ --deep ];
				break;
			default:
				break;
		}
	}
	
	if( !tree.HasFlag(wxTR_HIDE_ROOT) )
		tree.Expand( rootId );
	
	fclose(fp);
	return true;
}
// ----------------------------------------------------------------------------
bool ShowBBSListDialog(wxString &_addr, wxString &_name)
{
	frm_BBSList f;
	if( wxID_OK == f.ShowModal() )
	{
		if( ! f.getSiteInfo( _addr, _name ) )	return false;
		return true;
	}
	else	return false;
}
// ----------------------------------------------------------------------------
#endif
