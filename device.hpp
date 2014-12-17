#ifndef DEVICE_HPP
#define DEVICE_HPP
// Use cases
//  particle system
//  checkpoint
//  ordinary object with multiple materials
//  subgraph that gets rendered to a texture
// Assumptions/goals
//  A particular rendering strategy can be very tied to hardware
//  Shader / texture sorting ought to penetrate objects
//  Some way of controlling all object parameters via animator(?)
//
// Thoughts
//  Should render() call be replaced by controlled insertion into scene graph?
//  In that case, what exactly is inserted? The strategy and the parameters: a
//  primitive.
//
// Resource: Anything that is allocated on the device. Not things like
// parameters.

#include <cstdlib>
#include <boost/cstdint.hpp>
#include "vector.hpp"

class device;
class state_set;
class vertex_program;
class fragment_shader;

typedef boost::uint_fast16_t uint_fast16_t;

namespace vertex_attribute
{
    // TODO these should all be fixed in size (e.g. INT16 instead of SHORT)
    enum type
    {
        FLOAT1,
        FLOAT2,
        FLOAT3,
        FLOAT4,
        COLOR,
        UBYTE4,
        SHORT2,
        SHORT4,
        UBYTE4N,
        SHORT2N,
        SHORT4N,
        USHORT2N,
        USHORT4N,
        UDEC3N,
        DEC3N,
        FLOAT16_2,
        FLOAT16_4,
        UNUSED
    };

    enum StandardAttribute
    {
        POSITION,
        WEIGHTS,
        NORMAL,
        PRIMARY_COLOR,
        SECONDARY_COLOR,
        FOG_COORDINATE,

        TEXTURE_COORDINATE_0,
        TEXTURE_COORDINATE_1,
        TEXTURE_COORDINATE_2,
        TEXTURE_COORDINATE_3,
        TEXTURE_COORDINATE_4,
        TEXTURE_COORDINATE_5,
        TEXTURE_COORDINATE_6,
        TEXTURE_COORDINATE_7,

        N_STANDARD_ATTRIBUTES
    };
}
namespace standard_attribute {
    // Flags for get_standard_vertex_declaration.
    enum flags
    {
        DIFFUSE  = 1 << 0,
        NORMAL   = 1 << 1,
        SPECULAR = 1 << 2,
        // TODO rename to POSITION
        XYZ      = 1 << 3,
        TEX0     = 1 << 4,
        TEX1     = 1 << 5,
        TEX2     = 1 << 6,
        TEX3     = 1 << 7,
        TEX4     = 1 << 8,
        TEX5     = 1 << 9,
        TEX6     = 1 << 10,
        TEX7     = 1 << 11,
        TEX8     = 1 << 12,
    };
}
struct vertex_declaration {
    std::size_t stream;
    std::size_t attribute;
    std::size_t offset;
    vertex_attribute::type type;
};

void get_standard_vertex_declaration(uint_fast16_t flags,
        vertex_declaration const** ppdeclaration, std::size_t* count);
class vertex_producer {
public:
    virtual ~vertex_producer();
    virtual std::size_t vertex_count() = 0;
    virtual void get_vertex_declaration(vertex_declaration const**,
            std::size_t* count) = 0;
    virtual void produce_vertices(std::size_t stream, void* data) = 0;
};

class texture_producer {
public:
	enum Format {
		RGB,
		RGBA
	};
	struct description
	{
		Format format;
		size_t width;
		size_t height;
	};
    virtual ~texture_producer();
	virtual void get_texture_description(description* pdesc) = 0;
    virtual void produce_texture(void* data) = 0;
};

class index_producer {
public:
    virtual ~index_producer();
    virtual std::size_t index_count() = 0;
    virtual void produce_indices(void* data) = 0;
};

// singleton
class strategy {
public:
    strategy(int const* resource_map);
    virtual ~strategy() = 0;
    //virtual void preload(resource_producer*);
    //virtual void init(scene*, resource_set);
    virtual void update(state_set*);
    virtual void present(device*, state_set*);
private:
    int const* const resource_map_;
};

class vertex_program {
public:
    enum language_t {
        CG,
        GLSL,
        HLSL,
        VP_1_0,
        VS_1_0
    };
    virtual ~vertex_program();
    virtual language_t language() const = 0;
    virtual std::size_t char_count() const = 0;
    virtual void produce_vertex_program(char* program) const = 0;
};

class fragment_shader {
public:
	virtual ~fragment_shader();
	virtual std::size_t char_count() const = 0;
	virtual void produce_fragment_shader(char* program) const = 0;
};

class device {
public:
    enum feature
    {
        CUSTOM_VERTEX_FORMAT,
        RENDER_TO_TEXTURE,
        VERTEX_PROGRAM_VP_1_0,
        VERTEX_PROGRAM_VS_1_0,
        VERTEX_PROGRAM_CG,
        VERTEX_PROGRAM_GLSL,
        VERTEX_PROGRAM_HLSL
    };
    virtual ~device();
    virtual state_set* create_state_set() = 0;
    virtual bool has_feature(int f) = 0;
    virtual void begin_frame() = 0;
    virtual void present(state_set*) = 0;
    virtual void end_frame() = 0;
    //void draw_triangle_list(std::size_t bias, std::size_t span_begin, std::size_t span_end,
    //    std::size_t triangle_count);
};

class state_set {
public:
    enum primitive_type {
        POINT_LIST,
        LINE_LIST,
        LINE_STRIP,
        TRIANGLE_LIST,
        TRIANGLE_STRIP,
        TRIANGLE_FAN
    };
	enum blending_mode {
		BLEND_OFF,
		BLEND_ADD,
		BLEND_TRANSPARENT
	};
    virtual void set_primitive_type(primitive_type) = 0;
    virtual void set_depth(float depth) = 0;
    virtual void set_texture(std::size_t index, texture_producer*) = 0;
    virtual void set_vertices(vertex_producer*) = 0;
    virtual void invalidate_stream(std::size_t index) = 0;
    virtual void set_indices(index_producer*) = 0;
    virtual void set_vertex_program(vertex_program* shader) = 0;
    virtual void set_fragment_shader(fragment_shader* shader) = 0;

	virtual void set_blending_mode(blending_mode mode) = 0;
    virtual void set_vertex_program_parameter(std::size_t index,
        vector4f const& value) = 0;

	virtual void set_vertex_range(size_t start, size_t count) = 0;

    virtual void commit() = 0;
    virtual bool less(state_set* other) = 0;
protected:
    virtual ~state_set();
};

inline strategy::strategy(int const* resource_map) :
    resource_map_(resource_map)
{
}

#endif	// DEVICE_HPP
