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

    static matrix4f const& identity()
    {
        static float values[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        static matrix4f const m(values);
        return m;
    }

	matrix4f(float const* rows)
	{
		for(size_t row=0; row!=4; ++row)
		{
			rows_[row][0] = rows[row*4 + 0];
			rows_[row][1] = rows[row*4 + 1];
			rows_[row][2] = rows[row*4 + 2];
			rows_[row][3] = rows[row*4 + 3];
		}
	}

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

    matrix4f& operator*=(matrix4f const& rhs)
	{
		return *this = *this * rhs;
	}

    matrix4f operator*(matrix4f const& rhs) const;

private:
    vector4f rows_[4];
};

std::ostream& operator<<(std::ostream& os, matrix4f const& m);

#endif
