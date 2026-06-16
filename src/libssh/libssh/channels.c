/* channels.c */
/* It has support for ... ssh channels */
/*
Copyright 2003 Aris Adamantiadis

This file is part of the SSH Library

The SSH Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version.

The SSH Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the SSH Library; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA. */

#include <string.h>
#include <stdlib.h>


#ifdef __WXMSW__
	#include <winsock2.h>
#else
	#include <netdb.h>
#endif

#include <unistd.h>
#include <stdio.h>

#include "libssh/libssh.h"
#include "libssh/ssh2.h"
#define WINDOWLIMIT 1024
#define WINDOWBASE 32000
CHANNEL *new_channel(SSH_SESSION *session){
    CHANNEL *channel=malloc(sizeof(CHANNEL));
    memset(channel,0,sizeof(CHANNEL));
    channel->session=session;
    if(!session->channels){
        session->channels=channel;
        channel->next=channel->prev=channel;
        return channel;
        }
    channel->next=session->channels;
    channel->prev=session->channels->prev;
    channel->next->prev=channel;
    channel->prev->next=channel;
    return channel;
}

/* assume you already closed it */
void channel_free(CHANNEL *channel){
    SSH_SESSION *session=channel->session;
    if(channel->next == channel){
        session->channels=NULL;
    } else {
        channel->prev->next=channel->next;
        channel->next->prev=channel->prev;
    }
    if(channel->stdout_buffer)
        buffer_free(channel->stdout_buffer);
    if(channel->stderr_buffer)
        buffer_free(channel->stderr_buffer);
    memset(channel,0,sizeof(CHANNEL));
    free(channel);
}

CHANNEL *open_session_channel(SSH_SESSION *session,int window,int maxpacket){
    CHANNEL *channel=new_channel(session);
    STRING *type=string_from_char("session");
    long foo;
    packet_clear_out(session);
    buffer_add_char(session->out_buffer,SSH2_MSG_CHANNEL_OPEN);
    channel->sender_channel=session->maxchannel++;
    channel->sender_maxpacket=maxpacket;
    channel->sender_window=window;
    ssh_say(2,"creating a channel %d with %d window and %d max packet\n",channel->sender_channel,
        window,maxpacket);
    buffer_add_ssh_string(session->out_buffer,type);
    buffer_add_long(session->out_buffer,htonl(channel->sender_channel));
    buffer_add_long(session->out_buffer,htonl(channel->sender_window));
    buffer_add_long(session->out_buffer,htonl(channel->sender_maxpacket));
    free(type);
    packet_send(session);
    ssh_say(2,"Sent a SSH_MSG_CHANNEL_OPEN for channel %d\n",channel->sender_channel);
    if(packet_read(session)){
        channel_free(channel);
        return NULL;
    }
    if(packet_translate(session)){
        channel_free(channel);
        return NULL;
    }
    switch(session->in_packet.type){
        case SSH2_MSG_CHANNEL_OPEN_CONFIRMATION:
            buffer_get_long(session->in_buffer,&foo);
            channel->recipient_channel=ntohl(foo);
            buffer_get_long(session->in_buffer,&foo);
            if(channel->sender_channel!=ntohl(foo)){
                ssh_set_error(session,INVALID_DATA,"server answered with sender chan num %d instead of given %d",
                ntohl(foo),channel->sender_channel);
                channel_free(channel);
                return NULL;
                }
            buffer_get_long(session->in_buffer,&foo);
            channel->recipient_window=ntohl(foo);
            buffer_get_long(session->in_buffer,&foo);
            channel->recipient_maxpacket=ntohl(foo);
            ssh_say(3,"Received a CHANNEL_OPEN_CONFIRMATION for channel %d:%d\n",channel->sender_channel,channel->recipient_channel);
            channel->open=1;
            return channel;
        case SSH2_MSG_CHANNEL_OPEN_FAILURE:
            {
                unsigned long code;
                STRING *error_s;
                char *error;
                buffer_get_long(session->in_buffer,&foo);
                buffer_get_long(session->in_buffer,&code);
                error_s=buffer_get_ssh_string(session->in_buffer);
                error=string_to_char(error_s);
                ssh_set_error(session,REQUEST_DENIED,"Channel opening failure : (%d) %s",ntohl(code),error);
                free(error);
                free(error_s);
                channel_free(channel);
                return NULL;
            }
        default:
            ssh_say(0,"Received packet %d\n say my author to support it !",session->in_packet.type);
            channel_free(channel);
            return NULL;
        }
    return NULL;
}

