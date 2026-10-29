/*
 *	This file is a part of the source code of "muktiwipe" program.
 *
 *	Copyright (c) 2020- Alexander Pankratov and IO Bureau SA.
 *	Modifications (c) 2026 Team-Mukti.
 *	All rights reserved.
 *
 *	The source code is distributed under the terms of 2-clause
 *	BSD license with the Commons Clause condition. See LICENSE
 *	file for details.
 */
#include "delete_file.h"

#include "utils.h"
#include "libp/_elpify.h"
#include "libp/_system_api.h"
#include "libp/_ntstatus.h"
#include "libp/_security.h"
#include "libp/_unlock.h"

#define HSRO (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)

//
// --force mode - take ownership, grant access, clear attributes and
// terminate locking processes in order to remove stubborn items
bool force_delete = false;

void set_force_delete(bool force)
{
	force_delete = force;
}

//
static
bool nt_not_found(dword code)
{
	return code == STATUS_OBJECT_NAME_NOT_FOUND;
}

static
dword delete_file_win32(const wstring & file)
{
	if (elp->DeleteFileW(file.c_str()))
		return 0;

	dword code = GetLastError();

	if (code == ERROR_FILE_NOT_FOUND)
		return 0;

	return code;
}

static
dword delete_file_ntapi(const wstring & file)
{
	UNICODE_STRING     name;
	OBJECT_ATTRIBUTES  attr;
	NTSTATUS           status;

	if (ntdll.RtlDosPathNameToNtPathName_U_WithStatus)
	{
		status = ntdll.RtlDosPathNameToNtPathName_U_WithStatus(elpify(file).c_str(), &name, NULL, NULL);
	}
	else
	if (ntdll.RtlDosPathNameToNtPathName_U)
	{
		status = ntdll.RtlDosPathNameToNtPathName_U(elpify(file).c_str(), &name, NULL, NULL) ?
		         STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	}
	else
	{
		return (dword) -1;
	}

	if (status != STATUS_SUCCESS)
		return status;

	InitializeObjectAttributes(&attr, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = ntdll.NtDeleteFile(&attr);
	ntdll.RtlFreeUnicodeString(&name);

	return status; // 0 = success
}

//
static
void force_recover(const wstring & path, dword code)
{
	if (force_locked_code(code))
		kill_lockers(path);

	force_make_deletable(path);
}

bool delete_file(const wstring & file, dword attrs, bool ntapi, api_error_cb * err)
{
	dword code = 0;

	for (dword attempt = 0; ; attempt++)
	{
		if (attempt == 0)
		{
			if ( (attrs & HSRO) &&
			     ! elp->SetFileAttributes(file.c_str(), attrs & ~HSRO))
			{
				__on_api_error("SetFileAttributes", file);
			}
		}
		else
		{
			elp->SetFileAttributes(file.c_str(), FILE_ATTRIBUTE_NORMAL);
		}

		code = ntapi ? delete_file_ntapi(file)
		             : delete_file_win32(file);

		if (! code)
			return true;

		if (! force_delete || attempt > 0 || ! force_denied_code(code))
			break;

		force_recover(file, code);
	}

	if (ntapi)
	{
		if (nt_not_found(code))
			return false; // silently, as before

		__on_api_error_ex("NtDeleteFile", code, file);
		return false;
	}

	__on_api_error_code("DeleteFile", code, file);
	return false;
}

bool delete_folder(const wstring & folder, dword attrs, api_error_cb * err)
{
	dword code = 0;

	for (dword attempt = 0; ; attempt++)
	{
		if (attempt == 0)
		{
			if ( (attrs & HSRO) &&
			     ! elp->SetFileAttributes(folder.c_str(), attrs & ~HSRO))
			{
				__on_api_error("SetFileAttributes", folder);
			}
		}
		else
		{
			elp->SetFileAttributes(folder.c_str(), FILE_ATTRIBUTE_NORMAL);
		}

		if (elp->RemoveDirectoryW(folder.c_str()))
			return true;

		code = GetLastError();

		if (__not_found(code))
			return true;

		if (! force_delete || attempt > 0 || ! force_denied_code(code))
			break;

		// a process running with its CWD inside the target blocks
		// the deletion without holding any named handle
		if (code == ERROR_CURRENT_DIRECTORY)
			SetCurrentDirectoryW(L"C:\\");

		force_recover(folder, code);
	}

	__on_api_error_code("RemoveDirectory", code, folder);
	return false;
}
