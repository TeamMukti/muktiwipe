/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Basic types and common STL includes.
 *
 *	This file was implemented to complete the published "byenow"
 *	source code (muktiwipe is derived from it), which depends on
 *	libp but does not ship it.
 */
#ifndef _LIBP_TYPES_H_
#define _LIBP_TYPES_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>

#include <string>
#include <vector>
#include <deque>
#include <set>
#include <map>
#include <algorithm>
#include <memory>
#include <utility>

using namespace std;

//
typedef unsigned int        uint;
typedef unsigned long       dword;      // matches Win32 DWORD (always 32-bit)
typedef unsigned long long  qword;

//
#define __no_copying(X)       X(const X &) = delete; X & operator = (const X &) = delete;
#define __interface(X)        /* marker macro, no code */

#endif
