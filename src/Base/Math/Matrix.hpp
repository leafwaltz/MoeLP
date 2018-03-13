#ifndef MoeLP_Base_Matrix
#define MoeLP_Base_Matrix

#include "../../../3rdParty/eigen/Eigen/Dense"

namespace MoeLP
{
	template<typename Scalar, int Rows, int Cols, int Options = 0>
	class Matrix : public Eigen::Matrix<Scalar, Rows, Cols, Options>
	{

	};
}


#endif
