/* auth.c deals with authentication methods */
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

#include "libssh/libssh.h"
#include "libssh/ssh2.h"
#include <string.h>

static int ask_userauth(SSH_SESSION *session){
    if(session->auth_service_asked)
        return 0;
    else {
        if(ssh_service_request(session,"ssh-userauth"))
            return -1;
        else
            session->auth_service_asked++;
    }
    return 0;
    
}


static int wait_auth_status(SSH_SESSION *session){
    int err=AUTH_ERROR;
    int cont=1;
    STRING *can_continue;
    char partial=0;
    char *c_cont;
    while(cont){
        if(packet_read(session))
            break;
        if(packet_translate(session))
            break;
        switch(session->in_packet.type){
            case SSH2_MSG_USERAUTH_FAILURE:
                can_continue=buffer_get_ssh_string(session->in_buffer);
                if(!can_continue || buffer_get_char(session->in_buffer,&partial)!=1 ){
                    ssh_set_error(session,INVALID_DATA,"invalid SSH_MSG_USERAUTH_FAILURE message");
                    return AUTH_ERROR;
                }
                c_cont=string_to_char(can_continue);
                if(partial){
                    err=AUTH_PARTIAL;
                    ssh_set_error(session,LIBSSH_NO_ERROR,"partial success, authentications that can continue : %s",c_cont);
                }
                else {
                    err=AUTH_DENIED;
                    ssh_set_error(session,REQUEST_DENIED,"Access denied. authentications that can continue : %s",c_cont);
                }
                free(can_continue);
                free(c_cont);
                cont=0;
                break;
            case SSH2_MSG_USERAUTH_PK_OK:
            case SSH2_MSG_USERAUTH_SUCCESS:
                err=AUTH_SUCCESS;
                cont=0;
                break;
            case SSH2_MSG_USERAUTH_BANNER:
                {
                    STRING *banner=buffer_get_ssh_string(session->in_buffer);
                    if(!banner){
                        ssh_say(1,"The banner message was invalid. continuing though\n");
                        break;
                    }
                    ssh_say(2,"Received a message banner\n");
                    if(session->banner)
                        free(session->banner); /* erase the older one */
                    session->banner=banner;
                    break;
                }
            default:
                packet_parse(session);
                break;
        }
    }
    return err;
}

/* use the "none" authentication question */

int ssh_userauth_none(SSH_SESSION *session,char *username){
    STRING *user;
    STRING *service;
    STRING *method;
    if(!username)
        if(!(username=session->options->username)){
            if(options_default_username(session->options))
                return AUTH_ERROR;
            else
                username=session->options->username;
        }
    if(ask_userauth(session))
        return AUTH_ERROR;
    user=string_from_char(username);
    method=string_from_char("none");
    service=string_from_char("ssh-connection");
    packet_clear_out(session);
    buffer_add_char(session->out_buffer,SSH2_MSG_USERAUTH_REQUEST);
    buffer_add_ssh_string(session->out_buffer,user);
    buffer_add_ssh_string(session->out_buffer,service);
    buffer_add_ssh_string(session->out_buffer,method);
    free(service);
    free(method);
    free(user);
    packet_send(session);
    return wait_auth_status(session);
}

int ssh_userauth_offer_pubkey(SSH_SESSION *session, char *username,int type, STRING *publickey){
    STRING *user;
    STRING *service;
    STRING *method;
    STRING *algo;
    int err=AUTH_ERROR;
    if(!username)
        if(!(username=session->options->username)){
            if(options_default_username(session->options))
                return AUTH_ERROR;
            else
                username=session->options->username;
        }
    if(ask_userauth(session))
        return AUTH_ERROR;
    user=string_from_char(username);
    service=string_from_char("ssh-connection");
    method=string_from_char("publickey");
    algo=string_from_char(ssh_type_to_char(type));

    packet_clear_out(session);
    buffer_add_char(session->out_buffer,SSH2_MSG_USERAUTH_REQUEST);
    buffer_add_ssh_string(session->out_buffer,user);
    buffer_add_ssh_string(session->out_buffer,service);
    buffer_add_ssh_string(session->out_buffer,method);
    buffer_add_char(session->out_buffer,0);
    buffer_add_ssh_string(session->out_buffer,algo);
    buffer_add_ssh_string(session->out_buffer,publickey);
    packet_send(session);
    err=wait_auth_status(session);
    free(user);
    free(method);
    free(service);
    free(algo);
    return err;
}

