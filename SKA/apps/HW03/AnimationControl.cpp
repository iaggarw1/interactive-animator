//-----------------------------------------------------------------------------
// HW02 - Builds with SKA Version 4.0
//-----------------------------------------------------------------------------
// AnimationControl.cpp
//    Animation controller for multiple characters.
//    This is intended to be used for HW2 - COMP 259 fall 2017.
//-----------------------------------------------------------------------------
// SKA configuration
#include <Core/SystemConfiguration.h>
// C/C++ libraries
#include <cstdio>
#include <complex>
// SKA modules
#include <Core/Utilities.h>
//#include <Animation/RawMotionController.h>
#include <Animation/AnimationException.h>
#include <Animation/Skeleton.h>
#include <DataManagement/DataManager.h>
#include <DataManagement/DataManagementException.h>
#include <DataManagement/BVH_Reader.h>
// local application
#include "AppConfig.h"
#include "AnimationControl.h"
#include "RenderLists.h"
#include "OpenMotionSequenceController.h"

// global single instance of the animation controller
AnimationControl anim_ctrl;

enum MOCAP_TYPE { BVH, AMC };

struct LoadSpec {
	MOCAP_TYPE mocap_type;
	float scale;
	Color color;
	string motion_file;
	string skeleton_file;
	LoadSpec(MOCAP_TYPE _mocap_type, float _scale, Color _color, string _motion_file, string _skeleton_file=string(""))
		: mocap_type(_mocap_type), scale(_scale), color(_color), motion_file(_motion_file), skeleton_file(_skeleton_file) { }
};

/*
const short NUM_CHARACTERS = 3;
LoadSpec load_specs[NUM_CHARACTERS] = {
	LoadSpec(AMC, 1.0f, Color(0.8f,0.4f,0.8f), string("02/02_01.amc"), string("02/02.asf")),
	LoadSpec(AMC, 1.0f, Color(1.0f,0.4f,0.3f), string("16/16_55.amc"), string("16/16.asf")),
	LoadSpec(BVH, 0.2f, Color(0.0f,1.0f,0.0f), string("avoid/Avoid 9.bvh"))
};
*/

const short NUM_CHARACTERS = 2; // Changed to 2 for convienience sake
LoadSpec load_specs[NUM_CHARACTERS] = {
	LoadSpec(BVH, 0.2f, Color(1.0f,0.0f,0.0f), string("walk_circle/walk_circle_1.bvh")),
	LoadSpec(BVH, 0.2f, Color(0.0f,1.0f,0.0f), string("walk_circle/walk_circle_2.bvh"))
	// LoadSpec(BVH, 0.2f, Color(0.0f,0.0f,1.0f), string("walk_circle/walk_circle_3.bvh"))
};

// Name of bone to track with dropped boxes
//const string tracking_target("ltoes"); // most ASF skeletons
const string tracking_target("LeftToeBase"); // most BVH skeletons
//const string tracking_target(""); // no tracking boxes

Object* createMarkerBox(Vector3D position, Color _color)
{
	ModelSpecification markerspec("Box", _color);
	markerspec.addSpec("length", "0.5");
	markerspec.addSpec("width", "0.5");
	markerspec.addSpec("height", "0.5");
	Object* marker = new Object(markerspec,	position, Vector3D(0.0f, 0.0f, 0.0f));
	return marker;
}

