/******************************************************************************
 * Hackerboat Beaglebone orientation module
 * orientation.cpp
 * This module stores orientations and functions for manipulating them
 * see the Hackerboat documentation for more details
 * Written by Pierce Nichols, Aug 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/
 
#include <cmath>
#include <cstdlib>
#include <string>
#include "rapidjson/rapidjson.h"
#include <chrono>
#include <ctime>
#include "orientation.hpp"
#include <GeographicLib/MagneticModel.hpp>
#include <GeographicLib/Geocentric.hpp>
#include <GeographicLib/Constants.hpp>
#include "easylogging++.h"

using namespace GeographicLib;
using namespace std;
using namespace rapidjson;

bool Orientation::parse (Value& input) {
	bool result = true;

	result += GetVar("pitch", this->pitch, input);
	result += GetVar("roll", this->roll, input);
	result += GetVar("heading", this->heading, input);

	return (result && this->isValid() && this->normalize());
}

Value Orientation::pack () const {
	Value d;
	int packResult = 0;

	packResult += PutVar("pitch", this->pitch, d);
	packResult += PutVar("roll", this->roll, d);
	packResult += PutVar("heading", this->heading, d);
	
	return d;
}

bool Orientation::isValid () {
	return (std::isfinite(roll) && std::isfinite(pitch) && std::isfinite(heading));
}

bool Orientation::normalize () {
	//VLOG(2) << "Un-normalized orientation: " << *this;
	pitch 	+= 180.0;
	roll 	+= 180.0;
	pitch 	= normAxis(pitch, 360.0, 0.0);
	roll 	= normAxis(roll, 360.0, 0.0);
	heading = normAxis(heading, 360.0, 0.0);
	pitch 	-= 180.0;
	roll 	-= 180.0;
	//VLOG(2) << "Normalized orientation: " << *this;
	
	return this->isValid();
}

double Orientation::normAxis (double val, const double max, const double min) const {
	double span = (max - min);
	while (val < min) val += span;
	while (val > max) val -= span;
	return val;
}

double Orientation::headingError (double target) {
	if (!isValid() || !std::isfinite(target)) {
		LOG(ERROR) << "Attempting to get heading error from an invalid Orientation " << *this << " or to an invalid target course" << to_string(target);
		return NAN;
	}
	return normAxis((target - heading), 180.0, -180.0);
}

Orientation Orientation::makeTrue () {
	if (!this->isMagnetic()) {
		//LOG(ERROR) << "Course " << *this << " is already true";
		return *this;
	}
	Orientation output = *this;
	output.magnetic = false;
	output.heading += declination;
	output.normalize();
	//LOG(DEBUG) << "Magnetic declination is " << to_string(declination) << " new true Orientation is: " << output;
	return output;
}

Orientation Orientation::makeMag () {
	if (this->isMagnetic()) {
		//LOG(ERROR) << "Course " << *this << " is already magnetic";
		return *this;
	}
	Orientation output = *this;
	output.magnetic = true;
	output.heading -= declination;
	output.normalize();
	//LOG(DEBUG) << "Magnetic declination is " << to_string(declination) << " new magnetic Orientation is: " << output;
	return output;
}

bool Orientation::updateDeclination (Location loc) {
	if (!loc.isValid()) {
		LOG(ERROR) << "Attempted to get magnetic declination for invalid location " << loc;
		return false;
	}
	// time information for the mag model
	sysclock thisTime = std::chrono::system_clock::now();
	time_t tt = std::chrono::system_clock::to_time_t(thisTime);
	tm utc_tm = *gmtime(&tt);
	MagneticModel mag("emm2015");
	// intermediate values
	double Bx, By, Bz, H;
	// output values
	double strength, inclination;
	
	// Grab the magnetic components
	mag(utc_tm.tm_year + 1900, loc.lat, loc.lon, 0, Bx, By, Bz);
	
	// convert intermediate representation to the output
	MagneticModel::FieldComponents(Bx, By, Bz, H, strength, declination, inclination);
	LOG(DEBUG) << "Magnetic declination at location " << loc << " is " << declination;
	
	return true;
}


double Orientation::declination = 0;
