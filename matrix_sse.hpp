#ifndef MATRIX_HPP
#define MATRIX_HPP
#include "vector.hpp"


namespace detail {
    template <std::size_t index>
    class initializer {
    public:
        initializer(vector4f* rows) : rows_(rows)
        {
        }
        initializer<index+1> operator,(float v)
        {
            rows_[index/4][index%4] = v;
            return initializer<index+1>(rows_);
        }
    private:
        vector4f* rows_;
    };
    template <>
    class initializer<16> {
    public:
        initializer(vector4f*)
        {
        }
    };
}

class matrix4f {
public:
    matrix4f()
    {
    }

    /*static matrix4f const& identity()
    {
        static float values[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        static matrix4f const m(values);
        return m;
    }*/

    detail::initializer<1> operator=(float v)
    {
        rows_[0][0] = v;
        return detail::initializer<1>(rows_);
    }
    vector4f& operator[](std::size_t i)
    {
        return rows_[i];
    }
    vector4f const& operator[](std::size_t i) const
    {
        return rows_[i];
    }

    matrix4f& operator*=(matrix4f const& rhs);

    matrix4f operator*(matrix4f const& rhs) const
    {
        return matrix4f(*this)*=rhs;
    }

private:
    vector4f rows_[4];
};

std::ostream& operator<<(std::ostream& os, matrix4f const& m);

#endif
