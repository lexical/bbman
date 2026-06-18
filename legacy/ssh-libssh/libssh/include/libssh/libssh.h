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

#ifndef _LIBSSH_H
#define _LIBSSH_H
#include <libssh/config.h>
#define LIBSSH_VERSION "libssh-0.1" // oops ..
#define MAX_PACKET_LEN 262144
#define OPENSSL_CRYPTO
#define OPENSSL_BIGNUMS

#ifdef __WXMSW__
//	#include <winsock2.h>
#else
	#include <netdb.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/select.h>
	#include <sys/time.h>
	#include <netinet/in.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* Define this if you want to debug crypto systems */
/* it's usefull when you are debugging the lib */
/*#define DEBUG_CRYPTO */

/* wrapper things */

#ifdef OPENSSL_CRYPTO
#include <openssl/dsa.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>
typedef SHA_CTX SHACTX;
typedef MD5_CTX MD5CTX;
typedef HMAC_CTX HMACCTX;
#define SHA_DIGEST_LEN SHA_DIGEST_LENGTH
#define MD5_DIGEST_LEN MD5_DIGEST_LENGTH

#endif /* OPENSSL_CRYPTO */
#ifdef OPENSSL_BIGNUMS
#include <openssl/bn.h>
typedef BIGNUM*  bignum;
typedef BN_CTX* bignum_CTX;

#define bignum_new() BN_new()
#define bignum_free(num) BN_clear_free(num)
#define bignum_set_word(bn,n) BN_set_word(bn,n)
#define bignum_bin2bn(bn,datalen,data) BN_bin2bn(bn,datalen,data)
#define bignum_bn2hex(num) BN_bn2hex(num)
#define bignum_rand(rnd, bits, top, bottom) BN_rand(rnd,bits,top,bottom)
#define bignum_ctx_new() BN_CTX_new()
#define bignum_ctx_free(num) BN_CTX_free(num)
#define bignum_mod_exp(dest,generator,exp,modulo,ctx) BN_mod_exp(dest,generator,exp,modulo,ctx)
#define bignum_num_bytes(num) BN_num_bytes(num)
#define bignum_num_bits(num) BN_num_bits(num)
#define bignum_is_bit_set(num,bit) BN_is_bit_set(num,bit)
#define bignum_bn2bin(num,ptr) BN_bn2bin(num,ptr)

#endif /* OPENSSL_BIGNUMS */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/* wrapper.c */
MD5CTX *md5_init(void);
void md5_update(MD5CTX *c, const void *data, unsigned long len);
void md5_final(unsigned char *md,MD5CTX *c);
SHACTX *sha1_init(void);
void sha1_update(SHACTX *c, const void *data, unsigned long len);
void sha1_final(unsigned char *md,SHACTX *c);
void sha1(unsigned char *digest,int len,unsigned char *hash);
#define HMAC_SHA1 1
#define HMAC_MD5 2
HMACCTX *hmac_init(const void *key,int len,int type);
void hmac_update(HMACCTX *c, const void *data, unsigned long len);
void hmac_final(HMACCTX *ctx,unsigned char *hashmacbuf,int *len);


/* strings and buffers */

typedef struct string_struct {
	unsigned long size;
	char string[MAX_PACKET_LEN];
} STRING;


typedef struct buffer_struct {
    char *data;
    int used;
    int allocated;
    int pos;
} BUFFER;

/* i should remove it one day */
typedef struct packet_struct {
	int valid;
	unsigned long len;
	unsigned char type;
} PACKET;

typedef struct {
	char cookie[16];
	char **methods;
} KEX;

typedef struct {
    int type;
    char *type_c; /* Don't free it ! it is static */
    DSA *dsa_pub;
    RSA *rsa_pub;
} PUBLIC_KEY;

typedef struct {
    int type;
    DSA *dsa_priv;
    RSA *rsa_priv;
} PRIVATE_KEY;

typedef struct {
    int type;
    DSA_SIG *dsa_sign;
    STRING *rsa_sign;
} SIGNATURE;


/* some types for public keys */
#define TYPE_DSS 1
#define TYPE_RSA 2
#define TYPE_RSA1 3


/* the offsets of methods */
#define KEX_ALGO 0
#define KEX_HOSTKEY 1
#define KEX_CRYPT_C_S 2
#define KEX_CRYPT_S_C 3
#define KEX_MAC_C_S 4
#define KEX_MAC_S_C 5
#define KEX_COMP_C_S 6
#define KEX_COMP_S_C 7
#define KEX_LANG_C_S 8
#define KEX_LANG_S_C 9

#define AUTH_SUCCESS 0
#define AUTH_DENIED 1
#define AUTH_PARTIAL 2
#define AUTH_ERROR -1
#define ERROR_BUFFERLEN 1024

