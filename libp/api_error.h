/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Uniform reporting of Win32 and NT API errors.
 *
 *	api_error.code holds either a Win32 error code (below 0x10000) or
 *	an NTSTATUS value (0xF0000000 bit pattern set). api_error.args is
 *	the UTF-8 text of the arguments of the failed call, typically the
 *	path it was invoked on.
 */
#ifndef _LIBP_API_ERROR_H_
#define _LIBP_API_ERROR_H_

#include "types.h"

//
struct api_error
{
	dword   code;
	string  func;
	string  args;
};

typedef vector<api_error> api_error_vec;

//
struct api_error_cb
{
	virtual ~api_error_cb() { }
	virtual void on_api_error_x(const api_error & e) = 0;
};

//
struct api_error_trace : api_error_cb
{
	api_error_vec all;

	virtual void on_api_error_x(const api_error & e) { all.push_back(e); }
};

//
void api_error_report(api_error_cb * cb, dword code, const char * func, const wstring & args);

//
#define __on_api_error(func, args) \
	do { dword __err_code = GetLastError(); api_error_report(err, __err_code, func, args); } while (0)

#define __on_api_error_ex(func, code, args) \
	api_error_report(err, code, func, args)

// same as above, for error codes that were not fetched via GetLastError()
#define __on_api_error_code(func, code, args) \
	api_error_report(err, code, func, args)

#endif
