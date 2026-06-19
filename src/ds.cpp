/******************************************************************************
 * Name:        ds.cpp
 * Purpose:     Site info (ip, port, username, password, etc...)
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef DS_CPP
#define DS_CPP
#include "ds.h"
#ifdef HAVE_LIBSECRET
#include <libsecret/secret.h>
#endif
// ============================================================================

void SiteInfo::Init()
{
	name = wxEmptyString;
	ip = wxEmptyString;
	username = wxEmptyString;
	message = wxEmptyString;
	password = wxEmptyString;
	port = 23;
	autoopen = false;
	protocol = SOCK_TELNET;
	connection_username = wxEmptyString;
}
// ----------------------------------------------------------------------------
static const wxChar *SITEINFO_V2_PREFIX = _T("BBMAN_SITEINFO_V2:");
static const wxChar *SITEINFO_SECRET_PREFIX = _T("keyring:");

#ifdef HAVE_LIBSECRET
static wxString GetSiteInfoSecretService(const SiteInfo *si, const wxString& field)
{
	return _T("BBMan site ") + field + _T(":") + si->name + _T(":") +
		si->ip + _T(":") + wxString::Format(_T("%d"), si->port) + _T(":") +
		si->username + _T(":") + wxString::Format(_T("%d"), si->protocol);
}
#endif

#ifdef HAVE_LIBSECRET
static const SecretSchema BBMAN_SITE_SECRET_SCHEMA = {
	"net.sourceforge.bbman.SiteInfo",
	SECRET_SCHEMA_NONE,
	{
		{ "field", SECRET_SCHEMA_ATTRIBUTE_STRING },
		{ "name", SECRET_SCHEMA_ATTRIBUTE_STRING },
		{ "host", SECRET_SCHEMA_ATTRIBUTE_STRING },
		{ "port", SECRET_SCHEMA_ATTRIBUTE_STRING },
		{ "username", SECRET_SCHEMA_ATTRIBUTE_STRING },
		{ "protocol", SECRET_SCHEMA_ATTRIBUTE_STRING },
		{ NULL, (SecretSchemaAttributeType)0 }
	},
	0, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static bool StoreSiteInfoSecretWithLibsecret(const SiteInfo *si, const wxString& field, const wxString& value)
{
	wxString service = GetSiteInfoSecretService(si, field);
	wxString port = wxString::Format(_T("%d"), si->port);
	wxString protocol = wxString::Format(_T("%d"), si->protocol);

	wxCharBuffer serviceUtf8 = service.utf8_str();
	wxCharBuffer fieldUtf8 = field.utf8_str();
	wxCharBuffer valueUtf8 = value.utf8_str();
	wxCharBuffer nameUtf8 = si->name.utf8_str();
	wxCharBuffer hostUtf8 = si->ip.utf8_str();
	wxCharBuffer portUtf8 = port.utf8_str();
	wxCharBuffer usernameUtf8 = si->username.utf8_str();
	wxCharBuffer protocolUtf8 = protocol.utf8_str();

	GError *error = NULL;
	gboolean ok = secret_password_store_sync(&BBMAN_SITE_SECRET_SCHEMA,
		SECRET_COLLECTION_DEFAULT, serviceUtf8.data(), valueUtf8.data(), NULL, &error,
		"field", fieldUtf8.data(),
		"name", nameUtf8.data(),
		"host", hostUtf8.data(),
		"port", portUtf8.data(),
		"username", usernameUtf8.data(),
		"protocol", protocolUtf8.data(),
		NULL);
	if( error != NULL )
		g_error_free(error);

	return ok == TRUE;
}

static wxString LoadSiteInfoSecretWithLibsecret(const SiteInfo *si, const wxString& field)
{
	static bool warned = false;
	wxString port = wxString::Format(_T("%d"), si->port);
	wxString protocol = wxString::Format(_T("%d"), si->protocol);

	wxCharBuffer fieldUtf8 = field.utf8_str();
	wxCharBuffer nameUtf8 = si->name.utf8_str();
	wxCharBuffer hostUtf8 = si->ip.utf8_str();
	wxCharBuffer portUtf8 = port.utf8_str();
	wxCharBuffer usernameUtf8 = si->username.utf8_str();
	wxCharBuffer protocolUtf8 = protocol.utf8_str();

	GError *error = NULL;
	gchar *secret = secret_password_lookup_sync(&BBMAN_SITE_SECRET_SCHEMA, NULL, &error,
		"field", fieldUtf8.data(),
		"name", nameUtf8.data(),
		"host", hostUtf8.data(),
		"port", portUtf8.data(),
		"username", usernameUtf8.data(),
		"protocol", protocolUtf8.data(),
		NULL);
	if( error != NULL )
	{
		if( ! warned )
		{
			wxString msg = wxString::FromUTF8(error->message);
			wxLogWarning(_T("Unable to read BBMan keyring secrets: %s"), msg);
			warned = true;
		}
		g_error_free(error);
	}
	if( secret == NULL )
	{
		if( ! warned )
		{
			wxLogWarning(_T("Unable to find BBMan keyring secret for %s"), field);
			warned = true;
		}
		return wxEmptyString;
	}

	wxString value = wxString::FromUTF8(secret);
	secret_password_free(secret);
	return value;
}
#endif

static wxString StoreSiteInfoSecret(const SiteInfo *si, const wxString& field, const wxString& value)
{
	if( value.IsEmpty() )
		return wxEmptyString;

#ifdef HAVE_LIBSECRET
	wxString service = GetSiteInfoSecretService(si, field);
	if( StoreSiteInfoSecretWithLibsecret(si, field, value) )
		return wxString(SITEINFO_SECRET_PREFIX) + service;
#else
	wxUnusedVar(si);
	wxUnusedVar(field);
#endif

	return SCD_des_encrypt( GetLoginPassword() , value );
}

static wxString LoadSiteInfoSecret(const SiteInfo *si, const wxString& field, const wxString& value)
{
	if( value.IsEmpty() )
		return wxEmptyString;

	wxString prefix(SITEINFO_SECRET_PREFIX);
	if( value.StartsWith(prefix) )
	{
#ifdef HAVE_LIBSECRET
		wxString secret = LoadSiteInfoSecretWithLibsecret(si, field);
		if( ! secret.IsEmpty() )
			return secret;
#else
		wxUnusedVar(si);
		wxUnusedVar(field);
#endif

		return wxEmptyString;
	}

	return SCD_des_decrypt( GetLoginPassword() , value );
}

static wxString PackSiteInfoField(const wxString& field)
{
	return wxString::Format(_T("%lu:"), (unsigned long)field.Length()) + field;
}

static bool UnpackSiteInfoField(const wxString& data, size_t& offset, wxString& field)
{
	size_t colon = data.find(_T(':'), offset);
	if( colon == wxString::npos )
		return false;

	long len;
	if( ! data.Mid(offset, colon - offset).ToLong(&len) || len < 0 )
		return false;

	offset = colon + 1;
	if( offset + (size_t)len > data.Length() )
		return false;

	field = data.Mid(offset, len);
	offset += len;
	return true;
}

static bool ParseSiteInfoV2(const wxString& data, SiteInfo *si)
{
	wxString prefix(SITEINFO_V2_PREFIX);
	if( ! data.StartsWith(prefix) )
		return false;

	size_t offset = prefix.Length();
	wxString port, autoopen, protocol;
	if( ! UnpackSiteInfoField(data, offset, si->name) ||
		! UnpackSiteInfoField(data, offset, si->ip) ||
		! UnpackSiteInfoField(data, offset, port) ||
		! UnpackSiteInfoField(data, offset, autoopen) ||
		! UnpackSiteInfoField(data, offset, si->username) ||
		! UnpackSiteInfoField(data, offset, si->password) ||
		! UnpackSiteInfoField(data, offset, si->message) ||
		! UnpackSiteInfoField(data, offset, protocol) )
		return false;

	long value;
	if( port.ToLong(&value) )
		si->port = value;
	if( protocol.ToLong(&value) )
		si->protocol = value;
	si->autoopen = (autoopen == _T("y"));

	if( si->protocol != SOCK_SSH )
		si->protocol = SOCK_TELNET;
	if( si->port == 0 )
		si->port = ( si->protocol == SOCK_SSH ) ? 22 : 23;

	si->password = LoadSiteInfoSecret(si, _T("password"), si->password);
	si->message = LoadSiteInfoSecret(si, _T("message"), si->message);

	return true;
}
// ----------------------------------------------------------------------------
void SiteInfo::Set(wxString str)
{
	Init();

	if( ParseSiteInfoV2(str, this) )
		return;

	wxString tmp;

	name = str.BeforeFirst(':');	str = str.AfterFirst(':');
	ip = str.BeforeFirst(':');	str = str.AfterFirst(':');
	tmp = str.BeforeFirst(':');	str = str.AfterFirst(':');
	long value;
	if( tmp.ToLong(&value) )	port = value;
	else	port = 0;
	tmp = str.BeforeFirst(':');	str = str.AfterFirst(':');
	if(tmp == _T("y") )	autoopen = true;
	username = str.BeforeFirst(':');	str = str.AfterFirst(':');
	password = str.BeforeFirst(':');	str = str.AfterFirst(':');
	message = str.BeforeFirst(':');		str = str.AfterFirst(':');
	tmp = str.BeforeFirst(':');	str = str.AfterFirst(':');
	if( tmp.ToLong(&value) )	protocol = value;

	if( protocol != SOCK_SSH )	protocol = SOCK_TELNET;
	if( port == 0 )	port = ( protocol == SOCK_SSH ) ? 22 : 23;

	//¥N±K½X¥Î DES ¸Ñ±K
	password = LoadSiteInfoSecret(this, _T("password"), password);
	message = LoadSiteInfoSecret(this, _T("message"), message);
}
// ----------------------------------------------------------------------------
wxString SiteInfo::Get()
{
	wxString tmp_pass = StoreSiteInfoSecret(this, _T("password"), password);
	wxString tmp_message = StoreSiteInfoSecret(this, _T("message"), message);

	return wxString(SITEINFO_V2_PREFIX)
		+ PackSiteInfoField(name)
		+ PackSiteInfoField(ip)
		+ PackSiteInfoField(wxString::Format(_T("%d"), port))
		+ PackSiteInfoField(autoopen ? _T("y") : _T("n"))
		+ PackSiteInfoField(username)
		+ PackSiteInfoField(tmp_pass)
		+ PackSiteInfoField(tmp_message)
		+ PackSiteInfoField(wxString::Format(_T("%d"), protocol));
}


// ============================================================================
#endif
