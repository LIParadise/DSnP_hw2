/****************************************************************************
  FileName     [ cmdReader.cpp ]
  PackageName  [ cmd ]
  Synopsis     [ Define command line reader member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2007-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/
#include <cassert>
#include <cstring>
#include <cctype>        // isspace()
#include "cmdParser.h"

using namespace std;

//----------------------------------------------------------------------
//    Extrenal funcitons
//----------------------------------------------------------------------
void mybeep();
char mygetc(istream &);
ParseChar getChar(istream &);

//----------------------------------------------------------------------
//    Member Function for class Parser
//----------------------------------------------------------------------
void CmdParser::readCmd()
{
  if (_dofile.is_open())
  {
    readCmdInt(_dofile);
    _dofile.close();
  }
  else
    readCmdInt(cin);
}

void CmdParser::readCmdInt(istream &istr)
{
  resetBufAndPrintPrompt();

  while (1)
  {
    ParseChar pch = getChar(istr);
    if (pch == INPUT_END_KEY)
      break;
    switch (pch)
    {
    case LINE_BEGIN_KEY:
    case HOME_KEY:
      moveBufPtr(_readBuf);
      break;
    case LINE_END_KEY:
    case END_KEY:
      moveBufPtr(_readBufEnd);
      break;
    case BACK_SPACE_KEY: /* TODO */ //...done
      {
        if( _readBufPtr == _readBuf ){
          mybeep();
        }else{
          moveBufPtr( _readBufPtr -1 );
          deleteChar();
        }
      }
      break;
    case DELETE_KEY:
      deleteChar();
      break;
    case NEWLINE_KEY:
      addHistory();
      cout << char(NEWLINE_KEY);
      resetBufAndPrintPrompt();
      break;
    case ARROW_UP_KEY:
      moveToHistory(_historyIdx - 1);
      break;
    case ARROW_DOWN_KEY:
      moveToHistory(_historyIdx + 1);
      break;
    case ARROW_RIGHT_KEY: /* TODO */ // ...done
      moveBufPtr( _readBufPtr + 1 );
      break;
    case ARROW_LEFT_KEY: /* TODO */ // ...done
      moveBufPtr( _readBufPtr - 1 );
      break;
    case PG_UP_KEY:
      moveToHistory(_historyIdx - PG_OFFSET);
      break;
    case PG_DOWN_KEY:
      moveToHistory(_historyIdx + PG_OFFSET);
      break;
    case TAB_KEY: /* TODO */ // ...done
      // insert spaces till reach ( n*TAB_POSITION ), where n in natural number.
      // if it's now at n*TAB_POSITION, insert TAB_POSITION spaces,
      // so that we have (n+1)*TAB_POSITION characters.
      {
        int diff = ( _readBufPtr - _readBuf );
        if( diff % TAB_POSITION ){
          // insert till reach interger times of TAB_POSITION;
          diff = ( TAB_POSITION - (diff%TAB_POSITION) );
          insertChar( ' ', diff );
        }else{
          insertChar( ' ', TAB_POSITION );
        }
      }
      break;
    case INSERT_KEY: // not yet supported; fall through to UNDEFINE
    case UNDEFINED_KEY:
      mybeep();
      break;
    default: // printable character
      insertChar(char(pch));
      break;
    }
#ifdef TA_KB_SETTING
    taTestOnly();
#endif
  }
}

// This function moves _readBufPtr to the "ptr" pointer
// It is used by left/right arrowkeys, home/end, etc.
//
// Suggested steps:
// 1. Make sure ptr is within [_readBuf, _readBufEnd].
//    If not, make a beep sound and return false. (DON'T MOVE)
// 2. Move the cursor to the left or right, depending on ptr
// 3. Update _readBufPtr accordingly. The content of the _readBuf[] will
//    not be changed
//
// [Note] This function can also be called by other member functions below
//        to move the _readBufPtr to proper position.
bool CmdParser::moveBufPtr(char *const ptr)
{
  // TODO... done;
  if (ptr < _readBuf || ptr > _readBufEnd)
  {
    mybeep();
    return false;
  }

  if (ptr < _readBufPtr)
  {
    // use (char)8 to move backwards;
    while( _readBufPtr != ptr ){
      cout << (char)8;
      _readBufPtr --;
    }
  }
  else if (ptr > _readBufPtr)
  {
    // use information in our _readBuf[];
    while (_readBufPtr < ptr)
    {
      cout << *_readBufPtr;
      _readBufPtr++;
    }
  }
  return true;
}

