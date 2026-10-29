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
#include "libp/types.h"
#include "libp/string_utils.h"
#include "libp/enforce.h"
#include "libp/atomic.h"
#include "libp/_windows.h"
#include "libp/_elpify.h"
#include "libp/_console.h"
#include "libp/_filesys.h"
#include "libp/_system_api.h"
#include "libp/time.h"
#include "libp/_lang.h"

#include <objbase.h>
#include <shellapi.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")

#include "ultra_machine.h"
#include "delete_file.h"
#include "utils.h"

//
#define HEADER  msg(M_header)
#define SYNTAX  msg(M_syntax)

//
enum EXIT_CODES
{
	RC_ok               = 0,
	RC_cancelled        = 1,
	RC_whoops_seh       = 2,       // see wmain_seh()
	RC_whoops_cpp       = 3,       // see wmain()
	RC_unlikely         = 4,
	RC_ok_with_errors   = 10,      // ... + log10(error_count)

	// init errors
	RC_no_path          = 50,
	RC_invalid_arg      = 51,
	RC_not_confirmed    = 52,

	// path errors
	RC_path_not_found   = 60,
	RC_path_is_file     = 61,
	RC_path_is_root     = 62,
	RC_path_restricted  = 63,
	RC_path_cant_expand = 64,
	RC_path_cant_check  = 65,
};

//
struct context : ultra_mach_cb
{
	// config

	wstring          path;
	string           path_utf8;

	bool             preview;      // scan only
	bool             staged;       // scan, then delete
	bool             confirm;      // confirm delete (only with --journal)
	bool             journal;      // --journal: confirm + wait for Enter before closing
	bool             yolo;         // don't block deletion in c:\windows and c:\users
	bool             omni;         // path can point at a file
	bool             forced;       // --force requested
	bool             trusted;      // --trusted, force + auto-elevation
	bool             no_elevate;   // --no-elevate, suppress the UAC relaunch

	ultra_mach_conf  mach_conf;
	bool             cryptic;
	bool             show_bytes;
	bool             list_errors;

	// state

	bool             interactive;
	bool             enough;

	folder           root;
	dword            path_attrs;
	bool             is_a_file;
	api_error_vec    scanner_err;
	api_error_vec    deleter_err;
	usec_t           started;
	usec_t           finished;
	usec_t           reported;

	uint             mode;         // 0x01 - scanning, 0x02 - deleting
	ultra_mach_info  info;

	uint             exit_rc;

	//
	static context * self;

	//
	context();

	void init();
	void parse_args(int argc, wchar_t ** argv);
	void parse_uint(int argc, wchar_t ** argv, size_t & next, size_t & val);

	void syntax(int rc);
	void abort(int rc, const char * format, ...);
	void confirm_it();

	//
	bool on_ultra_mach_tick(const ultra_mach_info & info); // ultra_mach_cb
	void init_progress();
	void update_progress();

	void print_verbose_stats(bool scan);
	void print_cryptic_stats();

	void check_path();
	void check_restricted();
	void maybe_elevate(int argc, wchar_t ** argv);
	void process();
	void delete_file();

	void report();
	void report_errors();

	//
	static BOOL __stdcall on_console_event_proxy(dword type);
	BOOL on_console_event(dword type);
};

//
context * context::self = NULL;

/*
 *	When the program owns its console window (e.g. launched from the
 *	context menu), keep it open after completion so that the summary
 *	can be read. When attached to a shell, exit normally instead.
 */
static void press_enter_to_exit()
{
	dword plist[2] = { 0 };

	if (GetConsoleProcessList(plist, 2) != 1)
		return; // attached to a shell - it will print its prompt

	printf(msg(M_press_enter));
	fflush(stdout);

	int c;

	while ((c = getchar()) != '\n' && c != EOF)
		{ }
}

//
context::context()
{
	preview = false;
	staged  = false;
	confirm = false;   // direct deletion by default, --journal to ask
	journal = false;
	yolo    = false;
	omni    = false;
	forced  = false;
	trusted = false;
	no_elevate = false;

	cryptic = false;
	show_bytes = false;
	list_errors = false;

	interactive = false;
	enough = false;
	path_attrs = 0;
	is_a_file = false;

	started.raw = 0;
	finished.raw = 0;
	reported = usec();

	mode = 0x00;

	exit_rc = RC_ok;
}

