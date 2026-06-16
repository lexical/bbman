/******************************************************************************
 * Name:        frm_editterm.cpp
 * Purpose:     BBS ANSI editor (only useful if you are interested in BBS)
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef FRM_EDITTERM_CPP
#define FRM_EDITTERM_CPP
#include "frm_editterm.h"

#include <wx/caret.h>
#include <wx/file.h>
#include <wx/filedlg.h>

#include "editterm_win.h"

#include "scd_gtk_textctrl.h"	//GetMultilineTextFromUser()
// ============================================================================

enum
{
	MENU_FILE_BEGIN,

	MENU_OPENNEWWINDOW,
	MENU_OPENNEWFILE,
	MENU_OPENOLDFILE,
	MENU_SAVEFILE,
	MENU_SAVENEWFILE,
	MENU_QUIT,

	MENU_FILE_END,



    MENU_CHARPROP_BEGIN,

    MENU_CHARPROP_NORMAL,
    MENU_CHARPROP_BLINK,
    MENU_CHARPROP_UNBLINK,
    MENU_CHARPROP_HIGHLIGHT,
    MENU_CHARPROP_UNHIGHLIGHT,
    MENU_CHARPROP_UNDERLINE,
    MENU_CHARPROP_UNUNDERLINE,

	MENU_CHARPROP_CHCOLOR_1,	MENU_CHARPROP_CHCOLOR_2,	MENU_CHARPROP_CHCOLOR_3,
	MENU_CHARPROP_CHCOLOR_4,	MENU_CHARPROP_CHCOLOR_5,	MENU_CHARPROP_CHCOLOR_6,
 	MENU_CHARPROP_CHCOLOR_7,	MENU_CHARPROP_CHCOLOR_8,

	MENU_CHARPROP_BGCOLOR_1,	MENU_CHARPROP_BGCOLOR_2,	MENU_CHARPROP_BGCOLOR_3,
	MENU_CHARPROP_BGCOLOR_4,	MENU_CHARPROP_BGCOLOR_5,	MENU_CHARPROP_BGCOLOR_6,
 	MENU_CHARPROP_BGCOLOR_7,	MENU_CHARPROP_BGCOLOR_8,
 	
    MENU_CHARPROP_END,



	MENU_EDIT_BEGIN,

	MNU_DELETELINE,
	MNU_DELETELINETAIL,
	MNU_JOINLINE,
	MNU_COPY,
	MNU_COPY_ANSI,
	MNU_PASTE,
	MNU_PASTE_ANSI,
	MNU_SELECTALL,
	MNU_INPUT_LINE,

	MENU_EDIT_END,

	MENU_About
};

BEGIN_EVENT_TABLE(frame_EditTerm, wxFrame)
	EVT_MENU_RANGE(MENU_FILE_BEGIN, MENU_FILE_END, frame_EditTerm::OnFile)
	EVT_MENU_RANGE(MENU_EDIT_BEGIN, MENU_EDIT_END, frame_EditTerm::OnEdit)
	EVT_MENU_RANGE(MENU_CHARPROP_BEGIN, MENU_CHARPROP_END, frame_EditTerm::OnCharProperty)
	EVT_ACTIVATE(frame_EditTerm::OnActivate)
	EVT_MENU(MENU_About, frame_EditTerm::OnAbout)

	EVT_CLOSE(frame_EditTerm::OnClose)
END_EVENT_TABLE()


void frame_EditTerm::UpdateTitle()
{
	wxString t;
	t = gettext("BBMan - ANSI Editor");
	t += _T(" [ ");
 	if( filename == wxEmptyString )	t += gettext("New File");
  	else	t += filename;
  	if( editterm->isChanged() )	t += _T(" *");
   	t += _T(" ]");
	SetTitle( t );
}    

bool frame_EditTerm::SaveNewFile()
{
	wxString path = wxFileSelector( gettext("Save As...") , wxEmptyString, _T("NewFile.ans"), _T("ans") , _T("BBS ANSI Text (*.ans)|*.ans|All (*.*)|*.*") , wxFD_SAVE | wxFD_OVERWRITE_PROMPT , this );
	if( path.empty() )	return false;
	return SaveFile( path , editterm->GetAllContent(true) );
}    
bool frame_EditTerm::SaveFile()
{
    if( filename == wxEmptyString )	return SaveNewFile();
    else return SaveFile( filename , editterm->GetAllContent(true) );
}    
bool frame_EditTerm::SaveFile(wxString _filename, wxString content)
{
	FILE *fp = wxFopen( _filename , wxT( "wb" ) );
	fwrite( content.c_str() , 1, content.Len() , fp );
	fclose(fp);

    filename = _filename;
    editterm->setUnchanged();
    UpdateTitle();
    return true;
}    

void frame_EditTerm::OnFile(wxCommandEvent& event)
{
	int id = event.GetId();
	switch(id)
	{
		case MENU_OPENNEWWINDOW :
			ShowAnsiEditor();
  			break;
	    case MENU_OPENNEWFILE :
			if( editterm->isChanged() )
			{
				if( wxNO == wxMessageBox( gettext("Content modified but not save yet, are you sure to quit?") , wxEmptyString, wxYES_NO ) )
					break;
			}

       		editterm->abs_CleanAll();
       		editterm->gotoXY(0,0);
       		editterm->ScrollToCaret();
			editterm->repaint();

       		filename = wxEmptyString;
       		editterm->setUnchanged();
       		UpdateTitle();
	        break;
	    case MENU_OPENOLDFILE :
	        {
				if( editterm->isChanged() )
				{
					if( wxNO == wxMessageBox( gettext("Content modified but not save yet, are you sure to quit?") , wxEmptyString, wxYES_NO ) )
						break;
				}
    
				wxString path = wxFileSelector( gettext("Open") , wxEmptyString, wxEmptyString, _T("ans") , gettext("BBS color text file (*.ans)|*.ans|All (*.*)|*.*") , wxFD_OPEN | wxFD_FILE_MUST_EXIST , this );
				if( path.empty() )	return;
				
				FILE *fp = wxFopen( path , wxT( "rb" ) );
				if(fp == NULL)	return;
				wxString content;
				wxChar buf[2000];
				while( !feof(fp) )
				{
				    wxFgets( buf , 1999 , fp );
				    content += buf;
				}    
				fclose(fp);

        		editterm->abs_CleanAll();
//        		editterm->parse( (char*) content.c_str() );
				editterm->Paste( wxStringToCharPtr(content) , true );
        		editterm->gotoXY(0,0);
        		editterm->ScrollToCaret();
//				editterm->repaint();

        		filename = path;
        		editterm->setUnchanged();
        		UpdateTitle();
	        }    
	        break;
	    case MENU_SAVEFILE :
			SaveFile();
	        break;
	    case MENU_SAVENEWFILE :
	        SaveNewFile();
	        break;
		case MENU_QUIT :
  			Close();
  			break;
	}	
}    

void frame_EditTerm::OnEdit(wxCommandEvent& event)
{
	int id = event.GetId();
	switch(id)
	{
		case MNU_DELETELINE :
		    editterm->DeleteLine();
			break;
		case MNU_DELETELINETAIL :
			editterm->DeleteLineTail();
		    break;
		case MNU_JOINLINE :
      		editterm->JoinLine();
      		break;
	    case MNU_COPY :
			editterm->CopySelectionToClipboard(false);
			editterm->CancelSelection();
     		break;
	    case MNU_COPY_ANSI :
			editterm->CopySelectionToClipboard(true);
			editterm->CancelSelection();
     		break;
	    case MNU_PASTE : editterm->PasteFromClipboard(false);	break;
	    case MNU_PASTE_ANSI : editterm->PasteFromClipboard(true);	break;
	    case MNU_SELECTALL : editterm->SelectAll(); break;
	    case MNU_INPUT_LINE :
#ifdef __WXGTK__
			wxString line = GetMultilineTextFromUser( gettext("Please enter message you want to send"), gettext("Input a line"), this );
#else
			wxString line = wxGetTextFromUser( gettext("Please enter message you want to send"), gettext("Input a line"), wxEmptyString );
#endif
			if( ! line.IsEmpty() )
				editterm->Paste( wxStringToCharPtr(line) , false );         	
			break;
	}    
}
    
void frame_EditTerm::OnCharProperty(wxCommandEvent& event)
{
	int id = event.GetId();
	if( id >= MENU_CHARPROP_CHCOLOR_1 && id <= MENU_CHARPROP_CHCOLOR_8 )
		editterm->setSelectionColor( id - MENU_CHARPROP_CHCOLOR_1 + 30 );
	else if( id >= MENU_CHARPROP_BGCOLOR_1 && id <= MENU_CHARPROP_BGCOLOR_8 )
		editterm->setSelectionBgColor( id - MENU_CHARPROP_BGCOLOR_1 + 40 );
	else switch(id)
	{
		case MENU_CHARPROP_NORMAL :		editterm->setSelectionNormal();	break;
	    case MENU_CHARPROP_BLINK :		editterm->setSelectionBlink(true); break;
	    case MENU_CHARPROP_UNBLINK :	editterm->setSelectionBlink(false); break;
	    case MENU_CHARPROP_HIGHLIGHT :	editterm->setSelectionHighlight(true); break;
	    case MENU_CHARPROP_UNHIGHLIGHT :editterm->setSelectionHighlight(false); break;
	    case MENU_CHARPROP_UNDERLINE :	editterm->setSelectionUnderline(true); break;
	    case MENU_CHARPROP_UNUNDERLINE :editterm->setSelectionUnderline(false); break;
	}
}    

void frame_EditTerm::ShowCharPropContextMenu(const wxPoint& pos)
{
	wxMenu contextMenu;
	contextMenu.Append(MENU_CHARPROP_NORMAL , gettext("Normal (Cancel All Attributes)") );
	contextMenu.AppendSeparator();

	wxString name[] = { gettext("black"), gettext("red"), gettext("green"), gettext("yellow"), gettext("blue"), gettext("purple"), gettext("anil"), gettext("white") };

	wxMenu *menuTextColor = new wxMenu;
	for(int i=0;i<8;i++)
		menuTextColor->Append(MENU_CHARPROP_CHCOLOR_1 + i , name[i]);
	contextMenu.Append(0, gettext("Text Color"), menuTextColor);

	wxMenu *menuBackColor = new wxMenu;
	for(int i=0;i<8;i++)
		menuBackColor->Append(MENU_CHARPROP_BGCOLOR_1 + i , name[i]);
	contextMenu.Append(0, gettext("Background Color"), menuBackColor);

	contextMenu.AppendSeparator();
	contextMenu.Append(MENU_CHARPROP_BLINK , gettext("Blink") );
	contextMenu.Append(MENU_CHARPROP_UNBLINK , gettext("no blink") );
	contextMenu.AppendSeparator();
	contextMenu.Append(MENU_CHARPROP_HIGHLIGHT , gettext("Highlight") );
	contextMenu.Append(MENU_CHARPROP_UNHIGHLIGHT , gettext("no highlight") );
	contextMenu.AppendSeparator();
	contextMenu.Append(MENU_CHARPROP_UNDERLINE , gettext("Underline") );
	contextMenu.Append(MENU_CHARPROP_UNUNDERLINE , gettext("no underline") );

	PopupMenu(&contextMenu, pos);
}

frame_EditTerm::frame_EditTerm(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
	   : wxFrame(NULL, -1, title, pos, size, style)
{

	filename = wxEmptyString;

	wxMenuBar *menuBar = new wxMenuBar();

	wxMenu *menuFile = new wxMenu;
	menuFile->Append(MENU_OPENNEWWINDOW , gettext("New &Window") );
	menuFile->AppendSeparator();
	menuFile->Append(MENU_OPENNEWFILE , gettext("New File") );
	menuFile->Append(MENU_OPENOLDFILE , gettext("Open") );
	menuFile->Append(MENU_SAVEFILE , gettext("&Save\tCtrl-S") );
	menuFile->Append(MENU_SAVENEWFILE , gettext("Save As...") );
	menuFile->AppendSeparator();
	menuFile->Append(MENU_QUIT , gettext("&Quit\tAlt-F4") );
	menuBar->Append(menuFile, gettext("&File"));

	wxMenu *editMenu = new wxMenu;
	editMenu->Append(MNU_DELETELINE , gettext("Delete line\tCtrl-Y") );
	editMenu->Append(MNU_DELETELINETAIL , gettext("Delete Line Tail\tCtrl-K") );
	editMenu->Append(MNU_JOINLINE , gettext("Join next line\tCtrl-J") );
	editMenu->AppendSeparator();
	editMenu->Append(MNU_COPY , gettext("Copy (Plain Text) \tAlt-C") );
	editMenu->Append(MNU_PASTE, gettext("Paste (Plain Text) \tAlt-V") );
	editMenu->AppendSeparator();
	editMenu->Append(MNU_COPY_ANSI , gettext("&Copy (with ANSI Color) \tCtrl-Alt-C") );
	editMenu->Append(MNU_PASTE_ANSI, gettext("&Paste (with ANSI Color) \tCtrl-Alt-V") );
	editMenu->AppendSeparator();
	editMenu->Append(MNU_SELECTALL , gettext("Select &All\tAlt-A") );
	editMenu->AppendSeparator();
	editMenu->Append(MNU_INPUT_LINE, gettext("Input &Line\tAlt-L") );
	menuBar->Append(editMenu, gettext("&Edit"));

	menuCharProp = new wxMenu;
	menuCharProp->Append(MENU_CHARPROP_NORMAL , gettext("Normal (Cancel All Attributes)\tCtrl-N") );
	menuCharProp->AppendSeparator();

	wxMenuItem *mi;
	wxString name[] = { gettext("black"), gettext("red"), gettext("green"), gettext("yellow"), gettext("blue"), gettext("purple"), gettext("anil"), gettext("white") };

	wxMenu *menuTextColor = new wxMenu;
	for(int i=0;i<8;i++)
	{	mi = new wxMenuItem(NULL, MENU_CHARPROP_CHCOLOR_1 + i , wxString::Format(_T("%s\tCtrl-%d"), name[i].c_str(), (i+1)) );	/*mi->SetTextColour(TerminalColors[0][i]);*/	menuTextColor->Append(mi);	}
	menuCharProp->Append(0, gettext("Text Color"), menuTextColor);

	wxMenu *menuBackColor = new wxMenu;
	for(int i=0;i<8;i++)
	{	mi = new wxMenuItem(NULL, MENU_CHARPROP_BGCOLOR_1 + i , wxString::Format(_T("%s\tCtrl-Shift-%d"), name[i].c_str(), (i+1)) );	/*mi->SetBackgroundColour(TerminalColors[0][i]);*/	menuBackColor->Append(mi);	}
	menuCharProp->Append(0, gettext("Background Color"), menuBackColor);

	menuCharProp->AppendSeparator();

	menuCharProp->Append(MENU_CHARPROP_BLINK , gettext("Blink \tCtrl-B") );
	menuCharProp->Append(MENU_CHARPROP_UNBLINK , gettext("no blink \tCtrl-V") );
	menuCharProp->AppendSeparator();
	menuCharProp->Append(MENU_CHARPROP_HIGHLIGHT , gettext("Highlight \tCtrl-H") );
	menuCharProp->Append(MENU_CHARPROP_UNHIGHLIGHT , gettext("no highlight \tCtrl-G") );
	menuCharProp->AppendSeparator();
	menuCharProp->Append(MENU_CHARPROP_UNDERLINE , gettext("Underline \tCtrl-U") );
	menuCharProp->Append(MENU_CHARPROP_UNUNDERLINE , gettext("no underline \tCtrl-I") );

	menuBar->Append(menuCharProp, gettext("Text Attribute"));


	wxMenu *helpMenu = new wxMenu;
	helpMenu->Append(MENU_About, gettext("About BBMan color text editor\tF1") );
	menuBar->Append(helpMenu, gettext("&About"));


	SetMenuBar(menuBar);

