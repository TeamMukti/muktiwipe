/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Forced access recovery - used by muktiwipe --force / --trusted to
 *	take ownership, grant access and clear hostile attributes on items
 *	that cannot be deleted otherwise.
 */
#ifndef _LIBP_SECURITY_H_
#define _LIBP_SECURITY_H_

#include "types.h"

//
// enables SeTakeOwnershipPrivilege, SeRestorePrivilege and
// SeBackupPrivilege in the process token (idempotent)
void enable_force_privileges();

// sets the owner of the item to the Administrators group,
// like 'takeown /A' does
bool take_ownership(const wstring & path);

// replaces the DACL of the item with a full-control ACE for
// Everyone (S-1-1-0), inheritable by children
bool grant_everyone_full(const wstring & path);

// the combination of the above, to be retried after a delete failure
bool force_make_deletable(const wstring & path);

// the error codes that force recovery is worth trying for
bool force_denied_code(dword code);
bool force_locked_code(dword code);

#endif