void context::init()
{
	context::self = this;

	init_ext_system_api(NULL);

	if (! ntdll.NtQueryDirectoryFile ||
	    ! ntdll.NtDeleteFile ||
	    ! ntdll.RtlInitUnicodeString ||
	    ! ntdll.RtlFreeUnicodeString)
	{
		abort(RC_unlikely, msg(M_nt_api_missing));
	}

	// RtlDosPathNameToNtPathName_U_WithStatus is not required here,
	// it has a fallback in RtlDosPathNameToNtPathName_U

	interactive = is_interactive_console();

	if (interactive)
	{
		SetConsoleCtrlHandler(on_console_event_proxy, TRUE);
	}

	SetConsoleOutputCP(CP_UTF8);
}

void context::parse_args(int argc, wchar_t ** argv)
{
	for (size_t i=1; i < (size_t)argc; i++)
	{
		const wchar_t * arg = argv[i];

		if (! wcscmp(arg, L"-p") || ! wcscmp(arg, L"--preview"))
		{
			preview = true;
			continue;
		}

		if (! wcscmp(arg, L"-s") || ! wcscmp(arg, L"--staged"))
		{
			staged = true;
			continue;
		}

		if (! wcscmp(arg, L"-y") || ! wcscmp(arg, L"--yes"))
		{
			confirm = false;    // kept for backward compatibility - it is the default
			continue;
		}

		if (! wcscmp(arg, L"-j") || ! wcscmp(arg, L"--journal"))
		{
			journal = true;
			confirm = true;
			continue;
		}

		if (! wcscmp(arg, L"-x") || ! wcscmp(arg, L"--yolo"))
		{
			yolo = true;
			continue;
		}

		if (! wcscmp(arg, L"-f") || ! wcscmp(arg, L"--force"))
		{
			forced = true;
			set_force_delete(true);
			continue;
		}

		if (! wcscmp(arg, L"-T") || ! wcscmp(arg, L"--trusted"))
		{
			trusted = true;
			forced  = true;
			set_force_delete(true);
			continue;
		}

		if (! wcscmp(arg, L"--no-elevate"))
		{
			no_elevate = true;
			continue;
		}

		if (! wcscmp(arg, L"-o") || ! wcscmp(arg, L"--omni-delete"))
		{
			omni = true;
			continue;
		}

		if (! wcscmp(arg, L"-k") || ! wcscmp(arg, L"--keep-folder"))
		{
			mach_conf.keep_root = true;
			continue;
		}

		if (! wcscmp(arg, L"-1") || ! wcscmp(arg, L"--one-liner"))
		{
			cryptic = true;
			continue;
		}

		if (! wcscmp(arg, L"-b") || ! wcscmp(arg, L"--show-bytes"))
		{
			show_bytes = true;
			continue;
		}

		if (! wcscmp(arg, L"-e") || ! wcscmp(arg, L"--list-errors"))
		{
			list_errors = true;
			continue;
		}

		if (! wcscmp(arg, L"-t") || ! wcscmp(arg, L"--threads"))
		{
			parse_uint(argc, argv, i, mach_conf.threads);
			continue;
		}

		if (! wcscmp(arg, L"--scan-buf-kb"))
		{
			parse_uint(argc, argv, i, mach_conf.scanner_buf_size);

			if (mach_conf.scanner_buf_size > 64*1024)
				abort(RC_invalid_arg, msg(M_scan_buf_max));

			mach_conf.scanner_buf_size *= 1024;
			continue;
		}

		if (! wcscmp(arg, L"-n") || ! wcscmp(arg, L"--delete-ntapi"))
		{
			mach_conf.deleter_ntapi = true;
			continue;
		}

		if (! wcscmp(arg, L"--delete-batch"))
		{
			parse_uint(argc, argv, i, mach_conf.deleter_batch);
			continue;
		}

		if (arg[0] == L'-' || arg[0] == L'/')
			syntax(RC_invalid_arg);

		// otherwise it's a path

		if (path.size())
			syntax(RC_invalid_arg);

		path = arg;
	}

	if (path.empty())
		syntax(RC_no_path);

	if (path.size() == 2 && path[1] == L':' ||
	    path.size() == 3 && path[1] == L':' && path[2] == L'\\')
	{
		abort(RC_path_is_root, msg(M_root_not_supported));
	}

	if (path.back() == L'\\')
		path.pop_back();

	//
	path_utf8 = to_utf8(path);
}

