#ifdef WIN32
#else
#include <GL/glx.h>
#endif

#include <cstring>
#include <algorithm>
#include <boost/format.hpp>

#include "types.hpp"
#include "utility.hpp"
#include "gl_device.hpp"
#include "debug.hpp"

//#define FOR_EACH(it, container) \
//    for(typeof(container.begin()) it(container.begin()), \
//        it##_end(container.end()); \
//        it!=it##_end; ++it)



#define CHECK_CG_ERROR() \
    do { \
        CGerror error = cgGetError(); \
        if(error != CG_NO_ERROR) \
        { \
            char const* str = cgGetErrorString(error); \
            TRACE_OUT(__FILE__ "(" <<  __LINE__ << "): " << str); \
            throw std::runtime_error(str); \
        } \
    } while(0)

#define CHECK_GL_ERROR() \
	do { \
		GLenum error = glGetError(); \
		if(error != GL_NO_ERROR) \
		{ \
			TRACE_OUT(__FILE__ "(" << __LINE__ << "): GL error " << error); \
			throw std::runtime_error("GL error"); \
		} \
	} while(0)

using boost::format;

/*struct vertex_attribute_declaration {
    unsigned int stream;
    unsigned int resource;
    unsigned int offset;
    unsigned int size;
    GLenum type;
};*/

attribute_setup_function* const gl_device::ff_attrib_setup_functions[15] = {
    &gl_device::position_attrib_setup,
    &gl_device::weights_attrib_setup,
    &gl_device::normal_attrib_setup,
    &gl_device::primary_color_attrib_setup,
    &gl_device::secondary_color_attrib_setup,
    &gl_device::fog_coordinate_attrib_setup,

    &gl_device::texture_coordinate_attrib_setup<0>,
    &gl_device::texture_coordinate_attrib_setup<1>,
    &gl_device::texture_coordinate_attrib_setup<2>,
    &gl_device::texture_coordinate_attrib_setup<3>,
    &gl_device::texture_coordinate_attrib_setup<4>,
    &gl_device::texture_coordinate_attrib_setup<5>,
    &gl_device::texture_coordinate_attrib_setup<6>,
    &gl_device::texture_coordinate_attrib_setup<7>,
    &gl_device::texture_coordinate_attrib_setup<8>
};

namespace {
#ifdef WIN32
    template <typename Fun>
    void get_proc_address(char const* name, Fun& fun)
    {
        fun = reinterpret_cast<Fun>(wglGetProcAddress(name));
    }
#else
    template <typename Fun>
    void get_proc_address(char const* name, Fun& fun)
    {
        fun = reinterpret_cast<Fun>(glXGetProcAddressARB(
                    serialize_cast<GLubyte const*>(name)));
    }
#endif
}
gl_device::gl_device() :
	bound_vertex_program_(0),
	bound_fragment_shader_(0)
{
    setup_extensions();
    setup_cg();

    std::fill(array_state_,
        array_state_ + vertex_attribute::N_STANDARD_ATTRIBUTES, false);

    //glClearDepth(1.0f);
    glDisable(GL_DEPTH_TEST);
    //glShadeModel(GL_SMOOTH);
    //glMatrixMode(GL_PROJECTION);
    //glLoadIdentity();

	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//glMatrixMode(GL_MODELVIEW);

	//glDisable(GL_BLEND);
	//glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);

}

gl_state_set* gl_device::create_state_set()
{
    return new gl_state_set(this);
}
bool gl_device::has_feature(int)
{
    return false;
}

/*
    Generic
    Attribute   Conventional Attribute       Conventional Attribute Command
    ---------   ------------------------     ------------------------------
         0      vertex position              Vertex
         1      vertex weights 0-3           WeightARB, VertexWeightEXT
         2      normal                       Normal
         3      primary color                Color
         4      secondary color              SecondaryColorEXT
         5      fog coordinate               FogCoordEXT
         6      -                            -
         7      -                            -
         8      texture coordinate set 0     MultiTexCoord(TEXTURE0, ...)
         9      texture coordinate set 1     MultiTexCoord(TEXTURE1, ...)
        10      texture coordinate set 2     MultiTexCoord(TEXTURE2, ...)
        11      texture coordinate set 3     MultiTexCoord(TEXTURE3, ...)
        12      texture coordinate set 4     MultiTexCoord(TEXTURE4, ...)
        13      texture coordinate set 5     MultiTexCoord(TEXTURE5, ...)
        14      texture coordinate set 6     MultiTexCoord(TEXTURE6, ...)
        15      texture coordinate set 7     MultiTexCoord(TEXTURE7, ...)
       8+n      texture coordinate set n     MultiTexCoord(TEXTURE0+n, ...)
*/
// "The client may specify up to 5 plus the values of
//    MAX_TEXTURE_UNITS and MAX_VERTEX_ATTRIBS_ARB arrays:"
/*
      void VertexAttribPointerARB(uint index, int size, enum type, 
                                  boolean normalized, sizei stride, 
                                  const void *pointer);
*/
/*
An individual generic vertex attribute
    array is enabled or disabled by calling one of

      void EnableVertexAttribArrayARB(uint index);
      void DisableVertexAttribArrayARB(uint index);

    where <index> identifies the generic vertex attribute array to enable or
    disable
*/

