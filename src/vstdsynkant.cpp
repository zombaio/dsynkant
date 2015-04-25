/****************************************************************************
    
    DSSI wrapper for DSynkant

    dssidsynkant.cpp

    Copyleft (c) 2015 Nil Geisweiller
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/

#include "vstdsynkant.hpp"
#include "pluginterfaces/vst2.x/aeffectx.h"

#include <iostream>

using namespace dsynkant;

// Main function
extern "C"
{
#define VST_EXPORT __attribute__ ((visibility ("default")))

	extern VST_EXPORT AEffect * VSTPluginMain(audioMasterCallback audioMaster);

	AEffect* main_plugin(audioMasterCallback audioMaster) asm ("main");
#define main main_plugin

	VST_EXPORT AEffect *main(audioMasterCallback audioMaster)
	{
		return VSTPluginMain(audioMaster);
	}
}

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
	return new VSTDSynkant(audioMaster, 1, 0);
}

// Plugin implementation

VSTDSynkant::VSTDSynkant(audioMasterCallback audioMaster,
                         long numPrograms, long numParams)
	: AudioEffectX(audioMaster, numPrograms, numParams),
	  events(nullptr)
{

	// Plugin id
	setUniqueID(CCONST('D', 'S', 'y', 'n'));

	// stereo output
	setNumInputs(0);	
	setNumOutputs(2);	

	// hasVu();   // deprecated
	canProcessReplacing();
}

VSTDSynkant::~VSTDSynkant()
{
}

void VSTDSynkant::processEvent(VstEvent* e) {
	if (e->type == kVstMidiType) {
		VstMidiEvent* midi_e = (VstMidiEvent*) e;
		char* midiData = midi_e->midiData;
		midi(midiData[0], midiData[1], midiData[2]);
	}
	else if (e->type == kVstSysExType) {
		VstMidiSysexEvent* sysex_e = (VstMidiSysexEvent*) e;
		dsynkant.sysex_process(sysex_e->dumpBytes,
		                       (unsigned char*)sysex_e->sysexDump);
	}
	else std::cerr << "Vst event of type " << e->type
	               << " is not implemented" << std::endl;
}

VstInt32 VSTDSynkant::processEvents(VstEvents* ev) {
	for (VstInt32 i = 0; i < ev->numEvents; i++) {
		processEvent(ev->events[i]);
	}
	return 1;
}

void VSTDSynkant::midi(unsigned char status,
                       unsigned char byte1, unsigned char byte2)
{
	if (status == NOTE_ON or status == NOTE_OFF)
	{
		unsigned char pitch = byte1, velocity = byte2;
		if (status == NOTE_ON and velocity > 0)
			dsynkant.noteOn_process(0, pitch, velocity);
		else if (status == NOTE_OFF or (status == NOTE_ON and velocity == 0))
			dsynkant.noteOff_process(0, pitch);
	} else {
		std::cerr << "Midi event (status=" << status
		          << ", byte1=" << byte1
		          << ", byte2=" << byte2
		          << ") not implemented" << std::endl;
	}
}

void VSTDSynkant::process(float **inputs, float **outputs,
                          VstInt32 sampleFrames)
{
	
	int i, cue, block;
	
	// Outputs buffers
	float* p1 = outputs[0];
	float* p2 = outputs[1];

	// Process audio on midi events
	if (events)
	{
		cue = 0;
		for (i = 0; i < events->numEvents; i++)
		{
			VstEvent* e = events->events[i];
			block = e->deltaFrames - cue;
			if (block > 0) {
				dsynkant.audio_process(p1, p2, block);
				p1 += block;
				p2 += block;
			}
			processEvent(e);
			cue = e->deltaFrames;
		}
	}

	// Process audio
	dsynkant.audio_process(p1, p2, sampleFrames - cue);

	// Release events pointer
	events = nullptr;
}

void VSTDSynkant::processReplacing(float **inputs, float **outputs,
                                   VstInt32 sampleFrames)
{
	process(inputs, outputs, sampleFrames);
}

VstIntPtr VSTDSynkant::dispatcher(VstInt32 opCode, VstInt32 index, VstIntPtr value,
                                  void *ptr, float opt)
{
	int result = 0;

	switch (opCode)
	{

	case effSetSampleRate:  // Set sample rate
		std::cerr << "effSetSampleRate not implemented" << std::endl;
		break;
	case effProcessEvents:	// Process events
		events = (VstEvents*)ptr;
		result = 1;
		break;
	default:                // Default 
		result = AudioEffectX::dispatcher(opCode, index, value, ptr, opt);
	}

	return result;
}

// // Set param

// void VSTDSynkant::setParameter(VstInt32 index, float value)
// {
// }

// // Get param

// float VSTDSynkant::getParameter(VstInt32 index)
// {
// 	return 0.0f;
// }

// // Get param name

// void VSTDSynkant::getParameterName (VstInt32 index, char *text)
// {
// }

// // Get param value

// void VSTDSynkant::getParameterDisplay (VstInt32 index, char *text)
// {
// }
