#include <StSoundLibrary/StSoundLibrary.h>
#include <GL/glut.h>
#include <boost/scoped_ptr.hpp>
#include "gl_device.hpp"
#include "scene.hpp"
#include "quad_strategy.hpp"
#include "debug.hpp"
#include "SoundServer.h"
#include "star_scroller.hpp"
#include "cloud_object.hpp"
#include "logo.hpp"
#include "greetings.hpp"

using boost::scoped_ptr;

scoped_ptr<gl_device> device;
scoped_ptr<sorted_batch> batch;

YMMUSIC *g_pmusic;

static void soundServerCallback(void *pbuffer,long size)
{
	if (g_pmusic)
	{
		int samples = size / sizeof(ymsample);
		ymMusicCompute(g_pmusic, (ymsample*)pbuffer, samples);
	}
}

__int64 g_start_time;
__int64 g_timer_frequency;

star_scroller sc_object;
cloud_object cloud;
::logo logo;
::greetings greetings;


vector4f g_first_color;
vector4f g_second_color(0.1f, 0.1f, 0.3f, 0.0f);
float g_first_color_time;
float g_second_color_time = 20.0f;
int current_beat = 0;

void redraw_func()
{
	__int64 now;
	QueryPerformanceCounter((LARGE_INTEGER*) &now);
	float t = float(double(now - g_start_time)/g_timer_frequency);
	if(t > 213.0f)
		exit(0);

    // TODO rename start to begin
	static float last_t;
	if(t - last_t > 1.0f)
	{
		//std::cout << "time = " << t << std::endl;
		last_t = t;
	}
	batch->update(t);
	if(t < g_second_color_time)
	{
		float f = (t - g_first_color_time)/(g_second_color_time - g_first_color_time);
		vector4f c = g_first_color*(1.0f-f) + g_second_color*f;
		glClearColor(c[0], c[1], c[2], c[3]);

	}
	else
	{
		if(t >= 34.0f)
		{
			int beat = (t - 34.0f) * 187.5f/(60*3);
			if(beat != current_beat)
			{
				g_first_color_time = t;
				g_second_color_time = t + 0.4f;
				//g_first_color = g_second_color*1.5f;
				g_first_color[0] = 0.3f;
				g_first_color[1] = 0.3f;
				g_first_color[2] = 0.4f;
				g_first_color[3] = 0.0f;
				current_beat = beat;
			}
		}
	}


    //glClearColor(0.1f, 0.1f, 0.3f, 0.0f);

    device->begin_frame();
    batch->present();
    device->end_frame();
    glutSwapBuffers();
}

void idle_func()
{
	redraw_func();
	//TRACE_OUT("idle func");
}

void keyboard_func(unsigned char key, int x, int y)
{
    (void) key;
    (void) x;
    (void) y;
	if(key == 27)
		exit(0);
}

#ifdef _DEBUG
int main(int argc, char* argv[])
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	int argc = 1;
	char* argv[] = {"evolver.exe"};
#endif
    glutInit(&argc, argv);
    glutInitWindowSize(512*4/3, 512);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    glutCreateWindow("driver");
	glutFullScreen();
    glutDisplayFunc(redraw_func);
    glutKeyboardFunc(&keyboard_func);

    //int const Width = 512;
    //int const Height = 512;
    //gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.1f,100.0f);	// Calculate The Aspect Ratio Of The Window

    device.reset(new gl_device);
    batch.reset(new sorted_batch(device.get()));
    //quad_strategy quad(0);
    //quad.init(batch.get(), 0);

    //wavefront_object object("box.obj");
    //object.init(batch.get());

	sc_object.set_visible_stars(1000);
	sc_object.attach(batch.get());

	//cloud.set_visible_stars(1000);
	cloud.attach(batch.get());

	logo.attach(batch.get());

	greetings.attach(batch.get());

	QueryPerformanceCounter((LARGE_INTEGER*) &g_start_time);
	QueryPerformanceFrequency((LARGE_INTEGER*) &g_timer_frequency);
	glutIdleFunc(idle_func);

	CSoundServer sound_server;

	YMMUSIC *pmusic = ymMusicCreate();
	if(!ymMusicLoad(pmusic, "mad_max-there_are_many_sheep_in_mongolia.ym"))
		return 1;

	if(!sound_server.open(soundServerCallback,500))
		return 1;

	ymMusicPlay(pmusic);
	g_pmusic = pmusic;

	glutMainLoop();
	return 0;
}
