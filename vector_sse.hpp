#ifndef VECTOR_HPP
#define VECTOR_HPP
#include <cstdlib>
#include <xmmintrin.h>
#include <ostream>
#include <vector>

class scalar4f {
public:
    scalar4f() :
        v_(_mm_setzero_ps())
    {
    }
    scalar4f(float rhs) :
        v_(_mm_set_ps(0.0f, 0.0f, 0.0f, rhs))
    {
    }
    scalar4f(__m128 rhs) :
        v_(rhs)
    {
    }

    scalar4f operator+(scalar4f rhs)
    {
        return scalar4f(_mm_add_ss(v_, rhs.v_));
    }

    operator float() const
    {
        return *reinterpret_cast<float const*>(&v_);
    }
    __m128 m128() const
    {
        return v_;
    }

private:
    __m128 v_;
};

class vector4f {
public:
    vector4f() :
        v_(_mm_setzero_ps())
    {
    }
    vector4f(float x, float y = 0, float z = 0, float w = 0) :
        v_(_mm_set_ps(w, z, y, x))
    {
    }
    vector4f(float const* p) :
        v_(_mm_loadu_ps(p))
    {
    }
    vector4f(__m128 const* p) :
        v_(*p)
    {
    }
    vector4f(__m128 v) :
        v_(v)
    {
    }
    static vector4f aligned(float const* p)
    {
        return vector4f(_mm_load_ps(p));
    }

    vector4f operator+(vector4f rhs) const
    {
        return vector4f(_mm_add_ps(v_, rhs.v_));
    }
    vector4f operator*(vector4f rhs) const
    {
        return vector4f(_mm_mul_ps(v_, rhs.v_));
    }
    vector4f operator*(scalar4f const& rhs) const
    {
        __m128 const v = rhs.m128();
        return (*this)*vector4f(_mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0)));
    }

    float& operator[](std::size_t i)
    {
        return reinterpret_cast<float*>(&v_)[i];
    }
    float const& operator[](size_t i) const
    {
        return (*const_cast<vector4f*>(this))[i];
    }
    __m128 m128() const
    {
        return v_;
    }

private:
    __m128 v_;
};

template<typename T>
class aligned_allocator
{
public:
    typedef size_t     size_type;
    typedef ptrdiff_t  difference_type;
    typedef T*       pointer;
    typedef const T* const_pointer;
    typedef T&       reference;
    typedef const T& const_reference;
    typedef T        value_type;

    template<typename _Tp1>
    struct rebind
    { typedef aligned_allocator<_Tp1> other; };

    aligned_allocator() throw() { }

    aligned_allocator(const aligned_allocator&) throw() { }

    template<typename T1>
    aligned_allocator(const aligned_allocator<T1>&) throw() { }

    ~aligned_allocator() throw() { }

    pointer address(reference x) const { return &x; }

    const_pointer address(const_reference x) const { return &x; }

    pointer allocate(size_type n, const void* = 0)
    {
        if (__builtin_expect(n > this->max_size(), false))
            std::__throw_bad_alloc();

        void* ptr;
        if(0 != posix_memalign(&ptr, 16, n*sizeof(T)))
            std::__throw_bad_alloc();
        return static_cast<T*>(ptr);
    }

    // p is not permitted to be a null pointer.
    void deallocate(pointer p, size_type)
    { free(p); }

    size_type max_size() const throw() 
    { return size_t(-1) / sizeof(T); }

    void construct(pointer p, const T& __val) 
    { ::new (p) T(__val); }

    void destroy(pointer p) { p->~T(); }
};

typedef aligned_allocator<vector4f> vector4f_allocator;
typedef std::vector<vector4f, vector4f_allocator> vector4f_vector;

inline scalar4f sqrt(scalar4f const& v)
{
    return scalar4f(_mm_sqrt_ss(v.m128()));
}

inline scalar4f rsqrt(scalar4f const& v)
{
    return scalar4f(_mm_rsqrt_ss(v.m128()));
}

inline scalar4f dot(vector4f const& lhs, vector4f const& rhs)
{
    __m128 acc = (lhs*rhs).m128();
    // <a b c d> + <b a c d> -> <a+b a+b c+d c+d>
    acc = _mm_add_ps(acc, _mm_shuffle_ps(acc, acc, _MM_SHUFFLE(2, 3, 0, 1)));
    // <... c+d> + <... a+b> -> <... a+b+c+d>
    return scalar4f(_mm_add_ss(acc, _mm_shuffle_ps(acc, acc,
        _MM_SHUFFLE(1, 0, 2, 3))));
}

inline scalar4f length(vector4f const& v)
{
    return sqrt(dot(v, v));
}

inline vector4f normalize(vector4f const& v)
{
    return v*rsqrt(dot(v, v));
}

std::ostream& operator<<(std::ostream& os, vector4f const& v);

#endif
