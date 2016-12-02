/******************************************************************************
 * Hackerboat MQTT interface module
 * mqtt.cpp
 * This module provides a shim for interfacing with the PAHO MQTT client
 * in order to allow the boat to talk to Adafruit.IO and other brokers.
 *
 * See the Hackerboat documentation for more details
 * Written by Pierce Nichols, Oct 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/

#include <jansson.h>
#include <cstdlib>
#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <list>
#include "json_utilities.hpp"
#include "hal/config.h"
#include "private-config.h"
#include "hackerboatRoot.hpp"
#include "boatState.hpp"
#include "MQTTClient.h"
#include "mqtt.hpp"

using namespace std;

MQTT::MQTT (BoatState *me, string host, int port, string username, string key) :
	clientID(MQTT_CLIENTID), _name(username), _key(key) {
		_state = me;
		conn_opts.keepAliveInterval = MQTT_KEEPALIVE_SEC;
		conn_opts.username = _name.c_str();
		conn_opts.password = _key.c_str();
		uri = "tcp://" + host + ":" + to_string(port);
		MQTTClient_create(&client, uri.c_str(), clientID.c_str(), 
							MQTTCLIENT_PERSISTENCE_NONE, NULL);
		conn_opts.connectTimeout = MQTT_CONNECT_TIMEOUT;
		conn_opts.retryInterval = MQTT_RETRY_INTERVAL;
		conn_opts.cleansession = 1;
		conn_opts.ssl = NULL;
		MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
	}
	
int MQTT::connect () {
	int rc = MQTTClient_connect(client, &conn_opts);
    return rc;
}

void MQTT::disconnect () {
	MQTTClient_disconnect(client, MQTT_CONNECT_TIMEOUT);
}

bool MQTT::isConnected() {
	return static_cast<bool>(MQTTClient_isConnected(client));
}

void MQTT::setPubFuncMap (PubFuncMap *pubmap) {
	_pub = pubmap;
	pubit = _pub->begin();
	// this bit of code makes sure the token map matches the function map so we have usable tokens
	token = 0;
}

void MQTT::publishNext() {
	pubit->second(_state, pubit->first, &client);
	if (pubit == _pub->end()) {
		pubit = _pub->begin();
	} else pubit++;
}

int MQTT::publishAll() {
	int cnt = 0;
	for (auto r: *_pub) {
		r.second(_state, r.first, &client);
		cnt++;
	}
	return cnt;
}

bool MQTT::isDelivered(string topic) {
	return (token != 0);
}

void MQTT::setSubFuncMap (SubFuncMap *submap) {
	_sub = submap;
}

MQTT::~MQTT() {
    disconnect();
    MQTTClient_destroy(&client);
}

// publisher functions

void MQTT::transmit (string topic, string payload, MQTTClient* client) {
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	this->token = 0;
	pubmsg.payload = (void*)(payload.c_str());
	pubmsg.payloadlen = payload.length();
	MQTTClient_publishMessage(client, topic.c_str(), &pubmsg, &token);
	string logmsg = "topic: " + topic + "payload: " + payload;
}

void MQTT::pub_SpeedLocation(BoatState* state, string topic, MQTTClient* client) {
	string payload = to_string(state->lastFix.speed) + ",";
	payload += to_string(state->lastFix.fix.lat) + ",";
	payload += to_string(state->lastFix.fix.lon) + ",0.0";
	transmit(topic, payload, client);
}

void MQTT::pub_Mode(BoatState* state, string topic, MQTTClient* client) {
	string payload = state->boatModeNames.get(state->getBoatMode());
	payload += ":" + state->navModeNames.get(state->getNavMode());
	payload += ":" + state->autoModeNames.get(state->getAutoMode());
	payload += ":" + state->rcModeNames.get(state->getRCMode());
	transmit(topic, payload, client);
}

void MQTT::pub_Bearing(BoatState* state, string topic, MQTTClient* client) {
	string payload = to_string(state->orient->getOrientation()->heading);
	transmit(topic, payload, client);
}

void MQTT::pub_BatteryVoltage(BoatState* state, string topic, MQTTClient* client) {
	string payload = to_string(state->health->batteryMon);
	transmit(topic, payload, client);
}

void MQTT::pub_RudderPosition(BoatState* state, string topic, MQTTClient* client) {
	string payload = to_string(state->rudder->read());
	transmit(topic, payload, client);
}

void MQTT::pub_ThrottlePosition(BoatState* state, string topic, MQTTClient* client) {
	string payload = to_string(state->throttle->getThrottle());
	transmit(topic, payload, client);
}

void MQTT::pub_PID_K(BoatState* state, string topic, MQTTClient* client) {
	string payload = "{\"Kp\":" + to_string(get<0>(state->K));
	payload += ",\"Ki\":" + to_string(get<1>(state->K));
	payload += ",\"Kd\":" + to_string(get<2>(state->K)) + "}";
	transmit(topic, payload, client);
}

void MQTT::pub_Course(BoatState* state, string topic, MQTTClient* client) {
	string payload = to_string(state->lastFix.track);
	transmit(topic, payload, client);
}

// subscriber functions

void MQTT::sub_Command(BoatState* state, string topic, string payload) {
	json_error_t err;
	json_t *args, *input = json_loads(payload.c_str(), 0, &err);
	if (input) {
		string name = json_string_value(json_object_get(input, "Command"));
		args = json_object_get(input, "Argument");
		state->pushCmd(name, args);
		json_decref(input);
	}
}

// callback functions
void MQTT::delivered(void *context, MQTTClient_deliveryToken dt) {
	token = dt;
}

int MQTT::msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
	string topic = topicName;
	string msg = static_cast<char*>(message->payload);
	string logmsg = "topic: " + topic + "payload: " + msg;
	try {
		_sub->at(topic)(_state, topic, msg);
		return 0;
	} catch (...) {
		return -1;
	}
	return -1;
}

void MQTT::connlost(void *context, char *cause) {
	
}