void gl_device::begin_frame()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void gl_device::present(state_set* i_state_set)
{
    glLoadIdentity();
    TRACE_OUT_EMPTY();
    TRACE_OUT("gl_device::present()");
    using std::size_t;
    gl_state_set* state_set = down_cast<gl_state_set*>(i_state_set);

    state_set->commit();

    vertex_buffer_info const* pvb_info = state_set->pvertex_buffer_info_;
    bind_vertex_buffer_objects(pvb_info);
	CHECK_GL_ERROR();

    // Set up vertex arrays.
    // 1. vertex_buffer_info::used_attributes gives the arrays used
    // 2. enabled_arrays / array_state give the arrays currently active.
    // This is only for vertex arrays / VBO's, not shaders.
    // In the end it really all depends on what shader is active (and what
    // shader can be activated depends on the features). For now we assume the
    // fixed-function shader.
    // For vertex attributes (i.e. with vertex program extension) a separate
    // state array needs to be maintained (because EnableClientState constants
    // are >0x8000, they don't seem to use the same namespace as vertex
    // attribute).

    setup_vertex_array_states(pvb_info);

    vertex_program_info const* pvp_info = state_set->pvertex_program_info_;
    if(pvp_info)
    {
        bind_vertex_program(pvp_info->program_name);
		CHECK_GL_ERROR();
		set_vp_parameters(state_set);
		CHECK_GL_ERROR();
        glEnable(GL_VERTEX_PROGRAM_ARB);
		CHECK_GL_ERROR();
    }
    else
        glDisable(GL_VERTEX_PROGRAM_ARB);

	fragment_shader_info const* pfs_info = state_set->pfragment_shader_info_;
	if(pfs_info)
	{
		glEnable(GL_FRAGMENT_SHADER_ARB);
		bind_fragment_shader(pfs_info->program_name);
		CHECK_GL_ERROR();
		GLenum err = glGetError();
	}
	else
	{
		//glDisable(GL_FRAGMENT_SHADER_ARB);
		CHECK_GL_ERROR();
	}

	if(state_set->texture_info_.size())
	{
		glBindTexture(GL_TEXTURE_2D, state_set->texture_info_[0]->texture_name);
		glEnable(GL_TEXTURE_2D);
	}
	else
		glDisable(GL_TEXTURE_2D);

	glBlendFunc(state_set->blend_sfactor_, state_set->blend_dfactor_);

    //vbo_path(state_set);
    //glVertexAttrib3ARB(0, x, y, z);
    // TODO only use DrawElements if indices are set. set_indices(0) disables
    // them.
    if(state_set->pindex_buffer_info_)
    {
		//glBindTexture(GL_TEXTURE_2D, 1);
        index_buffer_info const* info = state_set->pindex_buffer_info_;
        TRACE_OUT(format("glDrawElements %d indices") % info->index_count);
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, info->server_buffer);
		CHECK_GL_ERROR();
        glDrawElements(GL_TRIANGLES, info->index_count, GL_UNSIGNED_SHORT, 0);
		CHECK_GL_ERROR();
    }
    else
    {
		//glBindTexture(GL_TEXTURE_2D, 1);
		size_t first_vertex = 0;
		size_t vertex_count = state_set->pvertex_buffer_info_->vertex_count;
		if(state_set->first_vertex_ != size_t(-1))
		{
			first_vertex = state_set->first_vertex_;
			vertex_count = state_set->vertex_count_;
		}
        glDrawArrays(GL_TRIANGLES, first_vertex, vertex_count);
		CHECK_GL_ERROR();
    }
}


void gl_device::end_frame()
{
}

void gl_device::set_vp_parameters(gl_state_set* ss)
{
	for(size_t i=0; i!=ss->vertex_program_parameter_.size(); ++i)
	{
		vector4f const& v = ss->vertex_program_parameter_[i];
		glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, i, v.data());
	}
}

