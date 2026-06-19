/******************************************************************************
 * Name:        scd_pty_ssh.h
 * Purpose:     SSH transport backed by system OpenSSH over a pseudo terminal.
 ******************************************************************************/

#ifndef SCD_PTY_SSH_H
#define SCD_PTY_SSH_H

#include "common.h"
#include <wx/socket.h>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <sys/types.h>

class SCD_PtySSH
{
private:
	int master_fd;
	pid_t child_pid;
	std::atomic<bool> connected;
	std::atomic<bool> stopping;
	std::atomic<bool> input_event_pending;
	wxEvtHandler *evt_handler;
	int evt_id;
	void *client_data;
	wxUint32 last_count;
	std::deque<char> input_buf;
	std::mutex input_mutex;
	std::thread reader_thread;

	void ReaderLoop();
	void PostSocketEvent(wxSocketNotify event_type);
	void StopChild();

public:
	SCD_PtySSH();
	~SCD_PtySSH();

	bool Connect(wxIPV4address& address, wxString username);
	void Close();
	bool Destroy();

	bool IsDisconnected();
	bool IsConnected();

	void Read(void *buffer, wxUint32 nbytes);
	void Write(const void *buffer, wxUint32 nbytes);
	void Unread(const void *buffer, wxUint32 nbytes);
	wxUint32 LastCount();

	void SetWindowSize(int cols, int rows);
	void SetEventHandler(wxEvtHandler& handler, int id = -1);
	void SetClientData(void *data);
	void* GetClientData();
};

#endif
