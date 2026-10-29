/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Bilingual user messages (English / French). The language is
 *	auto-detected from the Windows UI language and can be forced
 *	with the --lang=fr|en command line option.
 */
#ifndef _LIBP_LANG_H_
#define _LIBP_LANG_H_

#include "types.h"

//
enum LANG_ID
{
	LANG_EN = 0,
	LANG_FR = 1,
};

extern int g_lang;

// auto-detects the language from the system settings
void lang_init();

//
enum MESSAGE
{
	M_null = 0,

	M_header,
	M_syntax,
	M_nt_api_missing,
	M_scan_buf_max,
	M_root_not_supported,
	M_restricted,
	M_scanning,
	M_deleting,
	M_staged_tag,
	M_col_folders,
	M_col_files,
	M_col_bytes,
	M_col_errors,
	M_found_h,
	M_deleted_h,
	M_found,
	M_deleted,
	M_togo,
	M_cryptic,
	M_cryptic_togo,
	M_err_fullpath,
	M_err_not_found,
	M_err_generic,
	M_path_label,
	M_err_is_file,
	M_confirm_folder,
	M_confirm_file,
	M_confirm_prompt,
	M_cancelled,
	M_completed_hint,
	M_completed,
	M_done_in,
	M_errors_block,
	M_no_desc,
	M_press_enter,
	M_elev_refused,
	M_elev_hint,
	M_unlocked,

	M_count
};

const char * msg(MESSAGE id);

#endif
