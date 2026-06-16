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

#ifndef SCD_GTK_TEXTCTRL_H
#define SCD_GTK_TEXTCTRL_H
#include "common.h"
#include <wx/textctrl.h>

// ============================================================================

class CopyPasteDialog : public wxDialog
{
private:
	wxStaticText label;
	wxButton btnOK, btnCancel;

public:
	CopyPasteDialog();
	
	wxTextCtrl textctrl;

	void SetCaption(wxString cap)
	{	label.SetLabel(cap);	}

	void OnOK(wxCommandEvent& event)	{;}
	void OnCancel(wxCommandEvent& event)	{;}
	void OnApply(wxCommandEvent& event)	{;}
};

// ----------------------------------------------------------------------------

void init_CopyPasteDialog();
void destroy_CopyPasteDialog();
void ShowCopyDialog(wxString text);
wxString ShowPasteDialog();
wxString GetMultilineTextFromUser(const wxString& message, const wxString& caption , wxWindow *win);

// ============================================================================
#endif
#endif
