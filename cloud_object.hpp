#include "device.hpp"
#include "scene.hpp"
#include "matrix.hpp"
#include "types.hpp"

class cloud_object : public renderable, public vertex_producer, public texture_producer {
public:
	enum {
		N_STARS = 50000,
		TEXTURE_SIZE = 16
	};
	enum Object {
		NONE,
		SPHERE,
		CUBE,
		TORUS,
		BROKEN_TORUS
	};
	cloud_object();

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
	struct key_frame {
		float time;
		Object object;
		vector4f color;
		float scale;
		int star_count;
	};
	static key_frame key_frames[7];

	void setup_sphere(int16_t* pv);
	void setup_cube(int16_t* pv);
	void setup_torus(int16_t* pv);
	void setup_broken_torus(int16_t* pv);
	void setup_cylinder(int16_t* pv);

	state_set* state_set_;
	size_t visible_stars_;
	matrix4f orientation_;
	size_t current_frame_;
	Object current_object_;
	float last_time_;
	float last_frame_time_;
};