AnimationControl::AnimationControl() 
	: ready(false), run_time(0.0f), 
	global_timewarp(3.0f),
	next_marker_time(0.1f), marker_time_interval(0.1f), max_marker_time(20.0f)
{ 
	// Initializing new attributes in constructor
	first_sequence[0] = first_sequence[1] = true;
	prev_frame[0] = prev_frame[1] = -1;
	footfalls_processed = false;
	past_sequence_end[0] = past_sequence_end[1] = false;
	curr_time = 0.0f;

	// Time values obtained from observing animation reel
	char1_leftsteps = { // Time of Red character's steps; 14 total
		0.0f, 2.75f, 3.75f, 4.75f, 5.66f, 6.75f, 7.66f, 
		8.67f, 9.71f, 10.68f, 11.68f, 12.77f, 13.75f, 15.0f
	};
	char2_leftsteps = { // Time of Green character's steps; 23 total
		0.0f, 2.65f, 4.38f, 6.24f, 7.76f, 9.47f, 11.24f, 12.97f, 14.7f, 16.44f, 18.27f,
		19.92f, 21.49f, 23.32f, 25.09f, 26.89f, 28.52f, 30.46f, 32.19f, 33.92f, 35.56f,
		37.36f
	};
} 

AnimationControl::~AnimationControl()	
{		
	for (unsigned short c=0; c<characters.size(); c++)
		if (characters[c] != NULL) delete characters[c]; 
}

void AnimationControl::restart()
{ 
	render_lists.eraseErasables();
	run_time = 0; 
	updateAnimation(0.0f); 
	next_marker_time = marker_time_interval;
}

float AnimationControl::timeWarp(float run_time) {
	/* NOTES 
	*  - The system runs at 120fps. We know this because Red runs for ~1800 frames, and lasts 15 seconds
	*  - Green runs for ~38.5s, and has 4627 frames. This is also ~120fps
	*  - Red takes ~1 second between each left foot step, starting at ~2.75 -> Each step takes 120 frames
	*  - Green takes ~2s between each left foot step, starting at ~2.7
	*  - Red is ~38.9% faster than Green
	*  - Walking starts at frame 200, and one step is 120 frames for Red
	*  - warped_time = run_time * 0.389f; -> This makes Red's animation last as long as Green's
	*  ---------------------------------------------------------------------------------------------------
	*  Ideally, based off of the frames when the left toe hit the floor for Red (f1) and Green (f2),
	*  finding a time warp factor of f1/f2 would allow for Red to sync up with Green's footsteps.
	*  However, the collection of frames in the code wasn't working properly for some reason.
	*  The code probably would have been something like:
	*  
	*  for (int i = 0; i < char1_toeframes.size(); i++) {
	*		if (curr_time == char1_leftsteps[i]) {
	*			n = char1_toeframes[i] / char2_toeframes[i];
	*		}
	*  }
	* 
	*  That way, we could obtain a factor of f1/f2. Then, we'd simply alter run_time by doing:
	*  
	*	warped_time = run_time & n;
	* 
	*	For now, the time warp currently returned by this function is a simple math operation that
	*	gets the left step of Red relatively close to Green's, albeit they are off time with one
	*	another since Red takes longer to start than Green, and their foot paces are different.
	*/

	float warped_time = 0.0f;
	float n = 0.0f;

	n = fabs(toeposition[0][0] / toeposition[1][0]);
	warped_time = run_time * n;

	return warped_time;
}

