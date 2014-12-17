#include <vector>
#include <list>
#include <string>
#include <map>
#include "device.hpp"
#include "scene.hpp"

class wavefront_object : public vertex_producer {
public:

    wavefront_object(char const* file);

    void init(batch* s);
    std::size_t vertex_count();
    void get_vertex_declaration(vertex_declaration const**, std::size_t* count);
    void produce_vertices(std::size_t stream, void* data);

private:
    typedef uint_fast16_t index_t;
    struct index_tuple
    {
        index_t position;
        //index_t texcoord;
        index_t normal;
        bool operator<(index_tuple const& rhs) const
        {
            if(position < rhs.position)
                return true;
            else
                return position == rhs.position and normal < rhs.normal;
        }
    };
    struct obj_face
    {
        index_tuple index[3];
    };
    typedef std::vector<obj_face> obj_group_t;
    typedef std::map<index_tuple, index_t> index_map_t;

    struct group : public index_producer
    {
        std::size_t index_count();
        void produce_indices(void* data);
        std::vector<index_t> indices;
    };

    void load(char const* text, char const* end);
    void load_object(char const*& next, char const* end,
        std::vector<std::string>& line);
    void load_group(char const*& next, char const* end,
        std::vector<std::string>& line, obj_group_t& group);
    bool next_line(char const*& next, char const* end,
        std::vector<std::string>& line);
    void parse_index_tuple(std::string const& tuple_str, index_tuple& tuple);

    std::list<group> groups_;
    std::vector<vertex_declaration> vertex_declaration_;
    std::vector<char> vertex_buffer_;
    std::size_t stride_;
};

