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


#ifndef SCD_WXSFTP_CPP
#define SCD_WXSFTP_CPP
#include "scd_wxsftp.h"

#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wx/utils.h>
// ============================================================================

SCD_wxSFTP* CreateSFTP(SCD_wxSSH *_ssh)
{

	//**********************************

	SSH_SESSION *_tmp_session;
	SSH_OPTIONS *options;
	int auth=0;
    char hash[MD5_DIGEST_LEN];
	char *banner;

	options = options_new();
    options_set_host(options, wxStringToCharPtr(_ssh->ip) );
	options_set_port(options, _ssh->port );
    options_set_username(options, wxStringToCharPtr(_ssh->username) );

    _tmp_session = ssh_connect(options);	//嘗試連到遠端 ssh server
    if(!_tmp_session)	return NULL;	//連線失敗

    pubkey_get_hash(_tmp_session,hash);	//取得遠端 ssh server 的 hash code

    auth = ssh_userauth_autopubkey(_tmp_session);	//找出 localhost 上的 public key
    if(auth == AUTH_ERROR)	return NULL;	//如果 auth (非指登入) 失敗

	banner = ssh_get_issue_banner(_tmp_session);	//取得 banner (no use?)
	if(banner)	free(banner);

	if(auth == AUTH_SUCCESS);	//如果登入成功 (不須密碼) (anonymous login ?)
	else
	{
		bool b = (ssh_userauth_password(_tmp_session ,
			wxStringToCharPtr(_ssh->username),
			wxStringToCharPtr(_ssh->password) ) == AUTH_SUCCESS );
		if(!b)	return NULL;
	}

	//**********************************

//	SFTP_SESSION *_sftp_session = sftp_new(_ssh->session);

	SFTP_SESSION *_sftp_session = sftp_new(_tmp_session);
	if( ! _sftp_session )	return NULL;

	if( sftp_init(_sftp_session) != 0 )	//init failed
	{
		sftp_free(_sftp_session);
		return NULL;
	}

	SCD_wxSFTP *_ftp = new SCD_wxSFTP(/*this, */_sftp_session);
	_ssh->sftp_count ++;	//add sftp count
	_ftp->ssh_session = _tmp_session;	//temp
	return _ftp;
}

