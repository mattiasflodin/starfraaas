#include <cassert>

template <typename T, typename U>
struct down_cast_helper;

template <typename T, typename U>
struct down_cast_helper<T&, U>
{
    struct enabler {};
    static T& apply(U& v)
    {
#ifdef DEBUG
        T* cast_result = dynamic_cast<T*>(&v);
        assert(cast_result);
        return *cast_result;
#else
        return static_cast<T&>(v);
#endif
    }
};
template <typename T, typename U>
struct down_cast_helper<T*, U*>
{
    struct enabler {};
    static T* apply(U* p)
    {
#ifdef DEBUG
        T* cast_result = dynamic_cast<T*>(p);
        assert(cast_result);
        return cast_result;
#else
        return static_cast<T*>(p);
#endif
    }
};

template <typename T, typename U>
inline T down_cast(U& p, typename down_cast_helper<T, U>::enabler* = 0)
{
    return down_cast_helper<T, U>::apply(p);
}

template <typename T, typename U>
struct serialize_cast_helper;

template <typename T>
struct serialize_cast_helper<char*, T*> {
    struct enabler {};
    static char* apply(T* p)
    {
        return static_cast<char*>(static_cast<void*>(p));
    }
};

template <typename T>
struct serialize_cast_helper<T*, char*> {
    struct enabler {};
    static T* apply(char* p)
    {
        return static_cast<T*>(static_cast<void*>(p));
    }
};

template <typename T>
struct serialize_cast_helper<char const*, T const*> {
    struct enabler {};
    static char const* apply(T const* p)
    {
        return static_cast<char const*>(static_cast<void const*>(p));
    }
};

template <typename T>
struct serialize_cast_helper<T const*, char const*> {
    struct enabler {};
    static T const* apply(char const* p)
    {
        return static_cast<T const*>(static_cast<void const*>(p));
    }
};

template <typename T, typename U>
inline T serialize_cast(U p,
        typename serialize_cast_helper<T, U>::enabler* = 0)
{
    return serialize_cast_helper<T, U>::apply(p);
}

#define FOR_EACH(container_type, it, container) \
	for(container_type::const_iterator it(container.begin()), \
		it##_end(container.end()); \
		it!=it##_end; ++it)