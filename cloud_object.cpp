#include "cg_program_file.hpp"
#include "cloud_object.hpp"
#include <iostream>

cloud_object::cloud_object() :
	visible_stars_(0),
	orientation_(matrix4f::identity()),
	current_frame_(0),
	current_object_(NONE),
	last_time_(0),
	last_frame_time_(0)
{
}

void cloud_object::init_state_sets(batch* b)
{
    static cg_program_file vp("cloud_object.cg");

    state_set_ = b->create_state_set();
    state_set_->set_vertices(this);
    state_set_->set_vertex_program(&vp);
	state_set_->set_texture(0, this);

	float sqrt075 = sqrt(0.75f);
	state_set_->set_vertex_program_parameter(1, vector4f(-0.5f, -sqrt075/2, 0, 0));
	state_set_->set_vertex_program_parameter(2, vector4f(0.5f, -sqrt075/2, 0, 0));
	state_set_->set_vertex_program_parameter(3, vector4f(0, sqrt075/2, 0, 0));

	state_set_->set_blending_mode(state_set::BLEND_ADD);
}

void cloud_object::remove_state_sets(batch* b)
{
	b->remove_state_set(state_set_);
}

// At time t:
// Change number of stars
// Change object
// Change color
// Change scale
cloud_object::key_frame cloud_object::key_frames[7] = {
	{ 0, NONE, vector4f(1.0f, 0.0f, 1.0f), 0.25f, 0},
	{ 68.0f, SPHERE, vector4f(0.1f, 0.0f, 1.0f), 0.25f, 50000},
	{ 90.0f, CUBE, vector4f(0.15f, 1.0f, 0.0f), 0.15f, 50000},
	{ 120.0f, TORUS, vector4f(0.1f, 0.1f, 1.0f), 0.35f, 50000},
	{ 140.0f, BROKEN_TORUS, vector4f(0.8f, 0.1f, 0.0f), 0.23f, 50000},
	{ 178.0f, NONE, vector4f(0.8f, 0.1f, 0.0f), 0.23f, 0},
	{ 1000.0f, NONE, vector4f(1.0f, 0.0f, 1.0f), 0.25f, 0}
};

void cloud_object::update(float t)
{
	float pitch = 1.0f;
	float roll = 1.3f;
	float yaw = 0.7f;

	float dt = t - last_time_;
	last_time_ = t;

	pitch *= dt;
	roll *= dt;
	yaw *= dt;

	matrix4f yaw_m(matrix4f::identity());
	yaw_m[0][0] = cos(yaw);
	yaw_m[0][1] = sin(yaw);
	yaw_m[1][0] = -sin(yaw);
	yaw_m[1][1] = cos(yaw);

	matrix4f pitch_m(matrix4f::identity());
	pitch_m[0][0] = cos(pitch);
	pitch_m[0][2] = -sin(pitch);
	pitch_m[2][0] = sin(pitch);
	pitch_m[2][2] = cos(pitch);

	matrix4f roll_m(matrix4f::identity());
	roll_m[1][1] = cos(roll);
	roll_m[1][2] = sin(roll);
	roll_m[2][1] = -sin(roll);
	roll_m[2][2] = cos(roll);

	orientation_ *= yaw_m;
	orientation_ *= pitch_m;
	orientation_ *= roll_m;
	state_set_->set_vertex_program_parameter(4, orientation_[0]);
	state_set_->set_vertex_program_parameter(5, orientation_[1]);
	state_set_->set_vertex_program_parameter(6, orientation_[2]);

	size_t frame_i = 0;
	while(key_frames[frame_i].time < t)
		++frame_i;
	--frame_i;

	if(current_frame_ != frame_i)
	{
		current_frame_ = frame_i;
		last_frame_time_ = key_frames[frame_i].time;
		//std::cout << "Switch frame" << std::endl;
	}

	if(t - last_frame_time_ < 8.0f)
	{
		float dt = (t - last_frame_time_)/8.0f;
		key_frame const& f = key_frames[current_frame_];
		size_t previous_frame = current_frame_;
		if(previous_frame > 0)
			--previous_frame;
		key_frame const& pf = key_frames[previous_frame];

		if(dt < 0.5f)
		{
			set_visible_stars(size_t((1.0f - dt*2.0f) * pf.star_count));
		}
		else
		{
			if(current_object_ != f.object)
			{
				current_object_ = f.object;
				state_set_->invalidate_stream(0);
			}
			set_visible_stars(size_t((dt-0.5f)*2.0f * f.star_count));
		}

		vector4f color1 = pf.color;
		vector4f color2 = f.color;
		color1[3] = pf.scale;
		color2[3] = f.scale;
		vector4f color = color1*(1.0f-dt) + color2*dt;
		//color[3] = 0.25f;
		state_set_->set_vertex_program_parameter(0, color);
	}

	//std::cout << "visible_stars = " << visible_stars_ << std::endl;
	//set_visible_stars(1000);
	state_set_->set_vertex_range(0, visible_stars_*3);
} 

