/******************************************************************************
 * Name:        scd_terminal.cpp
 * Purpose:     terminal display, data storing
 * Author:      Xie, Chun-Da (jakky1)
 * E-mail:      jakky1@gmail.com
 * Created:     2004.7
 * Copyright:   (C) 2004 Xie, Chun-Da
 * Licence:     GPL : http://www.gnu.org/licenses/gpl.html
 * Modified by:
 ******************************************************************************/


#ifndef SCD_TERMINAL_H
#define SCD_TERMINAL_H

#include "common.h"
#include <wx/caret.h>
#include <wx/clipbrd.h>
#include <ctype.h>

// ============================================================================



typedef struct TerminalChar
{
public:
	//半形字 , 全形字第一個byte , 全形字第二個byte
	typedef enum { CH_CHAR , CH_WORDFIRST , CH_WORDLAST } CHAR_TYPE;

private:

	struct		//顏色屬性
	{
		unsigned char TextColor : 4;	//字體顏色
		unsigned char BackColor : 4;	//背景顏色
	} Color;

	struct 		//特殊屬性
	{
		unsigned char isHighlight : 1;	//高亮度
		unsigned char isUnderline : 1;	//底線
		unsigned char isBlink : 1;		//閃爍

		unsigned char chartype : 2;
		unsigned char linktype : 3;
	} property;

public:

	char ch;

	static bool getAlwaysHighlight();
	static void setAlwaysHighlight(bool b);

	//指定該字元是 [半形字] [全形字第一個 byte] [全形字第二個 byte]
	inline void setCharType( CHAR_TYPE t )
	{	property.chartype = t;	}
	inline CHAR_TYPE getCharType()
	{	return (CHAR_TYPE)property.chartype;	}

	//該字元是否為一有效聯結的一部份
	inline void setLinkType( LINK_TYPE _l )
	{	property.linktype = _l;	}
	inline LINK_TYPE getLinkType()
	{	return (LINK_TYPE)property.linktype;	}

	//字元特殊屬性
	inline void setHighlight(bool to_highlight)
	{	property.isHighlight = to_highlight ? 1 : 0;	}
	bool getHighlight()
	{	return (getAlwaysHighlight() || property.isHighlight) ? true : false;	}
	inline void setBlink(bool to_blink)
	{	property.isBlink = to_blink ? 1 : 0;	}
	inline bool getBlink()
	{	return property.isBlink ? true : false;	}
	inline void setUnderline(bool to_Underline)
	{	property.isUnderline = to_Underline ? 1 : 0;	}
	inline bool getUnderline()
	{
/*
#if defined(__WXGTK__)	//GTK 目前不支援底線字, 所以先取消底線功能
		return false;
#else
*/
		return property.isUnderline ? true : false;
//#endif
	}

	//字元顏色屬性
	inline void setTextColor(int ColorCode)
	{
		if( ColorCode >= 30 && ColorCode <= 37 )
			Color.TextColor = ColorCode - 30;
	}
	wxColour getTextColor(bool _selected = false);
	inline int getTextColorCode()
	{	return Color.TextColor + 30;	}

   	//字元背景屬性
	inline void setBgColor(int ColorCode)
	{
		if( ColorCode >= 40 && ColorCode <= 47 )
			Color.BackColor = ColorCode - 40;
	}
	wxColour getBgColor(bool _selected = false);
	inline int getBgColorCode()
	{	return Color.BackColor + 40;	}

	wxString getANSICode();
	wxString getDiffANSICode(TerminalChar & _c);

	bool withSameProperty(TerminalChar & ch);

	void setColorCode(int ColorCode);
	static TerminalChar getDefaultCharProperty();

};


//用來描述每個 terminal char 的屬性

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// *************** SCD_Terminal 類別 ***************
//
//1. 用來描述 terminal 的資料 (字串內容, 每個字的屬性, etc...)
//2. 負責繪圖
//     用 repaint() 全部重繪
//     用 repaint(int x, int y, int w, int h) 部分重繪
//3. parse() 接收資料, 例如從 wxSocketBase::Read() 取得的資料
//
// ********************************************************

class SCD_Terminal
{


//---------------------------------------------------------------
//如果 SCD_Terminal 將會在 wxScrolledWindow 上面繪圖的話,
//一定要設定 setScrolledWindow() 這個函式
//這個函式會讓 SCD_Terminal 在繪圖(repaint())以及移動游標(updateCaret())的時候,
//依照 wxScrolledWindow 的捲軸位置做出相對應的改變
//這樣 SCD_Terminal 才能在 wxScrolledWindow 中正常顯示
private:
    wxScrolledWindow *scrollwin;
public:
    inline void setScrolledWindow(wxScrolledWindow *win)
    {	scrollwin = win;	}
	int getLineCountInView();	//取得目前顯示區域一共顯示幾行資訊
	void ScrollToCaret();	//把捲軸捲動到游標所在之處
//---------------------------------------------------------------


private:
	wxPoint server_stored_cur_pos;

public:
	SCD_Terminal(wxWindow *win);
	~SCD_Terminal();

