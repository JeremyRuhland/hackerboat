/******************************************************************************
 * Hackerboat Beaglebone types module
 * stateStructTypes.hpp
 * This modules is compiled into the other modules to give a common interface
 * to the database(s)
 * see the Hackerboat documentation for more details
 * Written by Pierce Nichols, Jan 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/

#ifndef STATESTRUCTTYPES_H
#define STATESTRUCTTYPES_H
 
#include <jansson.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <string>
#include <memory>
#include <vector>
#include "config.h"
#include "location.hpp"
#include "gps.hpp"

using namespace string;

// buffer & string sizes
#define STATE_STRING_LEN		30
#define GPS_SENTENCE_LEN		120
#define	FAULT_STRING_LEN		1024
#define NAV_SOURCE_NAME_LEN		30
#define NAV_VEC_LIST_LEN		30

// value limits 

/**
 * @class hackerboatStateClass 
 * 
 * @brief Base class for holding various types of object used by the core functions of the Hackerboat
 *
 */

class hackerboatStateClass {
	public:
		virtual bool parse (json_t *input, bool seq = true);/**< Populate the object from the given json object. If seq is true, a sequence number element is expected */
		virtual json_t *pack (bool seq = true);				/**< Pack the contents of the object into a json object and return a pointer to that object. If seq is true, a sequence number element will be included */
		virtual bool isValid (void) const {return true;};	/**< Tests whether the current object is in a valid state */
		
	protected:	
		hackerboatStateClass(void) = default;
		json_t *packTimeSpec (timespec t);
		int parseTimeSpec (json_t *input, timespec *t);
};

/**
 * @class hackerboatStateClassStorable 
 * 
 * @brief Base class for holding various types of object used by the core functions of the Hackerboat
 *
 * This base class connects to a database of records containing instances of the object type.  
 *
 */

class hackerboatStateClassStorable : public hackerboatStateClass {
	public:
		hackerboatStateClassStorable(void);
		hackerboatStateClassStorable(const string file);		/**< Create a state object attached to the given file */
		int32_t getSequenceNum (void) {return _sequenceNum;};	/**< Get the sequenceNum of this object (-1 until populated from a file) */
		bool openFile(const string name);						/**< Open the given database file & store the name */
		bool openFile(void);									/**< Open the stored database file */
		bool closeFile(void);									/**< Close the open file */
		int32_t count (void);									/**< Return the number of records of the object's type in the open database file */
		bool writeRecord (void);								/**< Write the current record to the target database file */
		bool getRecord(int32_t select);							/**< Populate the object from the open database file */
		bool getLastRecord(void);								/**< Get the latest record */
		virtual bool insert(int32_t num) {return false;};		/**< Insert the contents of the object into the database table at the given point */
		bool append(void);										/**< Append the contents of the object to the end of the database table */
		
	protected:
		int32_t 	_sequenceNum = -1;	/**< sequence number */
		string 		_fileName;			/**< database filename (with path) */
		sqlite3 	*_db;				/**< database handle */
};

/**
 * @class orientationClass
 *
 * @brief An orientation, received from the Arduino 
 *
 */

class orientationClass : public hackerboatStateClass {
	public:
		orientationClass(double r, double p, double y):
							pitch(p), roll(r), yaw(y);
		bool normalize (void);
		double roll 	= NAN;
		double pitch 	= NAN;
		double heading 	= NAN;
	private:
		static const string _format = "{s:f,s:f,s:f}";
		static const double	maxVal = 180.0;
		static const double	minVal = -180.0;
};
		
/**
 * @class waypointClass
 *
 * @brief This is the class for storing & manipulating waypoints
 *
 */
 
class waypointClass : public hackerboatStateClassStorable {
	public:	
		enum actionEnum {
			STOP = 0,						
			HOME = 1,
			CONTINUE = 2,
		};
		typedef int16_t indexT;
		waypointClass (void);
		waypointClass (locationClass loc, actionEnum action = CONTINUE); 	/**< Create a waypoint at loc with action */
		
		bool			setAction(actionEnum action);			/**< Set the action to take when this waypoint is reached */
		actionEnum		getAction(void);						/**< Return the action that this waypoint is set to */

		waypointClass 	*getNextWaypoint(void);					/**< return the next waypoint to travel towards */
		bool			setNextWaypoint(waypointClass* next);		/**< Set the next waypoint to the given object (works only if it has a sequenceNum > 0; renumber indices as necessary */
		bool			setNextWaypoint(indexT index);			/**< As above, but set by current index; renumbering proceeds as above */
		indexT			getNextIndex(void);				/**< Return the index of the next waypoint */

