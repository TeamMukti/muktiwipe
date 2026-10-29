/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Console interaction - cursor position and line updates.
 */
#include "_console.h"

#include "_windows.h"

//
static HANDLE get_console_out()
{
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

	if (! out || out == INVALID_HANDLE_VALUE)
		return NULL;

	DWORD mode = 0;

	if (! GetConsoleMode(out, &mode))
		return NULL; // output is redirected

	return out;
}

bool is_interactive_console()
{
	return get_console_out() != NULL;
}

//
static CONSOLE_CURSOR_INFO cursor_saved;
static bool                cursor_saved_valid = false;

static void restore_cursor_state()
{
	if (cursor_saved_valid)
	{
		cursor_saved_valid = false;
		SetConsoleCursorInfo(get_console_out(), &cursor_saved);
	}
}

void show_console_cursor(bool show)
{
	HANDLE out = get_console_out();

	if (! out)
		return;

	CONSOLE_CURSOR_INFO info;

	if (! GetConsoleCursorInfo(out, &info))
		return;

	if (! show)
	{
		// remember the original (visible) state and restore it on
		// program exit, so that the hosting cmd.exe/terminal is not
		// left with an invisible cursor
		if (! cursor_saved_valid)
		{
			cursor_saved = info;
			cursor_saved_valid = true;
			atexit(restore_cursor_state);
		}

		info.bVisible = FALSE;
	}
	else
	{
		info.bVisible = TRUE;
	}

	SetConsoleCursorInfo(out, &info);
}

void move_console_cursor(int x, bool x_relative, int y, bool y_relative)
{
	HANDLE out = get_console_out();

	if (! out)
		return;

	CONSOLE_SCREEN_BUFFER_INFO info;

	if (! GetConsoleScreenBufferInfo(out, &info))
		return;

	COORD pos;

	pos.X = (SHORT) (x_relative ? info.dwCursorPosition.X + x : x);
	pos.Y = (SHORT) (y_relative ? info.dwCursorPosition.Y + y : y);

	SetConsoleCursorPosition(out, pos);
}

void wipe_console_line()
{
	HANDLE out = get_console_out();

	if (! out)
		return;

	CONSOLE_SCREEN_BUFFER_INFO info;

	if (! GetConsoleScreenBufferInfo(out, &info))
		return;

	dword count = info.dwSize.X - info.dwCursorPosition.X;

	if (! count)
		return;

	dword written = 0;

	FillConsoleOutputCharacterW(out, L' ', count, info.dwCursorPosition, &written);
	FillConsoleOutputAttribute(out, info.wAttributes, count, info.dwCursorPosition, &written);
}