void context::parse_uint(int argc, wchar_t ** argv, size_t & i, size_t & val)
{
	if (++i == argc)
		syntax(RC_invalid_arg);

	if (! swscanf(argv[i], L"%zu", &val))
		syntax(RC_invalid_arg);
}

//
void context::syntax(int rc)
{
	printf(HEADER);
	printf(SYNTAX);
	exit(rc);
}

void context::abort(int rc, const char * format, ...)
{
	va_list m;
	printf(HEADER);
	va_start(m, format);
	vprintf(format, m);
	va_end(m);
	exit(rc);
}

void context::confirm_it()
{
	// 'o' / 'oui' are accepted as well - the program is bilingual
	static const char * yes[] = { "y", "yes", "yep", "yup", "o", "oui" };
	char line[32];

	if (preview || ! confirm)
		return;

	if (! is_a_file) printf(msg(M_confirm_folder), path_utf8.c_str());
	else             printf(msg(M_confirm_file), path_utf8.c_str());

	printf(msg(M_confirm_prompt));
	fflush(stdout);

	if (! gets_s(line, sizeof line))
		exit(RC_not_confirmed);

	// trim leading and trailing whitespace (piped input often
	// carries a trailing space and/or carriage return)
	size_t len = strlen(line);
	size_t beg = 0;

	while (beg < len && strchr(" \t\r", line[beg]))
		beg++;

	while (len > beg && strchr(" \t\r", line[len-1]))
		len--;

	if (len > beg)
		memmove(line, line + beg, len - beg);

	line[len - beg] = '\0';

	for (auto & str : yes)
		if (! strcmp(line, str))
			return;

	printf(msg(M_cancelled));
	exit(RC_not_confirmed);
}

//
BOOL __stdcall context::on_console_event_proxy(dword type)
{
	__enforce(context::self);
	return context::self->on_console_event(type);
}

BOOL context::on_console_event(dword type)
{
	if (type == CTRL_C_EVENT)
	{
		printf("Ctrl-C\n");
		enough = true;
		return TRUE;
	}

	if (type == CTRL_CLOSE_EVENT)
		return TRUE; // terminate

//	if (type == CTRL_BREAK_EVENT)
//		// dump stats, return FALSE

	// CTRL_LOGOFF_EVENT, CTRL_SHUTDOWN_EVENT
	return FALSE; // terminate
}

/*
 *
 */
bool context::on_ultra_mach_tick(const ultra_mach_info & _info)
{
	if (enough)
		return false;

	if (mode == 0x01 || // scan
	    mode == 0x03)   // scan & delete
	{
		info = _info;
	}
	else
	if (mode == 0x02) // delete-after-scan
	{
		info.f_deleted = _info.f_deleted;
		info.d_deleted = _info.d_deleted;
		info.done      = _info.done;
		info.folders_togo = _info.folders_togo;
	}
	else
	{
		__enforce(false);
	}

	if (_info.scanner_err) append(scanner_err, *_info.scanner_err);
	if (_info.deleter_err) append(deleter_err, *_info.deleter_err);

	if (interactive)
		update_progress();

	return true;
}

