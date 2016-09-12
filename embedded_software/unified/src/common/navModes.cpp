/******************************************************************************
 * Hackerboat Beaglebone navigation modes module
 * navModes.cpp
 * This is the navigation sub-modes
 * see the Hackerboat documentation for more details
 * Written by Pierce Nichols, Sep 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/
 
#include <jansson.h>
#include <stdlib.h>
#include <string>
#include <chrono>
#include "hal/config.h"
#include "enumdefs.hpp"
#include "stateMachine.hpp"
#include "boatState.hpp"
#include "rcModes.hpp"
#include "autoModes.hpp"
#include "navModes.hpp"

NavModeBase *NavModeBase::factory(BoatState& state, NavModeEnum mode) {
	switch (mode) {
		case NavModeEnum::IDLE:
			return new NavIdleMode(state, state.getNavMode());
			break;
		case NavModeEnum::FAULT:
			return new NavFaultMode(state, state.getNavMode());
			break;
		case NavModeEnum::RC:
			return new NavRCMode(state, state.getNavMode());
			break;
		case NavModeEnum::AUTONOMOUS:
			return new NavAutoMode(state, state.getNavMode());
		case NavModeEnum::NONE:
		default:
			return new NavIdleMode(state, state.getNavMode());
			break;
	}
}

NavModeBase *NavIdleMode::execute () {
	_state.throttle->setThrottle(0);
	_state.rudder->write(0);
	_state.servoEnable.set();
	return this;
}

NavModeBase *NavFaultMode::execute () {
	return this;
}

NavModeBase *NavRCMode::execute () {
	return this;
}

NavModeBase *NavAutoMode::execute () {
	return this;
}