/* XXX should i keep those two functions ?*/
static CHANNEL *find_my_channel(SSH_SESSION *session,int num){
    // we assume we are always the sender
    CHANNEL *initchan,*channel;
    initchan=session->channels;
    for(channel=initchan;channel->sender_channel!=num;channel=channel->next){
        if(channel->next==initchan)
            return NULL;
    }
    return channel;
}

static CHANNEL *find_server_channel(SSH_SESSION *session,int num){
    CHANNEL *initchan,*channel;
    initchan=session->channels;
    for(channel=initchan;channel->recipient_channel !=num;channel=channel->next){
        if(channel==initchan)
            return NULL;
    }
    return channel;
}

static void grow_window(SSH_SESSION *session, CHANNEL *channel){
    int new_window=WINDOWBASE;
    packet_clear_out(session);
    buffer_add_char(session->out_buffer,SSH2_MSG_CHANNEL_WINDOW_ADJUST);
    buffer_add_long(session->out_buffer,htonl(channel->sender_channel));
    buffer_add_long(session->out_buffer,htonl(new_window));
    packet_send(session);
    ssh_say(3,"growing window to %d bytes\n",channel->sender_window + new_window);
    channel->sender_window+=new_window;
}

void change_window(SSH_SESSION *session){
    long channelnum;
    long bytes;
    CHANNEL *channel;
    int err;
    err=buffer_get_long(session->in_buffer,&channelnum);
    err += buffer_get_long(session->in_buffer,&bytes);
    if(err!= 2 * (sizeof(long))){
        ssh_say(0,"Error getting a window adjust message : invalid packet\n");
        return;
    }
    channelnum=ntohl(channelnum);
    bytes=ntohl(bytes);
    channel=find_my_channel(session,channelnum);
    if(!channel){
        ssh_say(0,"server specified invalid channel %d\n",channelnum);
        return ;
    }
    ssh_say(3,"Adding %d bytes to channel %d\n",bytes,channelnum);
    channel->recipient_window+=bytes;
}

/* is_stderr is set to 1 if the data are extended, ie stderr */
static void add_data(SSH_SESSION *session,int is_stderr){
    long channelnum;
    STRING *str;
    CHANNEL *channel;
    buffer_get_long(session->in_buffer,&channelnum);
    channel=find_my_channel(session,ntohl(channelnum));
    if(!channel){
        ssh_say(0,"Invalid channel %d\n",ntohl(channelnum));
        return;
    }
    str=buffer_get_ssh_string(session->in_buffer);

    if(!str){
        ssh_say(0,"Invalid data packet !\n");
        return;
    }
    ssh_say(3,"adding %d bytes data in %d\n",string_len(str),is_stderr);
    /* what shall we do in this case ? let's accept it anyway */
    if(string_len(str)>channel->sender_window)
        ssh_say(0,"Data packet too big for our window(%d vs %d)",string_len(str),channel->sender_window);
    if(!is_stderr){
        /* stdout */
        if(channel->write_fct){
            channel->write_fct(channel,str->string,string_len(str),channel->userarg);
        } else {
            channel_default_bufferize(channel,str->string,string_len(str),is_stderr);
        }
    } else {
        /* stderr */
        if(channel->write_err_fct){
            channel->write_err_fct(channel,str->string,string_len(str),channel->userarg);
        } else {
            channel_default_bufferize(channel,str->string,string_len(str),is_stderr);
        }
    }
    channel->sender_window-=string_len(str);
    if(channel->sender_window < WINDOWLIMIT)
        grow_window(session,channel); /* i wonder if this is the correct place to do that */
    free(str);
}

