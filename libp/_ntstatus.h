/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	NTSTATUS values used by muktiwipe.
 */
#ifndef _LIBP_NTSTATUS_H_
#define _LIBP_NTSTATUS_H_

#include "_ntapi_defs.h"

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                  ((NTSTATUS) 0x00000000L)
#endif
#define STATUS_UNSUCCESSFUL             ((NTSTATUS) 0xC0000001L)
#define STATUS_NO_MORE_FILES            ((NTSTATUS) 0x80000006L)
#define STATUS_CANNOT_DELETE            ((NTSTATUS) 0xC0000121L)
#define STATUS_DIRECTORY_NOT_EMPTY      ((NTSTATUS) 0xC0000101L)
#define STATUS_ACCESS_DENIED            ((NTSTATUS) 0xC0000022L)
#define STATUS_OBJECT_NAME_NOT_FOUND    ((NTSTATUS) 0xC0000034L)
#define STATUS_OBJECT_PATH_NOT_FOUND    ((NTSTATUS) 0xC000003AL)
#define STATUS_SHARING_VIOLATION        ((NTSTATUS) 0xC0000043L)
#define STATUS_DELETE_PENDING           ((NTSTATUS) 0xC0000056L)

#endif
