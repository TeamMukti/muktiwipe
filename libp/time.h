/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Monotonic microsecond clock.
 */
#ifndef _LIBP_TIME_H_
#define _LIBP_TIME_H_

#include "types.h"
#include <windows.h>

struct usec_t
{
	uint64_t raw;

	usec_t() : raw(0) { }

	uint64_t operator - (const usec_t & x) const { return raw - x.raw; }
	bool     operator < (const usec_t & x) const { return raw < x.raw; }
	bool     operator > (const usec_t & x) const { return raw > x.raw; }
};

inline usec_t usec()
{
	static LARGE_INTEGER freq = []()
		{
			LARGE_INTEGER f;
			QueryPerformanceFrequency(&f);
			return f;
		} ();

	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	usec_t t;
	t.raw = (uint64_t) (counter.QuadPart / freq.QuadPart * 1000000) +
			(uint64_t) ((counter.QuadPart % freq.QuadPart) * 1000000 / freq.QuadPart);
	return t;
}

#endif