void channel_send_eof(CHANNEL *channel /*int channelnum */){
    SSH_SESSION *session=channel->session;
    packet_clear_out(session);
    buffer_add_char(session->out_buffer,SSH2_MSG_CHANNEL_EOF);
    buffer_add_long(session->out_buffer,htonl(channel->sender_channel /*channelnum */));
    packet_send(session);
    ssh_say(1,"Sent a EOF on client channel %d\n",channel->sender_channel );
    channel->sender_eof=1;
}

int channel_write(CHANNEL *channel ,void *data,int len){
    SSH_SESSION *session=channel->session;
    /* I should send what i can and delay the sending of the rest */
    if(channel->recipient_window<len){
        ssh_set_error(session,REQUEST_DENIED,"recipient channel window to small : %d",channel->recipient_window);
        len=channel->recipient_window;
    }
    packet_clear_out(session);
    buffer_add_char(session->out_buffer,SSH2_MSG_CHANNEL_DATA);
    buffer_add_long(session->out_buffer,htonl(channel->recipient_channel));
    buffer_add_long(session->out_buffer,htonl(len));
    buffer_add_data(session->out_buffer,data,len);
    packet_send(session);
    /* the draft is clever : what is included in the len ? */
    channel->recipient_window-=len;
    return len;
}

void channel_eof(SSH_SESSION *session){
    unsigned long channelnum;
    CHANNEL *channel;
    buffer_get_long(session->in_buffer,&channelnum);
    channelnum=ntohl(channelnum);
    channel=find_server_channel(session,channelnum);
    if(!channel){
        ssh_say(0,"Can't find server channel %d!\n",channelnum);
        return;
    }
    channel->recipient_window=0;
    channel->recipient_eof=1;
}

static void channel_close(SSH_SESSION *session){
    unsigned long channelnum;
    CHANNEL *channel;
    buffer_get_long(session->in_buffer,&channelnum);
    channelnum=ntohl(channelnum);
    channel=find_server_channel(session,channelnum);
    if(!channel){
        ssh_say(0,"Can't find server channel %d!\n",channelnum);
        return;
    }
    channel->open=0;
    channel->recipient_eof=1;
}

void channel_input_request(SSH_SESSION *session){
    unsigned long channelnum;
    STRING *request_s;
    char *request;
    long status;
    buffer_get_long(session->in_buffer,&channelnum);
    channelnum=ntohl(channelnum);
    if(!find_server_channel(session,channelnum)){
        ssh_say(0,"Cannot find channel %d\n",channelnum);
        return;
    }
    request_s=buffer_get_ssh_string(session->in_buffer);
    if(!request_s){
        ssh_say(0,"Invalid MSG_CHANNEL_REQUEST\n");
        return;
    }
    buffer_get_char(session->in_buffer,(unsigned char *)&status);
    request=string_to_char(request_s);
    if(!strcmp(request,"exit-status")){
        buffer_get_long(session->in_buffer,&status);
        status=ntohl(status);
/* XXX do something with status, we might need it */
        free(request_s);
        free(request);
        return ;
    }
    if(!strcmp(request,"exit-signal")){
        STRING *signal_s;
        char *signal;
        char *core="(core dumped)";
        char i;
        signal_s=buffer_get_ssh_string(session->in_buffer);
        if(!signal_s){
            ssh_say(0,"Invalid MSG_CHANNEL_REQUEST\n");
            free(request_s);
            free(request);
            return;
        }
        signal=string_to_char(signal_s);
        buffer_get_char(session->in_buffer,&i);
        if(!i)
            core="";
        ssh_say(0,"Remote connection closed by signal SIG%s %s\n",signal,core);
        free(signal_s);
        free(signal);
        free(request_s);
        free(request);
        return;
    }
    ssh_say(0,"Unknown request %s\n",request);
    free(request_s);
    free(request);
}

