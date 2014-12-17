#include "device.hpp"
#include "scene.hpp"

class greetings : public renderable, public vertex_producer, public index_producer, public texture_producer {
public:
	enum {
		TILES = 32
	};
	greetings();

	void init_state_sets(batch* b);
	void remove_state_sets(batch* b);
	void update(float t);
    std::size_t vertex_count();
    std::size_t index_count();
    void get_vertex_declaration(vertex_declaration const**, std::size_t* count);
    void produce_vertices(std::size_t stream, void* data);
    void produce_indices(void* indices);

	void get_texture_description(description *pdesc);
	void produce_texture(void* data);

private:
	state_set* state_set_;
};
