/******************************************************************************
 * Name:		des.cpp
 * Purpose:     encrypt/decrypt password
 * Author:
 * E-mail:
 * Created:
 * Copyright:
 * Licence:
 * Modified by:
 ******************************************************************************/


#ifndef DES_H
#define DES_H

#include "common.h"
#include <string.h>
#include <ctype.h>

// ============================================================================

void SCD_des_encrypt(char *password, char *data);
void SCD_des_decrypt(char *password, char *data);

// ============================================================================

static inline int h2n(char h)
{
	if(h>='0'&&h<='9')	return h-'0';
	h = tolower(h);
	if(h>='a'&&h<='f')	return h-'a'+10;
	return -1;	//error
}

static inline char n2h(int n)
{
	if(n>=0&&n<=9)	return '0'+n;
	if(n>=10&&n<=15)	return 'a'+n-10;
	return '?';	//error
}

static inline void h2c(char *s)
{
	char *r,*w;
	for(r=s, w=s ; r[0]!='\0' && r[1]!='\0' ; w++ , r++, r++)
	{
		*w = h2n(r[0]) << 4 | h2n(r[1]);
	}
	*w = '\0';
}

static inline void c2h(char *s)
{
	int len = strlen(s);
	int r,w;
	s[len+len] = '\0';
 	for(r=len-1 , w=len+len-2 ; r>=0 ; r-- , w--, w--)
	{
		int a, h,l;
		a = s[r];
  		h = (a & 0xf0) >> 4;
		l = a & 0x0f;
		s[w] = n2h(h);
		s[w+1] = n2h(l);
	}
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

wxString SCD_des_decrypt(wxString _password , wxString _encoded_data);
wxString SCD_des_encrypt(wxString _password , wxString _decoded_data);

// ============================================================================
#endif
