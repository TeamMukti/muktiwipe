/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	String conversion and formatting.
 */
#ifndef _LIBP_STRING_UTILS_H_
#define _LIBP_STRING_UTILS_H_

#include "types.h"

//
string  to_utf8(const wstring & str);
wstring to_utf16(const string & str);

string  stringf(const char * format, ...);

#endif
