/******************************************************************************
 * Hackerboat Beaglebone orientation module
 * orientation.hpp
 * This module stores orientations and functions for manipulating them
 * see the Hackerboat documentation for more details
 * Written by Pierce Nichols, Aug 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/
 
#ifndef ORIENTATION_H
#define ORIENTATION_H
 
#include <jansson.h>
#include "hal/config.h"
#include <cmath>
#include <string>
#include "hackerboatRoot.hpp"

/**
 * @class Orientation
 *
 * @brief An orientation calculated from data received from the IMU
 *
 */

class Orientation : public HackerboatState {
	public:
		Orientation() = default;
		Orientation(double r, double p, double y) :
			pitch(p), roll(r), heading(y) {};
		bool parse (json_t *input);
		json_t *pack () const;
		bool isValid ();
		bool normalize (void);		/**< Normalize the roll/pitch/heading */
		double roll 	= NAN;
		double pitch 	= NAN;
		double heading 	= NAN;

	private:
		double normAxis (double val, const double max, const double min) const;
		static const double constexpr	maxRoll = 180.0;
		static const double constexpr	minRoll = -180.0;
		static const double constexpr	maxPitch = 180.0;
		static const double constexpr	minPitch = -180.0;
		static const double constexpr	maxHeading = 360.0;
		static const double constexpr	minHeading = 0.0;
};

#endif
