/******************************************************************************
 * Hackerboat Beaglebone types module
 * boneState.cpp
 * see the Hackerboat documentation for more details
 * Written by Pierce Nichols, Jan 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/
 
#include <jansson.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>
#include "config.h"
#include "json_utilities.hpp"
#include "location.hpp"
#include "logs.hpp"
#include "stateStructTypes.hpp"
#include "gps.hpp"
#include "boneState.hpp"
#include "arduinoState.hpp"
#include "sqliteStorage.hpp"

#include <string>
using namespace std;
 
const enumerationNameTable<boatModeEnum> boneStateClass::modeNames = {
	"Start", 
	"SelfTest", 
	"Disarmed", 
	"Fault",
	"Armed", 
	"Manual", 
	"WaypointNavigation",
	"LossOfSignal", 
	"ReturnToLaunch", 
	"ArmedTest",
	"None"
};

boneStateClass::boneStateClass() {
}

json_t *boneStateClass::pack (bool seq) const {
	json_t *output;
	output = json_pack("{s:o,s:o,s:o,s:o,s:o,s:o,s:o,s:i,s:f,s:f,s:f,s:b,s:o}",
			   "uTime", packTimeSpec(uTime),
			   "lastContact", packTimeSpec(lastContact),
			   "mode", json(mode),
			   "command", json(command),
			   "ardMode", json(ardMode),
			   "faultString", json_string(faultString.c_str()), // Pierce - fix this
			   "gps", gps.pack(seq /* Pierce: FIXME - what should we pass for seq? */),
			   "waypointNext", waypointNext,
			   "waypointStrength", waypointStrength,
			   "waypointAccuracy", waypointAccuracy,
			   "waypointStrengthMax", waypointStrengthMax,
			   "autonomous", autonomous,
			   "launchPoint", launchPoint.pack());
	if (seq) json_object_set(output, "sequenceNum", json_integer(_sequenceNum));
	return output;
}

bool boneStateClass::parse (json_t *input, bool seq = true) {
	json_t *inGNSS, *inUtime, *inLastContact, *inMode, *inCommand, *inArdMode, *inFaultString, *inLaunch;
	if (json_unpack(input, "{s:o,s:o,s:o,s:o,s:o,s:o,s:o,s:i,s:F,s:F,s:F,s:b,s:o}",
						"uTime", &inUtime,
						"lastContact", &inLastContact,
						"mode", &inMode,
						"command", &inCommand,
						"ardMode", &inArdMode,
						"faultString", &inFaultString,
						"gps", &inGNSS,
						"waypointNext", &waypointNext,
						"waypointStrength", &waypointStrength,
						"waypointAccuracy", &waypointAccuracy,
						"waypointStrengthMax", &waypointStrengthMax,
						"autonomous", &autonomous,
						"launchPoint", &inLaunch)) {
		return false;
	}
	if (seq) {
		json_t *seqIn = json_object_get(input, "sequenceNum");
		if (!seqIn)
			return false;
		_sequenceNum = json_integer_value(seqIn);
	}
	if (!::parse(inUtime, &uTime) ||
	    !::parse(inLastContact, &lastContact) ||
	    !::parse(inFaultString, &faultString) ||
	    !gps.parse(inGNSS, seq /* Pierce: FIXME - what should seq be? */ ) ||
	    !launchPoint.parse(inLaunch))
		return false;
	{
		Mode tmp_mode;
		if (!::parse(inMode, &tmp_mode))
			return false;
		if (!setMode(tmp_mode))
			return false;
	}
	{
		Mode tmp_mode;
		if (!::parse(inCommand, &tmp_mode))
			return false;
		if (!setCommand(tmp_mode))
			return false;
	}
	{
		arduinoModeEnum tmp_mode;
		if (!::parse(inArdMode, &tmp_mode))
			return false;
		if (!setArduinoMode(tmp_mode))
			return false;
	}
	return this->isValid();	
}

bool boneStateClass::isValid (void) const {
	if (!modeNames.valid(mode)) return false;
	if (!modeNames.valid(command)) return false;
	if (!arduinoStateClass::modeNames.valid(ardMode)) return false;
	if (!gps.isValid()) return false;
	if (!launchPoint.isValid()) return false;
	if (waypointStrength < 0) return false;
	if (waypointAccuracy < 0) return false;
	if (waypointStrengthMax < 0) return false;
	return true;
}

bool boneStateClass::insertFault (const string fault) {
	if (!this->hasFault(fault)) {
		faultString += fault + ":";
	}
	return true;
}

bool boneStateClass::hasFault (const string fault) {
	if (faultString.find(fault) != std::basic_string::npos) return true;
	return false;
}

bool boneStateClass::removeFault (const string fault) {
	size_t index;
	index = faultString.find(fault);
	if (index != std::string::npos) {
		faultString.erase(index, fault.length() + 1);	// captures the trailing colon
		return true;
	} else return false;
}

int boneStateClass::failCount (void) {
	size_t index = faultString.find(':');
	int cnt = 0;
	while (index != std::string::npos) {
		cnt++;
		index = faultString.find(':', index);
	}
	return cnt;
}

bool boneStateClass::setMode (boneModeEnum s) {
	if ((s > boneStateCount) || (s < 0)) return false;
	state = s;
	return true;
}

bool boneStateClass::setCommand (boneStateEnum c) {
	if ((c > boneStateCount) || (c < 0)) return false;
	command = c;
	return true;
}

bool boneStateClass::setArduinoMode (arduinoStateClass::Mode s) {
	if ((s > arduinoStateClass::arduinoStateCount) || (s < 0)) return false;
	ardState = s;
	return true;
}
		
void boneStateClass::initHashes (void) {
	for (int i = 0; i < boneStateCount; i++) {
		MurmurHash3_x86_32(boneStates[i].c_str(), boneStates[i].length(), HASHSEED, &(stateHashes[i]));
	}
}
