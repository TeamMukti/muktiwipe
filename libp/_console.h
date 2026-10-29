/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Console interaction - cursor position and line updates.
 */
#ifndef _LIBP_CONSOLE_H_
#define _LIBP_CONSOLE_H_

#include "types.h"

//
// returns true if the standard output is attached to a console
// (i.e. the cursor operations below can be used)
bool is_interactive_console();

// hides/shows the console cursor, the previous state is restored
// automatically when the program exits
void show_console_cursor(bool show);

// moves the console cursor
//  x,y - the new coordinates
//  x_relative, y_relative - true for coordinates relative to the current position
void move_console_cursor(int x, bool x_relative, int y, bool y_relative);

// erases the rest of the current line
void wipe_console_line();

#endif
