#ifdef WIN32
#include <windows.h>
#include <GL/gl.h>
#include "GL/glext.h"
#else
#include <GL/gl.h>
#endif
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <vector>
//#include <tr1/unordered_map>
#include <map>
#include "device.hpp"
#include "vector.hpp"
#include "debug.hpp"

class gl_device;
class gl_state_set;

typedef void attribute_setup_function(gl_device* device, void const* pointer,
    GLenum type, unsigned int size, unsigned int stride);
struct attribute_descriptor {
    attribute_setup_function* setup_function;
    std::size_t offset;
    GLenum type;
    unsigned int size;
};
typedef std::vector<attribute_descriptor> attribute_descriptor_list;
struct stream {
	stream() :
		pclient_buffer(0),
		server_buffer(0),
		stride(0),
		dirty(false)
	{
	}
    attribute_descriptor_list attribute_descriptors;
    mutable void* pclient_buffer;
    mutable GLuint server_buffer;
    GLuint stride;
    mutable bool dirty;
};
struct vertex_buffer_info {
	vertex_buffer_info() :
		use_count(0),
		vertex_count(0),
		producer(0)
	{
	}
    mutable std::size_t use_count;
    std::size_t vertex_count;
    vertex_producer* producer;
    std::vector<stream> streams;
    std::vector<std::size_t> used_attributes;
};

struct index_buffer_info {
	index_buffer_info() :
		use_count(0),
		index_count(0),
		producer(0),
		pclient_buffer(0),
		server_buffer(0)
	{
	}
    mutable std::size_t use_count;
    std::size_t index_count;
    index_producer* producer;
    mutable void* pclient_buffer;
    mutable GLuint server_buffer;
};

struct vertex_program_info {
	vertex_program_info() :
		use_count(0),
		cg_program(0),
		program_name(0)
	{
	}
    mutable std::size_t use_count;
    CGprogram cg_program;
    GLuint program_name;
};

struct fragment_shader_info {
	fragment_shader_info() :
		use_count(0),
		cg_program(0),
		program_name(0)
	{
	}
    mutable std::size_t use_count;
    CGprogram cg_program;
    GLuint program_name;
};

struct texture_info {
	mutable std::size_t use_count;
	texture_producer* producer;
	mutable GLuint texture_name;
};

// Must be declared before gl_device to allow covariant return type in
// create_state_set().
class gl_state_set : public state_set {
public:
    friend class gl_device;

    gl_state_set(gl_device* device);
    ~gl_state_set();

    void set_primitive_type(primitive_type);
    void set_depth(float depth);
    void set_texture(std::size_t index, texture_producer*);
    void set_vertices(vertex_producer*);
    void invalidate_stream(std::size_t index);
    void set_indices(index_producer*);
    void set_vertex_program(vertex_program* program);
    void set_fragment_shader(fragment_shader* shader);

	void set_blending_mode(blending_mode mode);
    void set_vertex_program_parameter(std::size_t index, vector4f const& value);

	void set_vertex_range(size_t start, size_t count);

    void commit();
    bool less(state_set* state_set);

private:
    void commit_vbo();

    gl_device* device_;
    vertex_buffer_info const* pvertex_buffer_info_;
    index_buffer_info const* pindex_buffer_info_;
    vertex_program_info const* pvertex_program_info_;
	fragment_shader_info const* pfragment_shader_info_;
	std::vector<texture_info const*> texture_info_;

    vector4f_vector vertex_program_parameter_;
    std::vector<bool> vertex_program_parameter_usage_;

	GLenum blend_sfactor_;
	GLenum blend_dfactor_;

	size_t first_vertex_;
	size_t vertex_count_;
};

class gl_device : public device {
    friend class gl_state_set;
public:
    gl_device();
    gl_state_set* create_state_set();
    bool has_feature(int f);
    void begin_frame();
    void present(state_set*);
    void end_frame();

private:
    //typedef std::tr1::unordered_map<vertex_producer*, vertex_buffer_info>
    //    VertexBufferTable;
    typedef std::map<vertex_producer*, vertex_buffer_info> VertexBufferTable;
    typedef std::map<index_producer*, index_buffer_info> IndexBufferTable;
    typedef std::map<vertex_program*, vertex_program_info> VertexProgramTable;
	typedef std::map<fragment_shader*, fragment_shader_info> FragmentShaderTable;
	typedef std::map<texture_producer*, texture_info> TextureTable;

	void set_vp_parameters(gl_state_set* ss);

    void setup_extensions();
    void setup_cg();
    void vbo_path(gl_state_set* state_set);