typedef struct ssh_options_struct {
    char *clientbanner; /* explicit banner to send */
    char *username;
    char *host;
    char *bindaddr;
    char *identity;
    int fd; /* specificaly wanted file descriptor, don't connect host */
    int port;
    int dont_verify_hostkey; /* Don't spare time, don't check host key ! unneeded to say it's dangerous and not safe */
    int use_nonexisting_algo; /* if user sets a not supported algorithm for kex, don't complain */
    char *wanted_methods[10]; /* the kex methods can be choosed. better use the kex fonctions to do that */
    void *wanted_cookie; /* wants a specific cookie to be sent ? if null, generate a new one */
    void *passphrase_function; /* this functions will be called if a keyphrase is needed. look keyfiles.c for more info */
    } SSH_OPTIONS;

typedef struct ssh_crypto_struct {
    bignum e,f,x,k;
    char session_id[SHA_DIGEST_LEN];
    
    char encryptIV[SHA_DIGEST_LEN];
    char decryptIV[SHA_DIGEST_LEN];

    char decryptkey[SHA_DIGEST_LEN*2];
    char encryptkey[SHA_DIGEST_LEN*2];

    char encryptMAC[SHA_DIGEST_LEN];
    char decryptMAC[SHA_DIGEST_LEN];
    char hmacbuf[EVP_MAX_MD_SIZE];
    struct crypto_struct *in_cipher, *out_cipher; /* the cipher structures */
    STRING *server_pubkey;
    int do_compress_out; /* idem */
    int do_compress_in; /* don't set them, set the option instead */
    void *compress_out_ctx; /* don't touch it */
    void *compress_in_ctx; /* really, don't */
} CRYPTO;
        
typedef struct channel_struct {
    struct channel_struct *prev;
    struct channel_struct *next;
    struct sshsession *session; /* SSH_SESSION pointer */
    int sender_channel;
    int recipient_channel;
    int recipient_window;
    int recipient_eof; /* end of file received */
    int sender_window;
    int sender_eof;
    int recipient_maxpacket;
    int sender_maxpacket;
    int open; /* shows if the channel is still opened */
    void (*write_fct)(struct channel_struct *channel, void *data, int len, void *userarg);
    /* this write function is a callback on some userdefined function which is used for writing datas *coming from remote ssh* */
    /* use channel_write() to write into a ssh pipe */
    void (*write_err_fct)(struct channel_struct *channel, void *data, int len, void *userarg);
    /* same as write_fct for stderr */
    BUFFER *stdout_buffer;
    BUFFER *stderr_buffer;
    void *userarg;
} CHANNEL;

typedef struct sshsession {
    int fd;
    SSH_OPTIONS *options;
    char *serverbanner;
    char *clientbanner;
    int protoversion;
    int send_seq;
    int recv_seq;
    int connected; /* !=0 when the user got a session handle */
    int alive;
    int auth_service_asked;
    int datatoread; /* reading now on socket will not block */
    STRING *banner; /* that's the issue banner from the server */
    BUFFER *in_buffer;
    PACKET in_packet;
    BUFFER *out_buffer;
    KEX server_kex;
    KEX client_kex;
    BUFFER *in_hashbuf;
    BUFFER *out_hashbuf;
    CRYPTO *current_crypto;
    CRYPTO *next_crypto;  /* next_crypto is going to be used after a SSH2_MSG_NEWKEYS */
    CHANNEL *channels; /* linked list of channels */
    int maxchannel;
    int error_code;
    char error_buffer[ERROR_BUFFERLEN];
} SSH_SESSION;

#define CLIENTBANNER "SSH-2.0-" LIBSSH_VERSION

/* errors */

enum ssh_error {LIBSSH_NO_ERROR, REQUEST_DENIED, INVALID_REQUEST, CONNECTION_LOST,FATAL,INVALID_DATA};
void ssh_set_error(SSH_SESSION *session,enum ssh_error code,char *descr,...);
char *ssh_get_error(SSH_SESSION *session); /* returns a static char array */
enum ssh_error ssh_error_code(SSH_SESSION *session);
void ssh_say(int priority,char *format,...);
void ssh_set_verbosity(int num);

 /* There is a verbosity level */
 /* 3 : packet level */
 /* 2 : protocol level */
 /* 1 : functions level */
 /* 0 : important messages only */
 /* -1 : no messages */

/* in client.c */

SSH_SESSION *ssh_connect(SSH_OPTIONS *options);
void ssh_disconnect(SSH_SESSION *session);
int ssh_service_request(SSH_SESSION *session,char *service);
char *ssh_get_issue_banner(SSH_SESSION *session);

/* string.h */


