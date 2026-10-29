/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Forced access recovery.
 */
#include "_security.h"

#include "_windows.h"
#include "_ntstatus.h"
#include "_elpify.h"

#include <sddl.h>
#include <aclapi.h>

#pragma comment(lib, "advapi32.lib")

//
void enable_force_privileges()
{
	static bool enabled = false;

	if (enabled)
		return;

	enabled = true;

	HANDLE token = NULL;

	if (! OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
		return;

	const wchar_t * priv_names[] =
	{
		SE_TAKE_OWNERSHIP_NAME,
		SE_RESTORE_NAME,
		SE_BACKUP_NAME,
	};

	for (auto & name : priv_names)
	{
		TOKEN_PRIVILEGES tp;

		memset(&tp, 0, sizeof tp);

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (LookupPrivilegeValueW(NULL, name, &tp.Privileges[0].Luid))
			AdjustTokenPrivileges(token, FALSE, &tp, sizeof tp, NULL, NULL);

		// ERROR_NOT_ALL_ASSIGNED is fine - the process may simply
		// lack the privilege, recovery will then do what it can
	}

	CloseHandle(token);
}

bool take_ownership(const wstring & path)
{
	enable_force_privileges();

	PSID admins = NULL;

	if (! ConvertStringSidToSidW(L"S-1-5-32-544", &admins))
		return false;

	dword rc = SetNamedSecurityInfoW((LPWSTR) elpify(path).c_str(), SE_FILE_OBJECT,
									 OWNER_SECURITY_INFORMATION, admins, NULL, NULL, NULL);

	LocalFree(admins);

	return rc == ERROR_SUCCESS;
}

bool grant_everyone_full(const wstring & path)
{
	PSID everyone = NULL;

	if (! ConvertStringSidToSidW(L"S-1-1-0", &everyone))
		return false;

	dword acl_size = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(dword) + GetLengthSid(everyone);

	vector<char> buf(acl_size);

	ACL * dacl = (ACL *) &buf[0];

	bool ok = InitializeAcl(dacl, acl_size, ACL_REVISION) &&
			  AddAccessAllowedAceEx(dacl, ACL_REVISION,
			                        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
			                        GENERIC_ALL, everyone);

	if (ok)
		ok = SetNamedSecurityInfoW((LPWSTR) elpify(path).c_str(), SE_FILE_OBJECT,
		                           DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
		                           NULL, NULL, dacl, NULL) == ERROR_SUCCESS;

	LocalFree(everyone);

	return ok;
}

bool force_make_deletable(const wstring & path)
{
	enable_force_privileges();

	// either of the two may fail (e.g. ownership change requires
	// privileges that a plain owner does not have), but as long as
	// one of them succeeds the deletion can proceed
	take_ownership(path);

	return grant_everyone_full(path);
}

//
bool force_denied_code(dword code)
{
	if (code & 0xF0000000)
		return code == STATUS_ACCESS_DENIED         ||
		       code == STATUS_SHARING_VIOLATION     ||
		       code == STATUS_DELETE_PENDING        ||
		       code == STATUS_CANNOT_DELETE         ||
		       code == STATUS_DIRECTORY_NOT_EMPTY;

	return code == ERROR_ACCESS_DENIED       ||
	       code == ERROR_SHARING_VIOLATION   ||
	       code == ERROR_LOCK_VIOLATION      ||
	       code == ERROR_DIR_NOT_EMPTY       ||
	       code == ERROR_CURRENT_DIRECTORY;
}

bool force_locked_code(dword code)
{
	if (code & 0xF0000000)
		return code == STATUS_SHARING_VIOLATION ||
		       code == STATUS_DELETE_PENDING    ||
		       code == STATUS_CANNOT_DELETE;

	return code == ERROR_SHARING_VIOLATION ||
	       code == ERROR_LOCK_VIOLATION;
}
