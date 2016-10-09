/******************************************************************************
 * Hackerboat MQTT interface module
 * mqtt.hpp
 * This module provides a shim for interfacing with the PAHO MQTT client
 * in order to allow the boat to talk to Adafruit.IO and other brokers.
 *
 * See the Hackerboat documentation for more details
 * Written by Pierce Nichols, Oct 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/

#ifndef MQTT_H
#define MQTT_H
 
#include <jansson.h>
#include <cstdlib>
#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <list>
#include "hal/config.h"
#include "private-config.h"
#include "hackerboatRoot.hpp"
#include "boatState.hpp"
extern "C" {
	#include "MQTTClient.h"
}

#define MQTT_KEEPALIVE_SEC		120
#define MQTT_CONNECT_TIMEOUT	5
#define MQTT_RETRY_INTERVAL		5
#define MQTT_CLIENTID			"TDRoW"

using namespace std;

typedef map<string, function<void(BoatState*, string topic, MQTTClient*)>> PubFuncMap;
typedef map<string, function<void(BoatState*, string topic, string payload)>> SubFuncMap;

#define MQTT_QOS 		(1)
#define MQTT_GROUP 		"hackerboat"
#define MQTT_DATATYPE	"csv"

class MQTT {
	public:
		MQTT (	BoatState *me,						/// The BoatState vector that data is taken from and read to
				string host 	= MQTT_HOST,		 
				int port 		= MQTT_PORT,
				string username	= MQTT_USERNAME,
				string key 		= MQTT_KEY);
		int connect ();
		void disconnect ();
		bool isConnected();
		void setPubFuncMap (PubFuncMap *pubmap);	/// A map of the publish functions to call, by topic
		void publishNext();							/// call the next function in the _pub function list
		int publishAll();							/// call all of the functions in the _pub function list. Returns the number of functions executed
		bool isDelivered(string topic);				/// check whether the last message on the given topic has been delivered yet
		void setSubFuncMap (SubFuncMap *submap);	/// Set a map of the functions to call for each subscribed topic
		MQTTClient* getClient () {return &client;};	/// Get a pointer to the current client
		~MQTT();									/// We need an explicit destructor to make sure we clean up everything
		
		// publisher functions
		void pub_SpeedLocation(BoatState* state, string topic, MQTTClient* client);	/// Publish the current GPS speed and location
		void pub_Mode(BoatState* state, string topic, MQTTClient* client);			/// Publish the current mode as a CSV list of the form <Boat>,<Nav>,<RC>,<Auto>
		void pub_Bearing(BoatState* state, string topic, MQTTClient* client);		/// Publish the current magnetic bearing, true bearing, and GPS course as a CSV list (in that order)
		void pub_BatteryVoltage(BoatState* state, string topic, MQTTClient* client);	/// Publish the current battery voltage
		void pub_RudderPosition(BoatState* state, string topic, MQTTClient* client);	/// Publish current rudder position
		void pub_PID_K(BoatState* state, string topic, MQTTClient* client);			/// Publish current PID values
		
		// subscriber functions
		void sub_Command(BoatState*, string topic, string payload);		/// Subscribe to commands from shore
		void sub_PID_K(BoatState*, string topic, string payload);		/// Subscribe to PID updates from shore
		
	private:
		BoatState *_state;
		PubFuncMap *_pub;	/// A map of the functions to call to publish different outgoing topics
		SubFuncMap *_sub;	/// A map of functions to call when different topics are received
		MQTTClient_deliveryToken token;	/// Delivery token for the last message
		PubFuncMap::iterator pubit;	// iterator pointed to next item to publish
		MQTTClient client;
		MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
		//MQTTClient_SSLOptions ssl_opts;
		bool connected;
		string uri;
		string _name;
		string _key;
		string clientID;
		static void delivered(void *context, MQTTClient_deliveryToken dt);
		static int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message);
		static void connlost(void *context, char *cause);
};



#endif /* MQTT_H */