#ifndef MoeLP_Base_Range
#define MoeLP_Base_Range

namespace MoeLP
{
	template<typename T>
	class Range
	{
		class Iterator
		{
		public:
			Iterator(size_t index, Range& range)
				: index(index), range_(range)
			{
				currentValue = range.begin_ + index * range_.step_;
			}

			T operator*()
			{
				return currentValue;
			}

			const Iterator* operator++()
			{
				currentValue += range_.step_;
				index++;
				return this;
			}

			const Iterator* operator--()
			{
				currentValue -= range_.step_;
				index--;
				return this;
			}

			bool operator==(const Iterator& other)
			{
				return (currentValue == other.currentValue) && (index == other.index);
			}

			bool operator!=(const Iterator& other)
			{
				return (currentValue != other.currentValue)|| (index != other.index);
			}

		private:
			int index;
			T currentValue;
			Range& range_;
		};

	public:
		Range(T begin, T end, T step = 1)
			: begin_(begin),
			end_(end),
			step_(step)
		{
			endIndex = (end_ - begin_) / step_;
			if (begin_ + step_ * endIndex != end_)
			{
				endIndex++;
			}
		}

		Iterator begin()
		{
			return Iterator(0, *this);
		}

		Iterator end()
		{
			return Iterator(endIndex, *this);
		}

	private:
		const T begin_;
		const T end_;
		const T step_;
		int endIndex;
	};
}

#endif