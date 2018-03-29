#ifndef MoeLP_Base_ThreadPool
#define MoeLP_Base_ThreadPool

#include "../Base.hpp"

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

namespace MoeLP
{
	template<typename F>
	class SyncQueue
	{
	public:
		SyncQueue(size_t maxSize)
			: maxSize(maxSize),
			stopFlag(false)
		{
		}

		~SyncQueue()
		{
		}

		void add(F&& func)
		{
			std::unique_lock<std::mutex> locker(mutex);
			fullCondition.wait(locker, [this] { return stopFlag || (queue_.size() <= maxSize); });

			if (stopFlag)
				return;

			queue_.push(std::forward<F>(func));
			emptyCondition.notify_one();
		}

		void get(F& func)
		{
			std::unique_lock<std::mutex> locker(mutex);
			emptyCondition.wait(locker, [this] { return stopFlag || (!queue_.empty()); });

			if (stopFlag)
				return;

			func = std::move(queue_.front());
			queue_.pop();
			fullCondition.notify_one();
		}

		void stop()
		{
			{
				std::lock_guard<std::mutex> locker(mutex);
				stopFlag = true;
			}
			fullCondition.notify_all();
			emptyCondition.notify_all();
		}

		bool empty()
		{
			std::lock_guard<std::mutex> locker(mutex);
			return queue_.empty();
		}

	private:
		size_t maxSize;
		bool stopFlag;

		std::queue<F> queue_;
		std::mutex mutex;
		std::condition_variable emptyCondition;
		std::condition_variable fullCondition;
	};

	class ThreadPool 
	{
	public:
		static const size_t maxTaskNum = 100;

		ThreadPool(size_t threads = std::thread::hardware_concurrency())
			: tasks(maxTaskNum), stop(false)
		{
			for (size_t i = 0; i<threads; i++)
				workers.emplace_back(
					[this]
			{
				for (;;)
				{
					std::function<void()> task;

					{
						std::unique_lock<std::mutex> lock(this->queue_mutex);
						this->condition.wait(lock,
							[this] { return this->stop || !this->tasks.empty(); });
						if (this->stop && this->tasks.empty())
							return;
						tasks.get(task);
					}

					task();
				}
			}
			);
		}

		template<class F, class... Args>
		auto add(F&& f, Args&&... args)
			-> std::future<typename std::result_of<F(Args...)>::type>
		{
			using return_type = typename std::result_of<F(Args...)>::type;

			auto task = std::make_shared<std::packaged_task<return_type()>>(
				std::bind(std::forward<F>(f), std::forward<Args>(args)...)
				);

			std::future<return_type> res = task->get_future();
			{
				std::unique_lock<std::mutex> lock(queue_mutex);

				MOE_ERROR(!stop, "Add a task to a stopped threadpool.");

				tasks.add([task]() { (*task)(); });
			}
			condition.notify_one();
			return res;
		}

		~ThreadPool()
		{
			{
				std::unique_lock<std::mutex> lock(queue_mutex);
				stop = true;
			}
			condition.notify_all();
			tasks.stop();
			for (std::thread &worker : workers)
				worker.join();
		}

	private:
		std::vector<std::thread> workers;
		SyncQueue<std::function<void()>> tasks;

		std::mutex queue_mutex;
		std::condition_variable condition;
		bool stop;
	};
}

#endif