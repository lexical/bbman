/******************************************************************************
 * Name:        scd_gtk_textctrl.cpp
 * Purpose:     fix the copy/paste problem of wxGTK (Windows has no such problem)
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifdef __WXGTK__

#ifndef SCD_GTK_TEXTCTRL_CPP
#define SCD_GTK_TEXTCTRL_CPP
#include "scd_gtk_textctrl.h"
#include "common.h"


// ----------------------------------------------------------------------------

CopyPasteDialog::CopyPasteDialog()
	: wxDialog( NULL, -1, wxEmptyString, wxDefaultPosition , wxSize(300, 100) )
{
	wxBoxSizer *sizer, *sub_sizer;
	sizer = new wxBoxSizer( wxVERTICAL );

//	label.Create(this, -1, wxEmptyString );
//	sizer->Add( & label , 0 , wxEXPAND | wxALL , 6  );

	btnOK.Create(this, wxID_OK , gettext("Paste") );
	btnCancel.Create(this, wxID_CANCEL, gettext("Cancel") );
	sub_sizer = new wxBoxSizer( wxHORIZONTAL );
	sub_sizer->Add( & btnOK , 0 , wxEXPAND | wxALL , 6  );
	sub_sizer->Add( & btnCancel , 0 , wxEXPAND | wxALL , 6  );
	sizer->Add( sub_sizer );

	textctrl.Create( this , -1 , "", wxDefaultPosition , wxDefaultSize, wxTE_MULTILINE );
	sizer->Add( & textctrl , 1 , wxEXPAND | wxALL , 0  );

	SetSizer(sizer);
	sizer->SetSizeHints( this );
	SetAutoLayout( TRUE );
}


static CopyPasteDialog *cpdlg = NULL;
void init_CopyPasteDialog()
{
	if( cpdlg != NULL )	return;
	cpdlg = new CopyPasteDialog();
	cpdlg->SetSize( 300, 300 );
	cpdlg->Show();
#ifndef __SAWFISH__
	cpdlg->Show(false);
#else
	cpdlg->EndModal(0);	//使用 sawfish wm 時必須使用這行，不然視窗無法被使用者關閉，但是即使這樣也沒辦法自動關閉，需由使用者手動關閉
#endif
}
void destroy_CopyPasteDialog()
{
	if(cpdlg)	cpdlg->Destroy();
	cpdlg = NULL;
}

void ShowCopyDialog(wxString text)
{
	init_CopyPasteDialog();
	cpdlg->textctrl.SetValue(text);
	cpdlg->textctrl.SetSelection(-1,-1);
	cpdlg->textctrl.Copy();	//欲保留被系統所複製的資料，整個視窗就不能被 free ，只能被隱藏，不然將會失效
//	cpdlg->ShowModal();
//	cpdlg->EndModal(0);
}

wxString ShowPasteDialog()
{
//	wxTheClipboard->Clear();
//	wxTheClipboard->Close();
	init_CopyPasteDialog();
	cpdlg->SetTitle( gettext("Paste") );
	cpdlg->SetCaption( gettext("Please use mouse to select next line, and close window.") );
	cpdlg->textctrl.SetValue( wxEmptyString );
	cpdlg->textctrl.SetSelection(-1,-1);
	cpdlg->textctrl.Paste();
//	cpdlg->textctrl.SetSelection(-1,-1);
	if( wxID_OK == cpdlg->ShowModal() )
		return cpdlg->textctrl.GetValue();
	else
		return wxEmptyString;
//	cpdlg->EndModal(0);
//	return cpdlg->textctrl.GetValue();
}


wxString GetMultilineTextFromUser(const wxString& message, const wxString& caption , wxWindow *win)
{
	init_CopyPasteDialog();
	cpdlg->SetTitle(caption);
//	cpdlg->label.SetLabel(message);
	cpdlg->textctrl.Clear();
	cpdlg->textctrl.Paste();
	if( wxID_OK == cpdlg->ShowModal() )
		return cpdlg->textctrl.GetValue();
	else
		return wxEmptyString;
}

// ----------------------------------------------------------------------------
#endif
#endif
