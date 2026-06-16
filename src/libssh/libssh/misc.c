/* misc.c */
/* some misc routines than aren't really part of the ssh protocols but can be useful to the client */

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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#ifndef __WXMSW__
	#include <pwd.h>
#endif
#include <sys/types.h>

#ifdef __WXMSW__
	#include <winsock2.h>
#else
	#include <netdb.h>
#endif

#include "libssh/libssh.h"

/* if the program was executed suid root, don't trust the user ! */

static int is_trusted(){
	return 1;
/*
	if(geteuid()!=getuid())
        return 0;
    return 1;
*/
}

#ifndef __WXMSW__
static char *get_homedir_from_uid(int uid){
    struct passwd *pwd;
    char *home;
    while((pwd=getpwent())){
        if(pwd->pw_uid == uid){
            home=strdup(pwd->pw_dir);
            endpwent();
            return home;
        }
    }
    endpwent();
    return NULL;
}

static char *get_homedir_from_login(char *user){
    struct passwd *pwd;
    char *home;
    while((pwd=getpwent())){
        if(!strcmp(pwd->pw_name,user)){
            home=strdup(pwd->pw_dir);
            endpwent();
            return home;
        }
    }
    endpwent();
    return NULL;
}
#endif

char *ssh_get_user_home_dir(){
#ifdef __WXMSW__
	return strdup("C:\\");
#else
    char *home;
    char *user;
    int trusted=is_trusted();
    if(trusted){
        if((home=getenv("HOME")))
            return strdup(home);
        if((user=getenv("USER")))
            return get_homedir_from_login(user);
    }
    return get_homedir_from_uid(getuid());
#endif
}

/* we have read access on file */
int ssh_file_readaccess_ok(char *file){
    if(!access(file,R_OK))
        return 1;
    return 0;
}


unsigned long long ntohll(unsigned long long a){
#ifdef WORDS_BIGENDIAN
    return a;
#else
    unsigned long low=a & 0xffffffff;
    unsigned long high = a >> 32 ;
    low=ntohl(low);
    high=ntohl(high);
    return (( ((unsigned long long)low) << 32) | ( high));
#endif
}
