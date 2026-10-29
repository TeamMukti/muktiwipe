/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	File system queries.
 */
#ifndef _LIBP_FILESYS_H_
#define _LIBP_FILESYS_H_

#include "types.h"
#include "_windows.h"
#include "api_error.h"

// expands the path to a full path name
bool get_full_pathname(const wstring & path, wstring & full);

// retrieves the attributes and size of a file or a folder
bool get_file_info(const wstring & path, WIN32_FIND_DATA & data, api_error_cb * err);

#endif
