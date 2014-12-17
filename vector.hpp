#ifndef VECTOR_HPP
#define VECTOR_HPP
#include <cstdlib>
#include <xmmintrin.h>
#include <ostream>
#include <vector>

#include <cmath>

class vector4f {
public:
    vector4f()
    {
		v_[0] = 0;
		v_[1] = 0;
		v_[2] = 0;
		v_[3] = 0;
    }
    vector4f(float x, float y = 0, float z = 0, float w = 0)
    {
		v_[0] = x;
		v_[1] = y;
		v_[2] = z;
		v_[3] = w;
    }
    vector4f(float const* p)
    {
		v_[0] = p[0];
		v_[1] = p[1];
		v_[2] = p[2];
		v_[3] = p[3];
    }
    vector4f operator+(vector4f const& rhs) const
    {
		return vector4f(*this) += rhs;
    }
	vector4f& operator+=(vector4f const& rhs)
	{
		v_[0] += rhs.v_[0];
		v_[1] += rhs.v_[1];
		v_[2] += rhs.v_[2];
		v_[3] += rhs.v_[3];
		return *this;
	}
    vector4f operator*(vector4f rhs) const
    {
        return vector4f(*this) *= rhs;
    }
	vector4f& operator*=(vector4f const& rhs)
	{
		v_[0] += rhs.v_[0];
		v_[1] += rhs.v_[1];
		v_[2] += rhs.v_[2];
		v_[3] += rhs.v_[3];
	}
    vector4f operator*(float rhs) const
    {
        return vector4f(*this) *= rhs;
    }
	vector4f& operator*=(float rhs)
	{
		v_[0] *= rhs;
		v_[1] *= rhs;
		v_[2] *= rhs;
		v_[3] *= rhs;
		return *this;
	}

    float& operator[](std::size_t i)
    {
        return reinterpret_cast<float*>(&v_)[i];
    }
    float const& operator[](size_t i) const
    {
        return (*const_cast<vector4f*>(this))[i];
    }

	float const* data() const
	{
		return v_;
	}

private:
    float v_[4];
};

typedef std::vector<vector4f> vector4f_vector;

inline float rsqrt(float v)
{
    return 1.0f/sqrt(v);
}

inline float dot(vector4f const& lhs, vector4f const& rhs)
{
	return lhs[0]*rhs[0]
		+ lhs[1]*rhs[1]
		+ lhs[2]*rhs[2]
		+ lhs[3]*rhs[3];
}

inline float length(vector4f const& v)
{
    return sqrt(dot(v, v));
}

inline vector4f normalize(vector4f const& v)
{
    return v*rsqrt(dot(v, v));
}

std::ostream& operator<<(std::ostream& os, vector4f const& v);

#endif