/* channel_handle is called by wait_packet, ie, when there is channel informations to handle . */
void channel_handle(SSH_SESSION *session, int type){
    ssh_say(3,"Channel_handle(%d)\n",type);
    switch(type){
        case SSH2_MSG_CHANNEL_WINDOW_ADJUST:
            change_window(session);
            break;
        case SSH2_MSG_CHANNEL_DATA:
            add_data(session,0);
            break;
        case SSH2_MSG_CHANNEL_EXTENDED_DATA:
            add_data(session,1);
            break;
        case SSH2_MSG_CHANNEL_EOF:
            channel_eof(session);
            break;
        case SSH2_MSG_CHANNEL_CLOSE:
            channel_close(session);
            break;
        case SSH2_MSG_CHANNEL_REQUEST:
            channel_input_request(session);
            break;
        default:
            ssh_say(0,"Unexpected message %d\n",type);
        }
}


int channel_request(CHANNEL *channel,char *request, BUFFER *buffer){
    STRING *request_s=string_from_char(request);
    SSH_SESSION *session=channel->session;
    int err;
    packet_clear_out(session);
    buffer_add_char(session->out_buffer,SSH2_MSG_CHANNEL_REQUEST);
    buffer_add_long(session->out_buffer,htonl(channel->recipient_channel));
    buffer_add_ssh_string(session->out_buffer,request_s);
    buffer_add_char(session->out_buffer,1);
    if(buffer)
        buffer_add_data(session->out_buffer,buffer_get(buffer),buffer_get_len(buffer));
    packet_send(session);
    ssh_say(3,"Sent a SSH_MSG_CHANNEL_REQUEST %s\n",request);
    free(request_s);
    err=packet_wait(session,SSH2_MSG_CHANNEL_SUCCESS);
    if(err)
        if(session->in_packet.type==SSH2_MSG_CHANNEL_FAILURE){
            ssh_say(2,"%s channel request failed\n",request);
            ssh_set_error(session,REQUEST_DENIED,"Channel request %s failed",request);
        }
        else
            ssh_say(3,"Received a %d code\n",session->in_packet.type);
    else
        ssh_say(3,"Received a SUCCESS\n");
    return err;
}

int channel_request_pty(CHANNEL *channel){
    STRING *term=string_from_char("xterm");
    BUFFER *buffer=buffer_new();
    int err;
    buffer_add_ssh_string(buffer,term);
    /* hu-ho. let's say they are "default" values */
    buffer_add_long(buffer,htonl(80));
    buffer_add_long(buffer,htonl(24));
    buffer_add_long(buffer,0);
    buffer_add_long(buffer,0);

    buffer_add_long(buffer,htonl(1));
    buffer_add_char(buffer,0);
    free(term);
    err=channel_request(channel,"pty-req",buffer);
    buffer_free(buffer);
    return err;
    return 0;
}

int channel_request_shell(CHANNEL *channel){
    int err=channel_request(channel,"shell",NULL);
    return err;
}

int channel_request_subsystem(CHANNEL *channel, char *system){
    BUFFER* buffer=buffer_new();
    int ret;
    STRING *subsystem=string_from_char(system);
    buffer_add_ssh_string(buffer,subsystem);
    free(subsystem);
    ret=channel_request(channel,"subsystem",buffer);
    buffer_free(buffer);
    return ret;
}
    
int channel_request_sftp( CHANNEL *channel){
    return channel_request_subsystem(channel, "sftp");
}
    