void gl_device::setup_extensions()
{
    struct queried_extension {
        char const* name;
        bool (gl_device::*flag);
    };
    static queried_extension const queried_extensions[] = {
        {"GL_ARB_vertex_buffer_object",
            &gl_device::vertex_buffer_object_available},
        {"GL_ARB_vertex_program",
            &gl_device::vertex_program_available}
    };

	vertex_buffer_object_available = true;
	vertex_program_available = true;
    char const* extensions = serialize_cast<char const*>(
            glGetString(GL_EXTENSIONS));
    for(size_t i=0; i!=1; ++i)
    {
        this->*queried_extensions[i].flag = false;
        std::size_t name_len = std::strlen(queried_extensions[i].name);
        char const* ext_start = extensions;
        while(*ext_start)
        {
            char const* ext_end = ext_start + 1;
            while(*ext_end && *ext_end != ' ')
                ++ext_end;
            if(ext_start + name_len == ext_end)
            {
                if(std::memcmp(queried_extensions[i].name,
                            ext_start, name_len) == 0)
                {
                    this->*queried_extensions[i].flag = true;
                    break;
                }
            }
            ext_start = ext_end + 1;
        }
    }

    get_proc_address("glClientActiveTextureARB", glClientActiveTextureARB);

    if(vertex_buffer_object_available) {
        get_proc_address("glBindBufferARB", glBindBufferARB);
        get_proc_address("glDeleteBuffersARB", glDeleteBuffersARB);
        get_proc_address("glGenBuffersARB", glGenBuffersARB);
        get_proc_address("glBufferDataARB", glBufferDataARB);
        get_proc_address("glMapBufferARB", glMapBufferARB);
        get_proc_address("glUnmapBufferARB", glUnmapBufferARB);
    }

    if(vertex_program_available) {
        get_proc_address("glProgramStringARB", glProgramStringARB);
        get_proc_address("glBindProgramARB", glBindProgramARB);
        get_proc_address("glDeleteProgramsARB", glDeleteProgramsARB);
        get_proc_address("glGenProgramsARB", glGenProgramsARB);
        get_proc_address("glProgramLocalParameter4fvARB",
            glProgramLocalParameter4fvARB);
    }
}

void gl_device::setup_cg()
{
    cg_context_ = cgCreateContext();
    //cg_vertex_profile_ = cgGLGetLatestProfile(CG_GL_VERTEX);
	//cg_fragment_profile_ = cgGLGetLatestProfile(CG_GL_FRAGMENT);
	cg_vertex_profile_ = CG_PROFILE_ARBVP1;
	cg_fragment_profile_ = CG_PROFILE_ARBFP1;
	
    cgGLSetOptimalOptions(cg_vertex_profile_);
    //cgGLSetOptimalOptions(cg_fragment_profile_);
    TRACE_OUT(format("cg_vertex_profile = %s")
        % cgGetProfileString(cg_vertex_profile_));
    TRACE_OUT(format("cg_fragment_profile_ = %s")
        % cgGetProfileString(cg_fragment_profile_));
}

void gl_device::vbo_path(gl_state_set* state_set)
{
    (void) state_set;
}

