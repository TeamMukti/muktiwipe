/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Enforcement of runtime invariants.
 *
 *	on_assert() is implemented by the application (see wmain.cpp).
 */
#ifndef _LIBP_ENFORCE_H_
#define _LIBP_ENFORCE_H_

void on_assert(const char * exp, const char * file, const char * func, int line);

#define __enforce(x) \
	do { if (! (x)) on_assert(#x, __FILE__, __func__, __LINE__); } while (0)

#endif