#if ! defined(__WXGTK__)
	CreateStatusBar();
	SetStatusText( GetStatusBarText() );
	SetStatusBarPane(-1);
#endif




	//設定 terminal 字型
	SetFont( GetCurrentFont() );


	editterm = NULL;
	editterm_win = new EditTerm_win(this);
	editterm = editterm_win->editterm;
	editterm->SetFont(GetCurrentFont());
	editterm->setEnableDoubleByteDetection(false);
	editterm->abs_CleanAll();
	editterm->gotoXY(0,0);

	wxSize cs = editterm->getCharSize();
	SetClientSize( cs.GetWidth() * 82 , cs.GetHeight() * 24 );

	UpdateTitle();
}

void frame_EditTerm::SetTerminalFont(wxFont fnt)
{
	editterm->SetFont(fnt);
	editterm_win->SetFont(fnt);
	editterm_win->Refresh(false);
	wxSize cs = editterm->getCharSize();
	SetClientSize( cs.GetWidth() * 82 , cs.GetHeight() * 24 );
}

void frame_EditTerm::OnClose(wxCloseEvent& event)
{
	if( event.CanVeto() )	//如果不是系統強制關閉視窗
		if( editterm->isChanged() )
			if( wxNO == wxMessageBox( gettext("Content modified but not save yet, are you sure to quit?") , wxEmptyString, wxYES_NO ) )
			{
				event.Veto();	//阻止系統關閉視窗 
				return;
			}

	CloseAnsiEditor(this);
	this->Destroy();	//關閉視窗			
}    

void frame_EditTerm::OnAbout(wxCommandEvent& WXUNUSED(event))
{	ShowAbout();	}

// ============================================================================
#endif
