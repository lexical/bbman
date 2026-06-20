/******************************************************************************
 * Name:        scd_editterm.cpp
 * Purpose:     ANSI Editor core
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef SCD_EDITTERM_H
#define SCD_EDITTERM_H

#include "common.h"
#include "scd_terminal.h"

// ============================================================================

class SCD_EditTerm : public SCD_Terminal , protected wxEvtHandler
{
private:
    bool blChanged;
    int line_capacity;	//term_data 允許放置多少行

	//名稱前面加上 abs_ 的函式只會修改資料結構, 不會重繪

	bool set_line_capacity(int _dc); //設定最大可容納的行數
	bool ensure_line_capacity(int _lc);	//確保有 _lc 行可以使用
	TerminalChar* AllocNewLine();	//配置一行所需的記憶體空間
	void abs_AddLines(int line_index = -1 , int line_count = 1 );
	void abs_DeleteLines(int line_index = -1 , int line_count = 1 );
public:
	void abs_CleanAll();




public:
    bool isChanged()	{	return blChanged;	}
    void setUnchanged()	{	blChanged = false;	}

public:
	SCD_EditTerm(wxWindow *win);
	~SCD_EditTerm();

	void keyEnter();
	void JoinLine(int _line = -1);	//和下一行合併
	void keyBackspace();
	void keyDelete();
	void keyEnd();
	void keyDown();
	void keyPageUp();
	void keyPageDown();
	void DeleteLineTail();
	void DeleteLine();
	void setSelectionProperty(int type, bool b, int color);
	void setSelectionNormal()
	{	setSelectionProperty(0, true, 0);	}
	void setSelectionBlink(bool b)
	{	setSelectionProperty(1, b, 0);	}
	void setSelectionHighlight(bool b)
	{	setSelectionProperty(2, b, 0);	}
	void setSelectionUnderline(bool b)
	{	setSelectionProperty(3, b, 0);	}
	void setSelectionColor(int color)
	{	setSelectionProperty(4, true, color);	}
	void setSelectionBgColor(int color)
	{	setSelectionProperty(5, true, color);	}


	void PasteFromClipboard(bool withANSI);
	void Paste(const wxString& text, bool withANSI);

	void OnAsciiChar(char ch);

	//**************** 事件 ****************//

	void OnKeyDown(wxKeyEvent& event);
	void OnChar(wxKeyEvent& event);

	void OnMouseLeftDown(wxMouseEvent& event);
	void OnMouseMotion(wxMouseEvent& event);

};


// ============================================================================
#endif