void context::init_progress()
{
	if (! cryptic)
	{
		printf("%s [%s] %s\n", preview ? msg(M_scanning) : msg(M_deleting), path_utf8.c_str(), (staged && ! preview) ? msg(M_staged_tag) : "");
		printf("\n");
		if (show_bytes) printf("           %10s  %10s  %10s  %10s\n", msg(M_col_folders), msg(M_col_files), msg(M_col_bytes), msg(M_col_errors));
		else            printf("           %10s  %10s  %10s\n",       msg(M_col_folders), msg(M_col_files), msg(M_col_errors));
	}

	if (! interactive)
		return;

	if (! cryptic)
	{
		if (show_bytes)
		{
			printf("%s%10s  %10s  %10s  %10s\n", msg(M_found_h),   "-", "-", "-", "-");
			printf("%s%10s  %10s  %10s  %10s\n", msg(M_deleted_h), "-", "-", "-", "-");
		}
		else
		{
			printf("%s%10s  %10s  %10s\n", msg(M_found_h),   "-", "-", "-");
			printf("%s%10s  %10s  %10s\n", msg(M_deleted_h), "-", "-", "-");
		}
	}
	else
	{
		printf("\n");
	}
}

void context::update_progress()
{
	usec_t now = usec();

	if (now - reported < 100*1000 && ! info.done)
		return;

	if (cryptic)
	{
		move_console_cursor(0, false, -1, true);

		print_cryptic_stats();
		wipe_console_line();
		printf("\n");
	}
	else
	{
		move_console_cursor(0, false, -2, true);

		print_verbose_stats(true);
		wipe_console_line();
		printf("\n");

		if (! preview)
		{
			print_verbose_stats(false);
			wipe_console_line();
		}

		printf("\n");
	}

	reported = now;
}

void context::print_verbose_stats(bool scan)
{
	const char * label = scan ? msg(M_found) : msg(M_deleted);
	const size_t & d = scan ? info.d_found : info.d_deleted;
	const size_t & f = scan ? info.f_found : info.f_deleted;
	const size_t & b = scan ? info.b_found : info.b_deleted;
	const size_t   e = scan ? scanner_err.size() : deleter_err.size();

	if (show_bytes) printf("%s  %10zu  %10zu  %10s  %10zu", label, d, f, format_bytes(b).c_str(), e);
	else            printf("%s  %10zu  %10zu  %10zu", label, d, f, e);

	if (scan && info.folders_togo) printf(msg(M_togo), info.folders_togo);
}

void context::print_cryptic_stats()
{
	if (show_bytes)
		printf(msg(M_cryptic),
			info.d_found, info.d_deleted,
			info.f_found, info.f_deleted,
			format_bytes(info.b_found).c_str(),
			format_bytes(info.b_deleted).c_str(),
			scanner_err.size(), deleter_err.size());
	else
		printf(msg(M_cryptic),
			info.d_found, info.d_deleted,
			info.f_found, info.f_deleted,
			scanner_err.size(), deleter_err.size());

	if (info.folders_togo) printf(msg(M_cryptic_togo), info.folders_togo);
}

//
void context::check_path()
{
	wstring full;

	if (! get_full_pathname(path, full))
	{
		printf(msg(M_err_fullpath), path_utf8.c_str());
		exit(RC_path_cant_expand);
	}

	path = full;

	//
	path_attrs = elp->GetFileAttributes(path.c_str());

	if (path_attrs == -1)
	{
		api_error e;

		e.code = GetLastError();
		if (__not_found(e.code))
		{
			printf(msg(M_err_not_found), path_utf8.c_str());
			exit(RC_path_not_found);
		}

		e.func = "GetFileAttributes";
		printf(msg(M_err_generic), error_to_str(e).c_str());
		printf(msg(M_path_label), path_utf8.c_str());
		exit(RC_path_cant_check);
	}

	is_a_file = ! (path_attrs & FILE_ATTRIBUTE_DIRECTORY);

	if (is_a_file && ! omni)
	{
		printf(msg(M_err_is_file), path_utf8.c_str());
		exit(RC_path_is_file);
	}
}

void context::check_restricted()
{
	if (yolo)
		return;

	vector<wstring> restricted;

	get_restricted_paths(restricted);

	for (auto & r : restricted)
	{
		if (_wcsnicmp(path.c_str(), r.c_str(), r.size()) != 0)
			continue;

		// match the prefix exactly or at a path component boundary
		if (path.size() != r.size() && path[r.size()] != L'\\')
			continue;

		abort(RC_path_restricted, msg(M_restricted), path_utf8.c_str());
	}
}