std::size_t cloud_object::vertex_count()
{
	return N_STARS*3;
}
void cloud_object::get_vertex_declaration(vertex_declaration const** pdeclaration, std::size_t* count)
{
	namespace va = vertex_attribute;
    static vertex_declaration const declaration[] = {
        {0, va::POSITION, 0, va::SHORT4}
		//{0, va::NORMAL, 12, va::FLOAT3}
    };
    *pdeclaration = declaration;
    *count = 1;

}
void cloud_object::produce_vertices(std::size_t stream, void* data)
{
	int16_t* pv = static_cast<int16_t*>(data);
	switch(current_object_) {
	case SPHERE:
		setup_sphere(pv);
		break;
	case CUBE:
		setup_cube(pv);
		break;
	case TORUS:
		setup_torus(pv);
		break;
	case BROKEN_TORUS:
		setup_broken_torus(pv);
		break;
	}
}

void cloud_object::setup_sphere(int16_t* pv)
{
	for(size_t i=0; i!=N_STARS; ++i)
	{
		/*int16_t x = int16_t(float(rand())/RAND_MAX * 65535 - 32768);
		int16_t y = int16_t(float(rand())/RAND_MAX * 65535 - 32768);
		int16_t z = int16_t(float(rand())/RAND_MAX * 32767);*/

		float dx = float(rand())/RAND_MAX*2.0f - 1.0f;
		float dy = float(rand())/RAND_MAX*2.0f - 1.0f;
		float dz = float(rand())/RAND_MAX*2.0f - 1.0f;
		float len = rsqrt(dx*dx + dy*dy + dz*dz);
		//len *= float(rand())/RAND_MAX;
		dx *= len;
		dy *= len;
		dz *= len;

		int16_t x = int16_t(dx * 32767);
		int16_t y = int16_t(dy * 32767);
		int16_t z = int16_t(dz * 32767);

		//x = 0;
		//y = 0;
		//z = 32767;

		pv[0] = x;
		pv[1] = y;
		pv[2] = z;
		pv[3] = 0;

		pv[4] = x;
		pv[5] = y;
		pv[6] = z;
		pv[7] = 1;

		pv[8] = x;
		pv[9] = y;
		pv[10] = z;
		pv[11] = 2;

		pv += 12;
	}
}

void cloud_object::setup_cube(int16_t* pv)
{
	for(size_t i=0; i!=N_STARS; ++i)
	{
		/*int16_t x = int16_t(float(rand())/RAND_MAX * 65535 - 32768);
		int16_t y = int16_t(float(rand())/RAND_MAX * 65535 - 32768);
		int16_t z = int16_t(float(rand())/RAND_MAX * 32767);*/

		float dx = float(rand())/RAND_MAX*2.0f - 1.0f;
		float dy = float(rand())/RAND_MAX*2.0f - 1.0f;
		float dz = float(rand())/RAND_MAX*2.0f - 1.0f;

		int16_t x = int16_t(dx * 32767);
		int16_t y = int16_t(dy * 32767);
		int16_t z = int16_t(dz * 32767);

		//x = 0;
		//y = 0;
		//z = 32767;

		pv[0] = x;
		pv[1] = y;
		pv[2] = z;
		pv[3] = 0;

		pv[4] = x;
		pv[5] = y;
		pv[6] = z;
		pv[7] = 1;

		pv[8] = x;
		pv[9] = y;
		pv[10] = z;
		pv[11] = 2;

		pv += 12;
	}
}

void cloud_object::setup_torus(int16_t* pv)
{
	float const inner_radius = 0.75f;
	float const outer_radius = 0.2f;
	for(size_t i=0; i!=N_STARS; ++i)
	{
		/*int16_t x = int16_t(float(rand())/RAND_MAX * 65535 - 32768);
		int16_t y = int16_t(float(rand())/RAND_MAX * 65535 - 32768);
		int16_t z = int16_t(float(rand())/RAND_MAX * 32767);*/

		float inner_radian = float(rand())/RAND_MAX*6.2832f;
		float outer_radian = float(rand())/RAND_MAX*6.2832f;

		vector4f point(cos(inner_radian), sin(inner_radian));
		vector4f radial_axis(point[0], point[1]);
		point[0] *= inner_radius;
		point[1] *= inner_radius;
		point += radial_axis*cos(outer_radian)*outer_radius;
		point[3] += outer_radius*sin(outer_radian);
		

		int16_t x = int16_t(point[0] * 32767);
		int16_t y = int16_t(point[1] * 32767);
		int16_t z = int16_t(point[3] * 32767);

		//x = 0;
		//y = 0;
		//z = 32767;

		pv[0] = x;
		pv[1] = y;
		pv[2] = z;
		pv[3] = 0;

		pv[4] = x;
		pv[5] = y;
		pv[6] = z;
		pv[7] = 1;

		pv[8] = x;
		pv[9] = y;
		pv[10] = z;
		pv[11] = 2;

		pv += 12;
	}
}

