/******************************************************************************
 * Name:        scd_wxsftp.cpp
 * Purpose:     a wrapper of SFTP in libssh library
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef SCD_WXSFTP_H
#define SCD_WXSFTP_H
#include "../common.h"
#include "libssh/sftp.h"
#include "scd_wxssh.h"
// ============================================================================

class SFTP_Transfer_Progress_Handler;

enum { ACTION_DU , ACTION_CALC_TOTAL_SIZE };
//typedef struct	//SCD_wxSFTP::Upload() SCD_wxSFTP::Download() need it
class t_DU_data		//SCD_wxSFTP::Upload() SCD_wxSFTP::Download() need it
{
public:
	enum { ICON_UNKNOWN = -1, ICON_DIR, ICON_FILE, ICON_UPLOAD, ICON_DOWNLOAD };

	SFTP_Transfer_Progress_Handler *handler;
	int id;
//	wxListCtrl *list;

	int deep;	//資料夾的深度
	int action;	//ACTION_DU / ACTION_CALC_TOTAL_SIZE
	bool isUpload;	//true:upload  false:download

	bool total_size_calced;
	long total_size;
	long total_trans_size;
	int total_trans_percentage;

	int file_type;	//ICON_UNKNOWN / ICON_DIR / ICON_FILE
	long file_size;

	wxString local_dir, remote_dir;
	wxString transfering_filename;


	void Init(SFTP_Transfer_Progress_Handler *_handler, int action, bool _isUpload);
	void UpdateLocalRemoteDirInfo(wxString _local, wxString _remote);
	void BeginTransNewFile(wxString _file);
	void AddTransSize(int size);
	void UpdateProgress();
	void Done();
};
//} t_DU_data;

class SFTP_Transfer_Progress_Handler
{
public:
	enum { SFTP_EVENT_ADDJOB, SFTP_EVENT_TRANS_NEW_FILE,
		SFTP_EVENT_UPDATE_PROGRESS, SFTP_EVENT_JOB_DONE,
		SFTP_EVENT_UPDATE_LOCAL_REMOTE_DIR };

	virtual void OnSFTP_Transfer_Event(t_DU_data *du, int event_type) = 0;
};

// ============================================================================

class SCD_wxSFTP
{
private:
//	SCD_wxSSH *ssh;
	SFTP_SESSION *session;
	wxString path;

public:
	SSH_SESSION *ssh_session;	//temp

public:
	SCD_wxSFTP(/*SCD_wxSSH *_ssh, */SFTP_SESSION *_session);
	~SCD_wxSFTP();

	bool cd(wxString _path);
	bool cdup();
	wxString GetPath();
	wxString GetPath(wxString _path, bool must_exists = true);	//must_exists==false will return a path which maybe not exist

	//--- directory browsering
	SFTP_DIR* opendir(wxString _dir);
	SFTP_ATTRIBUTES* readdir(SFTP_DIR *dir);	//return NULL when reach-the-end or error
	bool isDir(SFTP_ATTRIBUTES *attr);
	int dir_eof(SFTP_DIR *dir);
	int closedir(SFTP_DIR *dir);
	void attributes_free(SFTP_ATTRIBUTES *file);

	SFTP_ATTRIBUTES* stat(char *file);

	//--- file io
	SFTP_FILE* openfile(char *file, int access, SFTP_ATTRIBUTES *attr = NULL);
	int readfile(SFTP_FILE *file, void *dest, int len);
	int writefile(SFTP_FILE *file, void *source, int len);
	int closefile(SFTP_FILE *file);
	void seek(SFTP_FILE *file, int new_offset);
	unsigned long tell(SFTP_FILE *file);
	void sftp_rewind(SFTP_FILE *file);

	//--- file manipulation
	bool rm(char *file);
	bool rmdir(char *directory);
	bool mkdir(char *directory, int permission = 040755);	//attr can't be NULL
	bool rename(char *original, char *newname);
	int setstat(char *file, SFTP_ATTRIBUTES *attr);

	//upload & download
	bool Upload(wxArrayString _local_file_list, wxString _remote_dir, SFTP_Transfer_Progress_Handler *_handler);
	bool Download(wxString _local_dir, wxArrayString _remote_file_list, SFTP_Transfer_Progress_Handler *_handler);
private:
	bool m_Upload(t_DU_data &du, wxString _local_file, wxString _remote_dir, wxString _transfering_file = wxEmptyString );
	bool m_Download(t_DU_data &du, wxString _local_dir, wxString _remote_file, wxString _transfering_file = wxEmptyString );

public:
};

SCD_wxSFTP* CreateSFTP(SCD_wxSSH *_ssh);

// ============================================================================
#endif
