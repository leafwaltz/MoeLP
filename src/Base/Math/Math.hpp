#ifndef MoeLP_Base_Math
#define MoeLP_Base_Math

#include "Matrix.hpp"
#include "Vector.hpp"

#include <cmath>

#include "../Base.hpp"

namespace MoeLP
{
	inline double logSumExp(double x, double y)
	{
		const double a = max(x, y);
		return a + std::log(std::exp(x - a) + std::exp(y - a));
	}

	template<typename ...Other>
	inline double logSumExp(double arg1, double arg2, Other... other)
	{
		return logSumExp(logSumExp(arg1, arg2), other...);
	}
}


#endif
