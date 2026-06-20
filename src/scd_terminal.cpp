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


#ifndef SCD_TERMINAL_CPP
#define SCD_TERMINAL_CPP

#include "scd_terminal.h"

#include <string>

static wxString Big5BytesToWxString(const char *bytes, size_t len)
{
	static wxCSConv big5_conv("BIG5HKSCS");

	wxString text = wxEmptyString;
	size_t i = 0;
	while (i < len)
	{
		unsigned char first_ch = (unsigned char)bytes[i];
		if (first_ch >= 0x81 && first_ch <= 0xfe)
		{
			if (i + 1 < len)
			{
				unsigned char last_ch = (unsigned char)bytes[i + 1];
				if ((last_ch >= 0x40 && last_ch <= 0x7e) || (last_ch >= 0xa1 && last_ch <= 0xfe))
				{
					wxString ch_str(bytes + i, big5_conv, 2);
					if (!ch_str.IsEmpty())
					{
						text.Append(ch_str);
					}
					else
					{
						text.Append(_T("?"));
					}
					i += 2;
					continue;
				}
			}
			text.Append(_T("?"));
			i += 1;
		}
		else if (first_ch <= 0x7f)
		{
			text.Append((wxChar)first_ch);
			i += 1;
		}
		else
		{
			text.Append(_T("?"));
			i += 1;
		}
	}

	return text;
}

// ============================================================================

static bool always_highlight = false;	//有些人電腦上字體會顯的特別暗, 這個選項讓他們可以把所有字都變成高亮度
static int line_wraped_length = 80;


#define TerminalColorTypeCount 8