vertex_buffer_info const* gl_device::acquire_vertex_buffer(
    vertex_producer* producer)
{
    TRACE_OUT(format("acquire_vertex_buffer(%s)") % producer);

    using std::vector;
    using std::size_t;

    VertexBufferTable::iterator it = vertex_buffer_table_.find(producer);
    if(it != vertex_buffer_table_.end())
    {
        it->second.use_count++;
        return &it->second;
    }

    // Buffer does not exist. Create it.
    static GLenum const type_lut[] =
    {
        GL_FLOAT,           // FLOAT1
        GL_FLOAT,           // FLOAT2
        GL_FLOAT,           // FLOAT3
        GL_FLOAT,           // FLOAT4
        GL_UNSIGNED_INT,    // COLOR (needs check)
        GL_UNSIGNED_BYTE,   // UBYTE4
        GL_SHORT,           // SHORT2
        GL_SHORT,           // SHORT4
        GL_UNSIGNED_BYTE,   // UBYTE4N
        GL_SHORT,           // SHORT2N
        GL_SHORT,           // SHORT4N
        GL_UNSIGNED_SHORT,  // USHORT2N
        GL_UNSIGNED_SHORT,  // USHORT4N
        0,                  // UDEC3N ???
        0,                  // DEC3N ???
        0,                  // FLOAT16_2 (unsupported?)
        0,                  // FLOAT16_4 (unsupported?)
    };
    static unsigned int const dimension_lut[] = {
        1, 2, 3, 4,
        1,
        4,
        2, 4,
        4,
        2, 4,
        2, 4,
        3, 3,
        2, 4
    };
    static size_t const size_lut[] = {
        4, 8, 12, 16,
        4,
        4,
        4, 8,
        4,
        4, 8,
        4, 8,
        0, 0,
        4, 8
    };

    vertex_buffer_info& info = vertex_buffer_table_[producer];
    info.use_count = 1;
    info.producer = producer;
    info.vertex_count = producer->vertex_count();
    vertex_declaration const* declaration;
    size_t attribute_count;
    producer->get_vertex_declaration(&declaration, &attribute_count);

    // Check number of streams and allocate the stream vector.
    size_t stream_count = 0;
    for(size_t i=0; i!=attribute_count; ++i)
        stream_count = std::max(stream_count, declaration[i].stream + 1);
    info.streams.resize(stream_count);

    // Want list of function-pointer, type, offset, stride for each stream
    // Have: stream number, type, resource, offset
    for(size_t i=0; i!=attribute_count; ++i) {
        attribute_descriptor d;
        d.setup_function = ff_attrib_setup_functions[declaration[i].attribute];
        d.offset = declaration[i].offset;

        d.type = type_lut[declaration[i].type];
        d.size = dimension_lut[declaration[i].type];

        stream* s = &info.streams[declaration[i].stream];
        s->attribute_descriptors.push_back(d);
        // The stride is the maximum offset for this stream plus the size of
        // the attribute at that offset.
        s->stride = std::max(s->stride, declaration[i].offset
            + size_lut[declaration[i].type]);

        info.used_attributes.push_back(declaration[i].attribute);
        TRACE_OUT(format("    type: %x, dimension: %d, offset: %d, size: %s")
            % d.type % d.size % d.offset
            % size_lut[declaration[i].type]);
    }
    TRACE_OUT(format("  stride[0]: %d") % info.streams[0].stride);

    return &info;
}

void gl_device::commit_vertex_buffer(vertex_buffer_info const* info)
{
    TRACE_OUT("commit_vertex_buffer()");

    std::size_t const stream_count = info->streams.size();
    for(std::size_t stream_i = 0; stream_i != stream_count; ++stream_i)
    {
        stream const& s = info->streams[stream_i];
        if(vertex_buffer_object_available)
        {
            if(not s.server_buffer)
            {
                TRACE_OUT("  Create vbo");
                glGenBuffersARB(1, &s.server_buffer);
                TRACE_OUT(format("  Buffer name = %s") % s.server_buffer);
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, s.server_buffer);
                TRACE_OUT(format("  Buffer size = %s")
                    % (s.stride*info->vertex_count));
                glBufferDataARB(GL_ARRAY_BUFFER_ARB,
                    s.stride*info->vertex_count, 0, GL_STATIC_DRAW_ARB);
				CHECK_GL_ERROR();
				s.dirty = true;
            }
			if(s.dirty)
			{
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, s.server_buffer);
				void* p = glMapBufferARB(GL_ARRAY_BUFFER_ARB,
					GL_WRITE_ONLY_ARB);
				CHECK_GL_ERROR();
				TRACE_OUT(format("  Pointer = %s") % p);
				assert(p);
				info->producer->produce_vertices(stream_i, p);
				glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
				CHECK_GL_ERROR();	
				s.dirty = false;
			}
        }
        else
        {
            if(not s.pclient_buffer)
            {
                TRACE_OUT("  Create vertex array");
                void* p = std::malloc(s.stride*info->vertex_count);
                info->producer->produce_vertices(stream_i, p);
                s.pclient_buffer = p;
            }
        }
    }
}

void gl_device::release_vertex_buffer(vertex_buffer_info const* info)
{
    if(info->use_count == 1)
    {
        std::vector<stream>::const_iterator it = info->streams.begin();
        std::vector<stream>::const_iterator end = info->streams.end();
        while(it != end)
        {
            if(it->server_buffer)
                glDeleteBuffersARB(1, &it->server_buffer);
            if(it->pclient_buffer)
                std::free(it->pclient_buffer);
            ++it;
        }
        vertex_buffer_table_.erase(info->producer);
    }
    else
        info->use_count--;
}

