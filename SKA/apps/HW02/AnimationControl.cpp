//-----------------------------------------------------------------------------
// app0005 - Builds with SKA Version 4.0
//-----------------------------------------------------------------------------
// AnimationControl.cpp
//     Animation control for a two bone arm 
//     tracking a moving target.
//     This base version does not implement arm control.
//-----------------------------------------------------------------------------
#include "Core/SystemConfiguration.h"
#include <cstdio>
#include <complex>
#include "Models/ModelFactory.h"
#include "Core/Utilities.h"
#include "AppConfig.h"
#include "AnimationControl.h"

//---------------------------------------------
// Define target movement

static const int num_targets = 16;
Vector3D target_coords[num_targets] = {
	Vector3D(0.0f,   0.0f,  20.0f),
	Vector3D(20.0f,   0.0f,   0.0f),
	Vector3D(0.0f,   0.0f, -20.0f),
	Vector3D(-20.0f,   0.0f,   0.0f),
	Vector3D(0.0f,  20.0f,   0.0f),
	Vector3D(0.0f, -20.0f,   0.0f),
	Vector3D(0.0f,  10.0f,  10.0f),
	Vector3D(10.0f,   0.0f, -10.0f),
	Vector3D(-10.0f,  10.0f,   0.0f),
	Vector3D(0.0f, -10.0f,  10.0f),
	Vector3D(0.0f, 14.14f, 14.14f),
	Vector3D(0.0f, 7.071f, 17.071f),
	Vector3D(-7.071f,  0.0f, 17.071f),
	Vector3D(0.0f,   0.0f,   0.0f),
	Vector3D(0.0f,-14.14f,   0.0f),
	Vector3D(-5.0f,  -5.0f,-17.07f)
};

struct Angles
{
	Vector3D bone0;
	Vector3D bone1;
	Angles() { }
	Angles(const Vector3D& b0, const Vector3D& b1)
		: bone0(b0), bone1(b1) { }
};

// HW01 - Complete the assignment by giving the correct angles 
//        in the following table.
Angles joint_angles[num_targets] = {
	Angles(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, HALF_PI, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, PI, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f)),
	Angles(Vector3D(0.0f, 0.0f, 0.0f), Vector3D(0.0f, 0.0f, 0.0f))
};

// index of currently active target
static int current_target = 0;
// number of frames that target stays in each position
static int target_duration = 50;  
// count-down timer to next target change
static int target_timer = target_duration;
// current target position
Vector3D target_pos(0.0f, 15.0f, 7.0f);
// object drawn to visualize target position (declared in AppMain)
extern Object* target_box;

AnimationControl::AnimationControl()
{
	current_state = 1;
	current_time = 0.0f;
	target_move_delta = 2.0f; // target moves every 2 seconds
	next_target_move = current_time + target_move_delta;
}

AnimationControl::~AnimationControl()
{
	if (bones != NULL) { delete bones; bones = NULL; }
}

bool AnimationControl::updateAnimation(float _elapsed_time)
{
	if (!ready) return false;

	current_time += _elapsed_time;
	if (current_time >= next_target_move) {
		current_state = (current_state + 1) % num_targets;
		next_target_move = current_time + target_move_delta;
	}
	target->moveTo(target_coords[current_state]);
	bones->setAngles(0, joint_angles[current_state].bone0);
	bones->setAngles(1, joint_angles[current_state].bone1);
	bones->updateState(_elapsed_time);

	return true;
}

void AnimationControl::createAnimatedObjects(list<Object*>& render_list)
{
	bones = new BoneSequence();
	bones->addBone(10.0f);
	bones->addBone(10.0f);
	render_list.push_back(bones);

	bones->setAngles(0, Vector3D(0.0f, 0.0f, 0.0f));
	bones->setAngles(1, Vector3D(0.0f, 0.0f, 0.0f));
	bones->setVisibility(true);

	ModelSpecification boxspec("Box", Color(0.0f, 1.0f, 0.0f));
	boxspec.addSpec("length", "0.5");
	boxspec.addSpec("width", "0.5");
	boxspec.addSpec("height", "0.5");
	target = new Object(boxspec,
		target_coords[0],
		Vector3D(0.0f, 0.0f, 0.0f));
	target->setVisibility(true);
	render_list.push_back(target);

	ready = true;
}

AnimationControl anim_ctrl;

