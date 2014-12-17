#include "device.hpp"
#include "scene.hpp"

class star_scroller : public renderable, public vertex_producer, public texture_producer {
public:
	enum {
		N_STARS = 50000,
		TEXTURE_SIZE = 16
	};
	star_scroller();

	void init_state_sets(batch* b);
	void remove_state_sets(batch* b);
	void update(float t);
    std::size_t vertex_count();
    void get_vertex_declaration(vertex_declaration const**, std::size_t* count);
    void produce_vertices(std::size_t stream, void* data);

	void get_texture_description(description *pdesc);
	void produce_texture(void* data);

	void set_visible_stars(size_t visible_stars)
	{
		visible_stars_ = visible_stars;
	}

private:
	state_set* state_set_;
	size_t visible_stars_;
};
