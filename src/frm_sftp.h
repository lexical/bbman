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

#ifndef FRM_SFTP_H
#define FRM_SFTP_H
#include "common.h"
#include "scd_wxssh/scd_wxssh.h"
#include "scd_wxssh/scd_wxsftp.h"

#include <wx/frame.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
//#include <wx/dirctrl.h>
#include <wx/dnd.h>
// ============================================================================

void ShowSFTP(SCD_wxSSH *_ssh);
void DeleteSFTP(SCD_wxSSH *_ssh);

// ============================================================================

class SFTP_Browser : public wxListCtrl
{
public:
	SCD_wxSSH *ssh;
	SCD_wxSFTP *sftp;

private:
	wxMenu *menu;

	wxString path;

public:
	SFTP_Browser(SCD_wxSSH *_ssh , wxWindow *parent);
	~SFTP_Browser();

	wxString GetPath()	{	return path;	}
//	bool SetPath(wxString _path);
	void DeleteSelectedFiles();
	void mkdir();
	void rename();

	void cd(wxString _dir);
	void cdup();
	bool Refresh();

	void OnDownload(wxCommandEvent& event);
	void OnLabelEditEnd(wxListEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnBeginDrag(wxListEvent& event);
	void OnMouseRightDown(wxMouseEvent& event);
	void OnMouseLeftDoubleClick(wxMouseEvent& event);
	void OnFileCommand(wxCommandEvent& event);

private:
	DECLARE_EVENT_TABLE()
};

// ============================================================================

class FileBrowser_DropTarget : public wxTextDropTarget
{
public:
	FileBrowser_DropTarget()	{;}
	bool OnDropText(int x, int y, const wxString& data)
	{
		wxMessageBox("drop files");
		return true;
	}
};

class FileBrowser : public wxListCtrl
{
private:
	FileBrowser_DropTarget dtarget;
	wxString path;
	wxMenu *menu;

public:
	FileBrowser();
	FileBrowser(wxWindow *parent);
	~FileBrowser();
	void Init();

	wxString GetPath()	{	return wxGetCwd();	}
//	bool SetPath(wxString _path);
	void DeleteSelectedFiles();
	void mkdir();
	void rename();

	void cd(wxString _dir);
	void cdup();
	bool Refresh();

	void OnUpload(wxCommandEvent& event);
	void OnLabelEditEnd(wxListEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnBeginDrag(wxListEvent& event);
	void OnMouseRightDown(wxMouseEvent& event);
	void OnMouseLeftDoubleClick(wxMouseEvent& event);
	void OnFileCommand(wxCommandEvent& event);

private:
	DECLARE_EVENT_TABLE()
};

// ============================================================================

class frm_SFTP : public wxFrame
	, public SFTP_Transfer_Progress_Handler	//這是為了接收處理 sftp 傳輸時的 event
{
private:
//	SFTP_Browser sftp_browser;
//	wxGenericDirCtrl dirctrl;
	wxImageList *img_list;

	//因為 wxSplitterWindow 在被 delete 之前會 delete 其下的 children windows
	//因此必須把所有 children windows 以 new 運算子建立, 不可以變數宣告之
	wxSplitterWindow *fb_nb_splitter, trans_progress_splitter;
	wxListCtrl *progress_list;
public:
	//請勿直接存取下面兩變數
	FileBrowser *fb;
	wxNotebook *nb;

	frm_SFTP();
	~frm_SFTP();

	int FindSFTP(SCD_wxSSH *_ssh);
	void ShowSFTP(SCD_wxSSH *_ssh);
	void DeleteSFTP(SCD_wxSSH *_ssh);

	//這是用來接收處理 sftp 傳輸時的 event, 藉此顯示傳輸狀態
	//這裡要注意 multi-thread 的問題
	void OnSFTP_Transfer_Event(t_DU_data *du, int event_type);

private:
	DECLARE_EVENT_TABLE()
};

// ============================================================================
#endif
#endif
