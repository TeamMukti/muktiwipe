/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Unlocking files - terminates the processes that keep the file or
 *	folder open, via the built-in Restart Manager. Replaces external
 *	tools such as handle.exe / taskkill.
 */
#ifndef _LIBP_UNLOCK_H_
#define _LIBP_UNLOCK_H_

#include "types.h"

// registers the path with the Restart Manager, terminates the
// processes holding it open (except critical system processes and
// the process itself) and returns true if any process was killed
bool kill_lockers(const wstring & path);

#endif