//terminal 字體顏色表
static wxColour TerminalColors[2][TerminalColorTypeCount] =
{
	{	//正常字 (黑紅綠黃藍紫靛白)
		wxColour(  0,  0,  0), wxColour(128,  0,  0), wxColour(  0,128,  0), wxColour(128,128,  0),
		wxColour(  0,  0,128), wxColour(128,  0,128), wxColour(  0,128,128), wxColour(192,192,192)
	} ,
	{	//高亮度
		wxColour(128,128,128), wxColour(255,  0,  0), wxColour(  0,255,  0), wxColour(255,255,  0),
		wxColour(  0,  0,255), wxColour(255,  0,255), wxColour(  0,255,255), wxColour(255,255,255)
	}
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
wxColour TerminalChar::getTextColor(bool _selected)
{
	if( ! _selected )
		return TerminalColors[ getHighlight() ][ Color.TextColor ];
	else
	{
		wxColour c( TerminalColors[ getHighlight() ][ Color.TextColor ] );
		c.Set( ~c.Red(), ~c.Green(), ~c.Blue() );
		return c;
	}
}
// ----------------------------------------------------------------------------
wxColour TerminalChar::getBgColor(bool _selected)
{
	if( ! _selected )
	return TerminalColors[0][ Color.BackColor ];
	else
	{
		wxColour c( TerminalColors[0][ Color.BackColor ] );
		c.Set( ~c.Red(), ~c.Green(), ~c.Blue() );
		return c;
	}
}
// ----------------------------------------------------------------------------
bool TerminalChar::getAlwaysHighlight()
{	return always_highlight;	}
// ----------------------------------------------------------------------------
void TerminalChar::setAlwaysHighlight(bool b)
{	always_highlight = b;	}
// ----------------------------------------------------------------------------
wxString TerminalChar::getANSICode()
{
	wxString code;
	TerminalChar def_ch = getDefaultCharProperty();
	code = _T("\x1b[");
	code += _T("0");	//消除高亮度或閃爍或底線屬性的話
	if( getHighlight() )	code += _T(";1");
	if( getUnderline() )	code += _T(";4");
	if( getBlink() )	code += _T(";5");
	if( getTextColorCode() != def_ch.getTextColorCode() )	code += wxString::Format( _T(";%d"), getTextColorCode() );
	if( getBgColorCode()   != def_ch.getBgColorCode() )		code += wxString::Format( _T(";%d"), getBgColorCode() );
	code += _T("m");
	return code;
}
// ----------------------------------------------------------------------------
wxString TerminalChar::getDiffANSICode(TerminalChar & _c)
{
	wxString code;
	code = _T("\x1b[");

	if(  ( _c.getHighlight() && ! getHighlight() ) 	//如果需要消除高亮度或閃爍或底線屬性的話
		|| ( _c.getBlink() && ! getBlink() )
		|| ( _c.getUnderline() && ! getUnderline() ) )
	{
		TerminalChar def_ch = getDefaultCharProperty();
		code += _T(";");
		if( getHighlight() )	code += _T("1;");
		if( getUnderline() )	code += _T("4;");
		if( getBlink() )	code += _T("5;");
		if( getTextColorCode() != def_ch.getTextColorCode() )	code += wxString::Format( _T("%d;"), getTextColorCode() );
		if( getBgColorCode()   != def_ch.getBgColorCode() )		code += wxString::Format( _T("%d"), getBgColorCode() );
	}
	else
	{
		if( ! _c.getHighlight() && getHighlight() )	code += _T("1;");
		if( ! _c.getUnderline() && getUnderline() )	code += _T("4;");
		if( ! _c.getBlink() && getBlink() )	code += _T("5;");
		if( _c.getTextColorCode() != getTextColorCode() )	code += wxString::Format( _T("%d;"), getTextColorCode() );
		if( _c.getBgColorCode() != getBgColorCode() )		code += wxString::Format( _T("%d"), getBgColorCode() );
	}
	if( code.Last() == ';' )	code.RemoveLast();
	code += _T("m");
	return code;
}
// ----------------------------------------------------------------------------
bool TerminalChar::withSameProperty(TerminalChar & ch)
{
	if( getHighlight() != ch.getHighlight() )	return false;
	if( getBlink() != ch.getBlink() )	return false;
	if( getUnderline() != ch.getUnderline() )	return false;
	if( getTextColor() != ch.getTextColor() )	return false;
	if( getBgColorCode() != ch.getBgColorCode() )	return false;
	return true;
}
// ----------------------------------------------------------------------------
void TerminalChar::setColorCode(int ColorCode)
{
	if( ColorCode >= 30 && ColorCode <= 37 )
		setTextColor(ColorCode);
	else if( ColorCode >= 40 && ColorCode <= 47 )
		setBgColor(ColorCode);
	else if( ColorCode == 0 )
	{
		setTextColor(37);
		setBgColor(40);
		setHighlight(false);
		setBlink(false);
		setUnderline(false);
	}
	else if( ColorCode == 1 )
		setHighlight(true);
	else if( ColorCode == 4 )
		setUnderline(true);
	else if( ColorCode == 5 )
		setBlink(true);
	else if( ColorCode == 7 )	//啟動反相色
	{
		setTextColor(30);
		setBgColor(47);
	}
	else if( ColorCode == 27 )	//取消反相色
	{
		setTextColor(37);
		setBgColor(40);
	}
}
// ----------------------------------------------------------------------------
TerminalChar TerminalChar::getDefaultCharProperty()
{
	TerminalChar tc;
	tc.setTextColor(37);
	tc.setBgColor(40);
	tc.setHighlight(false);
	tc.setBlink(false);
	tc.setUnderline(false);
	tc.ch = '\0';
	tc.setCharType(CH_CHAR);
	tc.setLinkType(LINK_NONE);
	return tc;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
SCD_Terminal::SCD_Terminal(wxWindow *win)
{
	term_data = NULL;
	currentDC = NULL;
	scrollwin = NULL;
	parentWindow = win;
	isVisible = false;
	blEnableDrawing = true;

	SetFont( parentWindow->GetFont() );
	char_height = parentWindow->GetFont().GetPointSize();
	char_width  = char_height / 2;

	term_data = NULL;
	setColumnRow(80,24);

	gotoXY(0,0);
	blBlinkCharVisibility = true;
	setHasBlinkChar(false);
	OnLinkClickedFunc = NULL;

	selectState = 0;	//使用者尚未開始選取文字
	blEnableDoubleByteDetection = true;

	now_char_property = now_char_property.getDefaultCharProperty();
	bBell = false;

	ResetTerminal();
}
// ----------------------------------------------------------------------------
SCD_Terminal::~SCD_Terminal()
{
	for(int y=0;y<row_count;y++)	delete[] term_data[y];
	delete[] term_data;
}
// ----------------------------------------------------------------------------
void SCD_Terminal::ResetTerminal()
{
	m_scroll_region_top = 0;
	m_isset_scroll_region_bottom = false;
	application_cursor_keys = false;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_Terminal::SetFont(const wxFont& fnt)
{
	//設定字體, 字體大小, 游標
	term_fnt = fnt;

	wxClientDC dc(parentWindow);
	dc.SetFont(fnt);
	char_width  = dc.GetCharWidth();
	char_height = dc.GetCharHeight();

	wxCaret *caret = parentWindow->GetCaret();
	if(caret)	caret->SetSize( char_width, 2 );

	if( scrollwin )
	{
		int _x, _y;
		scrollwin->GetViewStart( &_x, &_y );
		scrollwin->SetScrollbars( char_width, char_height, 80, row_count, _x, _y );
	}
}
// ----------------------------------------------------------------------------
void SCD_Terminal::Show()
{
	if( getVisible() )	return;
	if( parentWindow == NULL )	return;

	isVisible = true;

	if(blEnableDrawing)	repaint();	//重繪視窗

	//顯示游標
	wxCaret *caret = parentWindow->GetCaret();
	if( caret == NULL )
	{
		wxMessageBox(_T("caret not found"));
		parentWindow = NULL;
	}
	else
	{
		caret->SetSize( char_width, 2 );
		caret->Show();
	}
}
// ----------------------------------------------------------------------------
void SCD_Terminal::setColumnRow(int _c, int _r)
{
	if( term_data == NULL )
	{	col_count = 0;	row_count = 0;	}

	if( _c == col_count && _r == row_count )	return;

	TerminalChar **old_term_data = term_data;

	term_data = new TerminalChar*[_r];
	for(int i=0;i<_r;i++)
		term_data[i] = new TerminalChar[_c];

	TerminalChar dch = TerminalChar::getDefaultCharProperty();
	int _k = (_c > col_count) ? col_count : _c;
	for(int y=0;y<_r;y++)
	{
		int x = 0;
		if( y < row_count && old_term_data )
		{	for(x=0;x<_k;x++)	term_data[y][x] = old_term_data[y][x];	}
		for(;x<_c;x++)	term_data[y][x] = dch;
	}

	//清除舊的記憶體
	if( old_term_data )
	{
	for(int y=0;y<row_count;y++)	delete[] old_term_data[y];
	delete[] old_term_data;
	}

	col_count = _c;
	row_count = _r;
	if( cur_x >= _c )	cur_x = _c - 1;
	if( cur_y >= _r )	cur_y = _r - 1;
}
// ----------------------------------------------------------------------------
void SCD_Terminal::BeginDrawing(wxDC *dc)	//避免系統因連續多次變更資料內容而造成畫面閃爍
{
	if( ! getVisible() )	return;
	currentDC = dc;
	if(currentDC == NULL)	return;
	if(scrollwin)	scrollwin->PrepareDC(*currentDC);

	if(parentWindow != NULL)
	{
		parentWindow->GetCaret()->Move(-100, -100);
		currentDC->SetFont( GetFont() );
	}
#if !wxCHECK_VERSION(2, 9, 0)
	currentDC->BeginDrawing();
#endif
	currentDC->SetPen(*wxTRANSPARENT_PEN);	//禁止 DrawRectangle() 時用 Pen 畫邊框
}
// ----------------------------------------------------------------------------
void SCD_Terminal::EndDrawing()
{
	if( ! getVisible() )	return;
#if !wxCHECK_VERSION(2, 9, 0)
	if( currentDC != NULL ) currentDC->EndDrawing();
#endif
	currentDC = NULL;
	updateCaret();
}
// ----------------------------------------------------------------------------
int SCD_Terminal::getLineCountInView()	//取得目前顯示區域一共顯示幾行資訊
{
	wxSize ws = getParentWindow()->GetClientSize();
	wxSize cs = getCharSize();
	return ws.GetHeight() / cs.GetHeight();
}
// ----------------------------------------------------------------------------
void SCD_Terminal::ScrollToCaret()
{
    if( scrollwin == NULL )	return;

	int lc = getLineCountInView();

	//如果游標所處位置超出捲軸顯示範圍, 則移動捲軸
	int _vx, _vy;
	scrollwin->GetViewStart(&_vx, &_vy);	//in scroll units
	if( cur_y < _vy )	scrollwin->Scroll( -1, cur_y );
	else if( cur_y >= _vy + (lc-1) )	scrollwin->Scroll( -1, cur_y - (lc-1) );
	//repaint();	//不需要 repaint() 因為在 wxScrolledWindow::Scroll() 的時候, wxScrolledWindow 就會自動 Refresh 需要重繪的部份
}
// ----------------------------------------------------------------------------
void SCD_Terminal::updateCaret()
{
	if( isDrawing() )	return;
	if( ! getVisible() )	return;

	wxCaret *c;
	c = getCaret();

	if( c != NULL )
	{
	int _x , _y;
	_x = cur_x * char_width;
	_y = cur_y * char_height + char_height - 2;
	if( scrollwin )	scrollwin->CalcScrolledPosition(_x,_y,&_x,&_y);
		c->Move( _x , _y );
	}
}
// ----------------------------------------------------------------------------
void SCD_Terminal::cleanScreen(int type)
{
	//type==0 : Erase from current position to end (inclusive)
	//type==1 : Erase from beginning to current position (inclusive)
	//type==2 : Erase entire display

	selectState = 0;

	TerminalChar defaultCharProperty = now_char_property.getDefaultCharProperty();
	for(int y=0;y<row_count;y++)
	{
		if( (type==0 && y>cur_y) || (type==1 && y<cur_y) || type==2 )
		{	CleanLine(y);	continue;	}
		else if(cur_y==y)
		{
			for(int x=0;x<col_count;x++)
			{
				if( (type==0 && x>=cur_x) || (type==1 && x<=cur_x) )
					term_data[y][x] = defaultCharProperty;
			}
		}
	}

	if(type==2)
	{
		setHasBlinkChar(false);
		gotoXY(0,0);
	}
	else repaintLine(cur_y);
}
// ----------------------------------------------------------------------------
void SCD_Terminal::CleanLine(int _y)
{
	TerminalChar defaultCharProperty = now_char_property.getDefaultCharProperty();

	for(int x=0;x<col_count;x++)
		term_data[_y][x] = defaultCharProperty;

	wxDC *dc = getDC();
	if(dc != NULL)
	{
	dc->SetBrush( wxBrush( TerminalChar::getDefaultCharProperty().getBgColor() , wxSOLID ) );
		dc->DrawRectangle(0, char_height * _y, char_width * col_count , char_height );
	}
}
// ----------------------------------------------------------------------------
void SCD_Terminal::CleanLineTail()
{
	TerminalChar defaultCharProperty = now_char_property.getDefaultCharProperty();

	for(int x=cur_x;x<col_count;x++)
		term_data[cur_y][x] = defaultCharProperty;
}
// ----------------------------------------------------------------------------
void SCD_Terminal::ClearWideCharAt(int _x, int _y)
{
	if( ! ( is_X_in_bound(_x) && is_Y_in_bound(_y) ) )	return;

	TerminalChar blank = now_char_property;
	blank.ch = ' ';
	blank.setCharType(TerminalChar::CH_CHAR);

	if( term_data[_y][_x].getCharType() == TerminalChar::CH_WORDFIRST && is_X_in_bound(_x + 1) )
		term_data[_y][_x + 1] = blank;
	else if( term_data[_y][_x].getCharType() == TerminalChar::CH_WORDLAST && is_X_in_bound(_x - 1) )
		term_data[_y][_x - 1] = blank;
}
// ----------------------------------------------------------------------------
void SCD_Terminal::abs_InsertChar(int _v)
{
	for(int i=col_count-1;i-_v>=cur_x;i--)	//把前面資料搬移到後面
		term_data[cur_y][i] = term_data[cur_y][i-_v];

	TerminalChar defaultCharProperty = now_char_property.getDefaultCharProperty();
	//把後面資料清空
	for(int i=0;i<_v;i++)
		term_data[cur_y][cur_x+i] = defaultCharProperty;
}
// ----------------------------------------------------------------------------
void SCD_Terminal::abs_DeleteChar(int _v)
{
	for(int i=cur_x+_v;i<col_count;i++)	//把後面資料搬移到前面
		term_data[cur_y][i-_v] = term_data[cur_y][i];

	TerminalChar defaultCharProperty = now_char_property.getDefaultCharProperty();
	//把後面資料清空
	for(int i=0;i<+_v;i++)
		term_data[cur_y][col_count-1-i] = defaultCharProperty;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_Terminal::ScrollUp(int _v)
{
	int bot;
	if(m_isset_scroll_region_bottom)	bot = m_scroll_region_bottom;
	else bot = row_count - 1;
	ScrollUp( m_scroll_region_top , bot , _v );
}
// ----------------------------------------------------------------------------
void SCD_Terminal::ScrollUp(int top, int bottom, int _v)	//把指定範圍的每一行往上搬移
{
	TerminalChar *tmp_lines[1000];

	if(_v > bottom - top)	_v = bottom - top;

	for(int i=0;i<_v;i++)	tmp_lines[i] = term_data[top+i];	//暫存前面 _v 行
	for(int y=top+_v;y<=bottom;y++) //由上而下, 搬移行資料
		term_data[y-_v] = term_data[y];

	wxDC *dc = getDC();

	for(int i=0;i<_v;i++)
	{
		term_data[bottom-_v+1+i] = tmp_lines[i];
		CleanLine(bottom-_v+1+i);
	}

	if(dc != NULL)
	{
		for(int y=top;y<=bottom;y++)
			repaintLine(y);
	}
}
// ----------------------------------------------------------------------------
void SCD_Terminal::ScrollDown(int _v)
{
	int bot;
	if(m_isset_scroll_region_bottom)	bot = m_scroll_region_bottom;
	else bot = row_count - 1;
	ScrollDown( m_scroll_region_top , bot , _v );
}
// ----------------------------------------------------------------------------
void SCD_Terminal::ScrollDown(int top, int bottom, int _v)
{
	TerminalChar *tmp_lines[1000];

	if(_v > bottom - top)	_v = bottom - top;

	for(int i=0;i<_v;i++)	tmp_lines[i] = term_data[bottom-i];	//暫存後面 _v 行
	for(int y=bottom-_v;y>=top;y--) //由下而上, 搬移行資料
		term_data[y+_v] = term_data[y];

	wxDC *dc = getDC();

	for(int i=0;i<_v;i++)
	{
		term_data[top+i] = tmp_lines[i];
		CleanLine(top+i);
	}

	if(dc != NULL)
	{
		for(int y=top;y<=bottom;y++)
			repaintLine(y);
	}
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
int SCD_Terminal::getLineLength(int _line)
{
	if( _line == -1 )	_line = cur_y;

	int i;
	for(i=col_count-1;i>=0 && term_data[_line][i].ch == '\0' ;i--);
	return i+1;
}

bool SCD_Terminal::isWord(char first, char last)
{
	unsigned char first_ch = (unsigned char)first;
	unsigned char last_ch = (unsigned char)last;

	return first_ch >= 0x81 && first_ch <= 0xfe &&
		( (last_ch >= 0x40 && last_ch <= 0x7e) ||
		(last_ch >= 0xa1 && last_ch <= 0xfe) );
}
// ----------------------------------------------------------------------------
bool SCD_Terminal::isCurrentAWord()	//游標目前所處位置是否為全形字的第一個 byte
{	return term_data[cur_y][cur_x].getCharType() == TerminalChar::CH_WORDFIRST;	}
// ----------------------------------------------------------------------------
bool SCD_Terminal::isLeftAWord()		//游標目前所處位置的左邊是否為全形字
{
	if( ! is_X_in_bound( cur_x - 1 ) )	return false;
	return term_data[cur_y][cur_x-1].getCharType() == TerminalChar::CH_WORDLAST;
}
// ----------------------------------------------------------------------------
int SCD_Terminal::parse(char *input , int len , int *scroll_count)
{
//	char *begin = input;
	wxDC *dc = NULL;
	if( getVisible() && getDC() == NULL )
	{
		dc = new wxClientDC( getParentWindow() );
		BeginDrawing(dc);
	}

	if( len < 0 )
		len = strlen(input);

	int unread_data_len = 0;	//如果最後遇到控制碼資料不完整的情況, 則停止分析最後的控制碼
								//並傳回最後沒分析的控制碼資料長度

	char *end = input + len;


	//記錄更新資訊, 方便重繪資料
	int last_line_index = cur_y;
	bool line_updated = false;


	while( input != end )	//如果還有資料
	{
		if( input > end )
		{

/* 有時程式會掉入這個 if 區塊 , 但是理論上不應該有機會進入這裡

		wxMessageBox(_T("parse() out of bound"));

wxString str = wxEmptyString;
for(char *r=input-10;r<end;r++)	str += wxString::Format("%c", *r);
wxMessageBox(str);

*/

		break;
		}


		if( *input == '\0' )	//忽略 字元'\0'
		{
			input ++;
			continue;
		}

		if( *input == '\x1b' )	//控制碼開頭
		{
			char *control_code_head = input;

			input ++;

			bool has_left_quote = false;
			bool has_question = false;

			if( *input == '[' )	input++;
			else if( *input == ']' )	//ssh/telnet server pwd
			{
				while(input!=end)
				{
					if(*input == '\x07')	{	input++;	break;	}
					input++;
				}
				continue;
			}
			else	//if there is no '[' follow the '\x1b'
			{
				switch(*input)
				{
					case '7' :
						server_stored_cur_pos = wxPoint( cur_x, cur_y );
						input++;
						break;
					case '8' :
						cur_x = server_stored_cur_pos.x;
						cur_y = server_stored_cur_pos.y;

						if( line_updated )	//如果換行前有資料更新過, 就重繪前一行的資料
						{
							adjustLineCharInfo( last_line_index );
							repaintLine( last_line_index );
							line_updated = false;
						}
						last_line_index = cur_y;
						input++;
						break;
					case 'M' :	// reverse index
						if( line_updated )
						{
							adjustLineCharInfo( last_line_index );
							repaintLine( last_line_index );
							line_updated = false;
						}
						if( cur_y == m_scroll_region_top )
							ScrollDown();
						else
							goUp();
						last_line_index = cur_y;
						input++;
						break;
					default :
						input++;
						break;
				}
				continue;
			}

			if( *input == '?' )	{ has_question = true; input++; }	//開頭的 '?' 可略過
/*
			if( input == end )	//如果 [控制碼開頭] 不完整, 則跳出
			{
//  				unread_data_len = 1;
				break;
			}
*/

			const int param_list_size = 32;
			int param_count, param_list[param_list_size];
			param_count = 0;
			for( int i=0; i<param_list_size; i++ )
				param_list[i] = -1;

			while( input != end )	// parse control code
			{
				if( isdigit( (unsigned char)*input ) )
				{
					if( param_count < param_list_size - 1 )
					{
						if( param_list[ param_count ] < 0 )
							param_list[ param_count ] = 0;

						if( param_list[ param_count ] < 1000000 )
							param_list[ param_count ] = param_list[ param_count ] * 10 + ( *input - '0' );
					}
				}
				else if( *input == ';' )
				{
					if( param_count < param_list_size - 1 )
					{
						if( param_list[ param_count ] < 0 )
							param_list[param_count] = 0;

						param_count ++;
						param_list[ param_count ] = -1;
					}
				}
				else //if( isalpha( *input ) )		//控制碼結尾
				{
				switch( *input )
				{
					case 'h' :	// set mode
						if( has_question && param_list[0] == 1 )
							application_cursor_keys = true;
						break;
					case 'l' :	// reset mode
						if( has_question && param_list[0] == 1 )
							application_cursor_keys = false;
						break;
					case 'm' :	//顏色碼
							if(param_list[0] < 0)
								now_char_property = TerminalChar::getDefaultCharProperty();
							else
							{
							for(int i=0;param_list[i] >= 0;i++)
							{
									if( param_list[i] == 5 )	setHasBlinkChar(true);
								now_char_property.setColorCode( param_list[i] );
								}
							}
						break;
					case 'H' :	//gotoXY
					case 'f' :	//gotoXY
							if( param_list[0] < 0 || param_list[1] < 0 )
								gotoXY(0,0);
							else
							{
						if( param_list[0] == 0 )	param_list[0] = 1;
						if( param_list[1] == 0 )	param_list[1] = 1;
								gotoXY( param_list[1] - 1 , param_list[0] - 1 );
							}
						break;
					case 'J' :	//清除螢幕
							if( param_list[0] != 1 && param_list[0] != 2)
								param_list[0] = 0;
						cleanScreen( param_list[0] );
						line_updated = false;
						break;
					case 'K' :	//清除行尾
							CleanLineTail();
							line_updated = true;
						break;
					case 'P' :	//刪除字元
							abs_DeleteChar( param_list[0] > 0 ? param_list[0] : 1 );
							line_updated = true;
						break;
						case '@' :	//插入字元
							abs_InsertChar( param_list[0] > 0 ? param_list[0] : 1 );
							line_updated = true;
							break;
					case 'L' :	//插入 n 行
//							if( param_list[0] > 0 )	ScrollUp( param_list[0] );
							{
							int c = 1;
							if( param_list[0] > 0 )	c = param_list[0];
							int tmp = m_scroll_region_top;
							m_scroll_region_top = cur_y;
							ScrollDown(c);
							m_scroll_region_top = tmp;
							}
						break;
					case 'M' :	//刪除 n 行
							{
							int c = 1;
							if( param_list[0] > 0 )	c = param_list[0];
							int tmp = m_scroll_region_top;
							m_scroll_region_top = cur_y;

							if(has_left_quote)	ScrollUp(c);
							else ScrollDown(c);

							m_scroll_region_top = tmp;
							}
						break;
						case 'S' :	// scroll up n lines
							ScrollUp(param_list[0] > 0 ? param_list[0] : 1);
							break;
						case 'T' :	// scroll down n lines
							ScrollDown(param_list[0] > 0 ? param_list[0] : 1);
							break;
					case 'r' :	//指定捲動視窗的範圍
							m_isset_scroll_region_bottom = false;
							if( param_list[0] < 0 )	m_scroll_region_top = 0;
							else if( param_list[1] < 0 )
							{	m_scroll_region_top = param_list[0] - 1;}
							else
							{
								m_scroll_region_top = param_list[0] - 1;
								m_scroll_region_bottom = param_list[1] - 1;
								m_isset_scroll_region_bottom = true;
							}
//wxMessageBox( wxString::Format("%d - %d (%d,%d,%d)", m_scroll_region_top, m_scroll_region_bottom , param_count, param_list[0] , param_list[1] ) );
						break;

					case 'A' :	//游標上移
						if(param_list[0] >= 0)	goUp( param_list[0] );
						else	goUp(1);
						break;
					case 'B' :	//游標下移
						if(param_list[0] >= 0)	goDown( param_list[0] );
						else	goDown(1);
						break;
					case 'C' :	//游標右移
						if(param_list[0] >= 0)	goRight( param_list[0] );
						else	goRight(1);
						break;
					case 'G' :	// cursor horizontal absolute
					case '`' :	// horizontal position absolute
						{
							int x = (param_list[0] > 0) ? param_list[0] - 1 : 0;
							gotoXY(x, cur_y);
						}
						break;
					case 'd' :	// vertical position absolute
						{
							int y = (param_list[0] > 0) ? param_list[0] - 1 : 0;
							gotoXY(cur_x, y);
						}
						break;
					case 's' :	// save cursor
						server_stored_cur_pos = wxPoint(cur_x, cur_y);
						break;
					case 'u' :	// restore cursor
						gotoXY(server_stored_cur_pos.x, server_stored_cur_pos.y);
						break;
					case 'D' :	//游標左移
						if(param_list[0] >= 0)	goLeft( param_list[0] );
						else	goLeft(1);
						break;
						default :	//未知控制碼
							break;
				}
				break;
				}
/*
				else
				{
					//未知控制碼
				}
*/
				input ++;
			}

			if( input >= end )	//如果到了 end 卻還沒分析完控制碼
			{
				unread_data_len = (int)(end - control_code_head);
				break;
			}
	}
	else if( *input == '\n' )	// \x0a
		{
			int c = 1;

			//由於常常有連續換行符號出現的情形,
			//如果有連續換行符號, 則一次讀取, 可以在需要 ScrollDown() 的時候,
		//減輕更新資料以及重新繪圖的負擔
		// '\n' 和 '\r' 常常成對出現
			input ++;
			while( input != end )
			{
			if( *input == '\n' )
			{	c++;	input++;	}
			else if( *input == '\r' )	input++;
	else	{	input--;	break;	}
			}

			//由於 goDown() 有可能會 ScrollDown()
			//每一行的行索引將會有所變動
			//因此要先更新資料
			if( line_updated )	//如果換行前有資料更新過, 就重繪前一行的資料
			{
				adjustLineCharInfo( last_line_index );
				repaintLine( last_line_index );
				line_updated = false;
			}

			//
			int bot = m_scroll_region_bottom;
			if( ! m_isset_scroll_region_bottom )	bot = row_count - 1;

			int tmp_c = bot - cur_y;
			int caret_down = (c<=tmp_c) ? c : tmp_c;
			if(caret_down > 0)	goDown(caret_down);
			if( c > caret_down )
			{
				ScrollUp(c - caret_down);
				if(scroll_count != NULL)	(*scroll_count)++;
			}
			//
//wxMessageBox( wxString::Format("b: %d - %d - %d - %d", c , cur_y , bot, m_scroll_region_bottom) );
/*
			int tmp_c = row_count - cur_y - 1;
			int caret_down = (c<=tmp_c) ? c : tmp_c;
			if(caret_down > 0)	goDown(caret_down);
			if( c > caret_down )	ScrollUp(0, row_count-1, c - caret_down );
*/
			last_line_index = cur_y;
	}
	else if( *input == '\r' )	// \x0d
			goLineHead();
	else if( *input == '\b' )
	{	goLeft();	}
	else if( *input == '\a' )
		{
			bBell = true;
/*
*end = '\0';
wxString tmp_str;
for(char *r=begin;r<end;r++)
	tmp_str += wxString::Format("%02x(%c) ", *r , isalnum(*r)?*r:' ');
wxMessageBox(tmp_str + _T("\n\n") + wxString(begin) );
*/
		}
	else if( *input == '\xff' )
	{	;	}
	else if( *input == '\t' )
		{
//			while(true)
			int c = 8 - cur_x % 8;
			for(int i=0;i<c;i++)
			{
				ClearWideCharAt(cur_x, cur_y);
				term_data[cur_y][cur_x] = now_char_property;
				term_data[cur_y][cur_x].ch = ' ';
				goRight();

//				if( cur_x % 8 == 0 )	break;
				if( cur_x >= col_count - 1 )	break;
			}
			line_updated = true;
		}
	else if( ! iscntrl(*input) )
	{
			if(reach_line_end)	//for ssh (輸出超過一行的長度時自動換行)
			{
				reach_line_end = false;
				goLineHead();
				if( cur_y >= row_count - 1 )	ScrollUp(0, row_count-1);
				else	goDown();
			}

			ClearWideCharAt(cur_x, cur_y);
			term_data[cur_y][cur_x] = now_char_property;
			term_data[cur_y][cur_x].ch = *input;

			goRight();
			line_updated = true;
	}


		if( last_line_index != cur_y )	//如果有換行的動作
		{
			if( line_updated )	//如果換行前有資料更新過, 就重繪前一行的資料
			{
				adjustLineCharInfo( last_line_index );
				repaintLine( last_line_index );
				line_updated = false;
			}
			last_line_index = cur_y;
		}

		//繼續分析下一個字元
        input ++;
	}

	if( line_updated )	//如果換行前有資料更新過, 就重繪前一行的資料
	{
		adjustLineCharInfo( last_line_index );
		repaintLine( last_line_index );
	}

	if( getVisible() && dc != NULL )
	{
		EndDrawing();
		delete dc;
	}

	return unread_data_len;
}
// ----------------------------------------------------------------------------
void SCD_Terminal::adjustLineCharInfo(int _y)
{
	int x = 0;
	if(_y == -1)	_y = cur_y;
	for(x=0;x<col_count-1;x++)
	{
		if( isWord( term_data[_y][x].ch , term_data[_y][x+1].ch ) )
		{

#if defined(__BSD__)

static char table[] = "┌┬┐├┼┤└┴┘┌┬┐├┼┤└┴┘┌┬┐├┼┤└┴┘│─╭╮╰╯▇";

bool flag = true;
char first = term_data[_y][x].ch;
char last = term_data[_y][x+1].ch;

if( (unsigned char)first >= 0xa0 )
{

//if( (unsigned char)first == 0xa1 && ! (((unsigned char)last >= 0x40 && (unsigned char)last <= 0x5c) || (unsigned char)last >= 0xad ) )	flag = false;	//bsd
//else if( (unsigned char)first == 0xa1 && ( (unsigned char)last == 0x5a || (unsigned char)last == 0xc3 || (unsigned char)last == 0xc5 || (unsigned char)last == 0xfe ) )    flag = false;	//bsd
if( (unsigned char)first == 0xa1 )
{
	unsigned char s = (unsigned char)last;
	if( s == 0x5a )	flag = false;	//5a will crash
	else if( s >= 0x40 && s <= 0x5f );
	else if( s >= 0x60 && s <= 0x6f );
	else if( s >= 0x70 && s <= 0x7e );
	else if( s == 0xc3 )	flag = false;
	else if( s == 0xc5 )	flag = false;
//	else if( s >= 0xa1 && s <= 0xcb );
	else if( s >= 0xa1 && s <= 0xcf );
//	else if( s >= 0xe4 && s <= 0xe5 );
//	else if( s >= 0xe8 && s <= 0xe9 );
//	else if( s >= 0xf0 && s <= 0xfd );

	else if( s >= 0xc2 && s <= 0xc7 );
	else if( s >= 0xd0 && s <= 0xdf );
	else if( s >= 0xe0 && s <= 0xef );
	else if( s >= 0xf0 && s <= 0xfd );	//fe will crash
	else flag = false;
}
//else if( (unsigned char)first == 0xa2 && (unsigned char)last >= 0x40 && (unsigned char)last <= 0x60 )	flag = false;	//bsd
//else if( (unsigned char)first == 0xa2 && (unsigned char)last >= 0xcc )	flag = false;	//bsd
else if( (unsigned char)first == 0xa2 )
{
	if( (unsigned char)last >= 0x41 && (unsigned char)last <= 0x49 );
	else if( (unsigned char)last >= 0x40 && (unsigned char)last <= 0x60 )	flag = false;
	else if( (unsigned char)last >= 0xcc )	flag = false;
}
else if( (unsigned char)first == 0xf9 )	flag = false;	//bsd 似乎沒辦法顯示大部分的表格符號
else if( (unsigned char)first == 0xa3 && (unsigned char)last >= 0xc0 )	flag = false;	//控制符號
else if( (unsigned char)first == 0xc8 )	flag = false;	//保留
else if( (unsigned char)first == 0xa0 )
{
	if( (unsigned char)last == 0xe6 )	flag = false;
	else if( (unsigned char)last == 0xe7 )	flag = false;
}

}

			if( ! flag )	//bsd 似乎沒辦法顯示大部分的表格符號
			{
				if( (unsigned char)first == 0xf9 && (unsigned char)last >= 0xdd && (unsigned char)last <= 0xfe )
				{
					term_data[_y][x].ch = table[ ( (unsigned char)last - 0xdd ) * 2 ];
					term_data[_y][x+1].ch = table[ ( (unsigned char)last - 0xdd ) * 2 + 1 ];
					term_data[_y][x].setCharType( TerminalChar::CH_WORDFIRST );
					term_data[_y][x+1].setCharType( TerminalChar::CH_WORDLAST );
				}
				else
				{
				term_data[_y][x].setCharType( TerminalChar::CH_CHAR );
				term_data[_y][x].setCharType( TerminalChar::CH_CHAR );
				}
			}
			else
#endif
			{
				term_data[_y][x].setCharType( TerminalChar::CH_WORDFIRST );
				term_data[_y][x+1].setCharType( TerminalChar::CH_WORDLAST );
			}
			x++;
		}
		else
			term_data[_y][x].setCharType( TerminalChar::CH_CHAR );
	}
	if(x==col_count-1)
		term_data[_y][x].setCharType( TerminalChar::CH_CHAR );


	//找尋有效連結

	int link_start_x;

	for(x=0;x<col_count-3;x++)
	{
		link_start_x = -1;
		if( term_data[_y][x].ch == ':' && term_data[_y][x+1].ch == '/' && term_data[_y][x+2].ch == '/' )
		{
			if( x >= 6 )
			{
				if( term_data[_y][x-6].ch == 't' && term_data[_y][x-5].ch == 'e' &&
					term_data[_y][x-4].ch == 'l' && term_data[_y][x-3].ch == 'n' &&
					term_data[_y][x-2].ch == 'e' && term_data[_y][x-1].ch == 't' )
				{	link_start_x = x - 6;	term_data[_y][x-6].setLinkType( LINK_TELNET );	}
			}
			if( x >= 4 )
			{
				if( term_data[_y][x-4].ch == 'h' && term_data[_y][x-3].ch == 't' &&
					term_data[_y][x-2].ch == 't' && term_data[_y][x-1].ch == 'p' )
				{	link_start_x = x - 4;	term_data[_y][x-4].setLinkType( LINK_HTTP );	}
				else if( term_data[_y][x-4].ch == 's' && term_data[_y][x-3].ch == 'f' &&
					term_data[_y][x-2].ch == 't' && term_data[_y][x-1].ch == 'p' )
				{	link_start_x = x - 4;	term_data[_y][x-4].setLinkType( LINK_SFTP );	}
			}
			if( x >= 3 )
			{
				if( term_data[_y][x-3].ch == 'f' && term_data[_y][x-2].ch == 't' &&
					term_data[_y][x-1].ch == 'p' )
				{	link_start_x = x - 3;	term_data[_y][x-3].setLinkType( LINK_FTP );	}
				else if( term_data[_y][x-3].ch == 'b' && term_data[_y][x-2].ch == 'b' &&
					term_data[_y][x-1].ch == 's' )
				{	link_start_x = x - 3;	term_data[_y][x-3].setLinkType( LINK_TELNET );	}
			}

		}
		else if(term_data[_y][x].ch == '@' && term_data[_y][x].getCharType() == TerminalChar::CH_CHAR )
		{
			int i;
			for(i=x; i>=0 && term_data[_y][i].ch != ' ' && term_data[_y][i].getCharType() == TerminalChar::CH_CHAR ;i--);
			link_start_x = i + 1;
			term_data[_y][link_start_x].setLinkType( LINK_EMAIL );
		}

		if( link_start_x >= 0 )
		{
			int tmp_x = x , link_end_x;
			bool has_dot_char = false;	//超連結字串中是否有字元 '.'
			for(x=link_start_x;x<col_count;x++)
			{
				if( term_data[_y][x].ch == ' ' || term_data[_y][x].ch == '\0' || term_data[_y][x].getCharType() != TerminalChar::CH_CHAR)
					break;
				else
					term_data[_y][x].setLinkType( term_data[_y][link_start_x].getLinkType() );
				if( term_data[_y][x].ch == '.' )	has_dot_char = true;
			}

			//檢查此字串是否符合超連結的構成要件，不是的話則取消其超連結屬性
			link_end_x = x;
			if( (link_end_x - link_start_x < 10) || (! has_dot_char) )
			{
				for(int k=link_start_x;k<link_end_x;k++)
					term_data[_y][k].setLinkType( LINK_NONE );
			}

			if( tmp_x > x )	x = tmp_x;	//加入這行是避免進入無限迴圈
		}
		else
		{
			term_data[_y][x].setLinkType( LINK_NONE );
		}

	}
}
// ----------------------------------------------------------------------------
inline bool SCD_Terminal::isCharSelected(int _x, int _y)
{
	if( selectState == 2 || selectState == 3 )
	{
		int s = start_y * col_count + start_x;
		int e = end_y * col_count + end_x;
		if( s > e )	{	int i = s;	s = e;	e = i;	}
		int n = _y * col_count + _x;
		return ( n >= s && n <= e );
	}
	else	return false;
}
// ----------------------------------------------------------------------------
void SCD_Terminal::MouseXY_to_TextXY(int m_x, int m_y, int & t_x, int & t_y)
{	//將滑鼠座標轉換成相對應的文字座標

	if( scrollwin )    	scrollwin->CalcUnscrolledPosition( m_x, m_y, &m_x, &m_y );

assert(char_width > 0);
assert(char_height > 0);

	t_x = m_x / char_width;		t_y = m_y / char_height;
	if( t_x < 0 ) t_x = 0;
	else if( t_x >= col_count ) t_x = col_count - 1;
	if( t_y < 0 ) t_y = 0;
	else if( t_y >= row_count ) t_y = row_count - 1;
}
// ----------------------------------------------------------------------------
void SCD_Terminal::repaintChar(int _x, int _y)
{
	wxDC *dc = getDC();
	if(dc == NULL) return;

	if( ! ( is_X_in_bound(_x) && is_Y_in_bound(_y) ) )	return;

	int _m, _n;	//這個文字的左上角的視窗座標
	_m = _x * char_width;
	_n = _y * char_height;

	TerminalChar & now_ch = term_data[_y][_x];

	char str[10];	//用來儲存要畫出的字

	//如果有被選取, 則要將色彩反相
	bool isSelected = isCharSelected( _x, _y );

	if( (! getBlinkCharVisibility()) && now_ch.getBlink() )
	{ //如果不顯示閃爍字元的話
	dc->SetBrush( wxBrush( now_ch.getBgColor(isSelected) , wxSOLID ) );
		dc->DrawRectangle( _m , _n , char_width , char_height);
	}
	else if( now_ch.ch == '\0' )
	{
	dc->SetBrush( wxBrush( now_ch.getBgColor(false) , wxSOLID ) );
		dc->DrawRectangle( _m , _n , char_width , char_height);
	}
	else if( now_ch.ch == ' ' )
	{
	dc->SetBrush( wxBrush( now_ch.getBgColor(isSelected) , wxSOLID ) );
		dc->DrawRectangle( _m , _n , char_width , char_height);
	}
	else if( now_ch.getCharType() == TerminalChar::CH_CHAR )
	{
		//畫背景色
	dc->SetBrush( wxBrush( now_ch.getBgColor(isSelected) , wxSOLID ) );
		dc->DrawRectangle( _m , _n , char_width , char_height);

		if( now_ch.ch < 0 )	return;	//加上這行避免 GTK 下會 crash :)
//fprintf(stderr, "char : %d -> %c\n", (int)now_ch.ch , now_ch.ch );

		//顯示字元
		bool blunderline = now_ch.getUnderline();
		if(blunderline)
		{
		wxFont fnt = dc->GetFont();
		fnt.SetUnderlined(true);
		dc->SetFont(fnt);
		}

		dc->SetTextForeground( now_ch.getTextColor(isSelected) );
		dc->DrawText( wxChar(now_ch.ch), _m , _n );
//		dc->DrawText( str, _m , _n );

		if(blunderline)
		{
		wxFont fnt = dc->GetFont();
		fnt.SetUnderlined(false);
		dc->SetFont(fnt);
		}
	}
	else if( now_ch.getCharType() == TerminalChar::CH_WORDFIRST )
	{
		TerminalChar & next_ch = term_data[_y][_x+1];

		bool isLastWordSelected = isCharSelected( _x + 1 , _y );
		bool isSameFront =
				( now_ch.getTextColor(isSelected) == next_ch.getTextColor(isLastWordSelected) )
				&& ( now_ch.getUnderline() == next_ch.getUnderline() );
		bool isSameBack = ( now_ch.getBgColor(isSelected) == next_ch.getBgColor(isLastWordSelected) );


		//畫背景色
		if( isSameBack )	//如果全形字兩個 byte 的底色都相同, 就一次畫好
		{
		dc->SetBrush( wxBrush( now_ch.getBgColor(isSelected) , wxSOLID ) );
			dc->DrawRectangle( _m , _n , char_width + char_width , char_height);
		}
		else
		{
		dc->SetBrush( wxBrush( now_ch.getBgColor(isSelected) , wxSOLID ) );
			dc->DrawRectangle( _m , _n , char_width , char_height);

		dc->SetBrush( wxBrush( next_ch.getBgColor(isLastWordSelected) , wxSOLID ) );
			dc->DrawRectangle( _m + char_width , _n , char_width , char_height);
		}

		//顯示字串
		str[0] = now_ch.ch;
		str[1] = next_ch.ch;
		str[2] = '\0';
		str[3] = '\0';

//fprintf(stderr, "word : %x %x %s\n", str[0], str[1], str );

		if( isSameFront )
		{
			bool blunderline = now_ch.getUnderline();
		if(blunderline)
		{
			wxFont fnt = dc->GetFont();
		fnt.SetUnderlined(true);
		dc->SetFont(fnt);
			}

			dc->SetTextForeground( now_ch.getTextColor(isSelected) );
			dc->DrawText( Big5BytesToWxString(str, 2), _m , _n );

		if(blunderline)
		{
			wxFont fnt = dc->GetFont();
		fnt.SetUnderlined(false);
		dc->SetFont(fnt);
			}
		}
		else	//如果全形字左右兩邊的字體顏色不一樣, 則分兩次畫
		{
			bool blunderline = now_ch.getUnderline();

			//顯示左半部
		if( blunderline )
		{
			wxFont fnt = dc->GetFont();
		fnt.SetUnderlined(true);
		dc->SetFont(fnt);
			}
/*
static wxEncodingConverter cv;
cv.Init( wxLocale::GetSystemEncoding() , wxFONTENCODING_UNICODE );
char text[10];
strcpy(text, str);
cv.Convert( text, str );
*/
			dc->SetTextForeground( now_ch.getTextColor(isSelected) );
			dc->SetClippingRegion( _m, _n, char_width, char_height );
			dc->DrawText( Big5BytesToWxString(str, 2), _m , _n );
			dc->DestroyClippingRegion();

			//顯示右半部
			if( blunderline != next_ch.getUnderline() )
			{
			blunderline = next_ch.getUnderline();
			wxFont fnt = dc->GetFont();
		fnt.SetUnderlined(blunderline);
		dc->SetFont(fnt);
			}

			dc->SetTextForeground( next_ch.getTextColor(isLastWordSelected) );
			dc->SetClippingRegion( _m + char_width, _n, char_width, char_height );
			dc->DrawText( Big5BytesToWxString(str, 2), _m , _n );
			dc->DestroyClippingRegion();

		if( blunderline )
		{
			wxFont fnt = dc->GetFont();
		fnt.SetUnderlined(false);
		dc->SetFont(fnt);
			}
		}
	}
	else// if( now_ch.getCharType() == TerminalChar::CH_WORDLAST )
	{
		repaintChar(_x-1, _y);
	}


	if( now_ch.getLinkType() != LINK_NONE )
	{
		dc->SetPen( *wxCYAN_PEN );
		dc->DrawLine( _m , _n + char_height - 1 , _m + char_width , _n + char_height - 1 );
		dc->SetPen(*wxTRANSPARENT_PEN);	//禁止 DrawRectangle() 時用 Pen 畫邊框
	}
}
// ----------------------------------------------------------------------------
void SCD_Terminal::repaintLine(int _y)
{
	wxDC *dc = getDC();
	if(dc == NULL)	return;
	if( _y == -1 )	_y = cur_y;

	for(int x=0;x<col_count;x++)
	{
		if( term_data[_y][x].getCharType() != TerminalChar::CH_WORDLAST )
			repaintChar(x, _y);
	}
}
// ----------------------------------------------------------------------------
void SCD_Terminal::repaint(bool eraseBackground)
{	if( getVisible() )	getParentWindow()->Refresh( eraseBackground );	}
// ----------------------------------------------------------------------------
void SCD_Terminal::OnPaint(wxDC *dc)
{
	if( ! getVisible() )	return;

	wxRegionIterator upd( getParentWindow()->GetUpdateRegion() ); // get the update rect list
	if(!upd)	return;

	int left = 10000, right = 0, top = 10000, bottom = 0;
	while(upd)
	{
		int l = upd.GetX();
		int t = upd.GetY();
		int r = l + upd.GetW();
		int b = t + upd.GetH();

		if( l < left ) left = l;
		if( r > right ) right = r;
		if( t < top ) top = t;
		if( b > bottom ) bottom = b;

		upd++;
	}

	BeginDrawing(dc);
	repaint( dc , left, top, right - left, bottom - top );
	EndDrawing();
}
// ----------------------------------------------------------------------------
void SCD_Terminal::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc( getParentWindow() );
	OnPaint(&dc);
}
// ----------------------------------------------------------------------------
void SCD_Terminal::repaint(wxDC *dc, int x, int y, int w, int h)
{
	if( ! getVisible() )	return;

	int left, top, right, bottom;

	left = x;  top = y;
	MouseXY_to_TextXY( left, top, left, top );
	right = left+w;		bottom = top+h;

	for(int j=top;j<=bottom;j++)
		for(int i=left;i<=right;i++)
			repaintChar(i, j);
}
// ----------------------------------------------------------------------------
void SCD_Terminal::setBlinkCharVisibility(bool _v)
{
	if( _v == blBlinkCharVisibility )	return;
	blBlinkCharVisibility = _v;

	if( ! getHasBlinkChar() )	return;	//如果目前沒有閃爍字元, 則直接跳出

	wxClientDC dc( parentWindow );
	BeginDrawing(&dc);

	for(int y=0;y<row_count;y++)
		for(int x=0;x<col_count;x++)
		{
			if( term_data[y][x].getBlink() )
				repaintChar(x,y);
		}

	EndDrawing();
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_Terminal::OnMouseLeftDown(wxMouseEvent& event)
{
	switch( selectState )
	{
		case 2:
		case 3:
			CancelSelection();
		default:
			selectState = 1;
			MouseXY_to_TextXY( event.GetX() , event.GetY() , start_x, start_y );
			break;
	}
}
// ----------------------------------------------------------------------------
void SCD_Terminal::OnMouseMotion(wxMouseEvent& event)
{
	int _x, _y;

	MouseXY_to_TextXY( event.GetX() , event.GetY() , _x, _y );

	//如果滑鼠沒有移動到別的字元, 則跳出, 可減少運算量
	if( end_y == _y && end_x == _x )	return;

	if( event.LeftIsDown() )
	{
		switch( selectState )
		{
			case 1:
				selectState = 2;
				getParentWindow()->CaptureMouse();
				end_x = _x;		end_y = _y;
				break;
			case 2:
				{
					int b, e;
					if( end_y > _y )	{	b = _y;	e = end_y;	}
					else				{	b = end_y;	e = _y;	}

					end_x = _x;		end_y = _y;

					wxClientDC dc( getParentWindow() );
					BeginDrawing(&dc);
					for(int i=b;i<=e;i++)	repaintLine(i);
					EndDrawing();
				}


				//如果有捲軸而且游標快要超過顯示區上下界線時, 則自動捲動捲軸
				if( scrollwin )
				{
			int d = 0;

				if( event.GetY() < 0 )	d = -1;
				else if( event.GetY() > scrollwin->GetClientSize().GetHeight() )	d = 1;

				if( d != 0 )
				{
						int vx, vy;
				scrollwin->GetViewStart(&vx, &vy);
				vy +=d;
				if(vy >= 0)	scrollwin->Scroll(-1, vy);
					}
				}

				break;
		}
	}

	if( term_data[_y][_x].getLinkType() == LINK_NONE )
		parentWindow->SetCursor(wxNullCursor);
	else
		parentWindow->SetCursor( wxCursor(wxCURSOR_HAND) );
}
// ----------------------------------------------------------------------------
void SCD_Terminal::OnMouseLeftUp(wxMouseEvent& event)
{
	switch( selectState )
	{
		case 1:
			{
				if( ! OnLinkClickedFunc )	break;	//如果沒有定義 OnLinkClickedFunc 則離開

				int _x, _y;
				MouseXY_to_TextXY( event.GetX() , event.GetY() , _x, _y );

				//如果有連結字串的話, 則啟動連結
				if( term_data[_y][_x].getLinkType() != LINK_NONE )
				{
					char link[1000];
					int link_start_x;
					int i;

					//尋找連結位址的開頭
					for(i=_x;i>=0;i--)
					if( term_data[_y][i].getLinkType() == LINK_NONE )
						break;
				link_start_x = i + 1;


					//複製連結位址
					for(i=link_start_x;i<col_count;i++)
					{
					if( term_data[_y][i].getLinkType() != LINK_NONE )
						link[i-link_start_x] = term_data[_y][i].ch;
						else	break;
					}
					link[i-link_start_x] = '\0';

					//啟動連結
					if( term_data[_y][_x].getLinkType() != LINK_NONE )
						OnLinkClickedFunc( link , term_data[_y][_x].getLinkType() );
				}
			}
			selectState = 0;
			break;
		case 2:
		selectState = 3;
			getParentWindow()->ReleaseMouse();
		break;
	}
}
// ----------------------------------------------------------------------------
void SCD_Terminal::OnMouseLeftDoubleClick(wxMouseEvent& event)
{	//滑鼠左鍵雙點擊可自動選取整個英文字, 或中文句子

	int _x, _y;
	int s, e;
	MouseXY_to_TextXY( event.GetX() , event.GetY() , _x, _y );
	TerminalChar::CHAR_TYPE t = term_data[_y][_x].getCharType();

	if(t == TerminalChar::CH_CHAR)
	{
		if( term_data[_y][_x].ch != ' ' )
		{
			for(s=_x; s>=0 && term_data[_y][s].getCharType() == TerminalChar::CH_CHAR && term_data[_y][s].ch != ' ';s--);	s++;
			for(e=_x; e<col_count && term_data[_y][e].getCharType() == TerminalChar::CH_CHAR && term_data[_y][e].ch != ' ';e++);	e--;
			if(s>=e)	return;
		}
		else return;
	}
	else
	{
		for(s=_x; s>=0 && term_data[_y][s].getCharType() != TerminalChar::CH_CHAR ;s--);	s++;
		for(e=_x; e<col_count && term_data[_y][e].getCharType() != TerminalChar::CH_CHAR ;e++);	e--;
		if(s>=e)	return;
	}

	start_x = s;	start_y = _y;
	end_x = e;	end_y = _y;
	selectState = 3;

	//重繪此行
	wxClientDC dc( getParentWindow() );
	BeginDrawing(&dc);
	repaintLine(_y);
	EndDrawing();
}
// ----------------------------------------------------------------------------
void SCD_Terminal::SelectAll()
{
	start_x = 0;	start_y = 0;
	end_x = col_count - 1;	end_y = row_count - 1;
	selectState = 3;

	wxClientDC dc( getParentWindow() );
	repaint();
}
// ----------------------------------------------------------------------------
void SCD_Terminal::CancelSelection()	//取消選取
{
	switch( selectState )
	{
		case 1:
		case 2:
		case 3:
		{
			int b, e;
			if( start_y < end_y )	{	b = start_y;	e = end_y;	}
			else					{	b = end_y;	e = start_y;	}

			selectState = 0;

			wxClientDC dc( getParentWindow() );
			BeginDrawing(&dc);
			for(int i=b;i<=e;i++)	repaintLine(i);
			EndDrawing();

			break;
		}
	}
}
// ----------------------------------------------------------------------------
wxString SCD_Terminal::GetAllContent(bool withANSI)			//取得所有文字
{
    int sx, sy, ex, ey, ss;
    ss = selectState;

    selectState = 3;
    sx = start_x;	sy = start_y;
    ex = end_x;		ey = end_y;
    start_x = 0;	start_y = 0;
    end_x = col_count - 1;		end_y = row_count - 1;

    wxString ret = GetSelectionContent(withANSI);

    selectState = ss;
    start_x = sx;	start_y = sy;
    end_x = ex;		end_y = ey;

    return ret;
}
wxString SCD_Terminal::GetSelectionContent(bool withANSI)	//取得所選取的文字
{
	if( selectState != 3 )	return wxEmptyString;


	//將選取的文字複製到緩衝區
	int s = start_y * col_count + start_x;
	int e = end_y * col_count + end_x;
	if( s > e )
	{
		int i;
		i = start_x;	start_x = end_x;	end_x = i;
		i = start_y;	start_y = end_y;	end_y = i;
	}

	//選取的字串的開頭和結尾若是只選到全型字的其中一半, 則也複製另外一半
	if( term_data[start_y][start_x].getCharType() == TerminalChar::CH_WORDLAST )
		start_x = start_x - 1;
	if( term_data[end_y][end_x].getCharType() == TerminalChar::CH_WORDFIRST )
		end_x = end_x + 1;

	int _x , _y;
	_y = start_y;
	_x = start_x;

	TerminalChar last_char_prop = TerminalChar::getDefaultCharProperty();

	std::string raw;
	if( withANSI )		raw.append("\x1b[m");
	while(1)
	{

		if( _y > end_y || (_y == end_y && _x > end_x) )	break;	// check boundary

		if( _x == 0 && _y > start_y )
		{
			raw.append("\r\n");
			if( withANSI && _y < row_count)
			{
				wxString ansi = term_data[_y][_x].getANSICode();
				wxCharBuffer ansi_buf = ansi.mb_str(wxConvUTF8);
				if( ansi_buf.data() ) raw.append(ansi_buf.data());
			}
		}

		if( _x == col_count || term_data[_y][_x].ch == '\0' )
		{	_x = 0;	_y ++;	continue;	}

		if( withANSI )
		{
			if( ! last_char_prop.withSameProperty( term_data[_y][_x] ) )
			{
				wxString ansi = term_data[_y][_x].getDiffANSICode( last_char_prop );
				wxCharBuffer ansi_buf = ansi.mb_str(wxConvUTF8);
				if( ansi_buf.data() ) raw.append(ansi_buf.data());
				last_char_prop = term_data[_y][_x];
			}
		}

		raw.push_back( term_data[_y][_x].ch );
		_x++;

	}
	if( withANSI )		raw.append("\x1b[m");

	return Big5BytesToWxString(raw.data(), raw.size());
}

#ifdef __WXGTK__
#include <iconv.h>
char* Big5ToUnicode(char *big5)
{
    static iconv_t cv;
    static bool bl_cv_inited = false;
    static char *inptr;
    static char text[1000], *outptr;
    size_t insize, outsize;
    if(!bl_cv_inited)
    {
        cv = iconv_open("BIG5" , "Unicode");
        bl_cv_inited = true;
    }
    inptr = (char*)big5;
wxMessageBox(inptr);
    insize = 1000;
    outptr = text;
    outsize = 1000;
#ifdef __LINUX__
    iconv( cv, &inptr, &insize, &outptr, &outsize );
#else
    iconv( cv, (const char**)&inptr, &insize, &outptr, &outsize );
#endif
wxMessageBox(text);
	return strdup(text);
}
#endif

#include <wx/encconv.h>
#include "scd_gtk_textctrl.h"
#include <wx/textctrl.h>
void SCD_Terminal::CopySelectionToClipboard(bool withANSI)	//將選取的文字複製到剪貼簿
{
	if( selectState != 3 )	return;
	CopyToClipboard( GetSelectionContent(withANSI) );
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_Terminal::OpenSelectionAsHyperlink()
{
	if( selectState != 3 )	return;
	int sx, sy, ex, ey;
	getSelectionInfo( &sx, &sy, &ex, &ey );
	int s = sy * col_count + sx;
	int e = ey * col_count + ex;
	char *link = new char[e-s+1];
	int i = 0;
	for(int y=sy;y<=ey;y++)
	{
		int line_len = getLineLength(y);
		bool b = false;
		for(int x=0;x<line_len;x++)
		{
			if( y==sy && x<sx )	continue;
			if( y==ey && x>ex )	continue;
			char ch = term_data[y][x].ch;
			if(!b)
			{
				if(ch == ' ')	continue;	//忽略每一行開頭的所有空白字元
				else b = true;
			}
			link[i++] = ch;
		}
	}
	link[i] = '\0';
	OpenHyperlink(link);
	delete[] link;
}
// ----------------------------------------------------------------------------
inline bool isEnglishChar(char c)
{	return ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' );	}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_Terminal::SetLineWrapedLength(int len)	//設定自動斷行的長度
{
	if( len < 3 || len > 80 )	return;
	line_wraped_length = len;
}
// ----------------------------------------------------------------------------
int SCD_Terminal::GetLineWrapedLength()
{	return line_wraped_length;	}
// ----------------------------------------------------------------------------
wxString SCD_Terminal::GetLineWrapedString(const wxString& text)
{
	wxCharBuffer big5_buf = wxStringToBig5Buffer(text);
	if( ! big5_buf.data() )	return wxEmptyString;
	return GetLineWrapedString( const_cast<char*>(big5_buf.data()) );
}
// ----------------------------------------------------------------------------
wxString SCD_Terminal::GetLineWrapedString(char *txt)	//將 txt 做自動斷行, 使每一行的字數不會超過指定的長度
{
	if( line_wraped_length <= 0 )	return Big5BytesToWxString(txt, strlen(txt));

	wxString output = wxEmptyString;
	char *r, *line_start;
	int now_line_len;
	line_start = r = txt;

	getXY( & now_line_len , NULL );

	bool commit = false;	//是否要換行
	while( *r != '\0' )
	{

		if( *r == '\x1b' && r[1] == '[' )	//跳過控制碼
		{
//wxMessageBox("cotrol code");
			r++;
			while(1)
			{
				r++;
				if( isalpha(*r) )	{	r++;	break;	}
				else if( *r == '\0' )	break;
			}
		}
		else
		{
			if( *r == '\r' )
			{	r++;	continue;	}
			else if( *r == '\n' )	//換行
			{	commit = true;	}
			else
			{
				if( isWord( r[0], r[1] ) && r[1] != '\0' )	//如果是中文字
				{
					if( now_line_len >= line_wraped_length - 1 )	//如果該中文字剛好在換行邊界
					{	commit = true;	}
					else
					{	r+=2;	now_line_len += 2;	}
				}
				else
				{
					if( now_line_len >= line_wraped_length )	//如果該換行了
					{	commit = true;	}
					else
					{	r++;	now_line_len ++;	}
				}
			}

			//commit 時, line_start 放在此行第一個字, r 放在最後一個字之後
			if( commit )	//如果目前此行長度超過換行長度, 則換行
			{
				bool newline = false;
				if(*r=='\n')	{	r++;	newline = true;	}
				else
				{
					if( now_line_len > 0 && isEnglishChar(*(r-1)) && isEnglishChar(*r) )	//如果有英文字被斷字
					{
						int c = now_line_len;
						char *p = r;
						for( ; c > 0 && isEnglishChar(*p) ; c-- , p-- );
						if( c > 0 )	{	now_line_len = c;	r = p;	}

						for( ; c > 0 && *p == ' ' ; c-- , p-- );
						now_line_len = c + 1;	r = p + 1;
					}
				}

				output += Big5BytesToWxString( line_start , r - line_start );
				if( ! newline )	output += wxString( _T("\r\n") , 2 );
				commit = false;

//    			for( ; *r == ' ' ; r ++ );	//跳過下一行的開頭空白
				line_start = r;
				now_line_len = 0;
			}
		}
	}

	if( now_line_len > 0 )	output += Big5BytesToWxString( line_start , strlen(line_start) );
	return output;
}
// ============================================================================
#endif