// ----------------------------------------------------------------------------
SCD_wxSFTP::SCD_wxSFTP(/*SCD_wxSSH *_ssh, */SFTP_SESSION *_session)
{
//	ssh = _ssh;
	session = _session;
	char *str = sftp_canonicalize_path(session, ".");
	path = str;
	free(str);
}
// ----------------------------------------------------------------------------
SCD_wxSFTP::~SCD_wxSFTP()
{
	sftp_free(session);
	session = NULL;
	ssh_disconnect(ssh_session);	//temp
//	ssh->DeleteSFTP(this);
//wxMessageBox( _T("~SCD_wxSFTP") );
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool SCD_wxSFTP::cd(wxString _path)
{
	wxString new_path = GetPath(_path);
//wxMessageBox( _T("cd : ") + path + _T(" -> ") + new_path );
	if(new_path != path)
	{	path = new_path;	return true;	}
	else	return false;
}
// ----------------------------------------------------------------------------
bool SCD_wxSFTP::cdup()
{	return cd( _T("..") );	}
// ----------------------------------------------------------------------------
wxString SCD_wxSFTP::GetPath()
{	return path;	}
// ----------------------------------------------------------------------------
wxString SCD_wxSFTP::GetPath(wxString _path, bool must_exists)
{
	wxString new_path;
	char ch = _path.GetChar(0);
	if( ch=='/' || ch == '~' )	new_path = _path;
	else	new_path = path + _T("/") + _path;
	if(!must_exists)	return new_path;
	char *pstr = sftp_canonicalize_path(session, wxStringToCharPtr(new_path) );
	if(pstr==NULL)	return path;
	new_path = pstr;
	free(pstr);
	return new_path;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_wxSFTP::attributes_free(SFTP_ATTRIBUTES *file)
{	sftp_attributes_free(file);	}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
SFTP_DIR* SCD_wxSFTP::opendir(wxString _dir)
{
//wxMessageBox("opendir " + _dir);
	if( _dir.IsEmpty() )
		return sftp_opendir(session, wxStringToCharPtr(path) );
	else
	{
		wxString tmp_path = GetPath(_dir);
//		if(tmp_path == path)	return NULL;
		/*else*/ return sftp_opendir(session, wxStringToCharPtr(tmp_path) );
	}
}
// ----------------------------------------------------------------------------
SFTP_ATTRIBUTES* SCD_wxSFTP::readdir(SFTP_DIR *dir)
{	return sftp_readdir(session, dir);	}
// ----------------------------------------------------------------------------
bool SCD_wxSFTP::isDir(SFTP_ATTRIBUTES *attr)
{	return ( (attr->permissions & 0100000) == 0);	}
// ----------------------------------------------------------------------------
int SCD_wxSFTP::dir_eof(SFTP_DIR *dir)
{	return sftp_dir_eof(dir);	}
// ----------------------------------------------------------------------------
int SCD_wxSFTP::closedir(SFTP_DIR *dir)
{	return sftp_dir_close(dir);	}
// ----------------------------------------------------------------------------
SFTP_ATTRIBUTES* SCD_wxSFTP::stat(char *file)
{
	wxString path, filename, ext;
	wxSplitPath(file, &path, &filename, &ext);
	if( ! ext.IsEmpty() )	filename = filename + _T(".") + ext;

	SFTP_ATTRIBUTES *ret = NULL;
	SFTP_DIR *dir = this->opendir(path);
//wxMessageBox( wxString("stat ") + wxString(file) + " in " + path );
	if(dir==NULL)
	{	wxMessageBox("stat error");	return NULL;	}

	while(true)
	{
		SFTP_ATTRIBUTES *tmp = readdir(dir);
		if(tmp==NULL)	break;
		if( filename.CompareTo(tmp->name) == 0 )
		{	ret = tmp; break;	}
	}

	closedir(dir);
	return ret;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
SFTP_FILE* SCD_wxSFTP::openfile(char *file, int access, SFTP_ATTRIBUTES *attr)
{	return sftp_open(session, file, access, attr);	}
// ----------------------------------------------------------------------------
int SCD_wxSFTP::readfile(SFTP_FILE *file, void *dest, int len)
{	return sftp_read(file, dest, len);	}
// ----------------------------------------------------------------------------
int SCD_wxSFTP::writefile(SFTP_FILE *file, void *source, int len)
{	return sftp_write(file, source, len);	}
// ----------------------------------------------------------------------------
int SCD_wxSFTP::closefile(SFTP_FILE *file)
{	return sftp_file_close(file);	}
// ----------------------------------------------------------------------------
void SCD_wxSFTP::seek(SFTP_FILE *file, int new_offset)
{	sftp_seek(file, new_offset);	}
// ----------------------------------------------------------------------------
unsigned long SCD_wxSFTP::tell(SFTP_FILE *file)
{	return sftp_tell(file);	}
// ----------------------------------------------------------------------------
void SCD_wxSFTP::sftp_rewind(SFTP_FILE *file)
{	sftp_rewind(file);	}
// ----------------------------------------------------------------------------
bool SCD_wxSFTP::rm(char *file)
{	return sftp_rm(session, file) == 0;	}
// ----------------------------------------------------------------------------
bool SCD_wxSFTP::rmdir(char *directory)
{	return sftp_rmdir(session, directory) == 0;	}
// ----------------------------------------------------------------------------
bool SCD_wxSFTP::mkdir(char *directory, int permission)	//attr can't be NULL
{
	SFTP_ATTRIBUTES at;
	at.flags = SSH_FILEXFER_ATTR_PERMISSIONS;
	at.permissions = permission;
	at.name = directory;
//wxMessageBox( _T("mkdir ") + wxString(directory) );
	return (sftp_mkdir(session, wxStringToCharPtr(GetPath(directory, false)) , &at) == 0);
}
// ----------------------------------------------------------------------------
bool SCD_wxSFTP::rename(char *original, char *newname)
{
	return sftp_rename(session, wxStringToCharPtr(GetPath(original, false))
			, wxStringToCharPtr(GetPath(newname, false)) ) == 0;
}
// ----------------------------------------------------------------------------
int SCD_wxSFTP::setstat(char *file, SFTP_ATTRIBUTES *attr)
{	return sftp_setstat(session, file, attr);	}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool SCD_wxSFTP::m_Upload(t_DU_data &du, wxString _local_file, wxString _remote_dir, wxString _transfering_file)
{
	if( du.deep == 0 )
	{
		wxString _local_dir;
		if( wxDirExists(_local_file) )	_local_dir = _local_file;
		else	_local_dir = wxPathOnly(_local_file);
		du.UpdateLocalRemoteDirInfo( _local_dir, _remote_dir );
	}

	wxString next_remote_dir = _remote_dir + _T("/") + wxFileNameFromPath(_local_file);

	if( wxDirExists(_local_file) )	//如果 _local_file 是資料夾
	{
		wxString next_local_file;

		if( du.action == ACTION_DU )
			this->mkdir( wxStringToCharPtr(next_remote_dir) );	//在遠端建立資料夾

		DIR *dp;
		struct dirent *ep;

		dp = ::opendir( wxStringToCharPtr(_local_file) );
		if(dp)
		{
			while( (ep = ::readdir (dp)) != NULL )
			{
				if( ep->d_name[0] == '.'
					&& ( ep->d_name[1] == '\0' || (ep->d_name[1] == '.' && ep->d_name[2] == '\0') ) )
					continue;

				next_local_file = _local_file + _T("/") + ep->d_name;
//				du.dlg->Update(0, _T("calculate total size : ") + _local_file );
//				if( du.action == ACTION_CALC_TOTAL_SIZE )	du.BeginTransNewFile(next_local_file);
				du.deep ++;
				m_Upload(du, next_local_file, next_remote_dir, _transfering_file + CharPtrTowxString(ep->d_name) + _T("/") );
				du.deep --;
//				if( du.action == ACTION_CALC_TOTAL_SIZE )	du.BeginTransNewFile(_local_file);
			}
		}
		::closedir(dp);

	}
	else
	{
		//取得檔案大小
		struct stat stat_buf;
		::stat( wxStringToCharPtr(_local_file) , &stat_buf );
		if( du.action == ACTION_CALC_TOTAL_SIZE )
		{
			du.total_size += stat_buf.st_size;
			return true;
		}
		else if( ! du.total_size_calced )
			du.total_size = stat_buf.st_size;



//wxMessageBox( _T("Upload local file :\n") + _local_file + _T("\n\nto remote :\n") + next_remote_dir );
		bool error_occured = false;
		SFTP_FILE *rf = this->openfile( wxStringToCharPtr(next_remote_dir) , O_WRONLY | O_CREAT /* | O_BINARY */ );
		if(!rf)	return false;
//wxMessageBox("start transffer");
/*
		struct stat *stat_buf = new struct stat;
		if( fstat(fd, stat_buf) == -1 );	//error occur
		int filesize = stat_buf->st_size;
		delete stat_buf;
*/


		#define BUFFER_SIZE 4096
		char buf[BUFFER_SIZE];
		int read_len;
		FILE *fp = fopen( wxStringToCharPtr(_local_file) , "rb" );
		if(fp==NULL)
		{	this->closefile(rf);	return false;	}

		du.BeginTransNewFile( _transfering_file + wxFileNameFromPath(_local_file) );
		while(true)	//儲存檔案內容
		{
			read_len = fread(buf, sizeof(char), BUFFER_SIZE, fp);
//wxMessageBox( wxString::Format("len = %d", read_len) );
			if(read_len == 0)	break;	//eof
			else if(read_len == -1)
			{	error_occured = true;	break;	}	//error
			else if(read_len != this->writefile(rf, buf, read_len))	//上傳檔案資料
			{	error_occured = true;	break;	}	//error

			du.AddTransSize(read_len);
		}
		fclose(fp);
		this->closefile(rf);

//wxMessageBox("end transffer");
		if(!rf || error_occured)	return false;
	}

	return true;
}
// ----------------------------------------------------------------------------
bool SCD_wxSFTP::Upload(wxArrayString _local_file_list, wxString _remote_dir, SFTP_Transfer_Progress_Handler *_handler)
{
	t_DU_data du;
	du.Init( _handler, t_DU_data::ICON_UPLOAD, true );

	int c = _local_file_list.GetCount();

	//先計算需要傳輸的資料大小
	du.action = ACTION_CALC_TOTAL_SIZE;
	du.total_size_calced = true;
	for(int i=0;i<c;i++)
		m_Upload( du , _local_file_list.Item(i), _remote_dir);

//	wxMessageBox( wxString::Format("total size : %d", du.total_size) );
	//開始傳輸資料
	du.action = ACTION_DU;
	du.total_trans_size = 0;
	for(int i=0;i<c;i++)
	{
		du.file_type = t_DU_data::ICON_UNKNOWN;
		m_Upload( du , _local_file_list.Item(i), _remote_dir);
	}

	du.Done();
//	_ssh->DeleteSFTP(_sftp);
	return true;
}
// ----------------------------------------------------------------------------
bool SCD_wxSFTP::m_Download(t_DU_data &du, wxString _local_dir, wxString _remote_file, wxString _transfering_file)
{
//if( du.action == ACTION_DU )	wxMessageBox( _T("m_Download() : ") + _local_dir + _T(" <= ") + _remote_file );
	if( du.file_type == t_DU_data::ICON_UNKNOWN )	//如果不知道 _remote_file 是否為資料夾
	{
		SFTP_DIR *rf = this->opendir( wxStringToCharPtr(_remote_file) );
		if(!rf)	du.file_type = t_DU_data::ICON_FILE;
		else
		{
			this->closedir(rf);
			du.file_type = t_DU_data::ICON_DIR;
		}
	}

	if( du.deep == 0 )
	{
		wxString _remote_dir;
		if( du.file_type == t_DU_data::ICON_DIR )	_remote_dir = _remote_file;
		else	_remote_dir = wxPathOnly(_remote_file);
		du.UpdateLocalRemoteDirInfo( _local_dir, _remote_dir );
	}


	wxString next_local_dir = _local_dir + _T("/") + wxFileNameFromPath(_remote_file);

	if( du.file_type == t_DU_data::ICON_DIR )	//如果 _remote_file 是資料夾
	{
		SFTP_DIR *rd = this->opendir( wxStringToCharPtr(_remote_file) );
		if(!rd)	{	wxMessageBox("error a");	return false;	}

		wxString next_remote_file;

		if( du.action == ACTION_DU )
		{
			if( ! wxMkdir(next_local_dir) )	//在 local 端建立資料夾
			{
				this->closedir(rd);
				{	wxMessageBox("error b");	return false;	}
			}
		}

		while(true)
		{
			SFTP_ATTRIBUTES *attr = this->readdir(rd);
			if(attr==NULL)	break;

			du.file_type = this->isDir(attr) ? t_DU_data::ICON_DIR : t_DU_data::ICON_FILE;
			if( du.file_type == t_DU_data::ICON_DIR
				&& attr->name[0] == '.'
				&& ( attr->name[1] == '\0' || (attr->name[1] == '.' && attr->name[2] == '\0') ) )
				continue;

			bool b = true;

			if( du.action == ACTION_CALC_TOTAL_SIZE && du.file_type == t_DU_data::ICON_FILE )
			{
				du.total_size += attr->size;
			}
			else	//dir 和 file 都可能進入此區
			{
				du.file_size = attr->size;

				next_remote_file = _remote_file + _T("/") + wxString(attr->name);

//				if( du.action == ACTION_CALC_TOTAL_SIZE )	du.dlg->Update(0, _T("calculate total size : ") + next_remote_file );

				wxString tmp;
				if( du.file_type == t_DU_data::ICON_DIR)	tmp = _transfering_file + CharPtrTowxString(attr->name) + _T("/");
				else tmp = _transfering_file;

				if( du.action == ACTION_CALC_TOTAL_SIZE )	du.BeginTransNewFile(tmp);
				du.deep ++;
				b = m_Download( du , next_local_dir , next_remote_file , tmp );
				du.deep --;
				if( du.action == ACTION_CALC_TOTAL_SIZE )	du.BeginTransNewFile(_transfering_file);

				if(!b)	wxMessageBox("error download " + next_remote_file + " to " + next_local_dir );
			}

			this->attributes_free(attr);
			if(!b)	break;
		}

		this->closedir(rd);
	}
	else
	{
		if( du.action == ACTION_CALC_TOTAL_SIZE
			|| ( du.action == ACTION_DU && du.file_size < 0 ) )
		{
			int size;
			SFTP_ATTRIBUTES *attr = this->stat( wxStringToCharPtr(_remote_file) );
			if(attr == NULL)	return false;	//找不到遠端檔案

//			du.total_size += attr->size;
			size = attr->size;
			this->attributes_free(attr);

			if( du.action == ACTION_CALC_TOTAL_SIZE )
			{	du.total_size += size;	return true;	}
			else if( ! du.total_size_calced )	du.total_size = size;
		}
		else
		{
			if( ! du.total_size_calced ) du.total_size = du.file_size;
		}


		bool error_occured = false;
		SFTP_FILE *rf = this->openfile( wxStringToCharPtr(_remote_file) , O_RDONLY );
		if(!rf)	return false;

		FILE *lf = fopen( wxStringToCharPtr(next_local_dir) , "wb" );
		if(!lf)	{	this->closefile(rf);	return false;	}



		#define BUFFER_SIZE 4096
		char buf[BUFFER_SIZE];
		int read_len;

		du.BeginTransNewFile( _transfering_file + wxFileNameFromPath(_remote_file) );
		while(true)	//儲存檔案內容
		{
			read_len = this->readfile(rf, buf, BUFFER_SIZE);

			if(read_len == 0)	break;	//eof
			else if(read_len == -1)
			{	error_occured = true;	break;	}	//error
			else if(read_len != (int)fwrite(buf, sizeof(char), read_len, lf))	//下載檔案資料
			{	error_occured = true;	break;	}	//error

			du.AddTransSize(read_len);
		}

		this->closefile(rf);
		fclose(lf);

		if(!rf || error_occured)	return false;
	}

	du.file_type = t_DU_data::ICON_UNKNOWN;
	du.file_size = -1;
	return true;
}
// ----------------------------------------------------------------------------
bool SCD_wxSFTP::Download(wxString _local_dir, wxArrayString _remote_file_list, SFTP_Transfer_Progress_Handler *_handler)
{
	t_DU_data du;
	du.Init( _handler, t_DU_data::ICON_DOWNLOAD, false);

	int c = _remote_file_list.GetCount();

	//先計算需要傳輸的資料大小

	du.action = ACTION_CALC_TOTAL_SIZE;
	du.total_size_calced = true;
	du.total_size = 0;
	for(int i=0;i<c;i++)
		m_Download( du , _local_dir, _remote_file_list.Item(i));

//	wxMessageBox( wxString::Format("total size : %d", du.total_size) );
	//開始傳輸資料
	du.action = ACTION_DU;
	du.total_trans_size = 0;
	for(int i=0;i<c;i++)
	{
//		wxMessageBox( "download " + _remote_file_list.Item(i) + " to " + _local_dir );
		du.file_type = t_DU_data::ICON_UNKNOWN;
		if( ! m_Download( du , _local_dir, _remote_file_list.Item(i)) )
			wxMessageBox("download error");
	}

	du.Done();
//	_ssh->DeleteSFTP(_sftp);
	return true;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void t_DU_data::Init(SFTP_Transfer_Progress_Handler *_handler, int action, bool _isUpload)
{
	static int _id = 0;
	id = _id ++;
	handler = _handler;
	deep = 0;

//	long item = list->InsertItem( list->GetItemCount() , _T("test") , (action == ICON_UPLOAD)?ICON_UPLOAD:ICON_DOWNLOAD );
//	list->SetItemData(item, id);

	action = ACTION_DU;
	total_size_calced = false;
	total_size = total_trans_size = 0;
	total_trans_percentage = 0;
	isUpload = _isUpload;

	file_type = ICON_UNKNOWN;
	file_size = -1;

	handler->OnSFTP_Transfer_Event(this, SFTP_Transfer_Progress_Handler::SFTP_EVENT_ADDJOB);
}
// ----------------------------------------------------------------------------
void t_DU_data::UpdateLocalRemoteDirInfo(wxString _local, wxString _remote)
{
	local_dir = _local;
	remote_dir = _remote;
	handler->OnSFTP_Transfer_Event(this, SFTP_Transfer_Progress_Handler::SFTP_EVENT_UPDATE_LOCAL_REMOTE_DIR);
}
// ----------------------------------------------------------------------------
void t_DU_data::BeginTransNewFile(wxString _file)
{
	transfering_filename = _file;

	//如果事先沒有計算傳輸總大小 -> 以此檔案大小作為分母
	//否則 -> 以傳輸總大小作為分母
	if( ! total_size_calced )
	{
		total_trans_size = 0;
		total_trans_percentage = 0;
	}

	handler->OnSFTP_Transfer_Event(this, SFTP_Transfer_Progress_Handler::SFTP_EVENT_TRANS_NEW_FILE);
	UpdateProgress();
}
// ----------------------------------------------------------------------------
void t_DU_data::AddTransSize(int size)
{
	total_trans_size += size;
	UpdateProgress();
}
// ----------------------------------------------------------------------------
void t_DU_data::UpdateProgress()
{
	if(total_size > 0)	//顯示傳輸進度
	{
		int perc = total_trans_size * 100 / total_size;
		if( perc >= 99 )	perc = 99;
		if( total_trans_percentage != perc )
		{
			if( total_trans_percentage == 0 && perc > 90 );	//如果這檔案傳輸的太快就不要更新 progress bar
			else
			{
				handler->OnSFTP_Transfer_Event(this, SFTP_Transfer_Progress_Handler::SFTP_EVENT_UPDATE_PROGRESS);
/*
				long item = FindItem();
				if(item >= 0)
				{
//					list->SetItem( item , 2 , wxString::Format("%02d%%", perc) );
//					AppProcessOtherEvents();
				}
*/
			}
			total_trans_percentage = perc;
		}
//wxMessageBox( wxString::Format("%d / %d", total_trans_size, total_size) );
	}
}
// ----------------------------------------------------------------------------
void t_DU_data::Done()
{
//	long item = FindItem();
//	if(item >= 0)	list->DeleteItem(item);
	handler->OnSFTP_Transfer_Event(this, SFTP_Transfer_Progress_Handler::SFTP_EVENT_JOB_DONE);
}
// ----------------------------------------------------------------------------

// ============================================================================
#endif
