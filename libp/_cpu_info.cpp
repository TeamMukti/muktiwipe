/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	CPU query - accounts for all processor groups on systems with
 *	more than 64 logical CPUs.
 */
#include "_cpu_info.h"

#include "_windows.h"

size_t get_cpu_count()
{
	typedef DWORD (WINAPI * GetActiveProcessorCount_t)(WORD);

	GetActiveProcessorCount_t get_active_processor_count =
		(GetActiveProcessorCount_t) GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetActiveProcessorCount");

	if (get_active_processor_count)
	{
		WORD all_groups = (WORD) -1;    // ALL_PROCESSOR_GROUPS

		DWORD count = get_active_processor_count(all_groups);

		if (count)
			return count;
	}

	SYSTEM_INFO si;

	GetSystemInfo(&si);

	return si.dwNumberOfProcessors ? si.dwNumberOfProcessors : 1;
}
