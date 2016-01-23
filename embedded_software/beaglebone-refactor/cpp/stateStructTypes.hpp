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
 
#include <jansson.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "config.h"

#define STATE_STRING_LEN		30
#define GPS_SENTENCE_LEN		120
#define	FAULT_STRING_LEN		1024
#define NAV_SOURCE_NAME_LEN		30
#define NAV_VEC_LIST_LEN		30

class hackerboatStateClass {
	public:
		virtual hackerboatStateClass(void);							
		virtual bool parse (json_t *input);			/**< Populate the object from the given json object */
		virtual json_t *pack (void);				/**< Pack the contents of the object into an outgoing json object */
		virtual bool isValid (void) {return true};	/**< Tests whether the current object is in a valid state */
	private:	
		static const char *jsonFormatString;		/**< Format string for the object */
};

class hackerboatStateClassStorable : public hackerboatStateClass {
	public:
		virtual hackerboatStateClass(const char *file, size_t len);	/**< Create a state object attached to the given file */
		int32_t getSequenceNum (void) {return sequenceNum};			/**< Get the sequenceNum of this object (-1 until populated from a file) */
		virtual bool openFile(const char *name, size_t len) {return false};	/**< Open the given database file & store the name */
		virtual bool openFile(void) {return false};					/**< Open the stored database file */
		virtual bool closeFile(void) {return true};					/**< Close the open file */
		virtual int32_t count (void) {return -1};					/**< Return the number of records of the object's type in the open database file */
		virtual bool writeRecord (void) {return false};				/**< Write the current record to the target database file */
		virtual bool getRecord(int32_t select) {return false};		/**< Populate the object from the open database file */

	private:
		int32_t 	sequenceNum;
		char 		*fileName;
		sqlite3 	*db;
}

class gpsFixClass : public hackerboatStateClassStorable {
	public:
		gpsFixClass (void);
		gpsFixClass (char *sentence, size_t len);			/**< Create a GPS fix from an incoming sentence */
		
		bool readSentence (char *sentence, size_t len);		/**< Populate class from incoming sentence */
		
		unsigned long 	uTime;				/**< Time of fix arrival */
		double			latitude;			/**< Latitude of last fix */
		double			longitude;			/**< Longitude of last fix */
		double			gpsHeading;			/**< True heading, according to GPS */
		double			gpsSpeed;			/**< Speed over the ground */
		char[GPS_SENTENCE_LEN]		GGA;	/**< GGA sentence from GPS */
		char[GPS_SENTENCE_LEN]		GSA;	/**< GSA sentence from GPS */
		char[GPS_SENTENCE_LEN]		GSV;	/**< GSV sentence from GPS */
		char[GPS_SENTENCE_LEN]		VTG;	/**< VTG sentence from GPS */
		char[GPS_SENTENCE_LEN]		RMC;	/**< RMC sentence from GPS */
};

class locationClass : public hackerboatStateClass {
	public:
		locationClass (void);
		locationClass (double lat, double lon);	/**< Create a location object at the given latitude & longitude */
		
		double _lat;		/**< Latitude in degrees north of the equator. Values from -90.0 to 90.0, inclusive. */
		double _lon;		/**< Longitude in degrees east of the prime meridian. Values from -180.0 to 180.0, inclusive. */			
};

class navVectorClass : public hackerboatStateClass {
	public:	
		navVectorClass (void);
		navVectorClass (char *src, size_t srcLen, double bearing, double strength);
		
		char[NAV_SOURCE_NAME_LEN]	source;		/**< Name of the source of this vector. */
		double 						bearing;	/**< Bearing of this vector in degrees, clockwise from true north. */
		double				 		strength;	/**< Relative strength of this vector */
};

class waypointClass : public hackerboatStateClassStorable {
	public:	
		waypointClass (void);
		waypointClass (locationClass loc, bool stop);
		
		locationClass	location;	/**< Location of the waypoint */
		bool			stop;		/**< Stop bit -- if true and this is the last waypoint, stop here and hold position. Otherwise, go back to the first waypoint. */
};

class boneStateClass : public hackerboatStateClassStorable {
	public:
	
