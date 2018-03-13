#ifndef MoeLP_Base_Traits
#define MoeLP_Base_Traits

#include "Base.hpp"

namespace MoeLP
{
	template<typename T>
	class has_default_constructor
	{
		template<typename C, typename = decltype(C())> static muint8 test(int);
		template<typename C> static muint16 test(...);

	public:
		static const bool value = sizeof(test<T>(0)) == 1;
	};
}

#endif