index_buffer_info const* gl_device::acquire_index_buffer(
    index_producer* producer)
{
    using std::size_t;
    IndexBufferTable::iterator it = index_buffer_table_.find(producer);
    if(it != index_buffer_table_.end())
    {
        it->second.use_count++;
        return &it->second;
    }

    // Buffer does not exist. Create it.
    index_buffer_info& info = index_buffer_table_[producer];
    info.use_count = 1;
    info.producer = producer;
    info.index_count = producer->index_count();
    info.server_buffer = 0;
    info.pclient_buffer = 0;

    return &info;
}


// TODO need to decide when vertex / index counts for buffers get updated

void gl_device::commit_index_buffer(index_buffer_info const* info)
{
    if(vertex_buffer_object_available)
    {
        if(not info->server_buffer)
        {
            glGenBuffersARB(1, &info->server_buffer);
            glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, info->server_buffer);
            glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 2*info->index_count,
                0, GL_STATIC_DRAW_ARB);
            void* p = glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
                GL_WRITE_ONLY_ARB);
            assert(p);
            info->producer->produce_indices(p);
            glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
        }
    }
    else
    {
        if(not info->pclient_buffer)
        {
            void* p = std::malloc(2*info->index_count);
            info->producer->produce_indices(p);
            info->pclient_buffer = p;
        }
    }
}

void gl_device::release_index_buffer(index_buffer_info const* info)
{
    // TODO delete the actual buffer object
    if(info->use_count == 1)
        index_buffer_table_.erase(info->producer);
    else
        info->use_count--;
}


vertex_program_info const* gl_device::acquire_vertex_program(
    vertex_program* program)
{
    using std::size_t;
    VertexProgramTable::iterator it = vertex_program_table_.find(program);
    if(it != vertex_program_table_.end())
    {
        it->second.use_count++;
        return &it->second;
    }

    std::size_t const size = program->char_count();
    std::vector<char> source(size + 1);
    program->produce_vertex_program(&source[0]);
    // TODO use boost::variant more when different modes are possible
    CGprogram cg_program = cgCreateProgram(cg_context_, CG_SOURCE, &source[0],
        cg_vertex_profile_, 0, 0);
    CGerror cg_error = cgGetError();
    if(CG_NO_ERROR == cg_error)
    {
        TRACE_OUT("  Compiling program.");
        if(not cgIsProgramCompiled(cg_program))
        {
            cgCompileProgram(cg_program);
            cg_error = cgGetError();
        } else
            TRACE_OUT("  Program already compiled.");
    }
    if(CG_COMPILER_ERROR == cg_error) {
        TRACE_OUT(format("Compilation error. Compiler output:%s\n")
                % cgGetLastListing(cg_context_));
        throw std::runtime_error("vertex program compilation error");
    }
    CHECK_CG_ERROR();

    char const* assembly = cgGetProgramString(cg_program, CG_COMPILED_PROGRAM);
    CHECK_CG_ERROR();
    assert(assembly);
    TRACE_OUT(format("Compiled code:\n%s") % assembly);

    // Create vertex program and assemble the code.
    GLuint program_name;
	TRACE_OUT("Call glGenProgramsARB");
    glGenProgramsARB(1, &program_name);
	TRACE_OUT("Call bind_vertex_program");
    bind_vertex_program(program_name);
	TRACE_OUT("Call glProgramStringARB");
    glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
        std::strlen(assembly), assembly);

    vertex_program_info* pinfo = &vertex_program_table_[program];
    pinfo->cg_program = cg_program;
    pinfo->program_name = program_name;

	TRACE_OUT("Done");
    return pinfo;
}

void commit_vertex_program(vertex_program_info const* info)
{
}

void gl_device::release_vertex_program(vertex_program_info const* info)
{
    // TODO delete the object - need back reference to do that
    //if(info->use_count == 1)
    //    vertex_program_table_.erase(kV
}

