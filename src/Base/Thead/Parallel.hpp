#ifndef MoeLP_Base_Parallel
#define MoeLP_Base_Parallel

#include "../Base.hpp"
#include "../Range.hpp"

#include <thread>
#include <vector>
#include <future>

namespace MoeLP
{
	/**
	 * @brief a multithread parallel foreach
	 * @param begin: the begin of the iterator
	 * @param end: the end of the iterator
	 * @param op: the operation in foreach
	 * @detail: you can set the template parameter ThreadNum to change the number of threads used in parallel foreach,
	 * by default or when set 0 it will be equal with the cpu thread number.
	 * @example: parallel_foreach<4>(it.begin(), it.end(), []{...}) means there will be 4 threads used
	 */
	template<size_t ThreadNum = 0, class Iterator, class Operation>
	void parallelForeach(Iterator begin, Iterator end, Operation op)
	{
		size_t threadNum = ThreadNum;

		if (ThreadNum == 0)
			threadNum = std::thread::hardware_concurrency();

		size_t batchSize = std::distance(begin, end) / threadNum;

		Iterator lastBatch = begin;

		if (batchSize > 0)
		{
			std::advance(lastBatch, batchSize*(threadNum - 1));

			std::vector<std::future<void>> temp;

			for (; begin != lastBatch; std::advance(begin, batchSize))
			{
				temp.emplace_back(std::async(
					[begin, batchSize, &op]
				{
					std::for_each(begin, begin + batchSize, op);
				}
				));
			}

			temp.emplace_back(std::async(
				[&begin, &end, &op]
			{
				std::for_each(begin, end, op);
			}
			));

			std::for_each(temp.begin(), temp.end(),
				[](std::future<void>& future)
			{
				future.get();
			}
				);
		}
		else
		{
			std::for_each(begin, end, op);
		}
	}
}

#endif