void cloud_object::setup_broken_torus(int16_t* pv)
{
	float const inner_radius = 0.75f;
	float const outer_radius = 0.2f;
	for(size_t i=0; i!=N_STARS; ++i)
	{
		/*int16_t x = int16_t(float(rand())/RAND_MAX * 65535 - 32768);
		int16_t y = int16_t(float(rand())/RAND_MAX * 65535 - 32768);
		int16_t z = int16_t(float(rand())/RAND_MAX * 32767);*/

		float inner_radian = float(rand())/RAND_MAX*6.2832f;
		float outer_radian = float(rand())/RAND_MAX*6.2832f;

		vector4f point(cos(inner_radian), sin(inner_radian));
		vector4f radial_axis(point[0], point[1]);
		point += radial_axis*cos(outer_radian);
		point[3] += sin(outer_radian);
		

		int16_t x = int16_t(point[0] * 32767);
		int16_t y = int16_t(point[1] * 32767);
		int16_t z = int16_t(point[3] * 32767);

		//x = 0;
		//y = 0;
		//z = 32767;

		pv[0] = x;
		pv[1] = y;
		pv[2] = z;
		pv[3] = 0;

		pv[4] = x;
		pv[5] = y;
		pv[6] = z;
		pv[7] = 1;

		pv[8] = x;
		pv[9] = y;
		pv[10] = z;
		pv[11] = 2;

		pv += 12;
	}
}

void cloud_object::setup_cylinder(int16_t* pv)
{
	float const inner_radius = 0.75f;
	float const outer_radius = 0.2f;
	for(size_t i=0; i!=N_STARS; ++i)
	{
		/*int16_t x = int16_t(float(rand())/RAND_MAX * 65535 - 32768);
		int16_t y = int16_t(float(rand())/RAND_MAX * 65535 - 32768);
		int16_t z = int16_t(float(rand())/RAND_MAX * 32767);*/

		float inner_radian = float(rand())/RAND_MAX*6.2832f;
		float outer_radian = float(rand())/RAND_MAX*6.2832f;

		vector4f point(cos(inner_radian), sin(inner_radian));
		vector4f radial_axis(point[0], point[1]);
		//point += radial_axis*cos(outer_radian);
		point[3] += sin(outer_radian);
		

		int16_t x = int16_t(point[0] * 32767);
		int16_t y = int16_t(point[1] * 32767);
		int16_t z = int16_t(point[3] * 32767);

		//x = 0;
		//y = 0;
		//z = 32767;

		pv[0] = x;
		pv[1] = y;
		pv[2] = z;
		pv[3] = 0;

		pv[4] = x;
		pv[5] = y;
		pv[6] = z;
		pv[7] = 1;

		pv[8] = x;
		pv[9] = y;
		pv[10] = z;
		pv[11] = 2;

		pv += 12;
	}
}
void cloud_object::get_texture_description(description *pdesc)
{
	pdesc->format = RGB;
	pdesc->width = TEXTURE_SIZE;
	pdesc->height = TEXTURE_SIZE;
}

void cloud_object::produce_texture(void* data)
{
	uint8_t* pv = static_cast<uint8_t*>(data);
	for(size_t y=0; y!=TEXTURE_SIZE; ++y)
	{
		for(size_t x=0; x!=TEXTURE_SIZE; ++x)
		{
			float dx = x - float(TEXTURE_SIZE-1)/2;
			float dy = y - float(TEXTURE_SIZE-1)/2;
			float d = sqrt(float(dx*dx + dy*dy)) * 0.25f;
			if(d > 1.0f)
				d = 1.0f;
			uint8_t c = uint8_t((1.0f - d)*255);
			pv[y*TEXTURE_SIZE*3 + 3*x + 0] = c;
			pv[y*TEXTURE_SIZE*3 + 3*x + 1] = c;
			pv[y*TEXTURE_SIZE*3 + 3*x + 2] = c;
		}
	}
	//memset(data, 0xff, TEXTURE_WIDTH*TEXTURE_HEIGHT*3);
}
