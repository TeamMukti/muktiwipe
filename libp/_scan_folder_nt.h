/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Fast directory scanning via NtQueryDirectoryFile, which returns
 *	the directory entries in large batches and does not pay the cost
 *	of opening every file with FindFirstFile/FindNextFile.
 */
#ifndef _LIBP_SCAN_FOLDER_NT_H_
#define _LIBP_SCAN_FOLDER_NT_H_

#include "types.h"
#include "_windows.h"
#include "api_error.h"

//
struct wc_range
{
	const wchar_t * first;
	const wchar_t * last;

	wc_range() : first(NULL), last(NULL) { }
	wc_range(const wchar_t * f, const wchar_t * l) : first(f), last(l) { }

	size_t size() const { return last - first; }

	void to_str(wstring & str) const { str.assign(first, last); }
};

//
struct fsi_info
{
	dword    attrs;
	uint64_t bytes;

	fsi_info() : attrs(0), bytes(0) { }
};

//
struct fsi_scan_cb
{
	virtual ~fsi_scan_cb() { }

	// called with the handle of the folder being scanned
	virtual void on_fsi_open(HANDLE h) { (void) h; }

	// return false to stop scanning
	virtual bool on_fsi_scan(const wc_range & name, const fsi_info & info) = 0;
};

//
// scans the folder and reports its entries via the callback
//  buf_size - size of the NT API enumeration buffer, in bytes
//  errors are reported via api_error_cb, which may be NULL
//  returns false if the folder could not be opened or enumerated
bool scan_folder_nt(const wchar_t * path, size_t buf_size, fsi_scan_cb * scan_cb, api_error_cb * err);

#endif
