/******************************************************************************
 * Hackerboat Beaglebone autonomous modes module
 * autoModes.hpp
 * This is the autonomous sub-modes
 * see the Hackerboat documentation for more details
 * Written by Pierce Nichols, Aug 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/
 
#ifndef AUTOMODES_H
#define AUTOMODES_H 
 
#include <stdlib.h>
#include <string>
#include <chrono>
#include "hal/config.h"
#include "enumdefs.hpp"
#include "stateMachine.hpp"
#include "boatState.hpp"
#include "configuration.hpp"

class AutoModeBase : public StateMachineBase<AutoModeEnum, BoatState> {
	public:
		static AutoModeBase* factory(BoatState& state, AutoModeEnum mode);			/**< Create a new object of the given mode */
		virtual AutoModeBase* execute () = 0;
		virtual ~AutoModeBase() {};
	protected:
		AutoModeBase (BoatState& state, AutoModeEnum last, AutoModeEnum thisMode) :	/**< Protected constructor so subclas constructors can call the superclass constructor */
			StateMachineBase<AutoModeEnum, BoatState> (state, last, thisMode) {};
};

class AutoIdleMode : public AutoModeBase {
	public:
		AutoIdleMode (BoatState& state, AutoModeEnum last = AutoModeEnum::NONE) : 
			AutoModeBase(state, last, AutoModeEnum::IDLE) {
				state.setAutoMode(AutoModeEnum::IDLE);
			};
		AutoModeBase* execute ();							/**< Execute the current state */
};

class AutoWaypointMode : public AutoModeBase {
	public:
		AutoWaypointMode (BoatState& state, AutoModeEnum last = AutoModeEnum::NONE) : 
			AutoModeBase(state, last, AutoModeEnum::WAYPOINT), 
			helm(&in, &out, &setpoint, 0, 0, 0, 0) {
				state.setAutoMode(AutoModeEnum::WAYPOINT);
				helm.SetMode(AUTOMATIC);
				helm.SetControllerDirection(Conf::get()->rudderDir());
				helm.SetSampleTime(Conf::get()->rudderPeriod());
				helm.SetOutputLimits(Conf::get()->rudderMin(), 
									Conf::get()->rudderMax());
			}; 
		AutoModeBase* execute ();							/**< Execute the current state */
	private:
		PID helm;
		double in = 0;
		double out = 0;
		double setpoint = 0;
		int throttleSetting = Conf::get()->autoDefaultThrottle();
};

class AutoReturnMode : public AutoModeBase {
	public:
		AutoReturnMode (BoatState& state, AutoModeEnum last = AutoModeEnum::NONE) : 
			AutoModeBase(state, last, AutoModeEnum::RETURN), 
			helm(&in, &out, &setpoint, 0, 0, 0, 0) {
				state.setAutoMode(AutoModeEnum::RETURN);
				helm.SetMode(AUTOMATIC);
				helm.SetControllerDirection(Conf::get()->rudderDir());
				helm.SetSampleTime(Conf::get()->rudderPeriod());
				helm.SetOutputLimits(Conf::get()->rudderMin(), 
									Conf::get()->rudderMax());
			}; 
		AutoModeBase* execute ();							/**< Execute the current state */
	private:
		PID helm;
		double in = 0;
		double out = 0;
		double setpoint = 0;
		int throttleSetting = Conf::get()->autoDefaultThrottle();
};

class AutoAnchorMode : public AutoModeBase {
	public:
		AutoAnchorMode (BoatState& state, AutoModeEnum last = AutoModeEnum::NONE) : 
			AutoModeBase(state, last, AutoModeEnum::ANCHOR), 
			helm(&in, &out, &setpoint, 0, 0, 0, 0) {
				state.setAutoMode(AutoModeEnum::ANCHOR);
				helm.SetMode(AUTOMATIC);
				helm.SetControllerDirection(Conf::get()->rudderDir());
				helm.SetSampleTime(Conf::get()->rudderPeriod());
				helm.SetOutputLimits(Conf::get()->rudderMin(), 
									Conf::get()->rudderMax());
			}; 
		AutoModeBase* execute ();							/**< Execute the current state */
		Location* getAnchorPoint() {return &(_state.anchorPoint);};
	private:
		//Location anchorPoint;
		PID helm;
		double in = 0;
		double out = 0;
		double setpoint = 0;
		int throttleSetting = 0;
};

#endif