    vertex_buffer_info const* acquire_vertex_buffer(vertex_producer* producer);
    void commit_vertex_buffer(vertex_buffer_info const* info);
    void release_vertex_buffer(vertex_buffer_info const* info);

    index_buffer_info const* acquire_index_buffer(index_producer* producer);
    void commit_index_buffer(index_buffer_info const* info);
    void release_index_buffer(index_buffer_info const* info);

    vertex_program_info const* acquire_vertex_program(vertex_program* program);
    void commit_vertex_program(vertex_program_info const* info);
    void release_vertex_program(vertex_program_info const* info);

	fragment_shader_info const* acquire_fragment_shader(fragment_shader* shader);
    void commit_fragment_shader(fragment_shader_info const* info);
    void release_fragment_shader(fragment_shader_info const* info);

	texture_info const* acquire_texture(texture_producer* producer);
	void commit_texture(texture_info const* pinfo);
	void release_texture(texture_info const* pinfo);

    void bind_vertex_buffer_objects(vertex_buffer_info const* pinfo);
    void setup_vertex_array_states(vertex_buffer_info const* pinfo);

    void bind_vertex_program(GLuint program);
	void bind_fragment_shader(GLuint program);

    static void position_attrib_setup(gl_device* device, void const* pointer,
        GLenum type, unsigned int size, unsigned int stride);
    static void weights_attrib_setup(gl_device* device, void const* pointer,
        GLenum type, unsigned int size, unsigned int stride);
    static void normal_attrib_setup(gl_device* device, void const* pointer,
        GLenum type, unsigned int size, unsigned int stride);
    static void primary_color_attrib_setup(gl_device* device,
        void const* pointer, GLenum type, unsigned int size,
        unsigned int stride);
    static void secondary_color_attrib_setup(gl_device* device,
        void const* pointer, GLenum type, unsigned int size,
        unsigned int stride);
    static void fog_coordinate_attrib_setup(gl_device* device,
        void const* pointer, GLenum type, unsigned int size,
        unsigned int stride);
    template<unsigned int Texture>
    static void texture_coordinate_attrib_setup(gl_device* device,
        void const* pointer, GLenum type, unsigned int size,
        unsigned int stride);

    static attribute_setup_function* const ff_attrib_setup_functions[15];

    VertexBufferTable vertex_buffer_table_;
    IndexBufferTable index_buffer_table_;
    VertexProgramTable vertex_program_table_;
	FragmentShaderTable fragment_shader_table_;
	TextureTable texture_table_;

    CGcontext cg_context_;
    CGprofile cg_vertex_profile_;
	CGprofile cg_fragment_profile_;

    // ARB_multitexture
    PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTextureARB;

    // ARB_vertex_buffer_object
    PFNGLBINDBUFFERPROC glBindBufferARB;
    PFNGLDELETEBUFFERSPROC glDeleteBuffersARB;
    PFNGLGENBUFFERSPROC glGenBuffersARB;
    PFNGLBUFFERDATAPROC glBufferDataARB;
    PFNGLMAPBUFFERPROC glMapBufferARB;
    PFNGLUNMAPBUFFERPROC glUnmapBufferARB;

    // ARB_vertex_program
    PFNGLPROGRAMSTRINGARBPROC glProgramStringARB;
    PFNGLBINDPROGRAMARBPROC glBindProgramARB;
    PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB;
    PFNGLGENPROGRAMSARBPROC glGenProgramsARB;
    PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glProgramLocalParameter4fvARB;

    bool vertex_buffer_object_available;
    bool vertex_program_available;

    // State shadowing data
    bool array_state_[vertex_attribute::N_STANDARD_ATTRIBUTES];
    std::vector<std::size_t> enabled_arrays_;
    std::vector<bool> attribute_state_;
    bool enabled_attribute_states_;

    GLuint bound_vertex_program_;
	GLuint bound_fragment_shader_;

    // Runtime data, used in various functions and put here to save execution
    // time (e.g.  allocation time).
    // <nothing so far>
};

inline void gl_device::bind_vertex_program(GLuint program)
{
    if(bound_vertex_program_ != program)
    {
		TRACE_OUT("Call glBindProgramARB");
        glBindProgramARB(GL_VERTEX_PROGRAM_ARB, program);
        bound_vertex_program_ = program;
    }
}

inline void gl_device::bind_fragment_shader(GLuint program)
{
    //if(bound_fragment_shader_ != program)
    {
        glBindProgramARB(GL_FRAGMENT_SHADER_ARB, program);
        bound_fragment_shader_ = program;
    }
}
