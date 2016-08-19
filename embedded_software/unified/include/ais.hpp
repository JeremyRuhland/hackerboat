/******************************************************************************
 * Hackerboat Beaglebone AIS module
 * ais.hpp
 * This module stores AIS data 
 * see the Hackerboat documentation for more details
 * Written by Pierce Nichols, Aug 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/
 
#ifndef AIS_H
#define AIS_H
 
#include <jansson.h>
#include "config.h"
#include <math.h>
#include <string>
#include <chrono>
#include "hackerboatRoot.hpp"
#include "location.hpp"

using std::chrono;

enum class aisMsgType : int {
	UNDEFINED		= 0,
	POSN_REPORT_A1	= 1,
	POSN_REPORT_A2	= 2,
	POSN_REPORT_A3	= 3,
	BASE_STATION	= 4,
	STATIC_DATA_A	= 5,
	POSN_REPORT_B	= 18,
	STATIC_DATA_B	= 24
};

enum class aisNavStatus : int {
	ENGINE				= 0,
	ANCHORED			= 1,
	NOT_UNDER_CMD		= 2,
	RESTRICTED_MANEUVER	= 3,
	CONSTRAINED_DRAUGHT	= 4,
	MOORED				= 5,
	AGROUND				= 6,
	FISHING				= 7,
	SAILING				= 8,
	HSC_NAV				= 9,
	WIG_NAV				= 10,
	AIS_SART			= 14,
	UNDEFINED			= 15
};

enum class aisShipType : int {
	UNAVAILABLE			= 0,	/**< Default value */
	WIG_ALL				= 20,	/**< Ekranoplans */
	WIG_HAZ_A			= 21,	/**< And another ekranoplan... */
	WIG_HAZ_B			= 22,	/**< Yet another ekranoplan... */
	WIG_HAZ_C			= 23,	/**< And another... */
	WIG_HAZ_D			= 24,	/**< Five ought to be enough for all the ekranoplans... */
	FISHING				= 30,	/**< But we only need one number for the oldest sort of vessel, amirite? */
	TOWING				= 31,	/**< An ordinary tow combination */
	TOWING_LARGE		= 32,	/**< A tow longer than 200m and/or wider than 25m */
	DREDGING			= 33,
	DIVING_OPS			= 34,
	MILITARY_OPS		= 35,
	SAILING				= 36,
	PLEASURE			= 37,
	HSC					= 40,
	HSC_HAZ_A			= 41,
	HSC_HAZ_B			= 42,
	HSC_HAZ_C			= 43,
	HSC_HAZ_D			= 44,
	HSC_NO_INFO			= 49,
	PILOT				= 50,
	SAR					= 51,
	TUG					= 52,
	PORT_TENDER			= 53,
	ANTI_POLLUTION		= 54,
	LAW_ENFORCEMENT		= 55,
	MEDICAL				= 58,
	NONCOMBATANT		= 59,
	PASSENGER			= 60,
	PASSENGER_HAZ_A		= 61,
	PASSENGER_HAZ_B		= 62,
	PASSENGER_HAZ_C		= 63,
	PASSENGER_HAZ_D		= 64,
	PASSENGER_NO_INFO	= 69,
	CARGO				= 70,
	CARGO_HAZ_A			= 71,
	CARGO_HAZ_B			= 72,
	CARGO_HAZ_C			= 73,
	CARGO_HAZ_D			= 74,
	CARGO_NO_INFO		= 79,
	TANKER				= 80,
	TANKER_HAZ_A		= 81,
	TANKER_HAZ_B		= 82,
	TANKER_HAZ_C		= 83,
	TANKER_HAZ_D		= 84,
	TANKER_NO_INFO		= 89,
	OTHER				= 90,
	OTHER_HAZ_A			= 91,
	OTHER_HAZ_B			= 92,
	OTHER_HAZ_C			= 93,
	OTHER_HAZ_D			= 94,
	OTHER_NO_INFO		= 99,
};

enum class aisEPFDType : int {
	UNDEFINED	= 0,
	GPS			= 1,
	GLONASS		= 2,
	GPS_GLONASS	= 3,
	LORAN_C		= 4,
	CHAYKA		= 5,
	INS			= 6,
	SURVEYED	= 7,
	GALILEO		= 8
};

class aisBaseClass : public hackerboatStateClassStorable {
	public:
		aisBaseClass ();
		aisBaseClass (json_t *packet);
		virtual bool prune ();		/**< Test if this contact should be pruned. If true, it deletes itself from the database and should be deleted upon return. */
		
		time_point<system_clock> 	lastContact;
		time_point<system_clock> 	lastTimestamp;
		int 			mmsi = -1;
		locationClass	fix;
		static const std::string msgClass = "AIS";
		std::string		device;	/**< Name of the device */
}

class aisShipClass : aisBaseClass {
	public:
		aisShipClass ();
		aisShipClass (json_t *packet);
		bool parseGpsdPacket (json_t *packet);
		bool project ();	/**< Project the position of the current contact now. */
		bool project (time_point<system_clock>);	/**< Project the position of this contact at time_point. */

		aisNavStatus	status;
		double			turn;
		double			speed;
		double			course;
		double			heading;
		int				imo;
		std::string		callsign;
		std::string		shipname;
		aisShipType		shiptype;
		int				to_bow;
		int				to_stern;
		int				to_port;
		int				to_starboard;
		aisEPFDType		epfd;
		
	private:
		hackerboatStateStorage *aisShipStorage = NULL;
	
};


#endif