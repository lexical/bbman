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


#ifndef SCD_EDITTERM_CPP
#define SCD_EDITTERM_CPP

#include "scd_terminal.h"
#include "scd_editterm.h"

// ============================================================================

SCD_EditTerm::SCD_EditTerm(wxWindow *win) : SCD_Terminal(win)
{
    blChanged = false;
    getColumnRowCount(NULL , & line_capacity );
}
SCD_EditTerm::~SCD_EditTerm()
{
}    
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool SCD_EditTerm::set_line_capacity(int _dc) //設定最大可容納的行數
{
    int row_count;
	getColumnRowCount(NULL , & row_count );
	if( _dc < row_count )	return false;

	TerminalChar **tmp_lines = term_data;
	term_data = new (TerminalChar*)[_dc];
	if(term_data==NULL)
	{
		term_data = tmp_lines;
		return false;
	}

	for(int i=0;i<row_count;i++) term_data[i] = tmp_lines[i];
	delete tmp_lines;

	return true;
}
// ----------------------------------------------------------------------------
bool SCD_EditTerm::ensure_line_capacity(int _lc)	//確保有 _lc 行可以使用
{
	if( _lc <= line_capacity )	return true;
	else return set_line_capacity( _lc + 100 );
}
// ----------------------------------------------------------------------------
TerminalChar* SCD_EditTerm::AllocNewLine()	//配置一行所需的記憶體空間
{
    int col_count;
	getColumnRowCount( &col_count , NULL );

	TerminalChar *line = new TerminalChar[ col_count ];

	TerminalChar dch = TerminalChar::getDefaultCharProperty();
	for(int i=0;i<col_count;i++)	line[i] = dch;

	return line;
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::abs_AddLines(int line_index , int line_count )
{
    if( line_index == -1 )	getXY(NULL, &line_index);
    int col_count, row_count;
	getColumnRowCount(&col_count , & row_count );

	ensure_line_capacity(row_count + line_count);

	int move_count = row_count - line_index;
	for(int c=0 , s=row_count-1 , d=s+line_count ; c<move_count ; c++ , s--, d--)
		term_data[d] = term_data[s];	//搬移行
	for(int i=0;i<line_count;i++)	//新增行
		term_data[i+line_index] = AllocNewLine();
		
	updateScrollBar(row_count + line_count);
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::abs_DeleteLines(int line_index , int line_count )
{
    if( line_index == -1 )	getXY(NULL, &line_index);
    int col_count, row_count;
	getColumnRowCount(&col_count , & row_count );

 	for(int i=0;i<line_count;i++)	delete term_data[i + line_index];	//刪除行資料
 	for(int d=line_index , s=line_index+line_count ; s < row_count ; s++ , d++ )
 		term_data[d] = term_data[s];	//搬移行資料

	ensure_line_capacity(row_count - line_count);
	updateScrollBar(row_count - line_count);
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::abs_CleanAll()
{
    int row_count;
    getColumnRowCount(NULL , &row_count);
    gotoXY(0,0);
    abs_AddLines(0, 1);
    abs_DeleteLines(1, row_count);
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_EditTerm::keyEnter()
{
   	int x, y;
   	getXY(&x, &y);

   	int col_count, row_count;
   	getColumnRowCount(&col_count, &row_count);

   	if( term_data[y][x].getCharType() == TerminalChar::CH_WORDLAST )
   		x--;	//若游標在全形字的中間, 則左移一格

   	int move_len = getLineLength() - x;
   	if(move_len < 0)	move_len = 0;


	abs_AddLines( y + 1 );	//新增一行, 插入在下一行

	TerminalChar dch = TerminalChar::getDefaultCharProperty();

	for(int i=0;i<move_len;i++)
	{
		term_data[y+1][i] = term_data[y][x+i];
		term_data[y][x+i] = dch;
	}

   	goDown();
   	goLineHead();
	repaint();

	blChanged = true;
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::JoinLine(int _line)	//和下一行合併
{
	if( _line == -1 )	getXY( NULL , &_line );

   	int col_count, row_count;
   	getColumnRowCount(&col_count, &row_count);
   	if( _line + 1 >= row_count )	return;

   	int now_line_len = getLineLength(_line);
   	if( now_line_len == col_count )	return;
   	int next_line_len = getLineLength(_line+1);

	int move_len = col_count - now_line_len;
	if( move_len > next_line_len )	move_len = next_line_len;

	if( term_data[_line+1][move_len-1].getCharType() == TerminalChar::CH_WORDFIRST )
		move_len --;

	for(int i=0;i<move_len;i++)	//把下一行的前面的字串複製到這行的行尾, 直到這行滿了或是下行沒東西可複製了為止
		term_data[_line][i+now_line_len] = term_data[_line+1][i];

	for(int i=move_len;i<next_line_len;i++)	//把下行行尾的字串往前搬
		term_data[_line+1][i-move_len] = term_data[_line+1][i];

	TerminalChar dch = TerminalChar::getDefaultCharProperty();
	for(int i=1;i<=move_len;i++)	//清除下行行尾
		term_data[_line+1][next_line_len-i] = dch;

	if( move_len > 0 && move_len < next_line_len )	//如果下行還有剩餘資料, 則重繪這行和下行
	{
		wxClientDC dc( getParentWindow() );
		BeginDrawing(&dc);
		repaintLine(_line);
		repaintLine(_line+1);
 		EndDrawing();
	}
	else	//否則, 刪除下行, 全部重繪
	{
		abs_DeleteLines( _line + 1 );
		repaint();
	}

	blChanged = true;
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::keyBackspace()
{
	int x, y;
	getXY(&x, &y);
	if( x == 0 )	//如果游標在行首
	{
       	if( y == 0 )	return;	//如果在第一行就跳出
       	goUp();
       	keyEnd();
       	JoinLine();
	}
	else
	{
		if( isLeftAWord() )	goLeft();
	    goLeft();
    	keyDelete();
	}

	blChanged = true;
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::keyDelete()
{
	//如果目前游標處於全型字的中間則讓游標左移一格
	int x, y;
	getXY(&x, &y);

	if( x >= getLineLength() )	//如果游標在行尾, 則合併下一行
	{
   		JoinLine();
   		return;
	}


	if( term_data[y][x].getCharType() == TerminalChar::CH_WORDLAST )
	{	goLeft();	x--;	}	//若游標在全形字的中間, 則左移一格

	int col_count;
	getColumnRowCount(&col_count , NULL);


	TerminalChar dch = TerminalChar::getDefaultCharProperty();
	if( isCurrentAWord() )	//若游標目前在中文字開頭, 則一次刪除兩個字元
	{
 		for(int i=x+2;i<col_count;i++)
	 		term_data[y][i-2] = term_data[y][i];
		term_data[y][col_count-1] = dch;
		term_data[y][col_count-2] = dch;
	}
	else    //否則, 一次只刪除一個字元
	{
		for(int i=x+1;i<col_count;i++)
	 		term_data[y][i-1] = term_data[y][i];
		term_data[y][col_count-1] = dch;
	}

	adjustLineCharInfo();	//SCD_Terminal::adjustLineCharInfo() 可標示被改變的這行的全型字半型字
	blChanged = true;

	//重繪
	wxClientDC dc( getParentWindow() );
	BeginDrawing(&dc);
	repaintLine();
	EndDrawing();
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::keyEnd()
{
	int x, y;
	getXY(&x, &y);
	x = getLineLength();
	int col_count;
	getColumnRowCount(&col_count, NULL);
	if( x >= col_count )	x = col_count - 1;
	gotoXY( x , y );
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::keyDown()
{    goDown();	}
// ----------------------------------------------------------------------------
void SCD_EditTerm::keyPageUp()
{
   	int x, y;
   	getXY(&x, &y);
	int line_count_in_view = getLineCountInView();

   	y = y - (line_count_in_view - 1);
	if(y < 0)	y = 0;

   	gotoXY(x,y);
   	ScrollToCaret();
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::keyPageDown()
{
   	int x, y;
   	getXY(&x, &y);
	int line_count_in_view = getLineCountInView();

	int row_count;
	getColumnRowCount( NULL, &row_count );

	y = y + line_count_in_view - 1;
	if( y >= row_count - 1 )	y = row_count - 1;

   	gotoXY(x, y);
   	ScrollToCaret();
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::DeleteLineTail()
{
	//如果目前游標處於全型字的中間則讓游標左移一格
	int x, y;
	getXY(&x, &y);
	if( term_data[y][x].getCharType() == TerminalChar::CH_WORDLAST )
		goLeft();

	CleanLineTail();

	wxClientDC dc( getParentWindow() );
	BeginDrawing(&dc);
	repaintLine();
	EndDrawing();

	blChanged = true;
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::DeleteLine()
{
   	int y , row_count;
   	getXY(NULL, &y);
   	getColumnRowCount(NULL , &row_count);
	if( y == row_count - 1 )	return;	//如果在行尾, 則跳出

	abs_DeleteLines();
	repaint();

	blChanged = true;
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::setSelectionProperty(int type, bool b, int color)
{
	int sx, sy, ex, ey;
	getSelectionInfo( &sx, &sy, &ex, &ey );

	TerminalChar default_tch = TerminalChar::getDefaultCharProperty();
	char ch;

	int col_count;
	getColumnRowCount(&col_count, NULL);


	wxClientDC dc( getParentWindow() );
	BeginDrawing(&dc);


    for(int i=sy;i<=ey;i++)
	{
		for(int j=0;j<col_count;j++)
		{
		    if( i == sy && j < sx )	continue;
		    if( i == ey && j > ex )	continue;

			switch( type )
			{
			    case 0 :
           		{
			        TerminalChar & tch = term_data[i][j];
			        ch = tch.ch;
			        default_tch.setCharType( tch.getCharType() );
			        tch = default_tch;
			        tch.ch = ch;
           			break;
				}
			    case 1 : term_data[i][j].setBlink(b);	break;
			    case 2 : term_data[i][j].setHighlight(b);	break;
			    case 3 : term_data[i][j].setUnderline(b);	break;
			    case 4 : term_data[i][j].setTextColor(color);	break;
			    case 5 : term_data[i][j].setBgColor(color);	break;
			}
		}
 		repaintLine(i);
	}

	if( type == 1 )	setHasBlinkChar(true);
	EndDrawing();
	blChanged = true;
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::PasteFromClipboard(bool withANSI)
{
	wxString text = GetTextFromClipboard();
	if( text.IsEmpty() )	return;
	else Paste( wxStringToCharPtr(text) , withANSI );
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::Paste(char *txt, bool withANSI)
{
	txt = strdup( wxStringToCharPtr( GetLineWrapedString(txt) ) );

	//計算欲貼上的文字總共有幾行
	char *p = txt;
	int newline_count = 0;
	while( *p != '\0' )
	{
		if( *p == '\n' )	newline_count ++;
		p ++;
	}

	int col_count;
	getColumnRowCount( &col_count, NULL );

	int bx, by;	//貼上前的游標座標
	getXY(&bx , &by);
	abs_AddLines( by+1 , newline_count );	//新增行數

	if( term_data[by][bx].getCharType() == TerminalChar::CH_WORDLAST )
	{	goLeft();	bx --;	}	//若游標在全形字中間, 則左移一格

	int b_line_len = getLineLength();
	int buf_len = b_line_len - bx;
	TerminalChar *buf = NULL;
	if( buf_len > 0 )
	{
		buf = new TerminalChar[ buf_len ];
		for(int i=0;i<buf_len;i++)	buf[i] = term_data[by][bx+i];	//複製貼上前的行尾
	}

	DeleteLineTail();	//刪除貼上前的行尾

   	EnableDrawing(false);
	parse(txt);	//貼上文字
	EnableDrawing(true);

	if( buf_len > 0 )
	{
		int ax, ay;	//貼上後的游標座標
		getXY(&ax , &ay);

		//把 [貼上前的行尾文字] 複製到 [貼上後的行尾]
		if( ax + buf_len <= col_count );
		else
		{
			abs_AddLines( ay+1 );
   			ax = 0;	ay ++;
		}
		for(int i=0;i<buf_len;i++)	term_data[ay][ax+i] = buf[i];
	}

	if(buf)	delete buf;
	free(txt);

	repaint();

	blChanged = true;
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::OnAsciiChar(char ch)
{
	int x, y;
	getXY(&x, &y);

	int col_count;
   	getColumnRowCount(&col_count, NULL);

	for(int i=col_count-1;i>x;i--)	term_data[y][i] = term_data[y][i-1];
	if(x > 0)	term_data[y][x] = term_data[y][x-1];
    term_data[y][x].ch = ch;
	adjustLineCharInfo();

	wxClientDC dc( getParentWindow() );
	BeginDrawing(&dc);
	repaintLine();
	EndDrawing();

	goRight();
	blChanged = true;
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::OnMouseLeftDown(wxMouseEvent& event)
{
    SCD_Terminal::OnMouseLeftDown(event);

    int x, y;
    MouseXY_to_TextXY( event.GetX(), event.GetY(), x, y);
    gotoXY(x,y);
    if( term_data[y][x].getCharType() == TerminalChar::CH_WORDLAST )	goLeft();	//確保由標不會停在全形字的第二個 byte
}
// ----------------------------------------------------------------------------
void SCD_EditTerm::OnMouseMotion(wxMouseEvent& event)
{
    SCD_Terminal::OnMouseMotion(event);

    if( selectState == 2 )	//正在選取文字中
	{
	    int x, y;
	    MouseXY_to_TextXY( event.GetX(), event.GetY(), x, y);
    	gotoXY(x,y);
	    if( term_data[y][x].getCharType() == TerminalChar::CH_WORDLAST )	goLeft();	//確保由標不會停在全形字的第二個 byte
	}
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SCD_EditTerm::OnChar(wxKeyEvent& event)
{
	int key = event.GetKeyCode();
//	wxMessageBox( wxString::Format("%d", key) );
	OnAsciiChar( (char)key );
}    
// ----------------------------------------------------------------------------
void SCD_EditTerm::OnKeyDown(wxKeyEvent& event)
{
	int key = event.GetKeyCode();

	if( key == WXK_DELETE )	keyDelete();
//	else if( key == WXK_RETURN || )	keyEnter();	//[Enter]
	else if( key == WXK_BACK )	keyBackspace();	//back space
	else if( key == 395 )	OnAsciiChar('.');	//鍵盤右邊, 數字鍵附近的 [小數點] 鍵
//	else if( isascii(key) )		event.Skip();	//交給 OnChar event 處理
	else if( key < 0x80 && ! iscntrl(key) )	event.Skip();	//交給 OnChar event 處理
	else
	{
		if( key >= 326 && key <= 335 )	//如果是鍵盤右邊的數字鍵
		{
  			char ch = (char)(key - 326) + '0';
			parse( (char*)&ch , 1 );
		}
		else
		{
			switch(key)
			{
				case 372 :	//for NumPad [Enter] in Linux/FreeBSD
				case WXK_RETURN :	keyEnter();	break;
/*
				case WXK_BACK :	output = "";	break;
				case WXK_TAB :	output = "";	break;
				case WXK_ESCAPE :	output = "";	break;
				case WXK_DELETE :	output = "\x1b\x5b\x33\x7e";	break;
*/
				case WXK_HOME :		goLineHead();	break;
				case WXK_END :		keyEnd();	break;
				case WXK_INSERT :	break;
				case WXK_UP :		goUp();		break;
				case WXK_DOWN :		keyDown();	break;
				case WXK_LEFT :		
           			if( isLeftAWord() )	goLeft();
           			goLeft();
              		break;
				case WXK_RIGHT :	
    		    	if( isCurrentAWord() )	goRight();
              		goRight();
                	break;
				case 312 :	keyPageUp();	break;	//page up
				case 313 :	keyPageDown();	break;	//page down

				default :
//wxMessageBox(wxString::Format("%d", key) );
					if( key < 0xff )	event.Skip();	//for 雙位元字
					break;
			}
		}
	}

}

// ============================================================================
#endif
