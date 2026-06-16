/******************************************************************************
 * Name:        frm_sftp.cpp
 * Purpose:     SFTP GUI frame
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/

#ifndef BBMAN_NO_SSH


#ifndef FRM_SFTP_CPP
#define FRM_SFTP_CPP
#include "frm_sftp.h"

#include <wx/dir.h>
#include <wx/dataobj.h>
#include <time.h>
#include <fcntl.h>

enum {
	MENU_UPLOAD, MENU_DOWNLOAD,

	MENU_FILECOMMAND_BEGIN,
	MENU_RM, MENU_MKDIR, MENU_RENAME,
	MENU_CD, MENU_CDUP, MENU_REFRESH,
	MENU_CLOSE_SFTP_TAB,
	MENU_FILECOMMAND_END,
};

enum { ICON_UNKNOWN = -1, ICON_DIR, ICON_FILE };
enum { ICON_UPLOAD, ICON_DOWNLOAD };

typedef struct DU_Thread_Data
{
	SCD_wxSSH *ssh;
	wxArrayString filelist;
	wxString path;
	SFTP_Transfer_Progress_Handler *handler;
};
// ============================================================================

frm_SFTP *SFTP_frame = NULL;
void ShowSFTP(SCD_wxSSH *_ssh)
{
	if( SFTP_frame == NULL )
	{
		SFTP_frame = new frm_SFTP();
		SFTP_frame->Show();
	}
	else	SFTP_frame->Raise();

	SFTP_frame->ShowSFTP(_ssh);
}

void DeleteSFTP(SCD_wxSSH *_ssh)
{
	if( SFTP_frame != NULL )
		SFTP_frame->DeleteSFTP(_ssh);
}

// ============================================================================

BEGIN_EVENT_TABLE(SFTP_Browser, wxListCtrl)
	EVT_LIST_BEGIN_DRAG(0, SFTP_Browser::OnBeginDrag)
	EVT_LIST_END_LABEL_EDIT(0, SFTP_Browser::OnLabelEditEnd)
	EVT_KEY_DOWN(SFTP_Browser::OnKeyDown)
	EVT_RIGHT_DOWN(SFTP_Browser::OnMouseRightDown)
	EVT_LEFT_DCLICK(SFTP_Browser::OnMouseLeftDoubleClick)
	EVT_MENU( MENU_DOWNLOAD, SFTP_Browser::OnDownload )
	EVT_MENU_RANGE( MENU_FILECOMMAND_BEGIN, MENU_FILECOMMAND_END, SFTP_Browser::OnFileCommand )
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
SFTP_Browser::SFTP_Browser(SCD_wxSSH *_ssh , wxWindow *parent)
	: wxListCtrl(parent, 0, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_HRULES | wxLC_EDIT_LABELS)
{
	ssh = _ssh;
	sftp = CreateSFTP(ssh);
	path = sftp->GetPath();

	menu = new wxMenu();
	menu->Append(MENU_DOWNLOAD, gettext("Download") );
	menu->Append(MENU_MKDIR, gettext("New folder") );
	menu->Append(MENU_RM, gettext("Delete") );
	menu->Append(MENU_RENAME, gettext("Rename") );
	menu->Append(MENU_CDUP, gettext("CDUP") );
	menu->Append(MENU_CD, gettext("Change Directory") );
	menu->Append(MENU_REFRESH, gettext("Refresh") );
	menu->AppendSeparator();
	menu->Append(MENU_CLOSE_SFTP_TAB, gettext("Close this tab") );

	InsertColumn( 0, gettext("Filename") );
	InsertColumn( 1, gettext("Size") , wxLIST_FORMAT_RIGHT );
	InsertColumn( 2, gettext("filetype") );
	InsertColumn( 3, gettext("Changed") );
	InsertColumn( 4, gettext("Permission") );
	InsertColumn( 5, gettext("Owner") );

	Refresh();
}
// ----------------------------------------------------------------------------
SFTP_Browser::~SFTP_Browser()
{
	delete menu;
	delete sftp;
//wxMessageBox("~SFTP_Browser()");
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
int DownloadUsingThread(void *data)
{
	DU_Thread_Data *dt = (DU_Thread_Data*)data;

	SCD_wxSFTP *_sftp = CreateSFTP(dt->ssh);
	if(!_sftp)	wxMessageBox( gettext("Can't init sftp connection!") );
	else _sftp->Download( dt->path, dt->filelist, dt->handler );

	delete dt;
	delete _sftp;	//always free after transfer
	return 0;
}
// ----------------------------------------------------------------------------
void SFTP_Browser::OnDownload(wxCommandEvent& event)
{
//	wxMessageBox( wxString::Format("Download %d files", GetSelectedItemCount()) );

	if( GetSelectedItemCount() <= 0 )	return;	//no item upload
//	if( nb->GetSelection() == -1 )	break;	//no page selected

//	sftp = ssh->CreateSFTP();

	wxArrayString filelist;

	long item = -1;
	while(true)
	{
		item = GetNextItem( item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		if(item == -1)	break;
		filelist.Add( sftp->GetPath( GetItemText(item) ) );
//		wxMessageBox("download file " + sftp->GetPath( GetItemText(item) ) );
	}
//	wxMessageBox( "to local dir : " + SFTP_frame->fb->GetPath() );
/*
	SCD_wxSFTP *_sftp = CreateSFTP(ssh);
	if(!_sftp)	wxMessageBox( gettext("Can't init sftp connection!") );
	else _sftp->Download( SFTP_frame->fb->GetPath() , filelist, SFTP_frame );
*/
	DU_Thread_Data *dutd = new DU_Thread_Data;
	dutd->ssh = ssh;
	dutd->filelist = filelist;
	dutd->path = SFTP_frame->fb->GetPath();
	dutd->handler = SFTP_frame;
	ExecFuncUsingThread( DownloadUsingThread, dutd );
}
// ----------------------------------------------------------------------------
void SFTP_Browser::OnFileCommand(wxCommandEvent& event)
{
	int id = event.GetId();
	switch(id)
	{
		case MENU_RM :	DeleteSelectedFiles();	break;
		case MENU_MKDIR :	mkdir();	break;
		case MENU_RENAME :	rename();	break;
		case MENU_CDUP : cdup();	break;
		case MENU_CD :
			{
				wxString line = wxGetTextFromUser( gettext("Please enter the new path."), wxEmptyString, sftp->GetPath(), this );
				if( ! line.IsEmpty() )	cd(line);
			}
			break;
		case MENU_REFRESH :	Refresh();	break;
		case MENU_CLOSE_SFTP_TAB :
			SFTP_frame->DeleteSFTP(ssh);
			break;
	}
}
// ----------------------------------------------------------------------------
void SFTP_Browser::OnBeginDrag(wxListEvent& event)
{
	wxMessageBox( "dragging file " + event.GetText() );
//	wxFileDataObject
}
// ----------------------------------------------------------------------------
void SFTP_Browser::OnLabelEditEnd(wxListEvent& event)
{
	wxString oldname = GetItemText(event.GetIndex());
	wxString newname = event.GetText();
	if( -1 != FindItem( -1 , newname ) )
	{
		wxMessageBox( wxString::Format("file [ %s ] already exists",
			wxStringToCharPtr(newname) ) );
	}
	else if( ! sftp->rename( wxStringToCharPtr(oldname) , wxStringToCharPtr(newname) ) )
	{
		event.Veto();
		wxMessageBox( wxString::Format("Can't rename file from [ %s ] to [ %s ]",
			wxStringToCharPtr(oldname), wxStringToCharPtr(newname) ) );
	}
}
// ----------------------------------------------------------------------------
void SFTP_Browser::OnKeyDown(wxKeyEvent& event)
{
	switch(event.m_keyCode)
	{
		case WXK_F2 :	rename();	break;
		case WXK_F5 :	Refresh();	break;
		case WXK_DELETE :	DeleteSelectedFiles();	break;
	}
}
// ----------------------------------------------------------------------------
void SFTP_Browser::OnMouseRightDown(wxMouseEvent& event)
{	PopupMenu(menu, event.GetPosition() );	}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SFTP_Browser::mkdir()
{
	wxString name = wxGetTextFromUser( gettext("Please Enter name for new folder") , wxEmptyString, _T("NewFolder") , this );
	if( ! name.IsEmpty() )
	{
//wxMessageBox(name);
//wxMessageBox(sftp->GetPath(name));

		if( sftp->mkdir( wxStringToCharPtr( sftp->GetPath() + _T("/") + name ) ) )
			InsertItem( GetItemCount() , name );
		else
			wxMessageBox( gettext("Cannot create new folder") );
	}
}
// ----------------------------------------------------------------------------
void SFTP_Browser::rename()
{
	long item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED );
	if(item != -1)	EditLabel(item);
}
// ----------------------------------------------------------------------------
void SFTP_Browser::DeleteSelectedFiles()
{
	long item = -1;
	bool b;
	while(true)
	{
		item = GetNextItem( item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		if(item == -1)	break;
//		wxMessageBox("want to delete " + GetItemText(item) );
		if( GetItemData(item) == ICON_DIR )
			b = sftp->rmdir( wxStringToCharPtr( sftp->GetPath(GetItemText(item)) ) );
		else
			b = sftp->rm( wxStringToCharPtr( sftp->GetPath(GetItemText(item)) ) );

		if(b)	{	DeleteItem(item);	item--;	}
		else	wxMessageBox( gettext("Can't delete file ") + GetItemText(item) );
	}
}
// ----------------------------------------------------------------------------
void SFTP_Browser::OnMouseLeftDoubleClick(wxMouseEvent& event)
{
	long item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED );
	if( item != -1 )
	{
		if( GetItemData(item) == ICON_DIR )
			cd( GetItemText(item) );
	}
}
// ----------------------------------------------------------------------------
void SFTP_Browser::cd(wxString _dir)
{
	if( sftp->cd(_dir) )	Refresh();
}
// ----------------------------------------------------------------------------
void SFTP_Browser::cdup()
{	cd( _T("..") );	}
// ----------------------------------------------------------------------------
bool SFTP_Browser::Refresh()
{
	SFTP_DIR *dir = sftp->opendir(wxEmptyString);
	if(dir==NULL)	return false;

	DeleteAllItems();

	int file_cnt = 0;
	while(true)
	{
		SFTP_ATTRIBUTES *attr = sftp->readdir(dir);
		if(attr==NULL)	break;

		bool isDir = sftp->isDir(attr);
		int img_id = isDir ? ICON_DIR : ICON_FILE;

		InsertItem( file_cnt , attr->name , img_id );
		SetItemData( file_cnt, img_id );	//因為之後無法直接取得 item image 屬性, 因此利用 SetItemData 儲存此屬性方便以後取得其 item image


		if( img_id == ICON_FILE )
			SetItem( file_cnt , 1 , wxString::Format("%d", (int)attr->size) );
		else
			SetItem( file_cnt , 1 , _T("<DIR>") );
//		SetItem( file_cnt , 2 , attr->size );
//		SetItem( file_cnt , 3 , ctime(attr->mtime) );
		SetItem( file_cnt , 4 , wxString::Format("%.8lo", attr->permissions) );
		SetItem( file_cnt , 5 , wxString(attr->owner) );

		sftp->attributes_free(attr);
		file_cnt ++;
	}
	sftp->closedir(dir);

	return true;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//wxString SFTP_Browser::GetPath()

// ----------------------------------------------------------------------------
//bool SFTP_Browser::SetPath(wxString _path)
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// ============================================================================
// ============================================================================
// ============================================================================

BEGIN_EVENT_TABLE(FileBrowser, wxListCtrl)
	EVT_LIST_BEGIN_DRAG(0, FileBrowser::OnBeginDrag)
	EVT_LIST_END_LABEL_EDIT(0, FileBrowser::OnLabelEditEnd)
	EVT_KEY_DOWN(FileBrowser::OnKeyDown)
	EVT_RIGHT_DOWN(FileBrowser::OnMouseRightDown)
	EVT_LEFT_DCLICK(FileBrowser::OnMouseLeftDoubleClick)
	EVT_MENU( MENU_UPLOAD, FileBrowser::OnUpload )
	EVT_MENU_RANGE( MENU_FILECOMMAND_BEGIN, MENU_FILECOMMAND_END, FileBrowser::OnFileCommand )
END_EVENT_TABLE()

// ----------------------------------------------------------------------------

FileBrowser::FileBrowser()
{
//	SetDropTarget(&dtarget);
//	DragAcceptFiles(true);

}

FileBrowser::FileBrowser(wxWindow *parent)
	: wxListCtrl(parent, 0, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_HRULES | wxLC_EDIT_LABELS)
{
//	FileBrowser();
	Init();
}
// ----------------------------------------------------------------------------
FileBrowser::~FileBrowser()
{
	delete menu;
}
// ----------------------------------------------------------------------------
void FileBrowser::Init()
{
	path = wxGetCwd();

	InsertColumn( 0, gettext("Filename") );
	InsertColumn( 1, gettext("Size") , wxLIST_FORMAT_RIGHT );
	InsertColumn( 2, gettext("filetype") );
	InsertColumn( 3, gettext("Changed") );

	menu = new wxMenu();
	menu->Append(MENU_UPLOAD, gettext("Upload") );
	menu->Append(MENU_MKDIR, gettext("New folder") );
	menu->Append(MENU_RM, gettext("Delete") );
	menu->Append(MENU_RENAME, gettext("Rename") );
	menu->Append(MENU_CD, gettext("Change Directory") );
	menu->Append(MENU_CDUP, gettext("CDUP") );
	menu->Append(MENU_REFRESH, gettext("Refresh") );

	Refresh();
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
int UploadUsingThread(void *data)
{
	DU_Thread_Data *dt = (DU_Thread_Data*)data;

	SCD_wxSFTP *_sftp = CreateSFTP(dt->ssh);
	if(!_sftp)	wxMessageBox( gettext("Can't init sftp connection!") );
	else _sftp->Upload( dt->filelist, dt->path, dt->handler );

	delete dt;
	delete _sftp;	//always free after transfer
	return 0;
}
// ----------------------------------------------------------------------------
void FileBrowser::OnUpload(wxCommandEvent& event)
{
	wxMessageBox( wxString::Format("upload %d files", GetSelectedItemCount()) );

	if( GetSelectedItemCount() <= 0 )	return;	//no item upload
//	if( nb->GetSelection() == -1 )	break;	//no page selected

	wxArrayString filelist;

	long item = -1;
	while(true)
	{
		item = GetNextItem( item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		if(item == -1)	break;
		filelist.Add( GetPath() + _T("/") + GetItemText(item) );
//		wxMessageBox("upload file " + GetItemText(item) );
	}

	int p = SFTP_frame->nb->GetSelection();
	if( p < 0 )
	{	wxMessageBox( gettext("No sftp browser window selected.") );	return;	}

	SFTP_Browser *_sb = (SFTP_Browser*) SFTP_frame->nb->GetPage(p);
	if(_sb==NULL)	wxMessageBox( gettext("No sftp browser window selected.") );

/*
	SCD_wxSFTP *_sftp = CreateSFTP(_sb->ssh);
	if(!_sftp)	wxMessageBox( gettext("Can't init sftp connection!") );
	else _sftp->Upload( filelist, _sb->sftp->GetPath(), SFTP_frame );
*/
	DU_Thread_Data *dutd = new DU_Thread_Data;
	dutd->ssh = _sb->ssh;
	dutd->filelist = filelist;
	dutd->path = _sb->sftp->GetPath();
	dutd->handler = SFTP_frame;
	ExecFuncUsingThread( UploadUsingThread, dutd );
}
// ----------------------------------------------------------------------------
void FileBrowser::OnFileCommand(wxCommandEvent& event)
{
	int id = event.GetId();
	switch(id)
	{
		case MENU_RM :	DeleteSelectedFiles();	break;
		case MENU_MKDIR :	mkdir();	break;
		case MENU_RENAME :	rename();	break;
		case MENU_CD :
			{
				wxString line = wxDirSelector( gettext("Change Directory"), wxGetCwd(), 0, wxDefaultPosition, this);
				if( ! line.IsEmpty() )	cd(line);
			}
			break;
		case MENU_CDUP :	cdup();		break;
		case MENU_REFRESH :	Refresh();	break;
	}
}
// ----------------------------------------------------------------------------
void FileBrowser::OnBeginDrag(wxListEvent& event)
{
	wxMessageBox( "dragging file " + event.GetText() );

}
// ----------------------------------------------------------------------------
void FileBrowser::OnLabelEditEnd(wxListEvent& event)
{
	wxString oldname = GetItemText(event.GetIndex());
	wxString newname = event.GetText();
	if( -1 != FindItem( -1 , newname ) )
	{
		wxMessageBox( wxString::Format("file [ %s ] already exists",
			wxStringToCharPtr(newname) ) );
	}
	else if( ! wxRenameFile( oldname , newname ) )
	{
		event.Veto();
		wxMessageBox( wxString::Format("Can't rename file from [ %s ] to [ %s ]",
			wxStringToCharPtr(oldname), wxStringToCharPtr(newname) ) );
	}
}
// ----------------------------------------------------------------------------
void FileBrowser::OnKeyDown(wxKeyEvent& event)
{
	switch(event.m_keyCode)
	{
		case WXK_F2 :	rename();	break;
		case WXK_F5 :	Refresh();	break;
		case WXK_DELETE :	DeleteSelectedFiles();	break;
	}
}
// ----------------------------------------------------------------------------
void FileBrowser::OnMouseRightDown(wxMouseEvent& event)
{	PopupMenu(menu, event.GetPosition() );	}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void FileBrowser::mkdir()
{
	wxString name = wxGetTextFromUser( gettext("Please Enter name for new folder") , wxEmptyString, _T("NewFolder") , this );
	if( ! name.IsEmpty() )
	{
		if( wxMkdir( wxStringToCharPtr(name) ) )
			InsertItem( GetItemCount() , name );
		else
			wxMessageBox( gettext("Cannot create new folder") );
	}
}
// ----------------------------------------------------------------------------
void FileBrowser::rename()
{
	long item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED );
	if(item != -1)	EditLabel(item);
}
// ----------------------------------------------------------------------------
void FileBrowser::DeleteSelectedFiles()
{
	long item = -1;
	bool b;
	while(true)
	{
		item = GetNextItem( item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		if(item == -1)	break;
//		wxMessageBox("want to delete " + GetItemText(item) );
		if( GetItemData(item) == ICON_DIR )
			b = wxRmdir( GetItemText(item) );
		else
			b = wxRemoveFile( GetItemText(item) );

		if(b)	{	DeleteItem(item);	item--;	}
		else	wxMessageBox( gettext("Can't delete file ") + GetItemText(item) );
	}
}
// ----------------------------------------------------------------------------
void FileBrowser::OnMouseLeftDoubleClick(wxMouseEvent& event)
{
	long item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED );
	if( item != -1 )
	{
		if( GetItemData(item) == ICON_DIR )
			cd( GetItemText(item) );
	}
}
// ----------------------------------------------------------------------------
void FileBrowser::cd(wxString _dir)
{
	wxSetWorkingDirectory(_dir);
	Refresh();
}
// ----------------------------------------------------------------------------
void FileBrowser::cdup()
{	cd( _T("..") );	}
// ----------------------------------------------------------------------------
#include <dirent.h>
#include <sys/stat.h>
bool FileBrowser::Refresh()
{
	wxDir dir;

	if( ! dir.Open(path) )
	{
		if( ! dir.Open( wxGetCwd() ) )	return false;
		else path = wxGetCwd();
	}

	DeleteAllItems();

	int file_cnt = 0;
/*
	wxString filename;
	bool b;

	for(int i=0;i<2;i++)	//先顯示 dir 再顯示 file
	{
		int img_id = (i==0?ICON_DIR:ICON_FILE);
		b = dir.GetFirst( &filename , wxEmptyString , i==0?(wxDIR_DIRS|wxDIR_HIDDEN):(wxDIR_FILES|wxDIR_HIDDEN) );
		while(b)
		{
			InsertItem( file_cnt , filename , img_id );
//			SetItem( file_cnt , 1 , wxString::Format("%d", attr->size) );
//			SetItem( file_cnt , 2 , attr->size );
//			SetItem( file_cnt , 3 , attr->size );

			b = dir.GetNext(&filename);
			file_cnt ++;
		}
	}
*/

	DIR *dp;
	struct dirent *ep;
	struct stat stat_buf;
	dp = opendir( wxStringToCharPtr(GetPath()) );
	if(dp)
	{
		while( (ep = readdir (dp)) != NULL )
		{
			char *filename = ep->d_name;
			stat( filename , &stat_buf );

			int img_id = (S_ISDIR(stat_buf.st_mode) != 0)?ICON_DIR:ICON_FILE;

			InsertItem( file_cnt , filename , img_id);
			SetItemData( file_cnt, img_id );	//因為之後無法直接取得 item image 屬性, 因此利用 SetItemData 儲存此屬性方便以後取得其 item image
			if( img_id == ICON_FILE )
				SetItem( file_cnt , 1 , wxString::Format("%d", (int)stat_buf.st_size ) );
			else
				SetItem( file_cnt , 1 , _T("<DIR>") );
//			SetItem( file_cnt , 2 , attr->size );
			SetItem( file_cnt , 3 , ctime( &(stat_buf.st_mtime) ) );

			file_cnt ++;
		}
	}
	closedir(dp);

	return true;
}
// ----------------------------------------------------------------------------

// ============================================================================
// ============================================================================
// ============================================================================

BEGIN_EVENT_TABLE(frm_SFTP, wxFrame)
//	EVT_MENU( MENU_DOWNLOAD, frm_SFTP::OnDownload )
//	EVT_MENU( MENU_UPLOAD, frm_SFTP::OnUpload )
END_EVENT_TABLE()

frm_SFTP::frm_SFTP() : wxFrame(NULL, -1, wxEmptyString, wxDefaultPosition, wxSize(600,300) )
{
//	stxtName.Create(this, -1, gettext("Connection Name") );
//	dirctrl.Create(this, 666, _T("C:\\"), wxDefaultPosition, wxDefaultSize, wxDIRCTRL_DIR_ONLY
//		| wxDIRCTRL_SHOW_FILTERS | wxDIRCTRL_EDIT_LABELS , "*.*");
//	dirctrl.Init();
//	dirctrl.SetDefaultPath(_T("C:\\"));
/*
	remote_listctrl.Create(this, 666, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
	remote_listctrl.InsertColumn( 0, gettext("Filename") );
	remote_listctrl.InsertColumn( 1, gettext("Size") , wxLIST_FORMAT_RIGHT );
	remote_listctrl.InsertColumn( 2, gettext("Changed") );
	remote_listctrl.InsertColumn( 3, gettext("Permission") );
	remote_listctrl.InsertColumn( 4, gettext("Owner") );
*/
/*
	remote_listctrl.InsertItem(0, _T("item1"));
	remote_listctrl.SetItem( 0 , 1 , _T("prop1") );
	remote_listctrl.InsertItem(1, _T("item2"));
	remote_listctrl.InsertItem(2, _T("item3"));
*/

	trans_progress_splitter.Create(this, 0);
	fb_nb_splitter = new wxSplitterWindow( &trans_progress_splitter , -1);

	progress_list = new wxListCtrl( &trans_progress_splitter , -1, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_HRULES );
	progress_list->InsertColumn( 0, gettext("Local File") );
	progress_list->InsertColumn( 1, gettext("Size") );
	progress_list->InsertColumn( 2, gettext("Remote File") );
	progress_list->InsertColumn( 3, gettext("Host") );
	progress_list->InsertColumn( 4, gettext("State") );


	fb = new FileBrowser( fb_nb_splitter );
//	fb->Connect( MENU_UPLOAD , wxEVT_COMMAND_MENU_SELECTED , (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) &frm_SFTP::OnUpload );
	nb = new wxNotebook( fb_nb_splitter , 0);

	//設定 splitter
	fb_nb_splitter->SetMinimumPaneSize(100);
	trans_progress_splitter.SetMinimumPaneSize(100);
	fb_nb_splitter->SplitVertically( fb, nb );
	trans_progress_splitter.SplitHorizontally( fb_nb_splitter, progress_list );
/*
	wxBoxSizer *sizer = new wxBoxSizer( wxHORIZONTAL );
	sizer->Add( &fb , 1 , wxEXPAND | wxALL , 3  );
	sizer->Add( &nb , 1 , wxEXPAND | wxALL , 3  );
	SetSizer(sizer);
	sizer->SetSizeHints( this );
	SetAutoLayout( TRUE );
*/

//	fb->SetImageList(img_list, wxIMAGE_LIST_NORMAL);

	//讀取圖示
	img_list = new wxImageList(16,16, true, 2);
	img_list->Add( GetProgramIcon(BBMAN_ICON_DIR) );
	img_list->Add( GetProgramIcon(BBMAN_ICON_FILE) );
	fb->SetImageList(img_list, wxIMAGE_LIST_SMALL);

	wxImageList *tmp_img_list = new wxImageList(16,16, true, 2);
	tmp_img_list->Add( GetProgramIcon(BBMAN_ICON_UPLOAD) );
	tmp_img_list->Add( GetProgramIcon(BBMAN_ICON_DOWNLOAD) );
	progress_list->AssignImageList(tmp_img_list, wxIMAGE_LIST_SMALL);

	fb->Init();
}
// ----------------------------------------------------------------------------
frm_SFTP::~frm_SFTP()
{
//	delete img_list;
	SFTP_frame = NULL;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void frm_SFTP::OnSFTP_Transfer_Event(t_DU_data *du, int event_type)
{
	long item;

	switch(event_type)
	{
		case SFTP_Transfer_Progress_Handler::SFTP_EVENT_ADDJOB :
			item = progress_list->InsertItem( progress_list->GetItemCount()
				, _T("trans_name") , du->isUpload ? ICON_UPLOAD : ICON_DOWNLOAD );
			progress_list->SetItemData( item, (long)du );	//以 ItemData 識別該傳輸單位
			progress_list->SetItem( item , 1 , gettext("calc total size...") );
			AppProcessOtherEvents();
			break;
		case SFTP_Transfer_Progress_Handler::SFTP_EVENT_UPDATE_LOCAL_REMOTE_DIR :
			item = progress_list->FindItem(-1, (long)du);
			if(item >= 0)
			{
				progress_list->SetItem( item , 3 , du->local_dir );
				progress_list->SetItem( item , 4 , du->remote_dir );
			}
			break;
		case SFTP_Transfer_Progress_Handler::SFTP_EVENT_JOB_DONE :
			item = progress_list->FindItem(-1, (long)du);
			if(item >= 0)	progress_list->DeleteItem(item);
			AppProcessOtherEvents();
			break;
		case SFTP_Transfer_Progress_Handler::SFTP_EVENT_TRANS_NEW_FILE :
			item = progress_list->FindItem(-1, (long)du);
			if(item >= 0)
				progress_list->SetItem( item , 2 , du->transfering_filename );
			break;
		case SFTP_Transfer_Progress_Handler::SFTP_EVENT_UPDATE_PROGRESS :
			item = progress_list->FindItem(-1, (long)du);
			progress_list->SetItem( item , 1 , wxString::Format("%02d%%", du->total_trans_percentage) );
			AppProcessOtherEvents();
			break;
	}
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
int frm_SFTP::FindSFTP(SCD_wxSSH *_ssh)
{
	SFTP_Browser *sb;
	int page_count = nb->GetPageCount();
	for(int i=0;i<page_count;i++)
	{
		sb = (SFTP_Browser*) nb->GetPage(i);
		if( sb && (sb->ssh == _ssh) )
		{	nb->SetSelection(i);	return i;	}
	}
	return -1;
}
// ----------------------------------------------------------------------------
void frm_SFTP::ShowSFTP(SCD_wxSSH *_ssh)
{
	SFTP_Browser *sb;

	int id = FindSFTP(_ssh);
	if(id >= 0)	//if already exist browser of _ssh
	{	nb->SetSelection(id);	return;	}

	//create a browser for _ssh
	SCD_wxSFTP *sftp = CreateSFTP(_ssh);
	if(sftp==NULL)	return;
	sb = new SFTP_Browser(_ssh, nb);
	sb->SetImageList(img_list, wxIMAGE_LIST_SMALL);
	nb->AddPage( sb, _ssh->ip );
//	sb->Connect( MENU_DOWNLOAD , wxEVT_COMMAND_MENU_SELECTED , (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) &frm_SFTP::OnDownload );
	nb->SetSelection( nb->GetPageCount() - 1 );
}
// ----------------------------------------------------------------------------
void frm_SFTP::DeleteSFTP(SCD_wxSSH *_ssh)
{
	int id = FindSFTP(_ssh);
	if(id == -1)	return;	//if already exist browser of _ssh
	SFTP_Browser *sb = (SFTP_Browser*)(nb->GetPage(id));
	nb->RemovePage(id);
	delete sb;
}

// ============================================================================
#endif
#endif
