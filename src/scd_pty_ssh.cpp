/******************************************************************************
 * Name:        scd_pty_ssh.cpp
 * Purpose:     SSH transport backed by system OpenSSH over a pseudo terminal.
 ******************************************************************************/

#include "scd_pty_ssh.h"

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <cstring>
#include <unistd.h>

SCD_PtySSH::SCD_PtySSH()
{
	master_fd = -1;
	child_pid = -1;
	connected = false;
	stopping = false;
	input_event_pending = false;
	evt_handler = NULL;
	evt_id = -1;
	client_data = NULL;
	last_count = 0;
}

SCD_PtySSH::~SCD_PtySSH()
{
	Destroy();
}

void SCD_PtySSH::SetEventHandler(wxEvtHandler& handler, int id)
{
	evt_handler = &handler;
	evt_id = id;
}

void SCD_PtySSH::SetClientData(void *data)
{
	client_data = data;
}

void* SCD_PtySSH::GetClientData()
{
	return client_data;
}

bool SCD_PtySSH::Connect(wxIPV4address& address, wxString username)
{
	if( connected ) return true;
	if( reader_thread.joinable() ) reader_thread.join();

	wxCharBuffer host_buf = address.Hostname().ToUTF8();
	wxString port_string = wxString::Format(_T("%d"), address.Service());
	wxCharBuffer port_buf = port_string.ToUTF8();
	wxCharBuffer user_buf = username.ToUTF8();

	if( !host_buf.data() || !port_buf.data() ) return false;

	pid_t pid = forkpty(&master_fd, NULL, NULL, NULL);
	if( pid < 0 )
	{
		master_fd = -1;
		return false;
	}

	if( pid == 0 )
	{
		setenv("TERM", "vt100", 1);
		if( user_buf.data() && user_buf.data()[0] != '\0' )
		{
			execlp("ssh", "ssh", "-tt", "-e", "none",
				"-o", "StrictHostKeyChecking=accept-new",
				"-o", "ServerAliveInterval=60",
				"-p", port_buf.data(), "-l", user_buf.data(),
				host_buf.data(), (char*)NULL);
		}
		else
		{
			execlp("ssh", "ssh", "-tt", "-e", "none",
				"-o", "StrictHostKeyChecking=accept-new",
				"-o", "ServerAliveInterval=60",
				"-p", port_buf.data(),
				host_buf.data(), (char*)NULL);
		}
		_exit(127);
	}

	child_pid = pid;
	int flags = fcntl(master_fd, F_GETFL, 0);
	if( flags >= 0 ) fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);

	connected = true;
	stopping = false;
	input_event_pending = false;
	reader_thread = std::thread(&SCD_PtySSH::ReaderLoop, this);
	PostSocketEvent(wxSOCKET_CONNECTION);
	return true;
}

void SCD_PtySSH::ReaderLoop()
{
	char buf[4096];
	while( !stopping )
	{
		ssize_t n = read(master_fd, buf, sizeof(buf));
		if( n > 0 )
		{
			bool should_post = false;
			{
				std::lock_guard<std::mutex> lock(input_mutex);
				input_buf.insert(input_buf.end(), buf, buf + n);
				should_post = !input_event_pending.exchange(true);
			}
			if( should_post ) PostSocketEvent(wxSOCKET_INPUT);
		}
		else if( n == 0 )
		{
			break;
		}
		else if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR )
		{
			usleep(10000);
		}
		else
		{
			break;
		}
	}

	connected = false;
	if( !stopping ) PostSocketEvent(wxSOCKET_LOST);
}

void SCD_PtySSH::PostSocketEvent(wxSocketNotify event_type)
{
	wxEvtHandler *handler = evt_handler;
	if( handler == NULL ) return;

	wxSocketEvent event(evt_id);
	event.m_event = event_type;
	event.m_clientData = client_data;
	wxPostEvent(handler, event);
}

void SCD_PtySSH::StopChild()
{
	stopping = true;
	input_event_pending = false;

	pid_t pid = child_pid;
	if( pid > 0 )
		kill(pid, SIGTERM);

	if( master_fd >= 0 )
	{
		close(master_fd);
		master_fd = -1;
	}

	if( reader_thread.joinable() )
		reader_thread.join();

	if( pid > 0 )
	{
		int status = 0;
		for( int i = 0; i < 200; i++ )
		{
			pid_t ret = waitpid(pid, &status, WNOHANG);
			if( ret == pid || (ret < 0 && errno == ECHILD) )
			{
				child_pid = -1;
				return;
			}
			usleep(10000);
		}

		kill(pid, SIGKILL);
		waitpid(pid, &status, 0);
		child_pid = -1;
	}
}

void SCD_PtySSH::Close()
{
	StopChild();
	connected = false;
	PostSocketEvent(wxSOCKET_LOST);
}

bool SCD_PtySSH::Destroy()
{
	evt_handler = NULL;
	StopChild();
	connected = false;
	return true;
}

bool SCD_PtySSH::IsDisconnected()
{
	return !connected;
}

bool SCD_PtySSH::IsConnected()
{
	return connected;
}

void SCD_PtySSH::Read(void *buffer, wxUint32 nbytes)
{
	bool needs_input_event = false;
	{
		std::lock_guard<std::mutex> lock(input_mutex);
		wxUint32 count = std::min<wxUint32>(nbytes, input_buf.size());
		char *out = static_cast<char*>(buffer);
		for( wxUint32 i = 0; i < count; i++ )
		{
			out[i] = input_buf.front();
			input_buf.pop_front();
		}
		last_count = count;
		if( input_buf.empty() )
			input_event_pending = false;
		else
			needs_input_event = true;
	}

	if( needs_input_event )
		PostSocketEvent(wxSOCKET_INPUT);
}

void SCD_PtySSH::Write(const void *buffer, wxUint32 nbytes)
{
	if( master_fd < 0 )
	{
		last_count = 0;
		return;
	}
	ssize_t n = write(master_fd, buffer, nbytes);
	last_count = (n > 0) ? n : 0;
}

void SCD_PtySSH::Unread(const void *buffer, wxUint32 nbytes)
{
	bool needs_input_event = false;
	{
		std::lock_guard<std::mutex> lock(input_mutex);
		const char *in = static_cast<const char*>(buffer);
		for( wxUint32 i = 0; i < nbytes; i++ )
			input_buf.push_front(in[nbytes - i - 1]);
		last_count = nbytes;
		needs_input_event = ! input_event_pending.exchange(true);
	}

	if( needs_input_event )
		PostSocketEvent(wxSOCKET_INPUT);
}

wxUint32 SCD_PtySSH::LastCount()
{
	return last_count;
}

void SCD_PtySSH::SetWindowSize(int cols, int rows)
{
	if( master_fd < 0 || cols <= 0 || rows <= 0 )
		return;

	struct winsize size;
	memset(&size, 0, sizeof(size));
	size.ws_col = cols;
	size.ws_row = rows;
	ioctl(master_fd, TIOCSWINSZ, &size);
}
