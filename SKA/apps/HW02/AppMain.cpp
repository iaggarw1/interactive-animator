//-----------------------------------------------------------------------------
// app0005 - Builds with SKA Version 4.0
//-----------------------------------------------------------------------------
// AppMain.cpp
//-----------------------------------------------------------------------------
// SKA configuration.
#include <Core/SystemConfiguration.h>
// C/C++ libraries
#include <cstdio>
#include <cstdlib>
#include <iostream>
using namespace std;
// openGL library
#include <GL/glut.h>
// SKA modules
#include <Core/BasicException.h>
#include <Core/SystemTimer.h>
#include <Core/Utilities.h>
#include <DataManagement/DataManagementException.h>
#include <Input/InputManager.h>
#include <Graphics/GraphicsInterface.h>
#include <Graphics/Lights.h>
#include <Graphics/Textures.h>
#include <Graphics/Graphics2D.h>
#include <Models/SphereModels.h>
// local application
#include "AppConfig.h"
#include "AnimationControl.h"
#include "CameraControl.h"
#include "InputProcessing.h"

// pointers to animated objects that need to be drawn (bones)
list<Object*> anim_render_list;
// pointerts to background objects that need to be drawn
list<Object*> bg_render_list;

static int window_height = 800;
static int window_width = 800;

static bool draw_coord_axis = true;
static bool draw_sky = false;
static bool draw_ground = false;

//  background color
static float clear_color[4] = { 0.1f, 0.1f, 0.1f, 0.1f};

void shutDown(int _exit_code)
{
	exit(_exit_code);
}

void drawHUD()
{
	// extract data to be displayed
	Vector3D a0 = anim_ctrl.getBoneAngle(0);
	Vector3D a1 = anim_ctrl.getBoneAngle(1);
	Vector3D p0 = anim_ctrl.getBoneEndpoint(0);
	Vector3D p1 = anim_ctrl.getBoneEndpoint(1);
	Vector3D t = anim_ctrl.getTargetPosition();
	float current_time = anim_ctrl.getCurrentTime();

	// setup for 2D text rendering
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	Color c1(1.0f,1.0f,0.0f,1.0f);
	Color c2(0.0f,1.0f,1.0f,1.0f);
	Color black(0.0f,0.0f,0.0f,1.0f);

	// build and draw strings

	float x = -0.9f;
	float y = 0.9f;
	string s;

	s = "Time  : " + toString<float>(current_time);
	renderString(x, y, 0.0f, c1, s.c_str());
	y -= 0.05f;

	renderString(x, y, 0.0f, c1, "Bone 0");
	y -= 0.05f;
	s = "     roll  : " + toString<float>(a0.roll);
	renderString(x, y, 0.0f, c1, s.c_str());
	y -= 0.05f;
	s = "     pitch  : " + toString<float>(a0.pitch);
	renderString(x, y, 0.0f, c1, s.c_str());
	y -= 0.05f;
	s = "     yaw  : " + toString<float>(a0.yaw);
	renderString(x, y, 0.0f, c1, s.c_str());
	y -= 0.05f;
	s = "     x  : " + toString<float>(p0.x);
	renderString(x, y, 0.0f, c1, s.c_str());
	y -= 0.05f;
	s = "     y  : " + toString<float>(p0.y);
	renderString(x, y, 0.0f, c1, s.c_str());
	y -= 0.05f;
	s = "     z  : " + toString<float>(p0.z);
	renderString(x, y, 0.0f, c1, s.c_str());

	y = 0.9f;
	x = -0.4f;
	y -= 0.05f;
	renderString(x, y, 0.0f, c1, "Bone 1");
	y -= 0.05f;
	s = "     roll  : " + toString<float>(a1.roll);
	renderString(x, y, 0.0f, c1, s.c_str());
	y -= 0.05f;
	s = "     pitch : " + toString<float>(a1.pitch);
	renderString(x, y, 0.0f, c1, s.c_str());
	y -= 0.05f;
	s = "     yaw   : " + toString<float>(a1.yaw);
	renderString(x, y, 0.0f, c1, s.c_str());
	y -= 0.05f;
	s = "     x  : " + toString<float>(p1.x);
	renderString(x, y, 0.0f, c1, s.c_str());
	y -= 0.05f;
	s = "     y  : " + toString<float>(p1.y);
	renderString(x, y, 0.0f, c1, s.c_str());
	y -= 0.05f;
	s = "     z  : " + toString<float>(p1.z);
	renderString(x, y, 0.0f, c1, s.c_str());

	y = 0.9f;
	x = 0.1f;

	y -= 0.05f;

	renderString(x, y, 0.0f, c1, "Tip");
	y -= 0.05f;
	s = "     x  : " + toString<float>(p1.x);
	renderString(x, y, 0.0f, c1, s.c_str());
	y -= 0.05f;
	s = "     y  : " + toString<float>(p1.y);
	renderString(x, y, 0.0f, c1, s.c_str());
	y -= 0.05f;
	s = "     z  : " + toString<float>(p1.z);
	renderString(x, y, 0.0f, c1, s.c_str());
}

