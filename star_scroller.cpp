#include "cg_program_file.hpp"
#include "star_scroller.hpp"
#include "types.hpp"
#include <iostream>

star_scroller::star_scroller() :
	visible_stars_(1)
{
}

void star_scroller::init_state_sets(batch* b)
{
    static cg_program_file vp("star_scroller.cg");

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

void star_scroller::remove_state_sets(batch* b)
{
	b->remove_state_set(state_set_);
}

struct star_count_frame {
	float time;
	int count;
};

star_count_frame star_count_frames[] = {
	{0, 200},
	{30, 200},
	{40, 50000},
	{69, 50000},
	{71, 1000},
	{1000, 1000}
};
void star_scroller::update(float t)
{
	float f = t * 0.1f;
	float x = fmod(3*(sin(1.5f*f+3) + 1.0f), 2.0f);
	float y = fmod(3*(sin(1.7f*f+1) + 1.0f), 2.0f);
	float z = fmod(5*(sin(2.3f*f) + 1.0f), 1.0f);
	state_set_->set_vertex_program_parameter(0, vector4f(x, y, z, 1.0));

	size_t frame_i = 0;
	while(star_count_frames[frame_i].time < t)
		++frame_i;
	f = (t - star_count_frames[frame_i-1].time)
		/ (star_count_frames[frame_i].time - star_count_frames[frame_i-1].time);
	set_visible_stars(star_count_frames[frame_i-1].count +
		f*(star_count_frames[frame_i].count - star_count_frames[frame_i-1].count));
	//set_visible_stars(1);

	//std::cout << "visible_stars = " << visible_stars_ << std::endl;
	state_set_->set_vertex_range(0, visible_stars_*3);
}

std::size_t star_scroller::vertex_count()
{
	return N_STARS*3;
}
void star_scroller::get_vertex_declaration(vertex_declaration const** pdeclaration, std::size_t* count)
{
	namespace va = vertex_attribute;
    static vertex_declaration const declaration[] = {
        {0, va::POSITION, 0, va::SHORT4}
		//{0, va::NORMAL, 12, va::FLOAT3}
    };
    *pdeclaration = declaration;
    *count = 1;

}
void star_scroller::produce_vertices(std::size_t stream, void* data)
{
	int16_t* pv = static_cast<int16_t*>(data);
	for(size_t i=0; i!=N_STARS; ++i)
	{
		int16_t x = int16_t(float(rand())/RAND_MAX * 65535 - 32768);
		int16_t y = int16_t(float(rand())/RAND_MAX * 65535 - 32768);
		int16_t z = int16_t(float(rand())/RAND_MAX * 32767);

		/*float dx = float(rand())/RAND_MAX*2.0f - 1.0f;
		float dy = float(rand())/RAND_MAX*2.0f - 1.0f;
		float dz = float(rand())/RAND_MAX*2.0f - 1.0f;
		float len = rsqrt(dx*dx + dy*dy + dz*dz);
		len *= float(rand())/RAND_MAX;
		dx *= len*0.25f;
		dy *= len*0.25f;
		dz *= len*0.25f;

		int16_t x = int16_t(dx * 32767);
		int16_t y = int16_t(dy * 32767);
		int16_t z = int16_t((dz + 1.0f)*0.5f * 32767);*/

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
/*void star_scroller::produce_vertices(std::size_t stream, void* data)
{
	float const sqrt2 = 1.0f; //sqrt(2.0f);
	for(size_t i=0; i!=N_STARS; ++i)
	{
		float x = float(rand())/RAND_MAX - 0.5f;
		float y = float(rand())/RAND_MAX - 0.5f;
		float z = float(rand())/RAND_MAX;
		//float x = 0.0f;
		//float y = 0.0f;
		//float z = -0.5f;
		pv[0] = x;
		pv[1] = y;
		pv[2] = z;
		pv[3] = -sqrt2;
		pv[4] = -sqrt2;
		pv[5] = 0.0f;

		pv[6] = x;
		pv[7] = y;
		pv[8] = z;
		pv[9] = sqrt2;
		pv[10] = -sqrt2;
		pv[11] = 0.0f;

		pv[12] = x;
		pv[13] = y;
		pv[14] = z;
		pv[15] = 0;
		pv[16] = sqrt2;
		pv[17] = 0.0f;

		pv += 18;
	}
}*/

void star_scroller::get_texture_description(description *pdesc)
{
	pdesc->format = RGB;
	pdesc->width = TEXTURE_SIZE;
	pdesc->height = TEXTURE_SIZE;
}

void star_scroller::produce_texture(void* data)
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