		/* Concrete implementations of stateClassStorable */
		bool parse (json_t *, bool);
		json_t *pack (bool) const;
		bool isValid (void) const;

		locationClass location;
	private:
		indexT			index = -1;					/**< Place of this waypoint in the waypoint list */ 
		sequence		nextWaypoint = -1;				/**< _sequenceNum of the next waypoint */
		actionEnum		act;						/**< Action to perform when reaching a location */	
		static const int8_t minActionEnum = 0;
		static const int8_t maxActionEnum = 3;

	protected:
		/* Concrete implementations of stateClassStorable */
		virtual hackerboatStateStorage& storage();
		virtual bool fillRow(sqliteParameterSlice) const;
		virtual bool readFromRow(sqliteRowReference, sequence);
};
static const char *string(waypointClass::actionEnum);

/**
 * @class boneStateClass
 *
 * @brief Class for storing the current state of the Beaglebone element
 *
 */

class boneStateClass : public hackerboatStateClassStorable {
	public:
		/**
		 * @brief Beaglebone state
		 */
		enum boneStateEnum {
			BONE_START			= 0,  		/**< Initial starting state         */
			BONE_SELFTEST		= 1,  		/**< Initial self-test            */
			BONE_DISARMED		= 2,  		/**< Disarmed wait state          */  
			BONE_FAULT			= 3,		/**< Beaglebone faulted           */ 
			BONE_ARMED			= 4,		/**< Beaglebone armed & ready to navigate */ 
			BONE_MANUAL			= 5,		/**< Beaglebone manual steering       */ 
			BONE_WAYPOINT		= 6,		/**< Beaglebone navigating by waypoints   */
			BONE_NOSIGNAL		= 7,		/**< Beaglebone has lost shore signal    */
			BONE_RETURN			= 8,		/**< Beaglebone is attempting to return to start point */
			BONE_ARMEDTEST		= 9,		/**< Beaglebone accepts all commands that would be valid in any unsafe state */
			BONE_NONE			= 10		/**< State of the Beaglebone is currently unknown	*/
		};
	
		bool insertFault (const string fault);	/**< Add the named fault to the fault string. Returns false if fault string is full */
		bool removeFault (const string fault);	/**< Remove the named fault to the fault string. Returns false if not present */
		bool hasFault (const string fault);		/**< Returns true if given fault is present */
		int faultCount (void);					/**< Returns the current number of faults */
		bool setState (boneStateEnum s);		/**< Set state to the given value */
		bool setCommand (boneStateEnum c);		/**< Set command to the given value */
		bool setArduinoState (arduinoStateClass::arduinoStateEnum s); /**< Set Arduino state to the given value */
		
		timespec 					uTime;				/**< Time the record was made */
		timespec					lastContact;		/**< Time of the last contact from the shore station */
		boneStateEnum				state = BONE_NONE;	/**< current state of the beaglebone */	
		string						stateString;		/**< current state of the beaglebone, human readable string */
		boneStateEnum				command = BONE_NONE;/**< commanded state of the beaglebone */
		string						commandString;		/**< commanded state of the beaglebone, human readable string */
		arduinoStateClass::arduinoStateEnum			ardState;		/**< current state of the Arduino */
		string						ardStateString;		/**< current state of the Arduino, human readable string */
		string						faultString;		/**< comma separated list of faults */
		gpsFixClass					gps;				/**< current GPS position */
		int32_t						waypointNext;		/**< ID of the current target waypoint */
		double						waypointStrength;	/**< Strength of the waypoint */
		double						waypointAccuracy;	/**< How close the boat gets to each waypoint before going to the next one */
		double						waypointStrengthMax;/**< Maximum waypoint strength */
		bool						autonomous;			/**< When set true, the boat will operate autonomously */	
		locationClass				launchPoint;		/**< Location from which the boat departed */
		
		static const string boneStates[] = {
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
		static const uint8_t boneStateCount = 11;
		
	private:
		void initHashes (void);								/**< Initialize state name hashes */
		static const char *_format = "{s:o,s:o,s:i,s:s,s:i,s:s,s:i,s:s,s:s,s:o,s:i,s:f,s:f,s:f,s:b,s:o}";
		static uint32_t stateHashes[boneStateCount];		/**< All the state names, hashed for easy lookup */
		

};


/**
 * @class arduinoStateClass
 *
 * @brief Class for storing the current state of the Arduino element
 *
 */

class arduinoStateClass : public hackerboatStateClassStorable {
	public:
	
