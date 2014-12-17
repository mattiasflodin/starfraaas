#include <sstream>
#include <fstream>
#include <boost/lexical_cast.hpp>
#include "utility.hpp"
#include "wavefront_object.hpp"
#include "cg_program_file.hpp"

#include <boost/format.hpp>
#include "debug.hpp"
#include "types.hpp"

wavefront_object::wavefront_object(char const* file) :
    stride_(0)
{
    std::vector<char> data;
    std::ifstream ifs(file);
    assert(ifs.good());
    ifs.seekg(0, std::ios_base::end);
    std::size_t size = static_cast<std::size_t>(ifs.tellg());
    ifs.seekg(0, std::ios_base::beg);
    data.resize(size);
    ifs.read(&data[0], size);

    load(&data[0], &data[0] + size);
}

void wavefront_object::init(batch* s)
{
    static cg_program_file vp("color_by_normal.cg");
    state_set* states = s->create_state_set();
    states->set_vertices(this);
    states->set_indices(&groups_.front());
    states->set_vertex_program(&vp);
}

std::size_t wavefront_object::vertex_count()
{
    return vertex_buffer_.size() / stride_;
}

void wavefront_object::get_vertex_declaration(
    vertex_declaration const** pdeclaration, std::size_t* count)
{
    *pdeclaration = &vertex_declaration_[0];
    *count = vertex_declaration_.size();
}

void wavefront_object::produce_vertices(std::size_t stream, void* data)
{
    (void) stream;
    memcpy(data, &vertex_buffer_[0], vertex_buffer_.size());
}

void wavefront_object::load(char const* text, char const* end)
{
    std::vector<std::string> line;
    char const* next = text;
    while(next_line(next, end, line))
    {
        if(line[0] == "o")
        {
            load_object(next, end, line);
            // Only load the first object encountered.
            break;
        }
    }
}
void wavefront_object::load_object(char const*& next, char const* end, std::vector<std::string>& line)
{
    using boost::lexical_cast;

    vector4f_vector positions;
    vector4f_vector normals;
    index_map_t index_map;
    std::list<obj_group_t> obj_groups;

    while(next_line(next, end, line))
    {
        if(line[0] == "v")
        {
            assert(line.size() == 4);
            float x = lexical_cast<float>(line[1]);
            float y = lexical_cast<float>(line[2]);
            float z = lexical_cast<float>(line[3]);
            positions.push_back(vector4f(x, y, z, 1));
        }
        else if(line[0] == "vn")
        {
            assert(line.size() == 4);
            float x = lexical_cast<float>(line[1]);
            float y = lexical_cast<float>(line[2]);
            float z = lexical_cast<float>(line[3]);
            normals.push_back(vector4f(x, y, z, 1));
        }
        else if(line[0] == "g")
        {
            obj_groups.push_back(obj_group_t());
            load_group(next, end, line, obj_groups.back());
        }
        else
            break;
    }

    // Insert all index tuples into the map. Check what vertex attributes are
    // used.
    bool use_position = true;
    bool use_normal = true;
    std::list<obj_group_t>::const_iterator group_it = obj_groups.begin();
    std::list<obj_group_t>::const_iterator const group_end = obj_groups.end();
    while(group_it != group_end)
    {
        obj_group_t::const_iterator face_it = group_it->begin();
        obj_group_t::const_iterator const face_end = group_it->end();
        while(face_it != face_end)
        {
            for(std::size_t i=0; i!=3; ++i)
            {
                index_map[face_it->index[i]] = 0;

                if(face_it->index[i].position == std::size_t(-1))
                    use_position = false;
                if(face_it->index[i].normal == std::size_t(-1))
                    use_normal = false;
                // TODO check indices
            }
            ++face_it;
        }
        ++group_it;
    }

    if(not use_position)
    {
        throw std::runtime_error("One or more faces is missing a "
                "position index");
    }

    if(not use_normal)
        normals.clear();

    // Compute vertex stride and generate the vertex declaration
    std::size_t stride = 3*sizeof(float);
    std::vector<vertex_declaration> declaration;
    declaration.clear();
    vertex_declaration item;
    item.stream = 0;
    item.attribute = vertex_attribute::POSITION;
    item.offset = 0;
    item.type = vertex_attribute::FLOAT3;
    declaration.push_back(item);
    if(use_normal)
    {
        stride += 3*sizeof(float);
        item.attribute = vertex_attribute::NORMAL;
        item.offset = 3*sizeof(float);
        declaration.push_back(item);
    }

    // Assign a "one-dimensional" index to each tuple. Generate the actual
    // vertex data that is referenced by each index.
    std::vector<char> vertex_buffer(index_map.size()*stride);

    index_map_t::iterator index_it = index_map.begin();
    index_map_t::const_iterator const index_end = index_map.end();
    index_t index = 0;
    TRACE_OUT("Index mappings:");
    using boost::format;
    while(index_it != index_end)
    {

        TRACE_OUT(format("<%d %d> -> %d")
                % index_it->first.position
                % index_it->first.normal
                % index);

        index_it->second = index;
        float* scalars = serialize_cast<float*>(&vertex_buffer[index*stride]);
        vector4f pos = positions[index_it->first.position];
        scalars[0] = pos[0];
        scalars[1] = pos[1];
        scalars[2] = pos[2];
        if(use_normal)
        {
            vector4f normal = normals[index_it->first.normal];
            scalars[3] = normal[0];
            scalars[4] = normal[1];
            scalars[5] = normal[2];
        }

        ++index_it;
        ++index;
    }

    // Generate mapped groups (i.e. the proper index buffers).
    // TODO use local groups_ and swap.
    groups_.clear();
    group_it = obj_groups.begin();
    std::size_t group_n = 0;
    while(group_it != group_end)
    {
        TRACE_OUT(format("Group %d:") % group_n);
        ++group_n;

        groups_.push_back(group());
        group& grp = groups_.back();

        obj_group_t::const_iterator face_it = group_it->begin();
        obj_group_t::const_iterator const face_end = group_it->end();
        while(face_it != face_end)
        {
            index_t i1 = index_map[face_it->index[0]];
            index_t i2 = index_map[face_it->index[1]];
            index_t i3 = index_map[face_it->index[2]];
            TRACE_OUT(format("  <%d %d %d>") % i1 % i2 % i3);
            grp.indices.push_back(i1);
            grp.indices.push_back(i2);
            grp.indices.push_back(i3);
            ++face_it;
        }
        ++group_it;
    }
    vertex_buffer_.swap(vertex_buffer);
    vertex_declaration_.swap(declaration);
    stride_ = stride;
}

