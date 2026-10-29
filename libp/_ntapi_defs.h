/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Minimal NT runtime definitions used for directory scanning and
 *	NT API file deletion. Declared locally to avoid a dependency on
 *	winternl.h, whose contents differ between compilers.
 */
#ifndef _LIBP_NTAPI_DEFS_H_
#define _LIBP_NTAPI_DEFS_H_

#include "types.h"
#include "_windows.h"

//
typedef LONG NTSTATUS;

#ifndef NT_SUCCESS
#define NT_SUCCESS(s)   (((NTSTATUS)(s)) >= 0)
#endif

//
typedef struct _LIBP_UNICODE_STRING {
	USHORT  Length;
	USHORT  MaximumLength;
	PWSTR   Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

//
typedef struct _LIBP_OBJECT_ATTRIBUTES {
	ULONG               Length;
	HANDLE              RootDirectory;
	PUNICODE_STRING     ObjectName;
	ULONG               Attributes;
	PVOID               SecurityDescriptor;
	PVOID               SecurityQualityOfService;
} OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

#ifndef InitializeObjectAttributes
#define InitializeObjectAttributes(p, n, a, r, s) \
	{ (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
	  (p)->RootDirectory = (r); \
	  (p)->ObjectName = (n); \
	  (p)->Attributes = (a); \
	  (p)->SecurityDescriptor = (s); \
	  (p)->SecurityQualityOfService = NULL; }
#endif

#ifndef OBJ_CASE_INSENSITIVE
#define OBJ_CASE_INSENSITIVE        0x00000040L
#endif

//
typedef struct _LIBP_IO_STATUS_BLOCK {
	union {
		NTSTATUS    Status;
		PVOID       Pointer;
	};
	ULONG_PTR   Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;

//
typedef struct _LIBP_FILE_DIRECTORY_INFORMATION {
	ULONG           NextEntryOffset;
	ULONG           FileIndex;
	LARGE_INTEGER   CreationTime;
	LARGE_INTEGER   LastAccessTime;
	LARGE_INTEGER   LastWriteTime;
	LARGE_INTEGER   ChangeTime;
	LARGE_INTEGER   EndOfFile;
	LARGE_INTEGER   AllocationSize;
	ULONG           FileAttributes;
	ULONG           FileNameLength;
	WCHAR           FileName[1];
} FILE_DIRECTORY_INFORMATION, * PFILE_DIRECTORY_INFORMATION;

// FILE_INFORMATION_CLASS value used by scan_folder_nt()
#define LIBP_FileDirectoryInformation   ((ULONG) 1)

//
typedef VOID     (NTAPI * libp_RtlInitUnicodeString_t)                (PUNICODE_STRING, PCWSTR);
typedef VOID     (NTAPI * libp_RtlFreeUnicodeString_t)                (PUNICODE_STRING);
typedef NTSTATUS (NTAPI * libp_NtDeleteFile_t)                        (POBJECT_ATTRIBUTES);
typedef NTSTATUS (NTAPI * libp_NtQueryDirectoryFile_t)                (HANDLE, HANDLE, PVOID, PVOID,
                                                                       PIO_STATUS_BLOCK, PVOID, ULONG,
                                                                       ULONG /* FILE_INFORMATION_CLASS */,
                                                                       BOOLEAN, PUNICODE_STRING, BOOLEAN);
typedef NTSTATUS (NTAPI * libp_RtlDosPathNameToNtPathName_U_WithStatus_t) (PCWSTR, PUNICODE_STRING, PWSTR *, PVOID);
typedef BOOLEAN  (NTAPI * libp_RtlDosPathNameToNtPathName_U_t)        (PCWSTR, PUNICODE_STRING, PWSTR *, PVOID);

#endif
