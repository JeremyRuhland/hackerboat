/******************************************************************************
 * Hackerboat Beaglebone navigation module
 * nav.hpp
 * This module stores locations and functions for manipulating them
 * see the Hackerboat documentation for more details
 * Written by Pierce Nichols, Feb 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/

#ifndef NAVIGATION_H
#define NAVIGATION_H 
 
#include <jansson.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "config.h"
#include "stateStructTypes.hpp"
#include "location.hpp"

/**
 * @class navVectorClass
 *
 * @brief Class for storing a navigation vector
 *
 */

class navVectorClass : public hackerboatStateClass {
	public:	
		navVectorClass (void) {};
		navVectorClass (std::string src, double bearing, double strength);
		bool isValid (void);					/**< Check for validity */	
		bool norm (void);						/**< Normalize the bearing */
		bool parse (json_t *input);				/**< Populate the object from the given json object */
		json_t *pack (void);					/**< Pack the contents of the object into a json object and return a pointer to that object*/
		navVectorClass add (navVectorclass a);	/**< Vector sum of the current vector and another vector */
		
		std::string	_source  	= "";		/**< Name of the source of this vector. */
		double 	_bearing 	= NAN;		/**< Bearing of this vector in degrees, clockwise from true north. */
		double	_strength 	= NAN;		/**< Relative strength of this vector */	
	protected:
		const char *getFormatString(void) {return _format;};		/**< Get format string for the object */
	private:
		static const char *_format = "{s:s,s:f,s:f}";	
};

/**
 * @class navClass
 *
 * @brief Stores the current state of the navigation computation 
 *
 */

class navClass : public hackerboatStateClassStorable {
	public:
		navClass (void) {};							
		bool parse (json_t *input);				/**< Populate the object from the given json object */
		json_t *pack (void);					/**< Pack the contents of the object into a json object and return a pointer to that object*/
		bool appendVector (navVectorClass vec);	/**< Add a navigation vector to the influence list */
		bool calc (double maxStrength);			/**< Calculate the course to the next waypoint and sum the navInfluences vectors */
		void clearVectors (void);				/**< Clear the contents of navInfluences */
		bool isValid(void);
		bool writeRecord (void);				/**< Write the current record to the target database file */
		bool getRecord(int32_t select);			/**< Populate the object from the open database file */
		
		locationClass	current;		/**< current location */	
		waypointClass	target;			/**< target waypoint */
		double			waypointStrength;
		double			magCorrection;	/**< Correction between sensed magnetic heading and true direction */
		navVector		targetVec;		/**< Vector to the target */
		navVector		total;			/**< Sum of target vector and all influences */
		
	protected:
		const char *getFormatString(void) {return _format;};		/**< Get format string for the object */
		
	private:
		static const char *_format = "{s:i,s:o,s:o,s:f,s:f,s:o,s:o,s:[o]}";	
		navVector[NAV_VEC_LIST_LEN]	navInfluences;	/**< Array to hold the influences of other navigation sources (i.e. collision avoidance) */
		uint16_t					influenceCount; /**< Number of influence vectors */
};

/** 
 * @class navigatorBase
 *
 * @brief Pure virtual base class for navigation elements other than waypoint
 *
 */

class navigatorBase {
	public:
		virtual navVectorClass calc(void) = 0;
		virtual bool isValid(void) = 0;
	protected:
		navigatorBase(void) = default;
};

/**
 * @class navDodgePointClass
 *
 * @brief 
 *
 */

class navDodgePointClass : public navigatorBase {
	public:
		navDodgePointClass(void) {};
		navDodgePointClass(locationClass point) {_point = point;};
		navDodgePointClass(locationClass point, double strength) {
			_point = point; 
			_strength = strength;
		}
		navVectorClass calc(void);
		void setLocation (locationClass point) {_point = point;};
		locationClass getLocation (void) {return _point;};
		void setStrength (double strength) {_strength = strength;};
		double getStrength (void) {return _strength;};
		bool isValid(void);
	private:
		locationClass _point;
		double _strength = 0;
};
 
/** 
 * @class navDitherClass
 *
 * @brief
 *
 */

class navDitherClass : public navigatorBase {
	public:
		navDitherClass(void) {};
		navDitherClass(double strength) {_strength = strength;};
		navDitherClass(double strength, double seed) {
			_seed = seed; 
			_strength = strength;
		}
		navVectorClass calc(void);
		void setStrength (double strength) {_strength = strength;};
		double getStrength (void) {return _strength;};
		void setSeed (double seed) {_seed = seed;};
		double getSeed (void) {return _seed;};
		bool isValid(void) {return (isnormal(_strength) && isnormal(_seed));};
	private:
		_strength = 0;
		_seed = HASHSEED;
};
 
#endif /* NAVIGATION_H */ 