#include "vector.hpp"

std::ostream& operator<<(std::ostream& os, vector4f const& v)
{
    os << '<' << v[0] << ' ' << v[1] << ' ' << v[2] << ' ' << v[3] << '>';
    return os;
}

