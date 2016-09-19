/******************************************************************************
 * Hackerboat orientation input module
 * hal/gpsInput.hpp
 * This module reads gps data
 *
 * See the Hackerboat documentation for more details
 * Written by Pierce Nichols, Sep 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <tuple>
#include <map>
#include "jansson.h"
#include "hal/config.h"
#include "gps.hpp"
#include "ais.hpp"
#include "hal/inputThread.hpp"
#include "hal/gpsdInput.hpp"
#include "pstream.h"

using namespace std;
using namespace redi;

GPSdInput::GPSdInput (string host, int port) :
	_host(host), _port(port) {};

bool GPSdInput::setHost (string host) {
	if (host != "") {
		_host = host;
		return true;
	}
	return false;
}

bool GPSdInput::setPort (int port) {
	if ((port > 0) && (port < 65535)) {
		_port = port;
		return true;
	}
	return false;
}

bool GPSdInput::connect () {
	if ((_port > 0) && (_port < 65535) && (_host != "")) {
		string cmd = "/usr/bin/gpspipe -w " + _host + ":" + to_string(_port);
		gpsdstream.open(cmd, pstreams::pstdout | pstreams::pstderr);
		std::this_thread::sleep_for(1ms);
		return (isConnected());
	}
	return false;
}

bool GPSdInput::disconnect () {
	gpsdstream.close();
	return true;
}

bool GPSdInput::isConnected () {
	if (gpsdstream.is_open() && (gpsdstream.exited() == 0)) return true;
	return false;
}

bool GPSdInput::begin() {
	if (this->connect()) {
		this->myThread = new std::thread (InputThread::InputThreadRunner(this));
		myThread->detach();
		return true;
	}
	return false;
}

bool GPSdInput::execute() {
	// grab the lock
	if (!lock && (!lock.try_lock_for(IMU_LOCK_TIMEOUT))) return false;
	bool result = true;
	json_error_t* inerr;
	string buf = "";
	while ((gpsdstream.sgetc() != '\n') && (gpsdstream.sgetc() != EOF)) {
		buf += gpsdstream.sbumpc();
	}
	if (buf != "") {
		json_t* input = json_loads(buf.c_str(), JSON_DECODE_ANY | JSON_REJECT_DUPLICATES, inerr);
		if (input) {
			json_t* objclass = json_object_get(input, "class");
			if (objclass) {
				string s;
				const char *p = json_string_value(objclass);
				size_t l = json_string_length(objclass);
				s.assign(p, l);
				if (s == "TPV") {
					result = _lastFix.parseGpsdPacket(input);
				} else if (s == "AIS") {
					AISShip newship;
					if (newship.parseGpsdPacket(input)) {
						_aisTargets.emplace(newship.getMMSI(), newship);
						result = true;
					} 
				} 
				json_decref(input);
				json_decref(objclass);
			}
			json_decref(input);
			result = false;
		} result = false;
	} else result = false;
	
	lock.unlock();
	return result;
}

map<int, AISShip> GPSdInput::getData() {
	return _aisTargets;
}

map<int, AISShip> GPSdInput::getData(AISShipType type) {
	map<int, AISShip> result;
	for (auto const &r : _aisTargets) {
		if (type == r.second.shiptype) {
			result.emplace(r.first, r.second);
		}
	}
	return result;
}

AISShip GPSdInput::getData(int MMSI) {
	return _aisTargets[MMSI];
}

AISShip GPSdInput::getData(string name) {
	for (auto const &r : _aisTargets) {
		if (name == r.second.shipname) {
			return r.second;
		}
	}
	return AISShip();
}

int GPSdInput::pruneAIS(Location loc) {
	int count = 0;
	for (auto &r : _aisTargets) {
		if (r.second.prune(loc)) {
			_aisTargets.erase(r.first);
			count++;
		}
	}
	return count;
}