void wavefront_object::load_group(char const*& next, char const* end,
    std::vector<std::string>& line, obj_group_t& group)
{
    while(next_line(next, end, line))
    {
        if(line[0] == "f")
        {
            assert(line.size() == 4);
            group.push_back(obj_face());
            obj_face& face = group.back();
            parse_index_tuple(line[1], face.index[0]);
            parse_index_tuple(line[2], face.index[1]);
            parse_index_tuple(line[3], face.index[2]);
        }
        else if(line[0] == "usemtl")
        {
        }
        else
            break;
    }
}

bool wavefront_object::next_line(char const*& next, char const* end,
    std::vector<std::string>& line)
{
    line.clear();

    if(next == end)
        return false;
    while(isspace(static_cast<unsigned char>(*next)))
    {
        ++next;
        if(next == end)
            return false;
    }
    while(*next == '#') {
        while(*next != '\n')
        {
            ++next;
            if(next == end)
                return false;
        }
        while(isspace(static_cast<unsigned char>(*next)))
        {
            ++next;
            if(next == end)
                return false;
        }
    }

    while(next != end) {
        char const* word_start = next;
        while(next != end and not isspace(static_cast<unsigned char>(*next)))
            ++next;
        line.push_back(std::string(word_start, next));
        while(next != end and isspace(static_cast<unsigned char>(*next))) {
            if(*next == '\n')
                return true;
            ++next;
        }
    }
    return true;
}

void wavefront_object::parse_index_tuple(std::string const& tuple_str,
    index_tuple& tuple)
{
    using boost::lexical_cast;
    using std::string;

    std::size_t end = tuple_str.find('/');
    assert(end != string::npos);
    if(end != 0)
        tuple.position = lexical_cast<index_t>(tuple_str.substr(0, end)) - 1;
    else
        tuple.position = std::size_t(-1);

    std::size_t start = end + 1;
    end = tuple_str.find('/', start);
    assert(end != string::npos);
    //tuple.get<1>() = lexical_cast<index_t>(tuple_str.substr(start, end));

    start = end + 1;
    assert(tuple_str.find('/', start) == string::npos);
    if(end != start)
        tuple.normal = lexical_cast<index_t>(tuple_str.substr(start)) - 1;
    else
        tuple.normal = std::size_t(-1);
}

std::size_t wavefront_object::group::index_count()
{
    return indices.size();
}
void wavefront_object::group::produce_indices(void* data)
{
    uint16_t* pi = static_cast<uint16_t*>(data);
    std::vector<index_t>::const_iterator it = indices.begin();
    std::vector<index_t>::const_iterator const end = indices.end();
    while(it != end)
    {
        *pi = static_cast<uint16_t>(*it);
        ++it;
        ++pi;
    }
}
