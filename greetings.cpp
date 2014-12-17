#include "greetings.hpp"
#include "types.hpp"
#include "cg_program_file.hpp"
#include "debug.hpp"
#include <gl\glaux.h>

static AUX_RGBImageRec* g_image;
static AUX_RGBImageRec* g_image_mask;

greetings::greetings()
{
	if(!g_image)
	{
		g_image = auxDIBImageLoad(L"greetings.bmp");
		g_image_mask = auxDIBImageLoad(L"greetings_mask.bmp");
	}
}

void greetings::init_state_sets(batch* b)
{
	static cg_program_file vp("greetings.cg");
    state_set_ = b->create_state_set();
    state_set_->set_vertices(this);
    state_set_->set_vertex_program(&vp);
	state_set_->set_indices(this);
	state_set_->set_texture(0, this);
	state_set_->set_blending_mode(state_set::BLEND_TRANSPARENT);
}
void greetings::remove_state_sets(batch* b)
{
	b->remove_state_set(state_set_);
}
void greetings::update(float t)
{
	float size;
	float amplitude;
	size = 0.8f;
	float f = (t - 8.0f)*0.8f;
	amplitude = 0.1f*cos(f);

	//float amplitude = 0.0f; //0.2f*sin(t);

	state_set_->set_vertex_program_parameter(0, vector4f(2*t, 3*t, amplitude, size));

	vector4f start_placement(0, -1.8, 0, 1.0f);
	vector4f middle_placement(0.0f, 0.0f, 0, 1.0f);
	vector4f end_placement(0.0f, 1.8f, 0, 1.0f);
	float const move_time = 180.0f;
	if(t < move_time)
	{
		state_set_->set_vertex_program_parameter(1, start_placement);
	}
	else if(t < move_time + 10.0f)
	{
		float dt = (t - move_time)/10.0f;
		state_set_->set_vertex_program_parameter(1, start_placement*(1.0f-dt) + middle_placement*dt);	
	}
	else if(t < move_time + 20.0f)
	{
		state_set_->set_vertex_program_parameter(1, middle_placement);
	}
	else if(t < move_time + 30.0f)
	{
		float dt = (t - move_time - 20.0f)/10.0f;
		state_set_->set_vertex_program_parameter(1, middle_placement*(1.0f-dt) + end_placement*dt);	
	}
	else
	{
		state_set_->set_vertex_program_parameter(1, end_placement);	
	}
	
}
std::size_t greetings::vertex_count()
{
	return (TILES+1)*(TILES+1);
}

std::size_t greetings::index_count()
{
	return 6*TILES*TILES;
}

void greetings::get_vertex_declaration(vertex_declaration const** pdeclaration, std::size_t* count)
{
	namespace va = vertex_attribute;
	static vertex_declaration const declaration[] = {
        {0, va::POSITION, 0, va::FLOAT2}
    };
    *pdeclaration = declaration;
    *count = 1;
}

void greetings::produce_vertices(std::size_t stream, void* data)
{
    float* pv = static_cast<float*>(data);
	for(size_t y=0; y!=TILES + 1; ++y)
	{
		for(size_t x=0; x!=TILES + 1; ++x)
		{
			pv[0] = 2.0f*float(x)/TILES - 1.0f;
			pv[1] = 1.0f - 2.0f*float(y)/TILES;
			pv += 2;
		}
	}
}

void greetings::produce_indices(void* indices)
{
    uint16_t* pi = static_cast<uint16_t*>(indices);
	for(size_t y=0; y!=TILES; ++y)
	{
		for(size_t x=0; x!=TILES; ++x)
		{
			pi[0] = y*(TILES+1)+x + 0;
			pi[1] = y*(TILES+1)+x + 1;
			pi[2] = (y+1)*(TILES+1)+x + 0;

			pi[3] = (y+1)*(TILES+1)+x + 0;
			pi[4] = y*(TILES+1)+x + 1;
			pi[5] = (y+1)*(TILES+1)+x + 1;

			/*TRACE_OUT("Tile " << (y*TILES+x));
			TRACE_OUT(" " << pi[0]);
			TRACE_OUT(" " << pi[1]);
			TRACE_OUT(" " << pi[2]);
			TRACE_OUT(" " << pi[3]);
			TRACE_OUT(" " << pi[4]);
			TRACE_OUT(" " << pi[5]);*/

			pi += 6;
		}
	}
}
void greetings::get_texture_description(description *pdesc)
{
	pdesc->format = RGBA;
	pdesc->width = g_image->sizeX;
	pdesc->height = g_image->sizeY;
}
void greetings::produce_texture(void* data)
{
	uint8_t* pb = static_cast<uint8_t*>(data);
	for(size_t i=0; i!=g_image->sizeX*g_image->sizeY; ++i)
	{
		pb[4*i] = g_image->data[3*i];
		pb[4*i + 1] = g_image->data[3*i + 1];
		pb[4*i + 2] = g_image->data[3*i + 2];
		pb[4*i + 3] = g_image_mask->data[3*i];
		//pb[4*i + 3] = 127;
		/*uint8_t limit = 0;
		if(pb[4*i] <= limit && pb[4*i + 1] <= limit && pb[4*i+2] <= limit)
			pb[4*i + 3] = 0;
		else
			pb[4*i + 3] = 250;*/
	}
}
