#include "matrix.hpp"

matrix4f matrix4f::operator*(matrix4f const& rhs) const
{
	matrix4f r;
	matrix4f const& lhs = *this;
    for(std::size_t row=0; row!=4; ++row)
    {
		for(std::size_t col=0; col!=4; ++col)
		{
			r[row][col] =
				lhs[row][0] * rhs[0][col]
				+ lhs[row][1] * rhs[1][col]
				+ lhs[row][2] * rhs[2][col]
				+ lhs[row][3] * rhs[3][col];
		}
    }
    return r;
}

std::ostream& operator<<(std::ostream& os, matrix4f const& m)
{
    for(std::size_t row=0; row!=4; ++row)
    {
        os << '|' << m[row][0] << ' ' << m[row][1] << ' ';
        os << m[row][2] << ' ' << m[row][3] << "|\n";
    }
    return os;
}

