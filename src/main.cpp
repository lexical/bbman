/******************************************************************************
 * Name:        main.cpp
 * Purpose:     program entry point
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef MAIN_CPP
#define MAIN_CPP
#include "main.h"

#include "common.h"
#include "login.h"
#include "scd_terminal.h"
#include <wx/image.h>

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------



class MyApp : public wxApp
{
private:
public:
	virtual bool OnInit();
	virtual int OnExit();
};


// Create a new application object: this macro will allow wxWindows to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also declares the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)
IMPLEMENT_APP(MyApp)



#ifndef __UNIX__
wxLocale *locale;
#endif
void init()
{

	wxString localePath = GetLocalePath();
#ifdef __UNIX__
	setlocale( LC_ALL , "" );
	bindtextdomain( "bbman", wxStringToCharPtr(localePath) );
	textdomain( "bbman" );
#else
	locale = new wxLocale( wxLANGUAGE_DEFAULT );
	locale->AddCatalogLookupPathPrefix( localePath );
	locale->AddCatalog( "bbman" );
//	wxMessageBox( locale->GetSysName() );
#endif

	init_Icons();
	init_sound();
	init_font();
	init_Naws();
	init_PreventIdle();
	Init_LineWrapedLength();

	//高亮度顯示所有字
	bool b;
	if( GetConfig()->Read( GetUserConfigPath(_T("/setting/always_highlight")) , &b ) )
		if(b)	TerminalChar::setAlwaysHighlight(true);

}

bool MyApp::OnInit()
{
	SetAppName(_T("bbman"));
	SetClassName(_T("bbman"));
	wxInitAllImageHandlers();
	SetApp(this);
	init();
	ShowTelnet();

	if( getLoginWhenStart() )	ShowLoginDialog();

	// success: wxApp::OnRun() will be called which will enter the main message
	// loop and the application will run. If we returned FALSE here, the
	// application would exit immediately.
	return TRUE;
}

int MyApp::OnExit()
{
	return 0;
}

// ----------------------------------------------------------------------------
#endif
