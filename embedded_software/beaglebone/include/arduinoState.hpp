/******************************************************************************
 * Hackerboat Beaglebone types module
 * arduinoState.hpp
 * see the Hackerboat documentation for more details
 * Written by Pierce Nichols, Jan 2016
 *
 * Version 0.1: First alpha
 *
 ******************************************************************************/

#ifndef ARDUINOSTATE_H
#define ARDUINOSTATE_H

#include <time.h>
#include <string>
#include "stateStructTypes.hpp"
#include "enumtable.hpp"
#include "location.hpp"

/**
 * @class arduinoStateClass
 *
 * @brief Class for storing the current state of the Arduino element
 *
 */

class arduinoStateClass : public hackerboatStateClassStorable {
	public:

		typedef arduinoModeEnum Mode;
		static const enumerationNameTable<arduinoModeEnum> modeNames;

		bool populate (void);	/**< Populate the object from the named interface */
		bool setCommand (arduinoModeEnum c);

		// Command functions...
		bool writeBoatMode(boatModeEnum s);
		bool writeCommand(void);
		int16_t writeThrottle(void);
		double writeHeadingTarget(void);
		double writeHeadingDelta(double delta);
		bool heartbeat(void);

		bool 				popStatus;			/**< State of whether the last call to populate() succeeded or failed */
		timespec			uTime;				/**< Time the record was made */
		arduinoModeEnum 		mode;				/**< The current mode of the arduino                    */
		arduinoModeEnum			command;			/**< Last state command received by the Arduino */
		int8_t		 		throttle;   			/**< The current throttle position                    */
		boatModeEnum 			bone;				/**< The current mode of the BeagleBone                */
		orientationClass		orientation;			/**< The current accelerometer tilt and magnetic heading of the boat  */
		float 				headingTarget;			/**< The desired magnetic heading                     */
		float 				internalVoltage;		/**< The battery voltage measured on the control PCB          */
		float 				batteryVoltage;			/**< The battery voltage measured at the battery            */
		float				motorVoltage;
		bool				enbButton;			/**< State of the enable button. off = 0; on = 0xff           */
		bool				stopButton;			/**< State of the emergency stop button. off = 0; on = 0xff       */
		long 				timeSinceLastPacket;		/**< Number of milliseconds since the last command packet received    */
		long 				timeOfLastPacket;		/**< Time the last packet arrived */
		long 				timeOfLastBoneHB;
		long 				timeOfLastShoreHB;
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
		arduinoModeEnum			originMode;

	private:
		bool 					setMode (arduinoModeEnum s);
		bool 					setBoatMode (boatModeEnum s);
		string					write(string func, string query);		/**< Write to a function on the Arduino */
};

#endif /* ARDUINOSTATE_H */