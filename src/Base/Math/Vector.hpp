#ifndef MoeLP_Base_Vector
#define MoeLP_Base_Vector

#include "../../../3rdParty/eigen/Eigen/Dense"

namespace MoeLP
{
	template<typename Scalar, int Dimension, int Options = 0>
	class Vector : public Eigen::Matrix<Scalar, Dimension, 1, Options>
	{
	};
}

#endif