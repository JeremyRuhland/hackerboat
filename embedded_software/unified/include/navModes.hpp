/******************************************************************************
 * Hackerboat Beaglebone navigation modes module
 * navModes.hpp
 * This is the navigation sub-modes
 * see the Hackerboat documentation for more details
 * Written by Pierce Nichols, Aug 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/
 
#ifndef NAVMODES_H
#define NAVMODES_H 
 
#include <stdlib.h>
#include <string>
#include <chrono>
#include "hal/config.h"
#include "enumdefs.hpp"
#include "stateMachine.hpp"
#include "boatState.hpp"
#include "rcModes.hpp"
#include "autoModes.hpp"
#include "util.hpp"

class NavModeBase : public StateMachineBase<NavModeEnum, BoatState> {
	public:
		static NavModeBase* factory(BoatState& state, NavModeEnum mode);			/**< Create a new object of the given mode */
		virtual NavModeBase* execute () = 0;
		virtual ~NavModeBase() {};
	protected:
		NavModeBase (BoatState& state, NavModeEnum last, NavModeEnum thisMode) :
			StateMachineBase<NavModeEnum, BoatState> (state, last, thisMode) {};	/**< Protected constructor to allow the subclasses to call the superclass constructor. */
		
};

class NavIdleMode : public NavModeBase {
	public:
		NavIdleMode (BoatState& state, NavModeEnum last = NavModeEnum::NONE) : 
			NavModeBase(state, last, NavModeEnum::IDLE) {
				state.setNavMode(NavModeEnum::IDLE);
			};
		NavModeBase* execute ();											/**< Execute the given mode. */
};

class NavFaultMode : public NavModeBase {
	public:
		NavFaultMode (BoatState& state, NavModeEnum last = NavModeEnum::NONE) : 
			NavModeBase(state, last, NavModeEnum::FAULT) {
				state.setNavMode(NavModeEnum::FAULT);
			};
		NavModeBase* execute ();											/**< Execute the given mode. */
};

class NavRCMode : public NavModeBase {
	public:
		NavRCMode (BoatState& state, NavModeEnum last = NavModeEnum::NONE, RCModeEnum submode = RCModeEnum::IDLE) : 
			NavModeBase(state, last, NavModeEnum::RC),
			_rcMode(RCModeBase::factory(state, submode)), _oldRCmode(NULL) {
				state.setNavMode(NavModeEnum::RC);
			};
		NavModeBase* execute ();											/**< Execute the given mode. */
		RCModeBase* getRCMode () {return _rcMode;};							/**< Get the current RC mode object */
		~NavRCMode () {														/**< Explicit destructor to make sure we take care of the submode */
			REMOVE(_rcMode);
			REMOVE(_oldRCmode);
		}
	private:
		RCModeBase* _rcMode;
		RCModeBase* _oldRCmode;
};

class NavAutoMode : public NavModeBase {
	public:
		NavAutoMode (BoatState& state, NavModeEnum last = NavModeEnum::NONE, AutoModeEnum submode = AutoModeEnum::IDLE) : 
			NavModeBase(state, last, NavModeEnum::AUTONOMOUS),
			_autoMode(AutoModeBase::factory(state, submode)), _oldAutoMode(NULL) {
				state.setNavMode(NavModeEnum::AUTONOMOUS);
			};
		NavModeBase* execute ();											/**< Execute the given mode. */
		AutoModeBase* getAutoMode () {return _autoMode;};					/**< Get the current autonomous mode object */
		~NavAutoMode () {													/**< Explicit destructor to make sure we take care of the submode */
			REMOVE(_autoMode);
			REMOVE(_oldAutoMode);
		}
	private:
		AutoModeBase* _autoMode;
		AutoModeBase* _oldAutoMode;
};

#endif
