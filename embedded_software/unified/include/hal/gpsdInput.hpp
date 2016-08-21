/******************************************************************************
 * Hackerboat orientation input module
 * hal/gpsInput.hpp
 * This module reads gps data
 *
 * See the Hackerboat documentation for more details
 * Written by Pierce Nichols, Aug 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/

#ifndef GPSINPUT_H
#define GPSINPUT_H

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <tuple>
#include "hal/config.h"
#include "gps.hpp"
#include "ais.hpp"
#include "hal/inputThread.hpp"

using std;

class gpsdInputClass : public inputThreadClass {
	public:
		gpsdInputClass() = default;					
		gpsdInputClass(string host, int port);			/**< Create a gpsd object pointing at the given host & port combination. */
		bool setHost (string host);						/**< Point the input listener at the given host. */
		bool setPort (int port);						/**< Point the input listener at the given port */
		bool connect ();								/**< Connect to the host */
		bool disconnect ();								/**< Disconnect from the host. */
		bool isConnected ();							/**< Returns true if connected. */
		gpsFixClass getFix();							/**< Returns last GPS fix (TSV report, more or less) */
		vector<tuple<int, aisBaseClass>> getData();		/**< Returns all AIS contacts */
		vector<tuple<int, aisBaseClass>> getData(aisShipType type);	/**< Returns AIS contacts of a particular ship type */
		aisBaseClass& getData(int MMSI);				/**< Returns AIS contact for given MMSI, if it exists. It returns a reference to a default (invalid) object if the given MMSI is not present. */
		aisBaseClass& getData(string name);				/**< Returns AIS contact for given ship name, if it exists. It returns a reference to a default (invalid) object if the given ship name is not present. */
		int pruneAIS();									/**< Call the prune() function of each AIS contact. */
		
	private:
		string 								_host;
		int 								_port;
		gpsFixClass 						_lastFix;
		vector<tuple<int, aisBaseClass>>	_aisTargets;
		
#endif