/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Simple work queue.
 */
#include "_simple_work_queue.h"

//
simple_work_queue::simple_work_queue()
{
	threads = 0;
	stopping = false;
	inited = false;
}

simple_work_queue::~simple_work_queue()
{
	work_item_vec out;

	cancel(out);
}

bool simple_work_queue::init(size_t _threads, void * ctx)
{
	(void) ctx;

	if (inited)
		return true;

	if (! _threads)
		return false;

	stopping = false;

	try
	{
		workers.reserve(_threads);

		for (size_t i=0; i < _threads; i++)
			workers.push_back(thread([this] { worker(); }));
	}
	catch (...)
	{
		// could not start all workers - shut down the ones that run
		stopping = true;
		cv_work.notify_all();

		for (auto & t : workers)
			if (t.joinable()) t.join();

		workers.clear();

		return false;
	}

	threads = _threads;
	inited = true;

	return true;
}

void simple_work_queue::worker()
{
	for (;;)
	{
		work_item * item = NULL;

		{
			unique_lock<std::mutex> lock(mx);

			cv_work.wait(lock, [this] { return stopping || ! pending.empty(); });

			if (stopping)
				return;

			item = pending.front();
			pending.pop_front();
		}

		item->execute();

		{
			std::lock_guard<std::mutex> lock(mx);

			done.push_back(item);
		}

		cv_done.notify_one();
	}
}

void simple_work_queue::enqueue(work_item * item)
{
	{
		std::lock_guard<std::mutex> lock(mx);

		pending.push_back(item);
	}

	cv_work.notify_one();
}

void simple_work_queue::collect(work_item_vec & out, size_t max_count)
{
	unique_lock<std::mutex> lock(mx);

	cv_done.wait(lock, [this] { return ! done.empty(); });

	while (! done.empty() && out.size() < max_count)
	{
		out.push_back(done.front());
		done.pop_front();
	}
}

void simple_work_queue::cancel(work_item_vec & out)
{
	if (! inited)
		return;

	{
		std::lock_guard<std::mutex> lock(mx);

		stopping = true;

		while (! pending.empty()) { out.push_back(pending.front()); pending.pop_front(); }
		while (! done.empty())    { out.push_back(done.front());    done.pop_front();    }
	}

	cv_work.notify_all();

	for (auto & t : workers)
		if (t.joinable()) t.join();

	// workers finish the item they are running before they exit,
	// such items land in done() and are picked up here
	{
		std::lock_guard<std::mutex> lock(mx);

		while (! done.empty()) { out.push_back(done.front()); done.pop_front(); }
	}

	workers.clear();

	threads = 0;
	inited = false;
}