bool AnimationControl::updateAnimation(float _elapsed_time)
{
	if (!ready) return false;

	// the global time warp can be applied directly to the elapsed time between updates
	float warped_elapsed_time = global_timewarp * _elapsed_time;

	if (!footfalls_processed) {
		run_time += warped_elapsed_time;
		for (unsigned short c = 0; c < characters.size(); c++)
		{
			if (characters[c] != NULL) characters[c]->update(run_time);

			// pull local time and frame out of each skeleton's controller
			// (dangerous upcast)
			OpenMotionSequenceController* controller = (OpenMotionSequenceController*)characters[c]->getMotionController();
			display_data.sequence_time[c] = controller->getSequenceTime();
			display_data.sequence_frame[c] = controller->getSequenceFrame();

			// Collecting toe position data from two characters during first playback cycle
			if (c < 2 && first_sequence[c]) { // c is the character index.
				int frame = controller->getSequenceFrame();
				Vector3D start, end;
				characters[c]->getBonePositions(tracking_target.c_str(), start, end);
				toeposition[c].push_back(end.getY());

				/* NOTE:
				*  Below is an attempt to collect the frames for Red and Green when
				*  their left toes hit the floor. This would be determined based off
				*  of the observed times when their left foots hit the floor.
				*  However, there seems to be an issue with the frame data collection when
				*  attempted this way.
				*/
				if (c == 0) {
					for (int i = 0; i < char1_leftsteps.size(); i++) {
						if (char1_leftsteps[i] == floorf(controller->getSequenceTime())) {
							char1_leftframes.push_back(frame);
						}
					}
				}
				else {
					for (int i = 0; i < char2_leftsteps.size(); i++) {
						if (char2_leftsteps[i] == floorf(controller->getSequenceTime())) {
							char2_leftframes.push_back(frame);
						}
					}
				}

				if (prev_frame[c] > frame) first_sequence[c] = false; // First cycle is complete.
				prev_frame[c] = frame;
			}
		}

		// Write collected data to a CSV file on first animation cycle for both character 1 and character 2
		if (!footfalls_processed && !first_sequence[0] && !first_sequence[1])
		{
			ofstream toedata("toepositions.csv");
			int l1 = toeposition[0].size();
			int l2 = toeposition[1].size();
			for (int f = 0; f < l1 || f < l2; f++)
			{
				if (f < l1 && f < l2) toedata << f << ", " << toeposition[0][f] << ", " << toeposition[1][f] << endl;
				else if (f < l1) toedata << f << ", " << toeposition[0][f] << ", " << 0 << endl;
				else toedata << f << ", " << 0 << ", " << toeposition[1][f] << endl;
			}
			footfalls_processed = true;
		}

		if (run_time >= next_marker_time && run_time <= max_marker_time)
		{
			Color color = Color(0.8f, 0.3f, 0.3f);
			Vector3D start, end;
			// drop box at the tracking target of the first skeleton
			if (tracking_target.length() > 0)
			{
				characters[0]->getBonePositions(tracking_target.c_str(), start, end);
				Object* marker = createMarkerBox(end, color);
				render_lists.erasables.push_back(marker);
				next_marker_time += marker_time_interval;
			}
		}
	}
	else if (footfalls_processed) { // Below ensures that the shorter animation will wait for the second one to finish before starting
		run_time += warped_elapsed_time;

		if (past_sequence_end[0] && past_sequence_end[1]) {
			run_time = 0.0f;
			past_sequence_end[0] = past_sequence_end[1] = false;
		}

		for (unsigned short c = 0; c < characters.size(); c++) {
			if (past_sequence_end[c])
				characters[c]->update(0.0f);
			else {
				if (c == 0)
					characters[c]->update(timeWarp(run_time)); // Give warped time to character 1
				else
					characters[c]->update(run_time);
			}

			OpenMotionSequenceController* controller =
				(OpenMotionSequenceController*)characters[c]->getMotionController(); 
			display_data.sequence_time[c] = controller->getSequenceTime();
			display_data.sequence_frame[c] = controller->getSequenceFrame(); 
			int frame = controller->getSequenceFrame(); 
			curr_time = floorf(controller->getSequenceTime()); 
			if (prev_frame[c] > frame) {
				past_sequence_end[c] = true;
			}
			prev_frame[c] = frame;
		}
	}
	return true;
}

static Skeleton* buildCharacter(
	Skeleton* _skel, 
	MotionSequence* _ms, 
	Color _bone_color, 
	const string& _description1, 
	const string& _description2,
	vector<Object*>& _render_list)
{
	if ((_skel == NULL) || (_ms == NULL)) return NULL;

	OpenMotionSequenceController* controller = new OpenMotionSequenceController(_ms);

	//! Hack. The skeleton expects a list<Object*>, we're using a vector<Object*>
	list<Object*> tmp;
	_skel->constructRenderObject(tmp, _bone_color);
	list<Object*>::iterator iter = tmp.begin();
	while (iter != tmp.end()) { _render_list.push_back(*iter); iter++; }
	//! EndOfHack.
	
	_skel->attachMotionController(controller);
	_skel->setDescription1(_description1.c_str());
	_skel->setDescription2(_description2.c_str());
	return _skel;
}

