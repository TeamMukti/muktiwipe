/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Access to ntdll entry points, resolved dynamically so that the
 *	program does not need an import dependency on ntdll.dll.
 */
#ifndef _LIBP_SYSTEM_API_H_
#define _LIBP_SYSTEM_API_H_

#include "types.h"
#include "string_utils.h"
#include "_ntapi_defs.h"

//
struct system_api_ntdll
{
	HMODULE mod;

	libp_RtlInitUnicodeString_t                     RtlInitUnicodeString;
	libp_RtlFreeUnicodeString_t                     RtlFreeUnicodeString;
	libp_NtDeleteFile_t                             NtDeleteFile;
	libp_NtQueryDirectoryFile_t                     NtQueryDirectoryFile;
	libp_RtlDosPathNameToNtPathName_U_WithStatus_t  RtlDosPathNameToNtPathName_U_WithStatus;
	libp_RtlDosPathNameToNtPathName_U_t             RtlDosPathNameToNtPathName_U;   // fallback

	system_api_ntdll();
};

extern system_api_ntdll ntdll;

void init_ext_system_api(const void * ctx);

//
inline string failed_with(dword code)
{
	if (code & 0xF0000000)
		return stringf("failed with status 0x%08lx", code);

	return stringf("failed with error %lu", code);
}

#endif
