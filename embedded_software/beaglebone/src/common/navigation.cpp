/******************************************************************************
 * Hackerboat Beaglebone navigation module
 * navigation.cpp
 * This module stores and processes navigation data
 * see the Hackerboat documentation for more details
 * Written by Pierce Nichols, Feb 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/

#include <jansson.h>
#include <stdlib.h>
#include <math.h>
#include "config.h"
#include "stateStructTypes.hpp"
#include "location.h"
#include "navigation.hpp"

navVectorClass::navVectorClass (string src, double bearing, double strength) {
	_source = src;
	_bearing = bearing;
	_strength = strength;
}

bool inline navVectorClass::isValid (void) const {
	if ((_bearing >= 0) && (_bearing <= 360) && 
		(_strength >= 0) && isnormal(_bearing) &&
		isnormal(_strength)) return true;
	return false;
}

bool navVectorClass::norm (void) {
	if (isnormal(_strength) && isnormal(_bearing)) {
		while (!this->isValid()) {
			_strength = abs(_strength);
			if (_bearing > 360) {
				_bearing -= 360;
			} else if (_bearing < 0) {
				_bearing += 360;
			}
		}
		return true;
	} else return false;
}

navVectorClass navVectorClass::add (navVectorClass a) {
	// formulas from http://math.stackexchange.com/questions/1365622/adding-two-polar-vectors
	navVectorClass out;
	if ((!a.isValid()) && (!this->isValid())) return out;
	double deltaBearing;
	double r1sq, r2sq, r1r2;
	deltaBearing = locationClass::deg2rad(a._bearing - this->_bearing);
	r1sq = pow(this->_strength, 2);
	r1sq = pow(a._strength, 2);
	r1r2 = a._strength * this->_strength;
	out._strength = sqrt(r1sq + r2sq + (2 * r1r2 * cos(deltaBearing)));
	out._bearing = this->_bearing + locationClass::rad2deg(acos((this->_strength + (a._strength * cos(deltaBearing)))/out._strength)); 
	return out;
}

bool navVectorClass::parse(json_t *input) {
	char buf[LOCAL_BUF_LEN];
	if (json_unpack(input, this->_format, "source", buf, "bearing", &_bearing, "strength", &_strength)) {
		return false;
	}
	this->_source.assign(buf, LOCAL_BUF_LEN);
	return this->isValid();	
}

json_t* navVectorClass::pack(void) {
	return json_pack(this->_format, , "bearing", _bearing, "strength", _strength));
}

bool navClass::parse(json_t *input) {
	json_t *navArrayIn, *currentIn, *targetIn, *targetVecIn, *totalIn;
	if (json_unpack(input, this->_format, 	"sequenceNum", &_sequenceNum,
											"current", currentIn,
											"target", targetIn,
											"waypointStrength", &waypointStrength,
											"magCorrection", &magCorrection,
											"targetVec", targetVecIn,
											"total", totalIn,
											"navInfluences", navArrayIn)) {
		return false;										
	}
	current.parse(currentIn));
	target.parse(targetIn));
	targetVec.parse(targetVecIn));
	total.parse(totalIn));
	influenceCount = json_array_size(navArrayIn);
	if (influenceCount > NAV_VEC_LIST_LEN) influenceCount = NAV_VEC_LIST_LEN;
	for (uint16_t i; i < influenceCount; i++) {
		navInfluences[i].parse(json_array_get(navInfluences, i));
	}
	free(navArrayIn);
	free(currentIn);
	free(targetIn);
	free(targetVecIn);
	free(totalIn);
	return this->isValid();
}

json_t* navClass::pack (void) {
	json_t *array;
	array = json_array();
	for (uint16_t i; i < influenceCount; i++) {
		json_array_append_new(array, navInfluences[i].pack());
	}
	return json_pack(this->_format,  	"sequenceNum", _sequenceNum,
										"current", current.pack(),
										"target", target.pack(),
										"waypointStrength", waypointStrength,
										"magCorrection", magCorrection,
										"targetVec", targetVec.pack(),
										"total", total.pack(),
										"navInfluences", array));
}

bool navClass::appendVector (navVectorClass vec) {
	if (vec.isValid() && (this->influenceCount < NAV_VEC_LIST_LEN)) {
		this->navVector[this->influenceCount] = vec;
		this->influenceCount++;
		return true;
	} else return false;
}

bool navClass::calc (double maxStrength) {
	targetVec._bearing = this->current.bearing(target.location, locationClass::RhumbLine);
	targetVec._strength = waypointStrength;
	if (targetVec.isValid()) {
		total = targetVec;
	} else return false;
	for (uint16_t i; i < influenceCount; i++) {
		if (navInfluences[i].isValid()) {
			total  = total.add(navInfluences[i]);
		} 
		if (!total.isValid()) return false;
	}
	if (total._strength > maxStrength) total.strength = maxStrength;
	return true;
}

void navClass::clearVectors (void) {
	this->influenceCount = 0;
}

bool navClass::isValid (void) const {
	bool out = false;
	if (current.isValid() && target.isValid() &&
		targetVec.isValid() && total.isValid() &&
		isnormal(waypointStrength) && isnormal(magCorrection)) {
		out = true;
	} else out = false;
	if (out && this->influenceCount > 0) {
		if (this->influenceCount <= NAV_VEC_LIST_LEN) {
			for (uint16_t i; i < influenceCount; i++) {
				out &= navInfluences[i].isValid();
			}
		} else out = false;
	}
	return out;
}