/* makestring returns a newly allocated string from a char * ptr */
STRING *string_from_char(char *what);
/* it returns the string len in host byte orders. str->size is big endian warning ! */
int string_len(STRING *str);
STRING *string_new(unsigned long size);
/* string_fill copies the data in the string. it does NOT check for boundary so allocate enough place with new_string */
/* right before */
void string_fill(STRING *str,void *data,int len);
/* returns a newly allocated char array with the str string and a final nul caracter */
char *string_to_char(STRING *str);

/* in dh.c */
/* this function must be executed once before everything*/
void ssh_crypto_init();
/* DH key generation */
void dh_generate_e(SSH_SESSION *session);
void dh_generate_x(SSH_SESSION *session);
STRING *dh_get_e(SSH_SESSION *session);
void dh_import_f(SSH_SESSION *session,STRING *f_string);
void dh_import_pubkey(SSH_SESSION *session,STRING *pubkey_string);
void dh_build_k(SSH_SESSION *session);
void make_sessionid(SSH_SESSION *session);
/* add data for the final cookie */
void hashbufin_add_cookie(SSH_SESSION *session,unsigned char *cookie);
void hashbufout_add_cookie(SSH_SESSION *session);
/* useful for debug */
void ssh_print_hexa(char *descr,unsigned char *what, int len);
void generate_session_keys(SSH_SESSION *session);
void ssh_get_random(void *,int);
/* this one can be called by the client to see the hash of the public key before accepting it */
int pubkey_get_hash(SSH_SESSION *session,char hash[MD5_DIGEST_LEN]);
/* returns 1 if server signature ok, 0 otherwise. The NEXT crypto is checked, not the current one */
int signature_verify(SSH_SESSION *session,STRING *signature);

bignum make_string_bn(STRING *string);
STRING *make_bignum_string(BIGNUM *num);

/* in crypt.c */
unsigned long packet_decrypt_len(SSH_SESSION *session,char *crypted);
int packet_decrypt(SSH_SESSION *session, void *packet,unsigned int len);
char *packet_encrypt(SSH_SESSION *session,void *packet,unsigned int len);
 /* it returns the hmac buffer if exists*/
int packet_hmac_verify(SSH_SESSION *session,BUFFER *buffer,char *mac);

/* in packet.c */
void packet_clear_out(SSH_SESSION *session);
void packet_parse(SSH_SESSION *session);
void packet_send(SSH_SESSION *session);
int packet_read(SSH_SESSION *session);
int packet_translate(SSH_SESSION *session);
int packet_wait(SSH_SESSION *session,int type);

/* in connect.c */
int ssh_connect_host(const char *host,const char *bind_addr, int port);
SSH_SESSION *ssh_session_new();
int ssh_fd_poll(SSH_SESSION *session);
//int ssh_select(CHANNEL **channels,CHANNEL **outchannels, int maxfd, fd_set *readfds, struct timeval *timeout);

/* in kex.c */
extern char *ssh_kex_nums[];
void send_kex(SSH_SESSION *session);
void list_kex(KEX *kex);
int set_kex(SSH_SESSION *session);
int ssh_get_kex(SSH_SESSION *session);
int verify_existing_algo(int algo,char *name);

/* in keys.c */
char *ssh_type_to_char(int type);
void publickey_free(PUBLIC_KEY *key);
PUBLIC_KEY *publickey_make_dss(BUFFER *buffer);
PUBLIC_KEY *publickey_make_rsa(BUFFER *buffer);
PUBLIC_KEY *publickey_from_string(STRING *pubkey_s);
SIGNATURE *signature_from_string(STRING *signature,PUBLIC_KEY *pubkey,int needed_type);
void signature_free(SIGNATURE *sign);
STRING *ssh_do_sign(SSH_SESSION *session,BUFFER *sigbuf, PRIVATE_KEY *privatekey);

/* in keyfiles.c */

PRIVATE_KEY *privatekey_from_file(SSH_SESSION *session,char *filename,int type,char *passphrase);
void private_key_free(PRIVATE_KEY *prv);
STRING *publickey_from_file(char *filename,int *_type);
STRING *publickey_from_next_file(SSH_SESSION *session,char **pub_keys_path,char **keys_path,
                            char **privkeyfile,int *type,int *count);

/* in channels.c */
CHANNEL *open_session_channel(SSH_SESSION *session,int window,int maxpacket);
CHANNEL *open_sftp_channel(SSH_SESSION *session, int window, int maxpacket);
void channel_free(CHANNEL *channel);
int channel_request_pty(CHANNEL *channel);
int channel_request_shell(CHANNEL *channel);
int channel_request_subsystem(CHANNEL *channel, char *system);
int channel_request_sftp(CHANNEL *channel);
int channel_request(CHANNEL *channel,char *request, BUFFER *buffer);
void channel_handle(SSH_SESSION *session, int type);
int channel_write(CHANNEL *channel,void *data,int len);
void channel_default_bufferize(CHANNEL *channel, void *data, int len, int is_stderr);
int channel_set_write_handler(CHANNEL *channel,
    void (*write_fct)(CHANNEL *channel, void *data, int len, void *userdefined),
    void *user);
