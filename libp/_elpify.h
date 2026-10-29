/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Extended-length path support ("elpify" = make an extended-length
 *	path). All file operations go through the elp API, which applies
 *	the \\?\ prefix and thus lifts the classic 260 char path limit.
 *
 *	Note - since UNICODE is defined, elp->DeleteFile() expands to
 *	elp->DeleteFileW() and so on, which is how the methods are named.
 */
#ifndef _LIBP_ELPIFY_H_
#define _LIBP_ELPIFY_H_

#include "types.h"
#include "_windows.h"

//
wstring elpify(const wstring & path);

//
struct elp_api
{
	dword GetFileAttributesW(const wchar_t * path);
	bool  DeleteFileW(const wchar_t * path);
	bool  SetFileAttributesW(const wchar_t * path, dword attrs);
	bool  RemoveDirectoryW(const wchar_t * path);
};

extern elp_api * elp;

#endif