// [Notes]
// 1. Delete the char at _readBufPtr
// 2. mybeep() and return false if at _readBufEnd
// 3. Move the remaining string left for one character
// 4. The cursor should stay at the same position
// 5. Remember to update _readBufEnd accordingly.
// 6. Don't leave the tailing character.
// 7. Call "moveBufPtr(...)" if needed.
//
// For example,
//
// cmd> This is the command
//              ^                (^ is the cursor position)
//
// After calling deleteChar()---
//
// cmd> This is he command
//              ^
//
bool CmdParser::deleteChar()
{
  // TODO... done;
  if( _readBufPtr == _readBufEnd ){
    mybeep();
    return false;
  }else{
    memmove( _readBufPtr, _readBufPtr+1, 
        sizeof(char) * (_readBufEnd - _readBufPtr ) );
    // move memory from (next to end) to (here to (end-1));
    _readBufEnd --;
    *_readBufEnd = 0;
    // update _readBufEnd position, reset its data;

    // update screen information.
    for( char* ptr = _readBufPtr; ptr < _readBufEnd; ptr ++){
      cout << *ptr;
    }
    cout << ' ';
    cout << '\b';
    for( char* ptr = _readBufPtr; ptr < _readBufEnd; ptr ++){
      cout << (char)8;
    }
  }
  return true;
}

// 1. Insert character 'ch' for "repeat" times at _readBufPtr
// 2. Move the remaining string right for "repeat" characters
// 3. The cursor should move right for "repeats" positions afterwards
// 4. Default value for "repeat" is 1. You should assert that (repeat >= 1).
//
// For example,
//
// cmd> This is the command
//              ^                (^ is the cursor position)
//
// After calling insertChar('k', 3) ---
//
// cmd> This is kkkthe command
//                 ^
//
void CmdParser::insertChar(char ch, int repeat)
{
  // TODO... done
  assert(repeat >= 1);

  // If (repeat) characters would cause buffer to explode,
  // reduce repeat so that they could fit in READ_BUF_SIZE as a cstring,
  // and then beep().
  // We'll update screen first, and then deal with buffer.

  // confine (repeat) size.
  int remaining = READ_BUF_SIZE - ( _readBufEnd - _readBuf ) - 1;
  if( repeat > remaining ){
    mybeep();
    repeat = remaining;
    if( remaining == 0 ){
      return ;
    }
  }

  // update screen.
  for( int i = 0; i < repeat; i++ ){
    cout << ch;
  }
  for( char* ptr = _readBufPtr; ptr < _readBufEnd; ptr ++ ){
    cout << *ptr;
  }
  for( char* ptr = _readBufPtr; ptr < _readBufEnd; ptr ++ ){
    cout << '\b';
  }

  // maintain buffer.
  memmove( _readBufPtr+repeat, _readBufPtr, 
      sizeof(char) * ( _readBufEnd - _readBufPtr ) );
  for( int i = 0; i < repeat; i++ ){
    *_readBufPtr = ch;
    _readBufPtr ++;
  }
  _readBufEnd = _readBufEnd + repeat;
  *_readBufEnd = 0;
  assert( (_readBufEnd - _readBuf ) < (READ_BUF_SIZE) );

}

// 1. Delete the line that is currently shown on the screen
// 2. Reset _readBufPtr and _readBufEnd to _readBuf
// 3. Make sure *_readBufEnd = 0
//
// For example,
//
// cmd> This is the command
//              ^                (^ is the cursor position)
//
// After calling deleteLine() ---
//
// cmd>
//      ^
//
void CmdParser::deleteLine()
{
  // TODO... done
  moveBufPtr( _readBuf );
  for( int i = 0; i <= ( _readBufEnd - _readBuf ); i++ ){
    cout << ' ';
  }
  for( int i = 0; i <= ( _readBufEnd - _readBuf ); i++ ){
    cout << (char)8 ; 
  }
  _readBufEnd = _readBuf;
  _readBufPtr = _readBuf;
  memset( _readBuf, 0, sizeof(char) * READ_BUF_SIZE );
  
}

