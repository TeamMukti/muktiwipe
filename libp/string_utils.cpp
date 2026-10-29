/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	String conversion and formatting.
 */
/*
 *	MSVCRT printf handles C99 conversions (%zu, %llu) since the UCRT,
 *	which is the runtime on every supported Windows version. Make sure
 *	MinGW does not swap the printf family for its own implementation
 *	that lacks support for some of the conversions used by muktiwipe.
 */
#ifdef __MINGW32__
#undef __USE_MINGW_ANSI_STDIO
#define __USE_MINGW_ANSI_STDIO 0
#endif

#include "string_utils.h"

#include <windows.h>

//
string to_utf8(const wstring & str)
{
	if (str.empty())
		return string();

	int size = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0, NULL, NULL);

	string out(size, '\0');

	WideCharToMultiByte(CP_UTF8, 0, str.c_str(), (int)str.size(), &out[0], size, NULL, NULL);

	return out;
}

wstring to_utf16(const string & str)
{
	if (str.empty())
		return wstring();

	int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);

	wstring out(size, L'\0');

	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &out[0], size);

	return out;
}

string stringf(const char * format, ...)
{
	va_list args;

	va_start(args, format);
	int len = vsnprintf(NULL, 0, format, args);
	va_end(args);

	if (len <= 0)
		return string();

	string out(len, '\0');

	va_start(args, format);
	vsnprintf(&out[0], len + 1, format, args);
	va_end(args);

	return out;
}
