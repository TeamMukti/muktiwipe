/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Uniform reporting of Win32 and NT API errors.
 */
#include "api_error.h"

#include "string_utils.h"

void api_error_report(api_error_cb * cb, dword code, const char * func, const wstring & args)
{
	if (! cb)
		return;

	api_error e;

	e.code = code;
	e.func = func;
	e.args = to_utf8(args);

	cb->on_api_error_x(e);
}