int channel_set_write_handler(CHANNEL *chan,
        void (*write_fct)(CHANNEL *channel, void *data, int len, void *userdefined),void *user){
    chan->write_fct=write_fct;
    chan->userarg=user;
    return 0;
}

int channel_set_stderr_write_handler(CHANNEL *chan,
        void (*write_err_fct)(CHANNEL *channel, void *data, int len, void *userdefined),void *user){
    chan->write_err_fct=write_err_fct;
    chan->userarg=user;
    return 0;
}

/* when data has been received from the ssh server, it can be applied to the known
    user function, with help of the callback, or inserted here */
void channel_default_bufferize(CHANNEL *channel, void *data, int len, int is_stderr){
    ssh_say(3,"placing %d bytes into channel buffer (stderr=%d)\n",len,is_stderr);
    if(!is_stderr){
        /* stdout */
        if(!channel->stdout_buffer)
            channel->stdout_buffer=buffer_new();
        buffer_add_data(channel->stdout_buffer,data,len);
    } else {
        /* stderr */
        if(!channel->stderr_buffer)
            channel->stderr_buffer=buffer_new();
        buffer_add_data(channel->stderr_buffer,data,len);
    }
}

/* reads into a channel and put result into buffer */
/* returns number of bytes read, 0 if eof or such and -1 in case of error */
/* if bytes != 0, the exact number of bytes are going to be read */
int channel_read(CHANNEL *channel, BUFFER *buffer,int bytes,int is_stderr){
    BUFFER *stdbuf=NULL;
    int len;
    buffer_reinit(buffer);
    /* maybe i should always set a buffer to avoid races between channel_default_bufferize and channel_read */
    if(channel->write_fct){
        ssh_set_error(channel->session,INVALID_REQUEST,"Specified channel hasn't got a default buffering system\n");
        return -1;
    }
    if(is_stderr){
        if(!channel->stderr_buffer)
            channel->stderr_buffer=buffer_new();
        stdbuf=channel->stderr_buffer;
    } else {
        if(!channel->stdout_buffer)
            channel->stdout_buffer=buffer_new();
        stdbuf=channel->stdout_buffer;
    }
    /* block reading if asked bytes=0 */
    while((buffer_get_rest_len(stdbuf)==0) || (buffer_get_rest_len(stdbuf) < bytes)){
        if(channel->recipient_eof && buffer_get_rest_len(stdbuf)==0)
            return 0;
        if(channel->recipient_eof)
            break; /* return the resting bytes in buffer */
        if(packet_read(channel->session)||packet_translate(channel->session))
            return -1;
        packet_parse(channel->session);
    }
    
    if(bytes==0){
        /* write the ful buffer informations */
        buffer_add_data(buffer,buffer_get_rest(stdbuf),buffer_get_rest_len(stdbuf));
        buffer_reinit(stdbuf);
    } else {
        len=buffer_get_rest_len(stdbuf);
        len= (len>bytes?bytes:len); /* read bytes bytes if len is greater, everything otherwise */
        buffer_add_data(buffer,buffer_get_rest(stdbuf),len);
        buffer_pass_bytes(stdbuf,len);
    }
    return buffer_get_len(buffer);
}

/* returns the number of bytes available, 0 if nothing is currently available, -1 if error */
int channel_poll(CHANNEL *channel, int is_stderr){
    BUFFER *buffer;
    if(is_stderr){
        buffer=channel->stderr_buffer;
        if(!buffer)
            buffer=channel->stderr_buffer=buffer_new();
    } else {
        buffer=channel->stdout_buffer;
        if(!buffer)
            buffer=channel->stdout_buffer=buffer_new();
    }
    while(buffer_get_len(buffer)==0){
        if(ssh_fd_poll(channel->session)){
            if(packet_read(channel->session)||packet_translate(channel->session))
                return -1;
            packet_parse(channel->session);
        } else
            return 0; /* nothing is available has said fd_poll */
    }
    return buffer_get_len(buffer);
}
