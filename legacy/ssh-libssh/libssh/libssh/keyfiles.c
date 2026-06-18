/* keyfiles.c */
/* This part of the library handles private and public key files needed for publickey authentication,*/
/* as well as servers public hashes verifications and certifications. Lot of code here handles openssh */
/* implementations (key files aren't standardized yet). */

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
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <openssl/pem.h>
#include <openssl/dsa.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include "libssh/libssh.h"
#define MAXLINESIZE 80

static int default_get_password(char *buf, int size,int rwflag, char *descr){
	return 0;
/*jakky1
    char *pass;
    char buffer[256];
    int len;
    snprintf(buffer,256,"Please enter passphrase for %s",descr);
    pass=getpass(buffer);
    snprintf(buf,size,"%s",buffer);
    len=strlen(buf);
    memset(pass,0,strlen(pass));
    return len;
*/
}

/* in case the passphrase has been given in parameter */
static int get_password_specified(char *buf,int size, int rwflag, char *password){
    snprintf(buf,size,"%s",password);
    return strlen(buf);
}

/* TODO : implement it to read both DSA and RSA at once */
PRIVATE_KEY  *privatekey_from_file(SSH_SESSION *session,char *filename,int type,char *passphrase){
    FILE *file=fopen(filename,"r");
    PRIVATE_KEY *privkey;
    DSA *dsa=NULL;
    RSA *rsa=NULL;
    if(!file){
        ssh_set_error(session,REQUEST_DENIED,"Error opening %s : %s",filename,strerror(errno));
        return NULL;
    }
    if(type==TYPE_DSS){
        if(!passphrase){
            if(session && session->options->passphrase_function)
                dsa=PEM_read_DSAPrivateKey(file,NULL, session->options->passphrase_function,"DSA private key");
            else
                dsa=PEM_read_DSAPrivateKey(file,NULL,(void *)default_get_password, "DSA private key");
        }
        else
            dsa=PEM_read_DSAPrivateKey(file,NULL,(void *)get_password_specified,passphrase);
        fclose(file);
        if(!dsa){
            ssh_set_error(session,FATAL,"parsing private key %s : %s",filename,ERR_error_string(ERR_get_error(),NULL));
        return NULL;
        }
    }
    else if (type==TYPE_RSA){
        if(!passphrase){
            if(session && session->options->passphrase_function)
                rsa=PEM_read_RSAPrivateKey(file,NULL, session->options->passphrase_function,"RSA private key");
            else
                rsa=PEM_read_RSAPrivateKey(file,NULL,(void *)default_get_password, "RSA private key");
        }
        else
            rsa=PEM_read_RSAPrivateKey(file,NULL,(void *)get_password_specified,passphrase);
        fclose(file);
        if(!rsa){
            ssh_set_error(session,FATAL,"parsing private key %s : %s",filename,ERR_error_string(ERR_get_error(),NULL));
        return NULL;
        }
    } else {
        ssh_set_error(session,FATAL,"Invalid private key type %d",type);
        return NULL;
    }    
    
    privkey=malloc(sizeof(PRIVATE_KEY));
    privkey->type=type;
    privkey->dsa_priv=dsa;
    privkey->rsa_priv=rsa;
    return privkey;
}

void private_key_free(PRIVATE_KEY *prv){
    if(prv->dsa_priv)
        DSA_free(prv->dsa_priv);
    if(prv->rsa_priv)
        RSA_free(prv->rsa_priv);
    memset(prv,0,sizeof(PRIVATE_KEY));
    free(prv);
}

STRING *publickey_from_file(char *filename,int *_type){
    BUFFER *buffer;
    int type;
    STRING *str;
    char buf[4096]; /* noone will have bigger keys that that */
                        /* where have i head that again ? */
    int fd=open(filename,O_RDONLY);
    int r;
    if(fd<0)
        return NULL;
    if(read(fd,buf,8)!=8){
        close(fd);
        return NULL;
    }
    buf[7]=0;
    if(!strcmp(buf,"ssh-dss"))
        type=TYPE_DSS;
    else if (!strcmp(buf,"ssh-rsa"))
        type=TYPE_RSA;
    else {
        close(fd);
        return NULL;
    }
    r=read(fd,buf,4095);
    close(fd);
    if(r<=0){
        return NULL;
    }
    buf[r]=0;
    buffer=base64_to_bin(buf);
    str=string_new(buffer_get_len(buffer));
    string_fill(str,buffer_get(buffer),buffer_get_len(buffer));
    buffer_free(buffer);
    if(_type)
        *_type=type;
    return str;
}


/* why recursing ? i'll explain. on top, publickey_from_next_file will be executed until NULL returned */
/* we can't return null if one of the possible keys is wrong. we must test them before getting over */
STRING *publickey_from_next_file(SSH_SESSION *session,char **pub_keys_path,char **keys_path,
                            char **privkeyfile,int *type,int *count){
    static char *home=NULL;
    char public[256];
    char private[256];
    char *priv;
    char *pub;
    STRING *pubkey;
    if(!home)
        home=ssh_get_user_home_dir();
    if(home==NULL) {
        ssh_set_error(session,FATAL,"User home dir impossible to guess");
        return NULL;
    }
    ssh_set_error(session,LIBSSH_NO_ERROR,"no public key matched");
    if((pub=pub_keys_path[*count])==NULL)
        return NULL;
    if((priv=keys_path[*count])==NULL)
        return NULL;
    ++*count;
    /* are them readable ? */
    snprintf(public,256,pub,home);
    ssh_say(2,"Trying to open %s\n",public);
    if(!ssh_file_readaccess_ok(public)){
        ssh_say(2,"Failed\n");
        return publickey_from_next_file(session,pub_keys_path,keys_path,privkeyfile,type,count);
    } 
    snprintf(private,256,priv,home);
    ssh_say(2,"Trying to open %s\n",private);
    if(!ssh_file_readaccess_ok(private)){
        ssh_say(2,"Failed\n");
        return publickey_from_next_file(session,pub_keys_path,keys_path,privkeyfile,type,count);
    }
    ssh_say(2,"Okay both files ok\n");
    /* ok, we are sure both the priv8 and public key files are readable : we return the public one as a string,
        and the private filename in arguments */
    pubkey=publickey_from_file(public,type);
    if(!pubkey){
        ssh_say(2,"Wasn't able to open public key file %s : %s\n",public,ssh_get_error(session));
        return publickey_from_next_file(session,pub_keys_path,keys_path,privkeyfile,type,count);
    }
    *privkeyfile=realloc(*privkeyfile,strlen(private)+1);
    strcpy(*privkeyfile,private);
    return pubkey;
}
