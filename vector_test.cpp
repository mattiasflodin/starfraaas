#include "vector.hpp"
#include "matrix.hpp"
#include <iostream>

vector4f foo(vector4f const& v)
{
    return normalize(v);
}

int main()
{
    vector4f v(0.0f, 1.0f, 2.0f, 3.0f);
    vector4f u(4.0f, 5.0f, 6.0f, 7.0f);
    vector4f w(_mm_shuffle_ps(v.m128(), u.m128(), _MM_SHUFFLE(0, 0, 0, 1)));
    std::cout << v << std::endl;
    std::cout << u << std::endl;
    std::cout << w << std::endl;

    return 0;
}