fragment_shader_info const* gl_device::acquire_fragment_shader(
    fragment_shader* shader)
{
    using std::size_t;
    FragmentShaderTable::iterator it = fragment_shader_table_.find(shader);
    if(it != fragment_shader_table_.end())
    {
        it->second.use_count++;
        return &it->second;
    }

    std::size_t const size = shader->char_count();
    std::vector<char> source(size + 1);
    shader->produce_fragment_shader(&source[0]);
    // TODO use boost::variant more when different modes are possible
    CGprogram cg_program = cgCreateProgram(cg_context_, CG_SOURCE, &source[0],
        cg_fragment_profile_, 0, 0);
    CGerror cg_error = cgGetError();
    if(CG_NO_ERROR == cg_error)
    {
        TRACE_OUT("  Compiling shader.");
        if(not cgIsProgramCompiled(cg_program))
        {
            cgCompileProgram(cg_program);
            cg_error = cgGetError();
        } else
            TRACE_OUT("  Program already compiled.");
    }
    if(CG_COMPILER_ERROR == cg_error) {
        TRACE_OUT(format("Compilation error. Compiler output:%s\n")
                % cgGetLastListing(cg_context_));
        throw std::runtime_error("vertex shader compilation error");
    }
    CHECK_CG_ERROR();

    char const* assembly = cgGetProgramString(cg_program, CG_COMPILED_PROGRAM);
    CHECK_CG_ERROR();
    assert(assembly);
    TRACE_OUT(format("Compiled code:\n%s") % assembly);

    // Create vertex shader and assemble the code.
    GLuint program_name;
    glGenProgramsARB(1, &program_name);
    bind_fragment_shader(program_name);
    glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
        std::strlen(assembly), assembly);

    fragment_shader_info* pinfo = &fragment_shader_table_[shader];
    pinfo->cg_program = cg_program;
    pinfo->program_name = program_name;

    return pinfo;
}

void commit_fragment_shader(fragment_shader_info const* info)
{
}

void gl_device::release_fragment_shader(fragment_shader_info const* info)
{
    // TODO delete the object - need back reference to do that
    //if(info->use_count == 1)
    //    fragment_shader_table_.erase(kV
}



texture_info const* gl_device::acquire_texture(texture_producer* producer)
{
	TRACE_OUT(format("acquire_texture(%s)") % producer);

	TextureTable::iterator it = texture_table_.find(producer);
    if(it != texture_table_.end())
    {
        it->second.use_count++;
        return &it->second;
    }

    // Buffer does not exist. Create it.
    texture_info& info = texture_table_[producer];
    info.use_count = 1;
    info.producer = producer;
    info.texture_name = 0;

    return &info;
}

void gl_device::commit_texture(texture_info const* pinfo)
{
	if(not pinfo->texture_name)
	{
		glGenTextures(1, &pinfo->texture_name);
		CHECK_GL_ERROR();
		glBindTexture(GL_TEXTURE_2D, pinfo->texture_name);
		CHECK_GL_ERROR();

		texture_producer::description desc;
		pinfo->producer->get_texture_description(&desc);

		size_t pixel_size = 3;
		GLenum pixel_format = GL_RGB;
		if(desc.format == texture_producer::RGBA)
		{
			pixel_size = 4;
			pixel_format = GL_RGBA;
		}
		std::vector<uint8_t> buf(desc.width*desc.height*pixel_size);
		pinfo->producer->produce_texture(&buf[0]);
		
		//glPixelStorei(GL_UNPACK_ALIGNMENT, pinfo->texture_name);
		//CHECK_GL_ERROR();
		glTexImage2D(GL_TEXTURE_2D, 0, pixel_size, desc.width, desc.height, 0, pixel_format, GL_UNSIGNED_BYTE, &buf[0]);
		CHECK_GL_ERROR();
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		CHECK_GL_ERROR();
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		CHECK_GL_ERROR();
		//GLenum err = glGetError();
	}
}

void gl_device::release_texture(texture_info const* pinfo)
{
    // TODO delete the actual texture object
    if(pinfo->use_count == 1)
        texture_table_.erase(pinfo->producer);
    else
        pinfo->use_count--;
}


gl_state_set::gl_state_set(gl_device* device) :
    device_(device),
    pvertex_buffer_info_(0),
    pindex_buffer_info_(0),
    pvertex_program_info_(0),
	pfragment_shader_info_(0),
	blend_sfactor_(GL_ONE),
	blend_dfactor_(GL_ZERO),
	first_vertex_(-1),
	vertex_count_(-1)
{
}
gl_state_set::~gl_state_set()
{
}
void gl_state_set::set_primitive_type(primitive_type)
{
}
void gl_state_set::set_depth(float depth)
{
    (void) depth;
}
void gl_state_set::set_texture(size_t index, texture_producer* producer)
{
    if(index >= texture_info_.size())
		texture_info_.resize(index + 1);

	if(texture_info_[index])
	{
		if(producer == texture_info_[index]->producer)
			return;
		device_->release_texture(texture_info_[index]);
	}
	texture_info_[index] = device_->acquire_texture(producer);
}
void gl_state_set::set_vertices(vertex_producer* producer)
{
    if(pvertex_buffer_info_)
    {
        if(producer == pvertex_buffer_info_->producer)
            return;
        device_->release_vertex_buffer(pvertex_buffer_info_);
    }
    pvertex_buffer_info_ = device_->acquire_vertex_buffer(producer);
}

