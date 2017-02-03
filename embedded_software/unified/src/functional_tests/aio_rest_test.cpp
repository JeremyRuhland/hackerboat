/******************************************************************************
 * Hackerboat Beaglebone MQTT Functional Test program
 * mqtt_test.cpp
 * This program is a functional test of the MQTT subsystem
 * see the Hackerboat documentation for more details
 *
 * Written by Pierce Nichols, Aug 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/
 
#include "hal/config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <chrono>
#include <iostream>
#include <thread>
#include "hal/gpsdInput.hpp"
#include "gps.hpp"
#include "easylogging++.h"
#include "aio-rest.hpp"

#define ELPP_STL_LOGGING 

INITIALIZE_EASYLOGGINGPP

using namespace std;

int main(int argc, char **argv) {
    // Load configuration from file
    el::Configurations conf("/home/debian/hackerboat/embedded_software/unified/setup/log.conf");
    // Actually reconfigure all loggers instead
    el::Loggers::reconfigureAllLoggers(conf);
	START_EASYLOGGINGPP(argc, argv);
	
	// Setting up environment...
	BoatState me;
	AIO_Rest myrest(&me);
	cout << "Creating publishing map..." << endl;
	PubFuncMap *mypubmap = new PubFuncMap {	{"SpeedLocation", new pub_SpeedLocation(&me, &myrest)},
											{"Mode", new pub_Mode(&me, &myrest)},
											{"MagneticHeading", new pub_MagHeading(&me, &myrest)},
											{"GPSCourse", new pub_GPSCourse(&me, &myrest)},
											{"BatteryVoltage", new pub_BatteryVoltage(&me, &myrest)},
											{"RudderPosition", new pub_RudderPosition(&me, &myrest)},
											{"ThrottlePosition", new pub_ThrottlePosition(&me, &myrest)},
											{"FaultString", new pub_FaultString(&me, &myrest)}};
	cout << "Publishing map created..." << endl;
	myrest.setPubFuncMap(mypubmap);
	for (int x = 0; x < 20; x++) {
		cout << "Calling publishNext()... " << to_string(x) << endl;
		myrest.publishNext();
		std::this_thread::sleep_for(500ms);
	}
	std::this_thread::sleep_for(1500ms);
	cout << "Calling publishAll()..." << endl;
	myrest.publishAll();
	
	return 0;
}