/*
 *	--trusted implies --force, which needs SeTakeOwnershipPrivilege /
 *	SeRestorePrivilege - only an elevated token holds them. When they
 *	are missing, relaunch the program elevated (UAC prompt), wait for
 *	it and propagate its exit code - this replaces the old nsudo
 *	wrapper for the context menu.
 */
static
bool process_can_take_ownership()
{
	HANDLE token = NULL;

	if (! OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
		return false;

	LUID luid;
	bool  held = false;

	if (LookupPrivilegeValueW(NULL, SE_TAKE_OWNERSHIP_NAME, &luid))
	{
		dword len = 0;

		GetTokenInformation(token, TokenPrivileges, NULL, 0, &len);

		if (len)
		{
			vector<char> buf(len);

			if (GetTokenInformation(token, TokenPrivileges, &buf[0], len, &len))
			{
				TOKEN_PRIVILEGES * tp = (TOKEN_PRIVILEGES *) &buf[0];

				for (dword i = 0; i < tp->PrivilegeCount; i++)
				{
					if (tp->Privileges[i].Luid.HighPart == luid.HighPart &&
					    tp->Privileges[i].Luid.LowPart  == luid.LowPart)
					{
						held = true; // present, enabled on demand later
						break;
					}
				}
			}
		}
	}

	CloseHandle(token);

	return held;
}

void context::maybe_elevate(int argc, wchar_t ** argv)
{
	if (! trusted || no_elevate || ! interactive || process_can_take_ownership())
		return;

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	wstring params;

	for (int i = 1; i < argc; i++)
	{
		wstring a = argv[i];

		for (size_t pos = 0; (pos = a.find(L'"', pos)) != -1; pos += 2)
			a.replace(pos, 1, L"\\\"");

		params += L'"' + a + L'"';

		if (i + 1 < argc)
			params += L' ';
	}

	wchar_t exe[MAX_PATH];

	if (! GetModuleFileNameW(NULL, exe, MAX_PATH))
		return;

	SHELLEXECUTEINFOW sei;

	memset(&sei, 0, sizeof sei);

	sei.cbSize       = sizeof sei;
	sei.fMask        = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;
	sei.lpVerb       = L"runas";
	sei.lpFile       = exe;
	sei.lpParameters = params.c_str();
	sei.nShow        = SW_SHOWNORMAL;

	// if the console belongs to this process alone (launched from the
	// context menu), dismiss it - the elevated instance gets its own
	bool  own_console = false;
	dword plist[2]    = { 0 };

	if (GetConsoleProcessList(plist, 2) == 1)
		own_console = true;

	if (own_console)
		FreeConsole();

	if (ShellExecuteExW(&sei) && sei.hProcess)
	{
		WaitForSingleObject(sei.hProcess, INFINITE);

		dword rc = RC_unlikely;

		GetExitCodeProcess(sei.hProcess, &rc);
		CloseHandle(sei.hProcess);

		exit(rc);
	}

	if (own_console)
		AttachConsole(ATTACH_PARENT_PROCESS); // best effort

	printf(msg(M_elev_refused));
	printf(msg(M_elev_hint));
	fflush(stdout);
}

void context::process()
{
	if (interactive)
		show_console_cursor(false);

	folder root;

	started = usec();
	root.self.name = path;
	root.self.info.attrs = path_attrs;

	init_progress();

	if (is_a_file)
	{
		delete_file();
	}
	else
	if (preview)
	{
		mode = 0x01;

		if (! ultra_mach_scan(root, mach_conf, this))
			exit(enough ? RC_unlikely : RC_cancelled);
	}
	else
	if (staged)
	{
		mode = 0x01;
		if (! ultra_mach_scan(root, mach_conf, this))
			exit(enough ? RC_unlikely : RC_cancelled);

		mode = 0x02;
		if (! ultra_mach_delete(root, true, mach_conf, this)) // prescanned
			exit(enough ? RC_unlikely : RC_cancelled);
	}
	else
	{
		mode = 0x03;
		if (! ultra_mach_delete(root, false, mach_conf, this)) // scan & delete
			exit(enough ? RC_unlikely : RC_cancelled);
	}

	finished = usec();
}

void context::delete_file()
{
	api_error_trace  err;
	WIN32_FIND_DATA  data;
	ultra_mach_info  temp;

	mode = preview ? 0x01 : 0x03;

	temp.f_found = 1;

	if (! get_file_info(path, data, &err))
	{
		temp.scanner_err = &err.all;
		goto out;
	}

	info.b_found = data.nFileSizeHigh;
	info.b_found <<= 32;
	info.b_found += data.nFileSizeLow;
	on_ultra_mach_tick(temp);

	if (preview)
		return;

	if (! ::delete_file(path, data.dwFileAttributes, mach_conf.deleter_ntapi, &err))
	{
		temp.deleter_err = &err.all;
		goto out;
	}

	info.f_deleted = 1;
	info.b_deleted += info.b_found;

out:
	on_ultra_mach_tick(temp);
}

void context::report()
{
	if (interactive)
		show_console_cursor(true);

	string elapsed   = format_usecs(finished - started);
	size_t err_count = scanner_err.size() + deleter_err.size();

	if (interactive)
	{
		if (cryptic)
		{
			move_console_cursor(0, false, -1, true);
			print_cryptic_stats();
			wipe_console_line();
			printf(msg(M_done_in), elapsed.c_str());
		}
		else
		{
			printf("\n");
			if (err_count && ! list_errors)
				printf(msg(M_completed_hint), elapsed.c_str());
			else
				printf(msg(M_completed), elapsed.c_str());
		}
	}
	else
	{
		if (cryptic)
		{
			print_cryptic_stats();
			printf(msg(M_done_in), elapsed.c_str());
		}
		else
		{
			print_verbose_stats(true);
			print_verbose_stats(false);
			printf("\n");
			printf(msg(M_completed), elapsed.c_str());
		}
	}

	if (err_count)
	{
		if (list_errors)
			report_errors();

		for (exit_rc = RC_ok_with_errors; err_count >= 10; err_count /= 10)
			exit_rc++;
	}
}

//
bool operator < (const api_error & a, const api_error & b)
{
	if (a.code != b.code)
		return a.code < b.code;

	if (a.args != b.args)
		return a.args < b.args;

	return a.func < b.func;
}

void context::report_errors()
{
	set<api_error> all;
	dword current = -1;

	for (auto & e : scanner_err) all.insert(e);
	for (auto & e : deleter_err) all.insert(e);

	printf(msg(M_errors_block));

	for (auto & e : all)
	{
		if (e.code != current)
		{
			const char * fmt = (e.code < 0x10000000) ? "  Code %lu - %s\n" : "  Code %08lx - %s\n";
			wstring desc;

			if (! get_error_desc(e.code, desc))
				desc = to_utf16(msg(M_no_desc));

			printf(fmt, e.code, to_utf8(desc).c_str());

			current = e.code;
		}

		printf("    %s\n", e.args.c_str());
	}
}

/*
 *
 */
int wmain_app(int argc, wchar_t ** argv)
{
	context x;

	// language: auto-detect, then honor an explicit --lang= option
	// (pre-scanned so that even the early messages are localized)
	lang_init();

	for (int i = 1; i < argc; i++)
	{
		if (! wcsncmp(argv[i], L"--lang=", 7))
		{
			if (! _wcsicmp(argv[i] + 7, L"fr"))
				g_lang = LANG_FR;
			else
			if (! _wcsicmp(argv[i] + 7, L"en"))
				g_lang = LANG_EN;
		}
	}

	x.init();

	x.parse_args(argc, argv);

	// --journal keeps the console window open (when the program owns
	// it) so that the summary can be read before it closes
	if (x.journal && x.interactive)
	{
		dword plist[2] = { 0 };

		if (GetConsoleProcessList(plist, 2) == 1)
			atexit(press_enter_to_exit);
	}

	x.check_path();

	x.check_restricted();

	x.maybe_elevate(argc, argv);

	x.confirm_it();

	x.process();

	x.report();

	return x.exit_rc;
}