void gl_state_set::invalidate_stream(std::size_t index)
{
	if(pvertex_buffer_info_)
		pvertex_buffer_info_->streams[0].dirty = true;
}

void gl_state_set::set_indices(index_producer* producer)
{
    if(pindex_buffer_info_)
    {
        if(producer == pindex_buffer_info_->producer)
            return;
        device_->release_index_buffer(pindex_buffer_info_);
    }
    pindex_buffer_info_ = device_->acquire_index_buffer(producer);
}
void gl_state_set::set_vertex_program(vertex_program* program)
{
    // TODO resource_holder encapsulating the reference counting?
    vertex_program_info const* pinfo = device_->acquire_vertex_program(program);
    if(pvertex_program_info_)
        device_->release_vertex_program(pvertex_program_info_);
    pvertex_program_info_ = pinfo;
}
void gl_state_set::set_fragment_shader(fragment_shader* shader)
{
	fragment_shader_info const* pinfo = device_->acquire_fragment_shader(shader);
	if(pfragment_shader_info_)
		device_->release_fragment_shader(pfragment_shader_info_);
	pfragment_shader_info_ = pinfo;
}

void gl_state_set::set_blending_mode(blending_mode mode)
{
	switch(mode)
	{
	case BLEND_OFF:
		blend_sfactor_ = GL_ONE;
		blend_dfactor_ = GL_ZERO;
		break;
	case BLEND_ADD:
		blend_sfactor_ = GL_ONE;
		blend_dfactor_ = GL_ONE;
		break;
	case BLEND_TRANSPARENT:
		blend_sfactor_ = GL_SRC_ALPHA;
		blend_dfactor_ = GL_ONE_MINUS_SRC_ALPHA;
		break;
	}
}

void gl_state_set::set_vertex_program_parameter(std::size_t index,
        vector4f const& value)
{
    if(vertex_program_parameter_.size() <= index)
    {
        vertex_program_parameter_.resize(index + 1);
        vertex_program_parameter_usage_.resize(index + 1);
    }
    vertex_program_parameter_[index] = value;
    vertex_program_parameter_usage_[index] = true;
}

void gl_state_set::set_vertex_range(size_t start, size_t count)
{
	first_vertex_ = start;
	vertex_count_ = count;
}

void gl_state_set::commit()
{
    if(pvertex_buffer_info_)
        device_->commit_vertex_buffer(pvertex_buffer_info_);
    if(pindex_buffer_info_)
        device_->commit_index_buffer(pindex_buffer_info_);
	FOR_EACH(std::vector<texture_info const*>, it, texture_info_)
	{
		device_->commit_texture(*it);
	}
	CHECK_GL_ERROR();
}

bool gl_state_set::less(state_set* state_set)
{
    return this < state_set;
}

void gl_device::bind_vertex_buffer_objects(vertex_buffer_info const* pinfo)
{
    //size_t const stream_count = pinfo->streams.size();
    //for(size_t stream_i=0; stream_i!=stream_count; ++stream_i)
	FOR_EACH(std::vector<stream>, stream_it, pinfo->streams)
    {
        //stream const& s = pinfo->streams[stream_i];
        stream const& s = *stream_it;
        TRACE_OUT(format("Bind buffer %s") % s.server_buffer);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, s.server_buffer);
        attribute_descriptor_list const& adl = s.attribute_descriptors;
        unsigned int const stride = s.stride;
        FOR_EACH(attribute_descriptor_list, it, s.attribute_descriptors)
        {
            it->setup_function(this, reinterpret_cast<void*>(
                it->offset), it->type, it->size, stride);
        }
    }
}

