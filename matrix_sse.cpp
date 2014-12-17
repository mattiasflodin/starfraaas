#include "matrix.hpp"

matrix4f& matrix4f::operator*=(matrix4f const& rhs)
{
    // a b c d     A B C D
    // e f g h     E F G H
    // i j k l  *  I J K L
    // m n o p     M N O P

    __m128 const rrow0 = rhs[0].m128();
    __m128 const rrow1 = rhs[1].m128();
    __m128 const rrow2 = rhs[2].m128();
    __m128 const rrow3 = rhs[3].m128();
    for(std::size_t row=0; row!=4; ++row)
    {
        // We want the result row with index [row].
        // Given the row <a b c d> with index [row] in *this, we want to sum
        // up:
        // aA aB aC aD
        // bE bF bG bH
        // cI cJ cK cL
        // dM dN dO dP
        __m128 lrow = rows_[row].m128();
        __m128 t0, t1;
        t0 = _mm_shuffle_ps(lrow, lrow, _MM_SHUFFLE(0, 0, 0, 0));
        t0 = _mm_mul_ps(t0, rrow0);
        t1 = _mm_shuffle_ps(lrow, lrow, _MM_SHUFFLE(1, 1, 1, 1));
        t0 = _mm_add_ps(t0, _mm_mul_ps(t1, rrow1));
        t1 = _mm_shuffle_ps(lrow, lrow, _MM_SHUFFLE(2, 2, 2, 2));
        t0 = _mm_add_ps(t0, _mm_mul_ps(t1, rrow2));
        t1 = _mm_shuffle_ps(lrow, lrow, _MM_SHUFFLE(3, 3, 3, 3));
        t0 = _mm_add_ps(t0, _mm_mul_ps(t1, rrow3));
        rows_[row] = vector4f(t0);
    }
    return *this;
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

