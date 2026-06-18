/* options.c */
/* handle pre-connection options */
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifndef __WXMSW__
	#include <pwd.h>
#endif
#include <sys/types.h>
#include "libssh/libssh.h"

SSH_OPTIONS *options_new(){
    SSH_OPTIONS *option=malloc(sizeof(SSH_OPTIONS));
    memset(option,0,sizeof(SSH_OPTIONS));
    option->port=22; /* set the default port */
    option->fd=-1; 
    return option;
}

void options_set_port(SSH_OPTIONS *opt, unsigned int port){
    opt->port=port;
}

void options_free(SSH_OPTIONS *opt){
    int i;
    if(opt->username)
        free(opt->username);
    if(opt->identity)
        free(opt->identity);
    /* we don't touch the banner. if the implementation did use it, they have to free it */
    if(opt->host)
        free(opt->host);
    if(opt->bindaddr)
        free(opt->bindaddr);
    for(i=0;i<10;i++)
        if(opt->wanted_methods[i])
            free(opt->wanted_methods[i]);
    memset(opt,0,sizeof(SSH_OPTIONS));
    free(opt);
}


void options_set_host(SSH_OPTIONS *opt, const char *hostname){
    char *ptr=strdup(hostname);
    char *ptr2=strchr(ptr,'@');
    if(opt->host) // don't leak memory
        free(opt->host);
    if(ptr2){
        *ptr2=0;
        opt->host=strdup(ptr2+1);
        if(opt->username)
            free(opt->username);
        opt->username=strdup(ptr);
        free(ptr);
    } else
        opt->host=ptr;
}

void options_set_fd(SSH_OPTIONS *opt, int fd){
    opt->fd=fd;
}

void options_set_bindaddr(SSH_OPTIONS *opt, char *bindaddr){
    opt->bindaddr=strdup(bindaddr);
}

void options_set_username(SSH_OPTIONS *opt,char *username){
    opt->username=strdup(username);
}

void options_set_identity(SSH_OPTIONS *opt, char *identity){
    opt->identity=strdup(identity);
}

/* what's the deal here ? some options MUST be set before authentication or key exchange,
 * otherwise default values are going to be used. what must be configurable :
 * Public key certification method *
 * key exchange method (dh-sha1 for instance)*
 * c->s, s->c ciphers *
 * c->s s->c macs *
 * c->s s->c compression (not implemented yet) */

/* they all return 0 if all went well, 1 or !=0 if not. the most common error is unmatched algo (unimplemented) */
/* don't forget other errors can happen if no matching algo is found in sshd answer */

int options_set_wanted_method(SSH_OPTIONS *opt,int method, char *list){
    if(method > 9 || method < 0){
        ssh_set_error(NULL,FATAL,"method %d out of range",method);
        return -1;
    }
    if( (!opt->use_nonexisting_algo) && !verify_existing_algo(method,list)){
        ssh_set_error(NULL,FATAL,"Setting method : no algorithm for method \"%s\" (%s)\n",ssh_kex_nums[method],list);
        return -1;
    }
    if(opt->wanted_methods[method])
        free(opt->wanted_methods[method]);
    opt->wanted_methods[method]=strdup(list);    
    return 0;
}

void set_use_nonexisting_algo(SSH_OPTIONS *opt){
    opt->use_nonexisting_algo=1;
}
/*
static char *get_username_from_uid(int uid){
    struct passwd *pwd;
    char *user;
    while((pwd=getpwent())){
        if(pwd->pw_uid == uid){
            user=strdup(pwd->pw_name);
            endpwent();
            return user;
        }
    }
    endpwent();
    ssh_set_error(NULL,FATAL,"uid %d doesn't exist !",uid);
    return NULL;
}
*/
/* this function must be called when no specific username has been asked. it has to guess it */
int options_default_username(SSH_OPTIONS *opt){
    char *user;
    if(opt->username)
        return 0;
    user=getenv("USER");
    if(user){
        opt->username=strdup(user);
        return 0;
    }
/*    user=get_username_from_uid(getuid());
    if(user){
        opt->username=user;
        return 0;
    }*/
    return -1;
}
/*
SSH_OPTIONS *ssh_getopt(int *argcptr, char **argv){
    int i;
    int argc=*argcptr;
    char *user=NULL;
    int port=22;
    int debuglevel=0;
    int usersa=0;
    int usedss=0;
    int compress=0;
    int cont=1;
    char *cipher=NULL;
    char *localaddr=NULL;
    char *identity=NULL;
    char **save=malloc(argc * sizeof(char *));
    int current=0;

    int saveoptind=optind; /* need to save 'em * /
    int saveopterr=opterr;
    SSH_OPTIONS *options;
    opterr=0; /* shut up getopt * /
    while(cont && ((i=getopt(argc,argv,"c:i:Cl:p:vb:rd12"))!=-1)){

        switch(i){
            case 'l':
                user=optarg;
                break;
            case 'p':
                port=atoi(optarg);
                break;
            case 'v':
                debuglevel++;
                break;
            case 'r':
                usersa++;
                break;
            case 'd':
                usedss++;
                break;
            case 'c':
                cipher=optarg;
                break;
            case 'i':
                identity=optarg;
                break;
            case 'b':
                localaddr=optarg;
                break;
            case 'C':
                compress++;
                break;
            case '2':
                break; /* only ssh2 support till now * /
            case '1':
                ssh_set_error(NULL,FATAL,"libssh does not support SSH1 protocol");
                cont =0;
                break;
            default:
                {
                char opt[3]="- ";
                opt[1]=optopt;
                save[current++]=strdup(opt);
                if(optarg)
                    save[current++]=argv[optind+1];
            }
        }
    }
    opterr=saveopterr;
    while(optind < argc)
        save[current++]=argv[optind++];
        
    if(usersa && usedss){
        ssh_set_error(NULL,FATAL,"either RSA or DSS must be chosen");
        cont=0;
    }
    ssh_set_verbosity(debuglevel);
    optind=saveoptind;
    if(!cont){
        free(save);
        return NULL;
    }
    /* first recopy the save vector into original's * /
    for(i=0;i<current;i++)
        argv[i+1]=save[i]; // don't erase argv[0]
    argv[current+1]=NULL;
    *argcptr=current+1;
    free(save);
    /* set a new option struct * /
    options=options_new();
    if(compress){
        if(options_set_wanted_method(options,KEX_COMP_C_S,"zlib"))
            cont=0;
        if(options_set_wanted_method(options,KEX_COMP_S_C,"zlib"))
            cont=0;
    }
    if(cont &&cipher){
        if(options_set_wanted_method(options,KEX_CRYPT_C_S,cipher))
            cont=0;
        if(cont && options_set_wanted_method(options,KEX_CRYPT_S_C,cipher))
            cont=0;
    }
    if(cont && usersa)
        if(options_set_wanted_method(options,KEX_HOSTKEY,"ssh-rsa"))
            cont=0;
    if(cont && usedss)
        if(options_set_wanted_method(options,KEX_HOSTKEY,"ssh-dss"))
            cont=0;
    if(cont && user)
        options_set_username(options,user);
    if(cont && identity)
        options_set_identity(options,identity);
    if(cont && localaddr)
        options_set_bindaddr(options,localaddr);
    options_set_port(options,port);
    if(!cont){
        options_free(options);
        return NULL;
    } else
        return options;   
}*/
