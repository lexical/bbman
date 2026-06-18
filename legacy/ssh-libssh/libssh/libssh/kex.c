/* kex.c is used well, in key exchange :-) */
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
#include "libssh/libssh.h"
#include "libssh/ssh2.h"
#ifdef HAVE_OPENSSL_BLOWFISH_H
#define BLOWFISH "blowfish-cbc"
#else
#define BLOWFISH ""
#endif
#ifdef HAVE_OPENSSL_AES_H
#define AES "aes256-cbc,aes192-cbc,aes128-cbc,"
#else
#define AES ""
#endif
#ifdef HAVE_LIBZ
#define ZLIB "none,zlib"
#else
#define ZLIB "none"
#endif
char *default_methods[]={
	"diffie-hellman-group1-sha1","ssh-dss,ssh-rsa",AES BLOWFISH, AES BLOWFISH,
	"hmac-sha1","hmac-sha1","none","none","","",NULL };
char *supported_methods[]={
    "diffie-hellman-group1-sha1","ssh-dss,ssh-rsa",AES BLOWFISH,AES BLOWFISH,
    "hmac-sha1","hmac-sha1",ZLIB,ZLIB,"","",NULL };
/* descriptions of the key exchange packet */
char *ssh_kex_nums[]={
	"kex algos","server host key algo","encryption client->server","encryption server->client",
	"mac algo client->server","mac algo server->client","compression algo client->server",
	"compression algo server->client","languages client->server","languages server->client",NULL};

/* tokenize will return a token of strings delimited by ",". the first element has to be freed */
static char **tokenize(char *chain){
    char **tokens;
    int n=1;
    int i=0;
    char *ptr=chain=strdup(chain);
    while(*ptr){
        if(*ptr==','){
            n++;
            *ptr=0;
        }
        ptr++;
    }
    /* now n contains the number of tokens, the first possibly empty if the list was empty too e.g. "" */
    tokens=malloc(sizeof(char *) * (n+1) ); /* +1 for the null */
    ptr=chain;
    for(i=0;i<n;i++){
        tokens[i]=ptr;
        while(*ptr)
            ptr++; // find a zero
        ptr++; // then go one step further
    }
    tokens[i]=NULL;
    return tokens;
}


/* find_matching gets 2 parameters : a list of available objects (in_d), separated by colons,*/
/* and a list of prefered objects (what_d) */
/* it will return a strduped pointer on the first prefered object found in the available objects list */

static char *find_matching(char *in_d, char *what_d){
    char ** tok_in, **tok_what;
    int i_in, i_what;
    char *ret;
    
    if( ! (in_d && what_d))
        return NULL; /* don't deal with null args */
    ssh_say(3,"find_matching(\"%s\",\"%s\") = ",in_d,what_d);
    tok_in=tokenize(in_d);
    tok_what=tokenize(what_d);
    for(i_in=0; tok_in[i_in]; ++i_in){
        for(i_what=0; tok_what[i_what] ; ++i_what){
            if(!strcmp(tok_in[i_in],tok_what[i_what])){
                /* match */            
                ssh_say(3,"\"%s\"\n",tok_in[i_in]);
                ret=strdup(tok_in[i_in]);
                /* free the tokens */
                free(tok_in[0]);
                free(tok_what[0]);
                free(tok_in);
                free(tok_what);
                return ret;
            }
        }
    }
    ssh_say(3,"NULL\n");
    free(tok_in[0]);
    free(tok_what[0]);
    free(tok_in);
    free(tok_what);
    return NULL;
}

int ssh_get_kex(SSH_SESSION *session){
    STRING *str;
    char *strings[11];
    int i=0;
    packet_wait(session,SSH2_MSG_KEXINIT);
    if(buffer_get_data(session->in_buffer,session->server_kex.cookie,16)!=16){
        ssh_set_error((session->connected?session:NULL),FATAL,"get_kex(): no cookie in packet");
        return -1;
    }
    hashbufin_add_cookie(session,session->server_kex.cookie);
    while(i<10){
        str=buffer_get_ssh_string(session->in_buffer);
        if(!str){
            i++;
            break;
        }
        buffer_add_ssh_string(session->in_hashbuf,str);
        strings[i]=string_to_char(str);
        free(str);
        i++;
    }
    strings[i]=NULL;
    /* copy the server kex info into an array of strings */
    session->server_kex.methods=malloc( (i+1) * sizeof(char **));
    memcpy(session->server_kex.methods,strings,(i+1)*sizeof(char **));
    return 0;
}

void list_kex(KEX *kex){
    int i=0;
#ifdef DEBUG_CRYPTO
    ssh_print_hexa("session cookie",kex->cookie,16);
#endif
    for(i=0;i<10;i++){
        ssh_say(2,"%s : %s\n",ssh_kex_nums[i],kex->methods[i]);
    }
}

/* set_kex basicaly look at the option structure of the session and set the output kex message */
/* it must be aware of the server kex message */
/* it can fail if option is null, not any user specified kex method matches the server one, if not any default kex matches */

int set_kex(SSH_SESSION *session){
    KEX *server = &session->server_kex;
    KEX *client=&session->client_kex;
    SSH_OPTIONS *options=session->options;
    int i;
    char *wanted;
    if(!options){
        ssh_set_error((session->connected?session:NULL),FATAL,"Options structure is null(client's bug)");
        return -1;
    }
    /* the client might ask for a specific cookie to be sent. useful for server debugging */
    if(options->wanted_cookie)
        memcpy(client->cookie,options->wanted_cookie,16);
    else
        ssh_get_random(client->cookie,16);
    client->methods=malloc(11 * sizeof(char **));
    for (i=0;i<10;i++){
        if(!(wanted=options->wanted_methods[i]))
            wanted=default_methods[i];
        client->methods[i]=find_matching(server->methods[i],wanted);
        if(!client->methods[i]){
            ssh_set_error((session->connected?session:NULL),FATAL,"kex error : did not find one of algos %s in list %s for %s",
            wanted,server->methods[i],ssh_kex_nums[i]);
            return -1;
        }
    }
    return 0;
}

/* this function only sends the predefined set of kex methods */    
void send_kex(SSH_SESSION *session){
    STRING *str;
    int i=0;
    KEX *kex=&session->client_kex;
    packet_clear_out(session);
    buffer_add_char(session->out_buffer,SSH2_MSG_KEXINIT);
    buffer_add_data(session->out_buffer,kex->cookie,16);
    hashbufout_add_cookie(session);
    list_kex(kex);
    for(i=0;i<10;i++){
        str=string_from_char(kex->methods[i]);
        buffer_add_ssh_string(session->out_hashbuf,str);
        buffer_add_ssh_string(session->out_buffer,str);
        free(str);
    }
    i=0;
    buffer_add_char(session->out_buffer,0);
    buffer_add_long(session->out_buffer,0);
    packet_send(session);
}

/* returns 1 if at least one of the name algos is in the default algorithms table */
int verify_existing_algo(int algo, char *name){
    char *ptr;
    if(algo>9 || algo <0)
        return -1;
    ptr=find_matching(supported_methods[algo],name);
    if(ptr){
        free(ptr);
        return 1;
    }
    return 0;
}
