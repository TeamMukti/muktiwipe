/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Simple work queue - a pool of worker threads that executes
 *	queued work items and hands the completed ones back to the
 *	caller of collect().
 */
#ifndef _LIBP_SIMPLE_WORK_QUEUE_H_
#define _LIBP_SIMPLE_WORK_QUEUE_H_

#include "types.h"
#include <mutex>
#include <condition_variable>
#include <thread>

//
struct work_item
{
	virtual ~work_item() { }
	virtual void execute() = 0;
};

typedef vector<work_item *> work_item_vec;

//
struct simple_work_queue
{
	simple_work_queue();
	~simple_work_queue();

	__no_copying(simple_work_queue);

	// starts the specified number of worker threads
	bool init(size_t threads, void * ctx);

	// queues a work item for execution
	void enqueue(work_item * item);

	// waits for completed work items and returns up to max_count
	// of them in 'out'
	void collect(work_item_vec & out, size_t max_count);

	// stops the workers, drains all queued (including completed but
	// not yet collected) items into 'out' and returns
	void cancel(work_item_vec & out);

//
	void worker();

	size_t                threads;
	vector<thread>        workers;

	std::mutex            mx;
	std::condition_variable  cv_work;  // wakes idle workers
	std::condition_variable  cv_done;  // wakes collect()

	std::deque<work_item *>  pending;
	std::deque<work_item *>  done;

	bool                  stopping;
	bool                  inited;
};

#endif
