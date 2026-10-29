/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Central windows.h include. The program operates on wide paths
 *	exclusively, so UNICODE is fixed for every translation unit.
 */
#ifndef _LIBP_WINDOWS_H_
#define _LIBP_WINDOWS_H_

#ifndef WINVER
#define WINVER          0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT    0x0601
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <exception>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#endif