void display(void)
{
	double elapsed_time = system_timer.elapsedTime();

	input_processor.processInputs(elapsed_time);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	camera.setSceneView();
	glMatrixMode(GL_MODELVIEW);

	time_fade_factor = 1.0f;
	list<Object*>::iterator biter = bg_render_list.begin();
	while (biter != bg_render_list.end())
	{
		Object* go = (Object*)(*biter);
		if (go->isVisible())
		{
			Matrix4x4 world_xform;
			(*biter)->render(world_xform);
		}
		biter++;
	}

	if (anim_ctrl.ready)
	{
		if (anim_ctrl.updateAnimation(elapsed_time))
		{
			list<Object*>::iterator aiter = anim_render_list.begin();
			while (aiter != anim_render_list.end())
			{
				Object* go = (Object*)(*aiter);
				if (go->isVisible())
				{
					Matrix4x4 world_xform;
					(*aiter)->render(world_xform);
				}
				aiter++;
			}
		}
	}

	drawHUD();

	glutSwapBuffers();
	checkOpenGLError(203);
}

void buildObjects()
{
	texture_manager.addTextureFilepath((char*)TEXTURE_FILE_PATH);
	if (draw_sky)
	{
		char* skymapfile = (char*)"skymap1.bmp";
		SphereModel* skymod = new InvertedSphereModel(800, 3, Color(1.0f,1.0f,0.5f), skymapfile);
		Object* sky = new Object(skymod,
			Vector3D(0.0f,0.0f,0.0f), Vector3D(0.0f,0.0f,0.0f));
		bg_render_list.push_back(sky);
	}

	if (draw_coord_axis)
	{
		ModelSpecification caxisspec("CoordinateAxis");
		caxisspec.addSpec("length", "100");
		Object* caxis = new Object(caxisspec,
			Vector3D(0.0f,0.0f,0.0f), Vector3D(0.0f,0.0f,0.0f));
		bg_render_list.push_back(caxis);
	}

	if (draw_ground)
	{
		ModelSpecification groundspec("Ground");
		Object* ground = new Object(groundspec,
			Vector3D(0.0f,0.0f,0.0f), Vector3D(0.0f,0.0f,0.0f), Vector3D(1.0f,1.0f,1.0f));
		bg_render_list.push_back(ground);
	}
}

void initializeRenderer()
{
    glEnable(GL_DEPTH_TEST);
	glClearDepth(1.0f);
	glShadeModel(GL_SMOOTH);
	glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void initializeGLUT(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(window_width, window_height);
	glutCreateWindow("Animation");
}

int main(int argc, char **argv)
{
	anim_ctrl.createAnimatedObjects(anim_render_list);
	if (!anim_ctrl.isReady())
		logout << "main(): Unable to load characters. Proceeding with no animation." << endl;

	try
	{
		initializeGLUT(argc, argv);
		initializeRenderer();
		input_manager.registerGlutHandlers();

		glutDisplayFunc(display);
		glutIdleFunc(display);

		initializeDefaultLighting();
		camera.initializeCamera(window_width, window_height);
		camera.setCameraPreset(3);
		buildObjects();
		checkOpenGLError(202);
		system_timer.reset();
		glutMainLoop();
	}
	catch (BasicException& excpt)
	{
		logout << "BasicException caught at top level." << endl;
		logout << "Exception message: " << excpt.msg << endl;
		logout << "Aborting program." << endl;
		cerr << "Aborting due to exception. See log file for details." << endl;
		exit(1);
	}
}