int channel_set_stderr_write_handler(CHANNEL *channel,
    void (*write_err_fct)(CHANNEL *channel, void *data, int len, void *userdefined),
    void *user);
void channel_send_eof(CHANNEL *channel);
int channel_read(CHANNEL *channel, BUFFER *buffer,int bytes,int is_stderr);
int channel_poll(CHANNEL *channel, int is_stderr);

/* in options.c */

SSH_OPTIONS *options_new();
void options_free(SSH_OPTIONS *opt);
int options_set_wanted_method(SSH_OPTIONS *opt,int method, char *list);
void options_set_username(SSH_OPTIONS *opt,char *username);
void options_set_port(SSH_OPTIONS *opt, unsigned int port);
SSH_OPTIONS *ssh_getopt(int *argcptr, char **argv);
/* this function must be called when no specific username has been asked. it has to guess it */
int options_default_username(SSH_OPTIONS *opt);
void options_set_host(SSH_OPTIONS *opt, const char *host);
/* don't connect to host, use fd instead */
void options_set_fd(SSH_OPTIONS *opt, int fd);
void options_set_bindaddr(SSH_OPTIONS *opt, char *bindaddr);
void options_set_identity(SSH_OPTIONS *opt, char *identity);

/* buffer.c */


void buffer_add_ssh_string(BUFFER *buffer,STRING *string);
void buffer_add_char(BUFFER *buffer, char data);
void buffer_add_long(BUFFER *buffer, unsigned long data);
void buffer_add_longlong(BUFFER *buffer,unsigned long long data);
void buffer_add_data(BUFFER *buffer, void *data, int len);
void buffer_add_data_begin(BUFFER *buffer,void *data,int len);

BUFFER *buffer_new();
void buffer_free(BUFFER *buffer);
void buffer_reinit(BUFFER *buffer);

/* buffer_get returns a pointer to the begining of the buffer. no position is taken into account */
void *buffer_get(BUFFER *buffer);
/* buffer_get_rest returns a pointer to the current position into the buffer */
void *buffer_get_rest(BUFFER *buffer);

/* same here */
int buffer_get_len(BUFFER *buffer);

/* buffer_get_rest_len returns the number of bytes which can be read */
int buffer_get_rest_len(BUFFER *buffer);

/* buffer_read_*() returns the number of bytes read, except for ssh strings */
int buffer_get_char(BUFFER *buffer,char *data);
int buffer_get_long(BUFFER *buffer,long *data);
int buffer_get_longlong(BUFFER *buffer, long long *data);

int buffer_get_data(BUFFER *buffer,void *data,int requestedlen);
/* buffer_get_ssh_string() is an exception. if the String read is too large or invalid, it will answer NULL. */
STRING *buffer_get_ssh_string(BUFFER *buffer);
/* buffer_pass_bytes acts as if len bytes have been read (used for padding) */
int buffer_pass_bytes_end(BUFFER *buffer,int len);
int buffer_pass_bytes(BUFFER *buffer, int len);

/* in base64.c */
BUFFER *base64_to_bin(char *source);

/* in auth.c */
/* these functions returns AUTH_ERROR is some serious error has happened,
  AUTH_SUCCESS if success,
  AUTH_PARTIAL if partial success,
  AUTH_DENIED if refused */
int ssh_userauth_none(SSH_SESSION *session,char *username);
int ssh_userauth_password(SSH_SESSION *session,char *username,char *password);
int ssh_userauth_offer_pubkey(SSH_SESSION *session, char *username,int type, STRING *publickey);
int ssh_userauth_pubkey(SSH_SESSION *session, char *username, STRING *publickey, PRIVATE_KEY *privatekey);
int ssh_userauth_autopubkey(SSH_SESSION *session);

/* gzip.c */
int compress_buffer(SSH_SESSION *session,BUFFER *buf);
int decompress_buffer(SSH_SESSION *session,BUFFER *buf);

/* wrapper.c */
int crypt_set_algorithms(SSH_SESSION *);
CRYPTO *crypto_new();
void crypto_free(CRYPTO *crypto);
bignum bignum_new();

/* in misc.c */
/* gets the user home dir. */
char *ssh_get_user_home_dir();
int ssh_file_readaccess_ok(char *file);

/* macro for byte ordering */
unsigned long long ntohll(unsigned long long);
#define htonll(x) ntohll(x)

#ifdef __cplusplus
} ;
#endif
#endif /* _LIBSSH_H */