	void ResetTerminal();	//用來重置 terminal 設定, 每次重新連線時必須呼叫

	void setColumnRow(int _c, int _r);	//設定行數, 列數


	//游標設定
private:
	//reach_line_end 記錄游標是否已經到了行尾
	//如果 col_count=80 , 則 reach_line_end 紀錄游標是否移到 cur_x = 80
	//因為實際上 cur_x 不能等於 80 因為會 array out-of-bound
	//因此用這變數記錄之
	//紀錄的用處是在 parse() 時如果欲在 cur_x=80 處書出字串的話
	//函式會自動跳到下一行 (for ssh)
	bool reach_line_end;

public:
	void getXY(int *_x, int *_y)	//取得游標位置
	{
		if(_x)	*_x = cur_x;
	 	if(_y)	*_y = cur_y;
	}
	void gotoXY(int _x, int _y)	//將游標移至 (_x,_y)
	{
		if( is_X_in_bound(_x) )	cur_x = _x;
		if( is_Y_in_bound(_y) )	cur_y = _y;

		updateCaret();
		reach_line_end = false;
	}
	inline void goUp(int _v = 1)			//游標往上
	{	gotoXY( cur_x , cur_y - _v );	ScrollToCaret();	}
	void goDown(int _v = 1)		//游標往下
	{	gotoXY( cur_x , cur_y + _v );	ScrollToCaret();	}
	void goLeft(int _v = 1)		//游標往左
	{	gotoXY( cur_x - _v , cur_y );	ScrollToCaret();	}
	void goRight(int _v = 1)		//游標往右
	{
		bool b = (cur_x >= col_count - 1);
		gotoXY( cur_x + _v , cur_y );
		ScrollToCaret();
		if(b)	reach_line_end = true;
	}
	void goLineHead()			//游標移到同一行的開頭
	{	gotoXY( 0 , cur_y );	}
	void updateCaret();			//更新游標位置


	//捲動視窗
private:
	int m_scroll_region_top , m_scroll_region_bottom;
	bool m_isset_scroll_region_bottom;
public:
	inline void ScrollUp(int _v = 1);		//上捲
	void ScrollUp(int top, int bottom, int _v = 1);
	inline void ScrollDown(int _v = 1);	//下捲
	void ScrollDown(int top, int bottom, int _v = 1);

	//清除資料
 	void cleanScreen(int type = 2);			//清除螢幕
	void CleanLineTail();		//清除行尾
	void CleanLine(int _y);		//清除整行

	void abs_InsertChar(int _v = 1);
	void abs_DeleteChar(int _v = 1);

	//輸入資料
	int parse(char *str , int len = -1 ,  int *scroll_count = NULL);		//輸出字串 (其中可以有控制碼)

	wxSize getWindowSize()	{	return wxSize( col_count*char_width, row_count*char_height );	}
	wxSize getCharSize()	{	return wxSize( char_width, char_height );	}

	//閃爍字元的 method
private:
	bool hasBlinkChar;
protected:
	inline void setHasBlinkChar(bool _v)	{	hasBlinkChar = _v;	}
public:
	inline bool getHasBlinkChar()	{	return hasBlinkChar;	}
	void setBlinkCharVisibility(bool _v);	//是否顯示閃爍字元
	inline bool getBlinkCharVisibility()	{	return blBlinkCharVisibility;	}

	//重新繪圖
	void OnPaint(wxPaintEvent& event);
	void OnPaint(wxDC *dc);
	void repaint(bool eraseBackground = false);
	void repaint(wxDC *dc, int x, int y, int w, int h);

protected:
	//繪圖
	void repaintChar(int _x, int _y);	//顯示位置於 (_x,_y) 的字元
	void repaintLine(int _y = -1);

	//將滑鼠座標轉換成相對應的文字座標
	void MouseXY_to_TextXY(int m_x, int m_y, int & t_x, int & t_y);

protected:
	TerminalChar  **term_data;	//terminal 中每個字的資料, 屬性
	void updateScrollBar(int linecount)
 	{
      	row_count = linecount;
      	if( scrollwin )	scrollwin->SetVirtualSize( getWindowSize() );
	}
private:
	TerminalChar  now_char_property;	//目前字型屬性

	int col_count, row_count;	//terminal 行列數
	int cur_x, cur_y;			//游標所在位置
public:
	int getLineLength(int _line = -1);
	inline void getColumnRowCount(int *_col, int *_row)
 	{
		if(_col)	*_col = col_count;
		if(_row)	*_row = row_count;
  	}

private:

	int screen_width, screen_height;	//畫面的長寬
	int char_width, char_height;	//字元寬度高度

	bool blBlinkCharVisibility;	//是否顯示閃爍字元

	bool is_X_in_bound(int _x)
	{	return ( _x >= 0 ) && ( _x < col_count );	}
	bool is_Y_in_bound(int _y)
	{	return ( _y >= 0 ) && ( _y < row_count );	}

	wxWindow *parentWindow;
	wxDC *currentDC;
	bool blEnableDrawing;

protected:
	void adjustLineCharInfo(int _y = -1);	//更新某行所有字元的 type 屬性 (判斷半形字和全形字)
	inline bool isWord(char first, char last);

public:
	//避免系統因連續多次變更資料內容而造成畫面閃爍
	void BeginDrawing(wxDC *dc);
	void EndDrawing();
	void EnableDrawing(bool b)	{	blEnableDrawing = b;	}
private:
	inline bool isDrawing()	{	return (currentDC != NULL);	}
	inline wxDC* getDC()	//取得目前的 DC
 	{
      	if( ! blEnableDrawing || ! getVisible() )	return NULL;
  		return currentDC;
    }

	wxCaret* getCaret()
 	{	return ( parentWindow == NULL ) ? NULL : parentWindow->GetCaret();	}

protected:
	bool isCurrentAWord();	//游標目前所處位置是否為全形字的第一個 byte
	bool isLeftAWord();		//游標目前所處位置的左邊是否為全形字

private: bool isVisible;
public:
	void Show();
	inline void Hide()	{	isVisible = false;	getCaret()->Hide();		}
	inline bool getVisible()	{	return isVisible;	}

	inline wxWindow* getParentWindow()	{	return parentWindow;	}

	wxFont term_fnt;
	void SetFont(const wxFont& fnt);
	wxFont& GetFont()	{	return term_fnt;	}

private:
	typedef void (*ON_LINK_CLICKED_FUNC)(char *, LINK_TYPE);
	ON_LINK_CLICKED_FUNC OnLinkClickedFunc;
public:
	void SetOnLinkClickedFunc( ON_LINK_CLICKED_FUNC _f )	{	OnLinkClickedFunc = _f;	}

	//***** 滑鼠事件, 用來管理 [選取文字] 的動作 *****//

protected:
	int start_x, start_y, end_x, end_y;	//文字選取範圍的開頭和結尾
	unsigned char selectState;	//0:尚未選取 1:剛按下左鍵 2:正在選取文字 3:結束選取 (但仍保留選取標示)
	inline bool isCharSelected(int _x, int _y);
	inline void getSelectionInfo(int *sx, int *sy, int *ex, int *ey)
	{
     	if( start_y < end_y
			|| ( start_y == end_y  &&  start_x <= end_x )  )
		{	*sx = start_x;	*sy = start_y;	*ex = end_x;	*ey = end_y;	}
		else
		{	*ex = start_x;	*ey = start_y;	*sx = end_x;	*sy = end_y;	}
	}

public:
	void OpenSelectionAsHyperlink();

	//ask if someone call you (find is any rung bell)
private:
	bool bBell;
public:
	bool isBell()
 	{
  		if( bBell )
  		{	bBell = false;	return true;	}
  		else return false;
	}


private:
    bool blEnableDoubleByteDetection;	//是否開啟雙位元偵測功能
public:
	inline void setEnableDoubleByteDetection(bool b)
    {
    	if( blEnableDoubleByteDetection == b )	return;
    	blEnableDoubleByteDetection = b;
    }
	inline bool getEnableDoubleByteDetection()
	{	return blEnableDoubleByteDetection;	}

protected:
	wxString GetLineWrapedString(char *txt);	//將 txt 做自動斷行, 使每一行的字數不會超過指定的長度
public:
	static void SetLineWrapedLength(int len);	//設定自動斷行的長度
	static int GetLineWrapedLength();

public:

	//選取文字, 複製選取文字
	void SelectAll();
	void CancelSelection();	//取消選取
	wxString GetSelectionContent(bool withANSI);	//取得所選取的文字
	wxString GetAllContent(bool withANSI);			//取得所有文字
	void CopySelectionToClipboard(bool withANSI);	//將選取的文字複製到剪貼簿
	inline bool isSelected()	{	return (selectState == 2 || selectState == 3);	}

	//滑鼠事件 (處理選取文字動作)
	void OnMouseLeftDown(wxMouseEvent& event);
	void OnMouseMotion(wxMouseEvent& event);
	void OnMouseLeftUp(wxMouseEvent& event);
	void OnMouseLeftDoubleClick(wxMouseEvent& event);
};

// ============================================================================
#endif
