/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Unlocking files via the Restart Manager.
 */
#include "_unlock.h"

#include "_windows.h"
#include "_lang.h"
#include "string_utils.h"

#include <restartmanager.h>
#include <tlhelp32.h>

#pragma comment(lib, "rstrtmgr.lib")

//
static
bool is_critical_process(const wchar_t * name)
{
	// killing any of these would crash or cripple the system
	static const wchar_t * list[] =
	{
		L"system", L"system idle process", L"registry", L"memory compression",
		L"smss.exe", L"csrss.exe", L"wininit.exe", L"winlogon.exe",
		L"services.exe", L"lsass.exe", L"svchost.exe",
	};

	for (auto & x : list)
		if (! _wcsicmp(name, x))
			return true;

	return false;
}

bool kill_lockers(const wstring & path)
{
	dword session = 0;
	wchar_t  key[CCH_RM_SESSION_KEY + 1] = L"";

	if (RmStartSession(&session, 0, key) != ERROR_SUCCESS)
		return false;

	bool killed = false;

	const wchar_t * paths[1] = { path.c_str() };

	if (RmRegisterResources(session, 1, paths, 0, NULL, 0, NULL) == ERROR_SUCCESS)
	{
		RM_PROCESS_INFO info[64];
		UINT   needed = 0;
		UINT   count  = 64;
		dword  reasons = 0;

		if (RmGetList(session, &needed, &count, info, &reasons) == ERROR_SUCCESS)
		{
			dword self = GetCurrentProcessId();

			for (UINT i = 0; i < count; i++)
			{
				dword pid = info[i].Process.dwProcessId;

				if (pid == self)
					continue;

				HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE,
				                          FALSE, pid);

				if (! proc)
					continue; // protected process - cannot be killed anyway

				wchar_t image[MAX_PATH] = L"";
				dword   size = MAX_PATH;

				if (QueryFullProcessImageNameW(proc, 0, image, &size))
				{
					const wchar_t * base = wcsrchr(image, L'\\');

					base = base ? base + 1 : image;

					if (! is_critical_process(base) && TerminateProcess(proc, 1))
					{
						printf(msg(M_unlocked), to_utf8(base).c_str(), pid);
						killed = true;
					}
				}

				CloseHandle(proc);
			}
		}
	}

	RmEndSession(session);

	if (killed)
		Sleep(300); // let the OS release the handles

	return killed;
}