		/**
		 * @brief An enum to store the current state of the Arduino.
		 */
		enum arduinoStateEnum {
			BOAT_POWERUP     	= 0,  		/**< The boat enters this state at the end of initialization */
			BOAT_ARMED			= 1,  		/**< In this state, the boat is ready to receive go commands over RF */
			BOAT_SELFTEST   	= 2,  		/**< After powerup, the boat enters this state to determine whether it's fit to run */
			BOAT_DISARMED   	= 3,  		/**< This is the default safe state. No external command can start the motor */
			BOAT_ACTIVE     	= 4,  		/**< This is the normal steering state */
			BOAT_LOWBATTERY   	= 5,  		/**< The battery voltage has fallen below that required to operate the motor */
			BOAT_FAULT    		= 6,  		/**< The boat is faulted in some fashion */
			BOAT_SELFRECOVERY 	= 7,   		/**< The Beaglebone has failed and/or is not transmitting, so time to self-recover*/
			BOAT_ARMEDTEST		= 8,		/**< The Arduino is accepting specific pin read/write requests for hardware testing. */
			BOAT_ACTIVERUDDER	= 9,		/**< The Arduino is accepting direct rudder commands */
			BOAT_NONE			= 10		/**< Provides a null value for no command yet received */
		};        

		arduinoStateClass (void);
		arduinoStateClass (const char *file, size_t len);
		
		bool populate (void);	/**< Populate the object from the named interface */
		bool setCommand (arduinoStateEnum c);
		
		// Command functions...
		bool writeBoneState(boneStateClass::boneStateEnum s);
		bool writeCommand(void);
		int16_t writeThrottle(void);
		double writeHeadingTarget(void);
		double writeHeadingDelta(double delta);
		bool heartbeat(void);
		
		bool 				popStatus;				/**< State of whether the last call to populate() succeeded or failed */
		timespec			uTime;					/**< Time the record was made */
		arduinoStateEnum 	state;					/**< The current state of the boat                    */
		arduinoStateEnum	command;				/**< Last state command received by the Arduino */
		int8_t		 		throttle;   			/**< The current throttle position                    */
		boneStateClass::boneStateEnum 	bone;		/**< The current state of the BeagleBone                */
		orientationClass	orientation;			/**< The current accelerometer tilt and magnetic heading of the boat  */
		float 				headingTarget;			/**< The desired magnetic heading                     */  
		float 				internalVoltage;		/**< The battery voltage measured on the control PCB          */
		float 				batteryVoltage;			/**< The battery voltage measured at the battery            */
		float				motorVoltage;
		bool				enbButton;				/**< State of the enable button. off = 0; on = 0xff           */
		bool				stopButton;				/**< State of the emergency stop button. off = 0; on = 0xff       */
		long 				timeSinceLastPacket;	/**< Number of milliseconds since the last command packet received    */
		long 				timeOfLastPacket;		/**< Time the last packet arrived */
		long 				timeOfLastBoneHB;	
		long 				timeOfLastShoreHB;
		string				stateString;
		string 				boneStateString;
		string				commandString;
		uint16_t			faultString;			/**< Fault string -- binary string to indicate source of faults */
		float 				rudder;
		int16_t				rudderRaw;
		int16_t				internalVoltageRaw;
		int16_t				motorVoltageRaw;
		float				motorCurrent;
		int16_t				motorCurrentRaw;
		float				Kp;
		float				Ki;
		float				Kd;
		float	 			magX;
		float 				magY;
		float 				magZ;
		float 				accX;
		float 				accY;
		float 				accZ;
		float 				gyroX;
		float 				gyroY;
		float 				gyroZ;
		bool	 			horn;
		bool				motorDirRly;
		bool				motorWhtRly;
		bool				motorYlwRly;
		bool				motorRedRly;
		bool				motorRedWhtRly;
		bool				motorRedYlwRly;
		bool				servoPower;
		long 				startStopTime;
		long				startStateTime;
		arduinoStateEnum	originState;
		
		static const string const arduinoStates[] = {
			"PowerUp", 
			"Armed", 
			"SelfTest", 
			"Disarmed", 
			"Active", 
			"LowBattery", 
			"Fault", 
			"SelfRecovery", 
			"ArmedTest", 
			"ActiveRudder", 
			"None"
		};
		static const uint8_t 	arduinoStateCount = 11;
		
	private:
		bool 					setState (arduinoStateEnum s);
		bool 					setBoneState (boneStateClass::boneStateEnum s);
		string					write(string func, string query);		/**< Write to a function on the Arduino */
		
};

#endif /* STATESTRUCTTYPES_H */
