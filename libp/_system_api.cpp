/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Access to ntdll entry points, resolved dynamically.
 */
#include "_system_api.h"

system_api_ntdll ntdll;

system_api_ntdll::system_api_ntdll()
{
	mod = NULL;

	RtlInitUnicodeString = NULL;
	RtlFreeUnicodeString = NULL;
	NtDeleteFile = NULL;
	NtQueryDirectoryFile = NULL;
	RtlDosPathNameToNtPathName_U_WithStatus = NULL;
	RtlDosPathNameToNtPathName_U = NULL;
}

void init_ext_system_api(const void * ctx)
{
	ntdll.mod = GetModuleHandleW(L"ntdll.dll");

	if (! ntdll.mod)
		return;

	#define __GET_PROC(name) \
		ntdll.name = (libp_##name##_t) GetProcAddress(ntdll.mod, #name)

	__GET_PROC(RtlInitUnicodeString);
	__GET_PROC(RtlFreeUnicodeString);
	__GET_PROC(NtDeleteFile);
	__GET_PROC(NtQueryDirectoryFile);
	__GET_PROC(RtlDosPathNameToNtPathName_U_WithStatus);
	__GET_PROC(RtlDosPathNameToNtPathName_U);

	#undef __GET_PROC
}