void gl_device::setup_vertex_array_states(vertex_buffer_info const* pinfo)
{
    TRACE_OUT("setup_vertex_array_states");
    using namespace vertex_attribute;
    static GLenum const client_state_lut[N_STANDARD_ATTRIBUTES] =
    {
        GL_VERTEX_ARRAY,    // POSITION
        0,                  // WEIGHTS TODO
        GL_NORMAL_ARRAY,    // NORMAL
        GL_COLOR_ARRAY,     // PRIMARY_COLOR
        0,                  // SECONDARY_COLOR TODO
        GL_FOG_COORD_ARRAY, // FOG_COORDINATE
        GL_TEXTURE0,
        GL_TEXTURE1,
        GL_TEXTURE2,
        GL_TEXTURE3,
        GL_TEXTURE4,
        GL_TEXTURE5,
        GL_TEXTURE6,
        GL_TEXTURE7
    };
    // TODO Flag everything in used_attributes. Enable any array that is in
    // used_attributes but is not set in the state array.

    typedef std::vector<std::size_t> index_vector;
    // Enable all arrays that were previously disabled but should be enabled.
    for(index_vector::const_iterator it = pinfo->used_attributes.begin();
            it != pinfo->used_attributes.end(); ++it)
    {
        if(not array_state_[*it])
        {
            if(*it >= TEXTURE_COORDINATE_0)
            {
                TRACE_OUT(format("  glClientActiveTextureARB(%x)")
                    % client_state_lut[*it]);
                glClientActiveTextureARB(client_state_lut[*it]);
                TRACE_OUT("  glEnableClientState(GL_TEXTURE_COORD_ARRAY)");
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            }
            else
            {
                TRACE_OUT(format("  glEnableClientState(%d -> %x)")
                    % *it % client_state_lut[*it]);
                glEnableClientState(client_state_lut[*it]);
            }
        }
    }
    // Un-flag everything that was previously enabled.
    for(index_vector::const_iterator it = enabled_arrays_.begin();
            it != enabled_arrays_.end(); ++it)
    {
        TRACE_OUT(format("  array_state[%d] = false") % *it);
        array_state_[*it] = false;
    }
    // Flag everything that should be enabled.
    for(index_vector::const_iterator it = pinfo->used_attributes.begin();
            it != pinfo->used_attributes.end(); ++it)
    {
        TRACE_OUT(format("  array_state[%d] = true") % *it);
        array_state_[*it] = true;
    }
    // Go through enabled_arrays and disable any array that shouldn't be
    // enabled.
    for(std::vector<std::size_t>::const_iterator it = enabled_arrays_.begin();
        it != enabled_arrays_.end(); ++it)
    {
        if(not array_state_[*it])
        {
            if(*it >= TEXTURE_COORDINATE_0)
            {
                TRACE_OUT(format("  glClientActiveTextureARB(%x)")
                    % client_state_lut[*it]);
                glClientActiveTextureARB(client_state_lut[*it]);
                TRACE_OUT("  glDisableClientState(GL_TEXTURE_COORD_ARRAY)");
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            }
            else
            {
                TRACE_OUT(format("  glDisableClientState(%x)")
                    % client_state_lut[*it]);
                glDisableClientState(client_state_lut[*it]);
            }
        }
    }
    enabled_arrays_ = pinfo->used_attributes;
}

void gl_device::position_attrib_setup(gl_device*, void const* pointer, GLenum type,
    unsigned int size, unsigned int stride)
{
    TRACE_OUT(format("  Vertex(size: %d, type: %x, stride: %d, pointer: %d)")
        % size % type % stride % reinterpret_cast<uint32_t>(pointer));
    glVertexPointer(size, type, stride, pointer);
	CHECK_GL_ERROR();
}
void gl_device::weights_attrib_setup(gl_device*, void const* pointer, GLenum type,
    unsigned int size, unsigned int stride)
{
    (void) pointer;
    (void) type;
    (void) size;
    (void) stride;
    // glWeightPointerARB(size, type, stride, pointer);
}

void gl_device::normal_attrib_setup(gl_device*, void const* pointer, GLenum type,
    unsigned int size, unsigned int stride)
{
    assert(size == 3);
    glNormalPointer(type, stride, pointer);
}

void gl_device::primary_color_attrib_setup(gl_device*, void const* pointer, GLenum type,
    unsigned int size, unsigned int stride)
{
    glColorPointer(size, type, stride, pointer);
}
void gl_device::secondary_color_attrib_setup(gl_device*, void const* pointer, GLenum type,
    unsigned int size, unsigned int stride)
{
    (void) pointer;
    (void) type;
    (void) size;
    (void) stride;
    //glSecondaryColorPointer(size, type, stride, pointer);
}
void gl_device::fog_coordinate_attrib_setup(gl_device*, void const* pointer, GLenum type,
    unsigned int size, unsigned int stride)
{
    (void) pointer;
    (void) type;
    (void) size;
    (void) stride;
    //glFogCoordPointer(type, stride, pointer);
}
template<unsigned int Texture>
void gl_device::texture_coordinate_attrib_setup(gl_device* device, void const* pointer,
    GLenum type, unsigned int size, unsigned int stride)
{
    device->glClientActiveTextureARB(GL_TEXTURE0 + Texture);
    glTexCoordPointer(size, type, stride, pointer);
}
