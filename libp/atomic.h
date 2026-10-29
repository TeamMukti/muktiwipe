/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Atomic operations on plain integer members. All counters used by
 *	muktiwipe are naturally aligned, so aliasing them to std::atomic is
 *	safe and lock-free on x86/x64.
 */
#ifndef _LIBP_ATOMIC_H_
#define _LIBP_ATOMIC_H_

#include <atomic>

template <class T>
void atomic_inc(T * val)
{
	reinterpret_cast< std::atomic<T> * >(val)->fetch_add(1, std::memory_order_relaxed);
}

template <class T>
void atomic_dec(T * val)
{
	reinterpret_cast< std::atomic<T> * >(val)->fetch_sub(1, std::memory_order_relaxed);
}

template <class T, class V>
void atomic_add(T * val, V add)
{
	reinterpret_cast< std::atomic<T> * >(val)->fetch_add((T)add, std::memory_order_relaxed);
}

#endif
