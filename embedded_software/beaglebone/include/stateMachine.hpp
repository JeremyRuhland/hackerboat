/******************************************************************************
 * Hackerboat Beaglebone State machine mode
 * hackerboatStateMachine.hpp
 * This program is the core vessel state machine module
 * see the Hackerboat documentation for more details
 * Written by Pierce Nichols, Feb 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/
 
#ifndef STATEMACHINE_H
#define STATEMACHINE_H 
 
#include <jansson.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "config.h"
#include "boneState.hpp"
#include "arduinoState.hpp"
#include "navigation.hpp"

class stateTimer {
	public:
		stateTimer (double duration, uint64_t frameTime);	/**< Duration in seconds */
		stateTimer (uint32_t duration) {_duration = duration;};	/**< Duration in frames */
		void setDuration (double duration, uint64_t frameTime);
		void setDuration (uint32_t duration) {_duration = duration;};
		void count (void) {_current++;};
		bool isComplete (void) {return (_current >= _duration);};
		void reset (void) {_current = 0;};
	private:
		uint32_t _duration;
		uint32_t _current;
};

class stateMachineBase {
	public:
		stateMachineBase (boneStateClass *state, arduinoStateClass *ard) {
			_state = state;
			_ard = ard;
			clock_gettime(CLOCK_REALTIME, &_start);
			_lastState = this->_state->mode;
		};
		virtual stateMachineBase *execute (void) = 0;
		boneStateClass *getState (void) {return _state;};
		arduinoStateClass *getArduino (void) {return _ard;}; 
		bool GNSSFail (void);
		bool arduinoFail (void);
		bool shoreFail (void);
		bool isDisarmed (void);
		bool isFaulted (void);
		
	protected:
		boneStateClass 		*_state;
		arduinoStateClass	*_ard;
		timespec			_start;
		boatModeEnum		_lastState;
	
};

class boneStartState : public stateMachineBase {
	public:
		boneStartState (boneStateClass *state, arduinoStateClass *ard) : stateMachineBase(state,ard) {};
		stateMachineBase *execute (void);
};

class boneSelfTestState : public stateMachineBase {
	public:
		boneSelfTestState (boneStateClass *state, arduinoStateClass *ard) : stateMachineBase(state,ard) {};
		stateMachineBase *execute (void);
};

class boneDisarmedState : public stateMachineBase {
	public:
		boneDisarmedState (boneStateClass *state, arduinoStateClass *ard) : stateMachineBase(state,ard) {};
		stateMachineBase *execute (void);
};

class boneArmedState : public stateMachineBase {
	public:
		boneArmedState (boneStateClass *state, arduinoStateClass *ard) : stateMachineBase(state,ard) {};
		stateMachineBase *execute (void);
};

class boneManualState : public stateMachineBase {
	public:
		boneManualState (boneStateClass *state, arduinoStateClass *ard) : stateMachineBase(state,ard) {};
		stateMachineBase *execute (void);
};

class boneWaypointState : public stateMachineBase {
	public:
		boneWaypointState (boneStateClass *state, arduinoStateClass *ard) : stateMachineBase(state,ard) {};
		stateMachineBase *execute (void);
	private:
		navClass 		_nav;
		waypointClass	_wp;
};

class boneNoSignalState : public stateMachineBase {
	public:
		boneNoSignalState (boneStateClass *state, arduinoStateClass *ard) : stateMachineBase(state,ard) {};
		stateMachineBase *execute (void);
	private:
		stateMachineBase *returnLastState (void);
};

class boneReturnState : public stateMachineBase {
	public:
		boneReturnState (boneStateClass *state, arduinoStateClass *ard) : stateMachineBase(state,ard) {};
		stateMachineBase *execute (void);
	private:
		navClass _nav;
};

class boneArmedTestState : public stateMachineBase {
	public:
		boneArmedTestState (boneStateClass *state, arduinoStateClass *ard) : stateMachineBase(state,ard) {};
		stateMachineBase *execute (void);
};

class boneFaultState : public stateMachineBase {
	public:
		boneFaultState (boneStateClass *state, arduinoStateClass *ard) : stateMachineBase(state,ard) {};
		stateMachineBase *execute (void);
};

#endif /* STATEMACHINE_H */