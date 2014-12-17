#include <iostream>
#include "matrix.hpp"

void foo(matrix4f& res)
{
    matrix4f m1;
    m1 = 1, 0, 0, 0,
         0, 1, 0, 0,
         0, 0, 1, 0,
         0, 0, 0, 1;
    matrix4f m2;
    m2 = 1, 0, 0, 1,
         0, 1, 0, 2,
         0, 0, 1, 3,
         0, 0, 0, 1;
    res = m2 * m2;
}

int main()
{
    matrix4f m;
    foo(m);
    std::cout << m << std::flush;
}
