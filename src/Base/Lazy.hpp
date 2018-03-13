#ifndef MoeLP_Base_Lazy
#define MoeLP_Base_Lazy

#include "Base.hpp"
#include "Memory.hpp"

#include <functional>
#include <type_traits>

namespace MoeLP
{
	template<typename T>
	class Lazy
	{
		struct Data
		{
			std::function<T()> evaluator;
			bool evaluated;
			T value;
		};

	public:
		Lazy() {}

		Lazy(const Lazy<T>& other)
			: data(other.data)
		{
		}

		/**
		 * @brief create an lazy evaluator from a function
		 * @param evaluator: the function
		 */
		Lazy(const std::function<T()>& evaluator)
		{
			data = Ptr<Data>::create(sizeof(Data));
			data->evaluator = evaluator;
			data->evaluated = false;
		}

		/**
		 * @brief create an lazy evaluator from a function
		 * @param func: the function
		 * @param args: the args of the function
		 */
		template<typename Function, typename... Args>
		Lazy(Function& func, Args&& ... args)
		{
			data = Ptr<Data>::create(sizeof(Data));
			data->evaluator = [&func, &args...]{ return func(args...); };
			data->evaluated = false;
		}

		Lazy<T>& operator=(const Lazy<T>& other)
		{
			data = other.data;
			return *this;
		}

		Lazy<T>& operator=(const std::function<T()>& evaluator)
		{
			data = Ptr<Data>::create(sizeof(Data));
			data->evaluator = evaluator;
			data->evaluated = false;
			return *this;
		}

		const T& value() const
		{
			if (!data->evaluated)
			{
				data->evaluated = true;
				data->value = data->evaluator();
			}

			return data->value;
		}

		const bool isEvaluated() const
		{
			return data->evaluated;
		}
		
		const bool empty() const
		{
			return !data;
		}

	private:
		Ptr<Data> data;
	};

	template<typename Function, typename... Args>
	Lazy<typename std::result_of<Function(Args...)>::type>
		lazy(Function&& func, Args&& ... args)
	{
		return Lazy<typename std::result_of<Function(Args...)>::type>
			(std::forward<Function>(func),
				std::forward<Args>(args)...);
	}
}
#endif