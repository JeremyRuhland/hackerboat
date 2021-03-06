/******************************************************************************
 * Hackerboat Beaglebone boat state module
 * boatState.hpp
 * see the Hackerboat documentation for more details
 * Written by Pierce Nichols, Aug 2016
 *
 * Version 0.1: First alpha
 *
 ******************************************************************************/

#ifndef BOATSTATE_H
#define BOATSTATE_H

#include <chrono>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <list>
#include "hackerboatRoot.hpp"
#include "enumtable.hpp"
#include "location.hpp"
#include "gps.hpp"
#include "hal/config.h"
#include "easylogging++.h"
#include "enumdefs.hpp"
#include "healthMonitor.hpp"
#include "waypoint.hpp"
#include "dodge.hpp"
#include "hal/relay.hpp"
#include "hal/gpio.hpp"
#include "hal/adcInput.hpp"
#include "hal/RCinput.hpp"
#include "hal/gpsdInput.hpp"
#include "hal/throttle.hpp"
#include "hal/servo.hpp"
#include "hal/orientationInput.hpp"
#include "util.hpp"
#include "rapidjson/rapidjson.h"

using namespace std;
using namespace rapidjson;

class BoatState;	// forward declaration so this compiles

class Command {
	public:
		Command (BoatState *state, const string cmd, const Value& args);
		Command (BoatState *state, const string cmd);
		std::string getCmd() {return _cmd;};
		Value& getArgs() {return _args;};
		bool execute ();
		Value pack () const;

	private:
		static const map<std::string, std::function<bool(Value&, BoatState*)>> _funcs;
		BoatState 		*_state = NULL;
		std::string 	_cmd;
		Value 			_args;
		Document 		root;

		// here begins the functions that implement incoming commands
		static bool SetMode(Value& args, BoatState *state);
		static bool SetNavMode(Value& args, BoatState *state);
		static bool SetAutoMode(Value& args, BoatState *state);
		static bool SetHome(Value& args, BoatState *state);
		static bool ReverseShell(Value& args, BoatState *state);
		static bool SetWaypoint(Value& args, BoatState *state);
		static bool SetWaypointAction(Value& args, BoatState *state);
		static bool DumpPathKML(Value& args, BoatState *state);
		static bool DumpWaypointKML(Value& args, BoatState *state);
		static bool DumpObstacleKML(Value& args, BoatState *state);
		static bool DumpAIS(Value& args, BoatState *state);
		static bool FetchWaypoints(Value& args, BoatState *state);
		static bool PushPath(Value& args, BoatState *state);
		static bool SetPID(Value& args, BoatState *state);
};

std::ostream& operator<< (std::ostream& stream, const Command& state);

class BoatState : public HackerboatState {
	public:
		static const EnumNameTable<BoatModeEnum> boatModeNames;
		static const EnumNameTable<NavModeEnum> navModeNames;
		static const EnumNameTable<AutoModeEnum> autoModeNames;
		static const EnumNameTable<RCModeEnum> rcModeNames;

		BoatState ();
		bool parse (Value& input);
		Value pack () const;
		bool isValid ();

		bool insertFault (const std::string fault);					/**< Add the named fault to the fault string. Returns false if fault string is full */
		bool removeFault (const std::string fault);					/**< Remove the named fault from the fault string. Returns false if not present */
		bool hasFault (const std::string fault) const;				/**< Returns true if given fault is present */
		int faultCount (void) const;								/**< Returns the current number of faults */
 		void clearFaults () {faultString = "";};					/**< Remove all faults */
		std::string getFaultString() {return faultString;};			/**< Returns the entire fault string */
		bool setBoatMode (BoatModeEnum m) {_boat = m; return true;};/**< Set boat mode to the given value */
		bool setBoatMode (std::string mode);						/**< Set boat mode to the given value */
		BoatModeEnum getBoatMode () {return _boat;};				/**< Return boat mode */
		bool setNavMode (NavModeEnum m) {_nav = m; return true;};	/**< Set nav mode to the given value */
		bool setNavMode (std::string mode);							/**< Set nav mode to the given value */
		NavModeEnum getNavMode () {return _nav;};
		bool setAutoMode (AutoModeEnum m) {_auto = m; return true;};/**< Set autonomous mode to the given value */
		bool setAutoMode (std::string mode);						/**< Set autonomous mode to the given value */
		AutoModeEnum getAutoMode () {return _auto;};
		bool setRCmode (RCModeEnum m) {_rc = m; return true;};		/**< Set RC mode to the given value */
		bool setRCmode (std::string mode);							/**< Set RC mode to the given value */
		RCModeEnum getRCMode () {return _rc;};
		int commandCnt () const {return cmdvec.size();};			/**< Return the number of commands waiting to be executed */
		void pushCmd (std::string name, const Value& args);			/**< Add a command to the back of the command queue */
		void pushCmd (std::string name) {Value m; pushCmd(name, m);};
		void flushCmds () {cmdvec.clear();};						/**< Empty the command queue */
		int executeCmds (int num = 0);								/**< Execute the given number of commands. 0 executes all available. Returns the number of commands successfully executed. */
		std::string getCSV();										/**< Export the current state as a line for a CSV file */
		std::string getCSVheaders();								/**< Generate CSV headers */
		ArmButtonStateEnum getArmState ();							/**< Get the current state of the arm & disarm inputs */
		std::string printCurrentWaypointNum();						/**< Print the current waypoint number, RETURN, ANCHOR, or NONE */
		Location getCurrentTarget();								/**< Returns the current target location, or an invalid Location if there isn't one right now */

		sysclock				lastContact;		/**< Time of last shore contact */
		sysclock				lastRC;				/**< Time of the last signal from the RC input */
		GPSFix					lastFix;			/**< Location of the last GPS fix */
		Location				launchPoint;		/**< Location of the launch point */
		Location				anchorPoint;		/**< Location of the anchor point */
		Waypoints				waypointList;		/**< Waypoints to follow */
		Dodge*					diversion;			/**< Avoid obstacles! */
		HealthMonitor*			health;				/**< Current state of the boat's health */
		Pin						disarmInput;		/**< Disarm input from power distribution box */
		Pin						armInput;			/**< Arm input from power distribution box */
		Pin						servoEnable;		/**< Pin to turn on the servo power output */
		Servo*					rudder = 0;			/**< Rudder servo */
		Throttle*				throttle = 0;		/**< Throttle object */
		RCInput*				rc = 0;				/**< RC input thread */
		ADCInput*				adc = 0;			/**< ADC input thread */
		GPSdInput*				gps = 0;			/**< GPS input thread */
		OrientationInput*		orient = 0;			/**< Orientation input thread */
		RelayMap*				relays = 0;			/**< Pointer to relay singleton */

		tuple<double, double, double> K;			/**< Steering PID gains. Proportional, integral, and differential, respectively. */

	private:
		std::list<Command*>	cmdvec;
		std::string 	faultString = "";
		BoatModeEnum 	_boat = BoatModeEnum::NONE;
		NavModeEnum		_nav = NavModeEnum::NONE;
		AutoModeEnum 	_auto= AutoModeEnum::NONE;
		RCModeEnum 		_rc = RCModeEnum::NONE;
		#ifndef DISTRIB_IMPLEMENTED
			bool buttonArmed = false;
			sysclock armedStart;
			sysclock disarmedStart;
		#endif /* DISTRIB_IMPLEMENTED */

};

#endif