int ssh_userauth_pubkey(SSH_SESSION *session, char *username, STRING *publickey, PRIVATE_KEY *privatekey){
    STRING *user;
    STRING *service;
    STRING *method;
    STRING *algo;
    STRING *sign;
    int err=AUTH_ERROR;
    if(!username)
        if(!(username=session->options->username)){
            if(options_default_username(session->options))
                return err;
            else
                username=session->options->username;
        }
    if(ask_userauth(session))
        return err;
    user=string_from_char(username);
    service=string_from_char("ssh-connection");
    method=string_from_char("publickey");
    algo=string_from_char(ssh_type_to_char(privatekey->type));
    
    
    /* we said previously the public key was accepted */
    packet_clear_out(session);
    buffer_add_char(session->out_buffer,SSH2_MSG_USERAUTH_REQUEST);
    buffer_add_ssh_string(session->out_buffer,user);
    buffer_add_ssh_string(session->out_buffer,service);
    buffer_add_ssh_string(session->out_buffer,method);
    buffer_add_char(session->out_buffer,1);
    buffer_add_ssh_string(session->out_buffer,algo);
    buffer_add_ssh_string(session->out_buffer,publickey);
    sign=ssh_do_sign(session,session->out_buffer,privatekey);
    if(sign){
        buffer_add_ssh_string(session->out_buffer,sign);
        free(sign);
        packet_send(session);
        err=wait_auth_status(session);
    }
    free(user);
    free(service);
    free(method);
    free(algo);
    return err;
}

int ssh_userauth_password(SSH_SESSION *session,char *username,char *password){
    STRING *user;
    STRING *service;
    STRING *method;
    STRING *password_s;
    int err;
    if(!username)
        if(!(username=session->options->username)){
            if(options_default_username(session->options))
                return -1;
            else
                username=session->options->username;
        }
    if(ask_userauth(session))
        return -1;
    user=string_from_char(username);
    service=string_from_char("ssh-connection");
    method=string_from_char("password");
    password_s=string_from_char(password);

    packet_clear_out(session);
    buffer_add_char(session->out_buffer,SSH2_MSG_USERAUTH_REQUEST);
    buffer_add_ssh_string(session->out_buffer,user);
    buffer_add_ssh_string(session->out_buffer,service);
    buffer_add_ssh_string(session->out_buffer,method);
    buffer_add_char(session->out_buffer,0);
    buffer_add_ssh_string(session->out_buffer,password_s);
    free(user);
    free(service);
    free(method);
    memset(password_s,0,strlen(password)+4);
    free(password_s);
    packet_send(session);
    err=wait_auth_status(session);
    return err;
}

static char *keys_path[]={NULL,"%s/.ssh/identity","%s/.ssh/id_dsa","%s/.ssh/id_rsa",NULL};
static char *pub_keys_path[]={NULL,"%s/.ssh/identity.pub","%s/.ssh/id_dsa.pub","%s/.ssh/id_rsa.pub",NULL};

/* this function initialy was in the client */
/* but the fools are the ones who never change mind */
int ssh_userauth_autopubkey(SSH_SESSION *session){
    int count=1; /* bypass identity */
    int type=0;
    int err;
    STRING *pubkey;
    char *privkeyfile=NULL;
    PRIVATE_KEY *privkey;
    char *id=NULL;
    // always testing none
    err=ssh_userauth_none(session,NULL);
    if(err==AUTH_ERROR || err==AUTH_SUCCESS){
        return err;
    }
    if(session->options->identity){
        ssh_say(2,"Trying identity file %s\n",session->options->identity);
        keys_path[0]=session->options->identity;
        /* let's hope alloca exists */
        id=malloc(strlen(session->options->identity)+1 + 4);
        sprintf(id,"%s.pub",session->options->identity);
        pub_keys_path[0]=id;
        count =0;
    }
    while((pubkey=publickey_from_next_file(session,pub_keys_path,keys_path, &privkeyfile,&type,&count))){
        err=ssh_userauth_offer_pubkey(session,NULL,type,pubkey);
        if(err==AUTH_ERROR){
            if(id){
                pub_keys_path[0]=NULL;
                keys_path[0]=NULL;
                free(id);
            }
            free(pubkey);
            return err;
        } else
        if(err != AUTH_SUCCESS){
            ssh_say(2,"Public key refused by server\n");
            free(pubkey);
            continue;
        }
        /* pubkey accepted by server ! */
        privkey=privatekey_from_file(session,privkeyfile,type,NULL);
        if(!privkey){
            ssh_say(0,"Reading private key %s failed (bad passphrase ?)\n",privkeyfile);
            free(pubkey);
            continue; /* continue the loop with other pubkey */
        }
        err=ssh_userauth_pubkey(session,NULL,pubkey,privkey);
        if(err==AUTH_ERROR){
            if(id){
                pub_keys_path[0]=NULL;
                keys_path[0]=NULL;
                free(id);
            }
            free(pubkey);
            private_key_free(privkey);
            return err;
        } else
        if(err != AUTH_SUCCESS){
            ssh_say(0,"Weird : server accepted our public key but refused the signature\nit might be a bug of libssh\n");
            free(pubkey);
            private_key_free(privkey);
            continue;
        }
        /* auth success */
        ssh_say(1,"Authentication using %s success\n",privkeyfile);
        free(pubkey);
        private_key_free(privkey);
        free(privkeyfile);
        if(id){
            pub_keys_path[0]=NULL;
            keys_path[0]=NULL;
            free(id);
        }
        return AUTH_SUCCESS;
    }
    ssh_say(1,"Tried every public key, none matched\n");
    ssh_set_error(session,LIBSSH_NO_ERROR,"no public key matched");
    if(id){
        pub_keys_path[0]=NULL;
        keys_path[0]=NULL;
        free(id);
    }

    return AUTH_DENIED;
}
