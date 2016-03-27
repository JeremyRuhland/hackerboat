/******************************************************************************
 * Hackerboat Beaglebone FastCGI program
 * hackerboatFastCGI.cpp
 * This program handles incoming REST requests
 * see the Hackerboat documentation for more details
 * Written by Pierce Nichols, Dec 2015
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/

#include <fcgi_stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <jansson.h>
#include <time.h>
#include "stateStructTypes.hpp"
#include "RESTdispatch.hpp"
#include "config.h"
#include "logs.hpp"
#include "boneState.hpp"
#include "arduinoState.hpp"
#include "navigation.hpp"
#include "gps.hpp"

#include <string>
#include <map>
#include <vector>

RESTdispatchClass *initRESTDispatch (void);

int main (void) {
	std::string	uri, method, query, contentLength, body;
	std::vector<std::string> tokens;
	char 		*ptr, *bodyBuf;
	char		response[LOCAL_BUF_LEN]			= {0};
	json_t		*responseJSON, *jsonFinal;
	size_t 		bodyLen, tokenLen;
	size_t		tokenPos = 0, nextTokenPos = 0;
	RESTdispatchClass*	root;
	logREST*	log = logREST::instance();
	logError*	err = logError::instance();
	
	// initialize the dispatch hierarchy
	root = initRESTDispatch();
	
	while (FCGI_Accept() >= 0) {
		// read in the critical environment variables and go to next iteration if any fail
		// Note that the assignment in if statements is deliberate to detect the null pointer
		if (ptr = getenv("REQUEST_URI")) {
			uri.assign(ptr);
		} else continue;
		if (ptr = getenv("REQUEST_METHOD")) {
			method.assign(ptr);
		} else continue;
		if (ptr = getenv("QUERY_STRING")) {
			query.assign(ptr);
		} else continue;
		if (ptr = getenv("CONTENT_LENGTH")) {
			contentLength.assign(ptr);
		} else continue;
		
		// if there is post data, read it in
		bodyLen = contentLength.stoi();
		if (bodyLen > 0) {
			bodyBuf = (char *)calloc(bodyLen, sizeof(char));
			if (bodyBuf == NULL) continue;
		}
		FCGI_fread(bodyBuf, 1, bodyLen, FCGI_stdin);
		body.assign(bodyBuf, bodyLen);
		free(bodyBuf);
		
		// print the first line of the response
		FCGI_printf("Content-type: application/json\r\n\r\n");
		
		// chop up the URI and move the elements into a vector of strings
		tokens.clear();
		tokenPos = 0;
		nextTokenPos = 0;
		while (uri.find_first_of("/\\", tokenPos) != std::string::npos) {
			tokenPos = uri.find_first_of("/\\", tokenPos);
			nextTokenPos = uri.find_first_of("/\\", tokenPos);
			if (nextTokenPos == std::string::npos) {
				tokenLen = nextTokenPos;
			} else {
				tokenLen = (nextTokenPos - tokenPos);
			}
			tokens.push_back(uri.substr(tokenPos, tokenLen));
		}
		
		// dispatch on the URI
		responseJSON = root->dispatch(tokens, 0, query, method, body);
		
		// add some things to the JSON response...
		jsonFinal = json_pack("{sOsissss}", "response", responseJSON, 
								"id", REST_ID, 
								"name", REST_NAME,
								"connected", "true");
		
		// print the result back to the client
		snprintf(response, LOCAL_BUF_LEN, "%s", json_dumps(jsonFinal, JSON_COMPACT));
		FCGI_printf("%s\r\n", response);

		// log everything
		log->open(REST_LOGFILE);
		log->write(tokens, tokenCnt, query, body, bodyLen, method, response);
		log->close();
		
		// clean up
		tokens.clear();
		json_decref(jsonFinal);
		tokenPos = 0;
	}
}

RESTdispatchClass *initRESTDispatch (void) {
	// data structure objects
	gpsFixClass				*gps = new gpsFixClass();
	navClass				*nav = new navClass();
	waypointClass			*waypoint = new waypointClass();
	boneStateClass			*boat = new boneStateClass();
	arduinoStateClass		*arduinoState = new arduinoStateClass();
	
	// leaf nodes
	resetArduinoRest		*reset = new resetArduinoRest();
	arduinoRESTClass		*arduino = new arduinoRESTClass();
	allDispatchClass		*boneAll = new allDispatchClass(boat);
	allDispatchClass		*gpsAll = new allDispatchClass(gps);
	allDispatchClass		*waypointAll = new allDispatchClass(waypoint);
	allDispatchClass		*navAll = new allDispatchClass(nav);
	allDispatchClass		*arduinoAll = new allDispatchClass(arduinoState);
	numberDispatchClass		*boneNum = new numberDispatchClass(boat);
	numberDispatchClass		*gpsNum = new numberDispatchClass(gps);
	numberDispatchClass		*waypointNum = new numberDispatchClass(waypoint);
	numberDispatchClass		*navNum = new numberDispatchClass(nav); 
	numberDispatchClass		*arduinoNum = new numberDispatchClass(arduinoState);
	countDispatchClass		*boneCount = new countDispatchClass(boat);
	countDispatchClass		*gpsCount = new countDispatchClass(gps); 
	countDispatchClass		*waypointCount = new countDispatchClass(waypoint);
	countDispatchClass		*navCount = new countDispatchClass(nav); 
	countDispatchClass		*arduinoCount = new countDispatchClass(arduinoState);
	appendDispatchClass		*waypointAppend = new appendDispatchClass(waypoint);
	insertDispatchClass		*waypointInsert = new insertDispatchClass(waypoint);
	
	// root & branch nodes
	boneStateRESTClass 		*boatREST = new boneStateRESTClass({{"all", boneAll}, 
																{"count", boneCount}});
	gpsRESTClass 			*gpsREST = new gpsRESTClass({{"all", gpsAll}, 
														{"count", gpsCount}});
	waypointRESTClass		*waypointREST = new waypointRESTClass({{"all", waypointAll}, 
																	{"count", waypointCount}, 
																	{"append", waypointAppend}, 
																	{"insert", waypointInsert}});
	navRESTClass			*navREST = new navRESTClass({{"all", navAll}, {"count", navCount}});
	arduinoStateRESTClass	*arduinoREST = new arduinoStateRESTClass({{"all", arduinoAll}, 
																		{"count", arduinoCount}});
	rootRESTClass			*root = new rootRESTClass({{"boneState",boatREST}, 
														{"gps", gpsREST}, 
														{"waypoint", waypointREST}, 
														{"nav", navREST}, 
														{"arduinoState", arduinoREST}, 
														{"arduino", arduino}, 
														{"resetArduino", reset}});

	boatREST->addNumber(boneNum);
	gpsREST->addNumber(gpsNum);
	waypointREST->addNumber(waypointNum);
	navREST->addNumber(navNum);
	arduinoREST->addNumber(arduinoNum);
	
	return root;
}
