/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	File system queries.
 */
#include "_filesys.h"

//
bool get_full_pathname(const wstring & path, wstring & full)
{
	dword len = 0;

	for (size_t size = 1024; size <= 32*1024; size *= 2)
	{
		vector<wchar_t> buf(size, 0);

		len = GetFullPathNameW(path.c_str(), (dword)size, &buf[0], NULL);

		if (! len)
			return false;

		if (len < size)
		{
			full.assign(&buf[0], len);
			return true;
		}
	}

	return false;
}

bool get_file_info(const wstring & path, WIN32_FIND_DATA & data, api_error_cb * err)
{
	HANDLE h = FindFirstFileW(path.c_str(), &data);

	if (h == INVALID_HANDLE_VALUE)
	{
		__on_api_error("FindFirstFile", path);
		return false;
	}

	FindClose(h);

	return true;
}
