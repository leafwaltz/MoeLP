#ifndef MoeLP_Base_Task
#define MoeLP_Base_Task

#include <functional>
#include <future>

namespace MoeLP
{
	template<typename T>
	class Task;

	template<typename ReturnType, typename... ArgTypes>
	class Task<ReturnType(ArgTypes...)>
	{
	public:
		typedef ReturnType resultType;

		Task(std::function<ReturnType(ArgTypes...)>&& func)
			: task(std::move(func))
		{
		}

		Task(std::function<ReturnType(ArgTypes...)>& func)
			: task(func)
		{
		}

		~Task()
		{
		}

		void wait()
		{
			std::async(task).wait();
		}

		template<typename... Args>
		ReturnType get(Args&&... args)
		{
			return std::async(task, std::forward<Args>(args)...).get();
		}

		std::shared_future<ReturnType> run()
		{
			return std::async(task);
		}

		template<typename F>
		auto then(F&& func) -> Task<typename std::result_of<F(ReturnType)>::type(ArgTypes...)>
		{
			typedef typename std::result_of<F(ReturnType)>::type returnType;

			auto temp = std::move(task);

			return Task<returnType(ArgTypes...)>(
				[temp, &func](ArgTypes&&... args)
				{
					std::future<ReturnType> lastFunc = std::async(temp, std::forward<ArgTypes>(args)...);
					return std::async(func, lastFunc.get()).get();
				}
			);
		}

	private:
		std::function<ReturnType(ArgTypes...)> task;
	};
}

#endif