		boneStateClass (void);
		bool insertFault (char* fault, size_t len);	/**< Add the named fault to the fault string. Returns false if fault string is full */
		bool removeFault (char* fault, size_t len);	/**< Remove the named fault to the fault string. Returns false if not present */
		
		unsigned long 				uTime;			/**< Time the record was made, in microseconds past the epoch */
		boneStateEnum				state;			/**< current state of the beaglebone */	
		char[STATE_STRING_LEN]		stateString;	/**< current state of the beaglebone, human readable string */
		boneStateEnum				command;		/**< commanded state of the beaglebone */
		char[STATE_STRING_LEN]		commandString;	/**< commanded state of the beaglebone, human readable string */
		arduinoStateEnum			ardState;		/**< current state of the Arduino */
		char[STATE_STRING_LEN]		ardStateString;	/**< current state of the Arduino, human readable string */
		char[FAULT_STRING_LEN]		faultString;	/**< comma separated list of faults */
		gpsFixClass					gps;			/**< current GPS position */
		int 						waypointNext;	/**< ID of the current target waypoint */
		double						waypointStrength;		/**< Strength of the waypoint */
		double						waypointAccuracy;		/**< How close the boat gets to each waypoint before going to the next one */
		double						waypointStrengthMax;	/**< Maximum waypoint strength */
		bool						offshore;		/**< When set true, the boat will operate autonomously */
};

class navClass : public hackerboatStateClassStorable {
	public:
		navClass (void);
		
		bool appendVector (navVectorClass vec);	/**< Add a navigation vector to the influence list */
		bool calc (void);						/**< Calculate the course to the next waypoint and sum the navInfluences vectors */
		void clearVectors (void);				/**< Clear the contents of navInfluences */
		
		locationStruct	current;		/**< current location */	
		locationStruct	target;			/**< target waypoint */
		double			magCorrection;	/**< Correction between sensed magnetic heading and true direction */
		navVector		targetVec;		/**< Vector to the target */
		navVector		total;			/**< Sum of target vector and all influences */
	
	private:
		navVector[NAV_VEC_LIST_LEN]	navInfluences;	/**< Array to hold the influences of other navigation sources (i.e. collision avoidance) */
		uint16_t					influenceCount; /**< Number of influence vectors */
};

class arduinoStateClass : public hackerboatStateClassStorable {
	public:
	
	arduinoStateClass(void);
	
	bool populate (char *interface, size_t len);	/**< Populate the object from the named interface */
	
	unsigned long		uTime;					/**< Time the record was made, in microseconds past the epoch */
	arduinoStateEnum 	state;					/**< The current state of the boat                    */
	arduinoStateEnum	command;
	throttleState 		throttle;   			/**< The current throttle position                    */
	boneStateEnum 		bone;					/**< The current state of the BeagleBone                */
	sensors_vec_t 		orientation;			/**< The current accelerometer tilt and magnetic heading of the boat  */
	float 				headingTarget;			/**< The desired magnetic heading                     */  
	float 				internalVoltage;		/**< The battery voltage measured on the control PCB          */
	float 				batteryVoltage;			/**< The battery voltage measured at the battery            */
	float				motorVoltage;
	int					enbButton;				/**< State of the enable button. off = 0; on = 0xff           */
	int	 				stopButton;				/**< State of the emergency stop button. off = 0; on = 0xff       */
	long 				timeSinceLastPacket;	/**< Number of milliseconds since the last command packet received    */
	long 				timeOfLastPacket;		/**< Time the last packet arrived */
	long 				timeOfLastBoneHB;	
	long 				timeOfLastShoreHB;
	char				stateString[30];
	char 				boneStateString[30];
	char				commandString[30];
	int					faultString;			/**< Fault string -- binary string to indicate source of faults */
	float 				rudder;
	int					rudderRaw;
	int					internalVoltageRaw;
	int					motorVoltageRaw;
	float				motorCurrent;
	int					motorCurrentRaw;
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
	uint8_t 			horn;
	uint8_t				motorDirRly;
	uint8_t				motorWhtRly;
	uint8_t				motorYlwRly;
	uint8_t				motorRedRly;
	uint8_t				motorRedWhtRly;
	uint8_t				motorRedYlwRly;
	uint8_t				servoPower;
	long 				startStopTime;
	long				startStateTime;
	arduinoStateEnum	originState;
};