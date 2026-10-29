/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Fast directory scanning via NtQueryDirectoryFile.
 */
#include "_scan_folder_nt.h"

#include "_elpify.h"
#include "_system_api.h"
#include "_ntstatus.h"
#include "_security.h"
#include "enforce.h"

//
extern bool force_delete; // set via set_force_delete() in delete_file.cpp

static
HANDLE open_folder(const wstring & path, api_error_cb * err)
{
	HANDLE h = CreateFileW(elpify(path).c_str(), FILE_LIST_DIRECTORY,
							FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

	if (h == INVALID_HANDLE_VALUE && force_delete)
	{
		dword code = GetLastError();

		if (force_denied_code(code))
		{
			// hostile ACL - take ownership and retry
			force_make_deletable(path);

			h = CreateFileW(elpify(path).c_str(), FILE_LIST_DIRECTORY,
							FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		}
	}

	if (h == INVALID_HANDLE_VALUE)
	{
		__on_api_error("CreateFile", path);
		return NULL;
	}

	return h;
}

static
bool scan_folder_entries(HANDLE h, size_t buf_size, fsi_scan_cb * scan_cb, const wstring & path, api_error_cb * err)
{
	vector<char> buf(buf_size ? buf_size : 8*1024);

	IO_STATUS_BLOCK  iosb;
	NTSTATUS         status;
	bool             first = true;

	for (;;)
	{
		status = ntdll.NtQueryDirectoryFile(h, NULL, NULL, NULL, &iosb,
											&buf[0], (ULONG) buf.size(),
											LIBP_FileDirectoryInformation,
											FALSE, NULL, first ? TRUE : FALSE);
		first = false;

		if (status == STATUS_NO_MORE_FILES)
			break;

		if (! NT_SUCCESS(status))
		{
			__on_api_error_ex("NtQueryDirectoryFile", status, path);
			return false;
		}

		char * ptr = &buf[0];
		char * end = &buf[0] + iosb.Information;

		while (ptr < end)
		{
			FILE_DIRECTORY_INFORMATION * di = (FILE_DIRECTORY_INFORMATION *) ptr;

			ptr += di->NextEntryOffset ? di->NextEntryOffset : (end - ptr);

			// skip the "." and ".." pseudo-entries
			if (di->FileNameLength == 2 * sizeof(WCHAR) &&
				di->FileName[0] == L'.' && di->FileName[1] == L'.')
			{
				continue;
			}

			if (di->FileNameLength == sizeof(WCHAR) && di->FileName[0] == L'.')
			{
				continue;
			}

			wc_range name(di->FileName, di->FileName + di->FileNameLength / sizeof(WCHAR));

			fsi_info info;

			info.attrs = di->FileAttributes;
			info.bytes = (uint64_t) di->EndOfFile.QuadPart;

			if (! scan_cb->on_fsi_scan(name, info))
				return true; // scanning stopped by the callback
		}
	}

	return true;
}

bool scan_folder_nt(const wchar_t * path, size_t buf_size, fsi_scan_cb * scan_cb, api_error_cb * err)
{
	__enforce(ntdll.NtQueryDirectoryFile);

	HANDLE h = open_folder(path, err);

	if (! h)
		return false;

	scan_cb->on_fsi_open(h);

	bool ok = scan_folder_entries(h, buf_size, scan_cb, path, err);

	CloseHandle(h);

	return ok;
}
