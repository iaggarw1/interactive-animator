//-----------------------------------------------------------------------------
// COMP 259 Fall 2023
// HW01 - Builds with SKA Version 4.0
//-----------------------------------------------------------------------------
// AnimationControl.h
//     Animation control for a two bone arm with
//     direct control of the angles at the two joints.
//-----------------------------------------------------------------------------
#ifndef ANIMATIONCONTROL_DOT_H
#define ANIMATIONCONTROL_DOT_H
#include "Core/SystemConfiguration.h"
#include <string>
#include <list>
using namespace std;
//#include "Animation/Skeleton.h"
//#include "Animation/MotionSequence.h"
//#include "Animation/Skeleton.h"
#include "Objects/Object.h"
#include "Objects/BoneSequence.h"

struct AnimationControl
{
	bool ready;

	BoneSequence* bones;
	Object* target;
	short current_state;
	float current_time;
	float next_target_move;
	float target_move_delta;

	AnimationControl();
	virtual ~AnimationControl();

	void createAnimatedObjects(list<Object*>& render_list);
	bool updateAnimation(float _elapsed_time);
	bool isReady() { return ready; }

	// pass data out for display on HUD
	Vector3D getBoneAngle(int index) { return bones->getAngles(index); }
	Vector3D getBoneEndpoint(int index) { return bones->getEndpoint(index); }
	Vector3D getTargetPosition() { return target->currPosition(); }
	float getCurrentTime() { return current_time; }
};

extern AnimationControl anim_ctrl;

#endif // ANIMATIONCONTROL_DOT_H