// This functions moves _historyIdx to index and display _history[index]
// on the screen.
//
// Need to consider:
// If moving up... (i.e. index < _historyIdx)
// 1. If already at top (i.e. _historyIdx == 0), beep and do nothing.
// 2. If at bottom, temporarily record _readBuf to history.
//    (Do not remove spaces, and set _tempCmdStored to "true")
// 3. If index < 0, let index = 0.
//
// If moving down... (i.e. index > _historyIdx)
// 1. If already at bottom, beep and do nothing
// 2. If index >= _history.size(), let index = _history.size() - 1.
//
// Assign _historyIdx to index at the end.
//
// [Note] index should not = _historyIdx
//
void CmdParser::moveToHistory(int index)
{
  // TODO... shall be done?
  if( !_tempCmdStored ){

    assert( _historyIdx == _history.size () );

    if( index > _historyIdx ){
      mybeep();
      return;
    }else if( _historyIdx == 0 && index < 0 ){
      mybeep();
      return;
    }else{
      // index inside good range;
      if( index < 0 )
        index = 0;
      _tempCmdStored = true;
      _history.push_back( string( _readBuf ) );
      _historyIdx = index;
      retrieveHistory();
    }

  }else{ // _tempCmdStored == true;

    assert( _history.size() > 1 );
    // there must exist "genuine history" and "temp history"
    // if there's no genuine history, we won't need the temp one.
    assert( _historyIdx < _history.size()-1 );
    // there shall be temporary string at _history[ size-1 ];
    // when we want to retrieve it, just pop it and set _tempCmdStored
    // to false, reset _historyIdx to _history.size();

    if( index >= _history.size()-1 ){
      // retrieve temp, pop it from _history;
      assert( _historyIdx < _history.size()-1 );
      index = _history.size() - 1 ;
      _historyIdx = index;
      retrieveHistory();
      _tempCmdStored = false;
      _history.pop_back();
    }else if( _historyIdx == 0 && index < 0 ){
      mybeep();
      return;
    }else{
      // index in range
      if( index < 0 ){
        index = 0;
      }
      _historyIdx = index;
      retrieveHistory();
    }

  } // if( !_tempCmdStored ){} else {}

}


// This function adds the string in _readBuf to the _history.
// The size of _history may or may not change. Depending on whether
// there is a temp history string.
//
// 1. Remove ' ' at the beginning and end of _readBuf
// 2. If not a null string, add string to _history.
//    Be sure you are adding to the right entry of _history.
// 3. If it is a null string, don't add anything to _history.
// 4. Make sure to clean up "temp recorded string" (added earlier by up/pgUp,
//    and reset _tempCmdStored to false
// 5. Reset _historyIdx to _history.size() // for future insertion
//
void CmdParser::addHistory()
{
  // TODO... done

  char* ptr_start = nullptr;
  char* ptr_back = nullptr;

  for( ptr_back = _readBufEnd-1; isspace(*ptr_back) && ( ptr_back >= _readBuf );
      ptr_back -- ) {}
  if( ptr_back < _readBuf ){
    return ;
  }
  for( ptr_start = _readBuf; isspace(*ptr_start) && ( ptr_start < _readBufEnd );
      ptr_start ++ ){}
  if( ptr_start >= _readBufEnd ){
    return;
  }
  if( ptr_back < ptr_start ){
    return ;
  }

  if( _tempCmdStored ){
    // discard last element in _history, using what in buffer instead.
    _history.pop_back();
  }
  _history.push_back( string( ptr_start, ptr_back - ptr_start + 1) );
  // (*(ptr_back)+1) shall be the 0 preceding a cstring;

  _historyIdx = _history.size();
  _tempCmdStored = false;

}

// 1. Replace current line with _history[_historyIdx] on the screen
// 2. Set _readBufPtr and _readBufEnd to end of line
//
// [Note] Do not change _history.size().
//
void CmdParser::retrieveHistory()
{
  deleteLine();
  strcpy(_readBuf, _history[_historyIdx].c_str());
  cout << _readBuf;
  _readBufPtr = _readBufEnd = _readBuf + _history[_historyIdx].size();
}