void AnimationControl::loadCharacters()
{
	data_manager.addFileSearchPath(AMC_MOTION_FILE_PATH);
	data_manager.addFileSearchPath(BVH_MOTION_FILE_PATH);

	Skeleton* skel = NULL;
	MotionSequence* ms = NULL;
	string descr1, descr2;
	char* filename1 = NULL;
	char* filename2 = NULL;
	Skeleton* character = NULL;
	pair<Skeleton*, MotionSequence*> read_result;

	for (short c = 0; c < NUM_CHARACTERS; c++)
	{
		if (load_specs[c].mocap_type == AMC)
		{
			try
			{
				filename1 = data_manager.findFile(load_specs[c].skeleton_file.c_str());
				if (filename1 == NULL)
				{
					logout << "AnimationControl::loadCharacters: Unable to find character ASF file <" << load_specs[c].skeleton_file << ">. Aborting load." << endl;
					throw BasicException("ABORT 1A");
				}
				filename2 = data_manager.findFile(load_specs[c].motion_file.c_str());
				if (filename2 == NULL)
				{
					logout << "AnimationControl::loadCharacters: Unable to find character AMC file <" << load_specs[c].motion_file << ">. Aborting load." << endl;
					throw BasicException("ABORT 1B");
				}
				try {
					read_result = data_manager.readASFAMC(filename1, filename2);
				}
				catch (const DataManagementException& dme)
				{
					logout << "AnimationControl::loadCharacters: Unable to load character data files. Aborting load." << endl;
					logout << "   Failure due to " << dme.msg << endl;
					throw BasicException("ABORT 1C");
				}
			}
			catch (BasicException&) {}
		}
		else if (load_specs[c].mocap_type == BVH)
		{
			try
			{
				filename1 = data_manager.findFile(load_specs[c].motion_file.c_str());
				if (filename1 == NULL)
				{
					logout << "AnimationControl::loadCharacters: Unable to find character BVH file <" << load_specs[c].motion_file << ">. Aborting load." << endl;
					throw BasicException("ABORT 2A");
				}
				try
				{
					read_result = data_manager.readBVH(filename1);
				}
				catch (const DataManagementException& dme)
				{
					logout << "AnimationControl::loadCharacters: Unable to load character data files. Aborting load." << endl;
					logout << "   Failure due to " << dme.msg << endl;
					throw BasicException("ABORT 2C");
				}
			}
			catch (BasicException&) {}
		}

		try
		{
			skel = read_result.first;
			ms = read_result.second;
			
			skel->scaleBoneLengths(load_specs[c].scale);
			ms->scaleChannel(CHANNEL_ID(0, CT_TX), load_specs[c].scale);
			ms->scaleChannel(CHANNEL_ID(0, CT_TY), load_specs[c].scale);
			ms->scaleChannel(CHANNEL_ID(0, CT_TZ), load_specs[c].scale);

			// create a character to link all the pieces together.
			descr1 = string("skeleton: ") + load_specs[c].skeleton_file;
			descr2 = string("motion: ") + load_specs[c].motion_file;

			character = buildCharacter(skel, ms, load_specs[c].color, descr1, descr2, render_lists.bones);
			if (character != NULL) characters.push_back(character);
		}
		catch (BasicException&) {}
		
		strDelete(filename1); filename1 = NULL;
		strDelete(filename2); filename2 = NULL;
	}

	display_data.num_characters = (short)characters.size();
	display_data.sequence_time.resize(characters.size());
	display_data.sequence_frame.resize(characters.size());

	if (characters.size() > 0) ready = true;
}


