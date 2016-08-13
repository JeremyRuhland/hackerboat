/******************************************************************************
 * Hackerboat adc128d818 driver module
 * hal/adc128d818.hpp
 * This module reads analog data from ADC128D818 devices
 * Based on https://github.com/bryanduxbury/adc128d818_driver
 *
 * See the Hackerboat documentation for more details
 * Written by Pierce Nichols, Aug 2016
 * 
 * Version 0.1: First alpha
 *
 ******************************************************************************/

#ifndef ADC128D818_H
#define ADC128D818_H

#include <errno.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "hal/config.h"
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>

#define ADC128D818_INTERNAL_REF		2.5

enum class reference_mode_t {
  INTERNAL_REF = 0, EXTERNAL_REF = 1
};

enum class conv_mode_t {
  LOW_POWER, CONTINUOUS, ONE_SHOT
};

enum class operation_mode_t {
  SINGLE_ENDED_WITH_TEMP = 0,
  SINGLE_ENDED = 1, 
  DIFFERENTIAL = 2,
  MIXED = 3
};

class ADC128D818 {
	public:
		ADC128D818(uint8_t address, std::string devpath);	/**< Create an ADC object for a sensor on bus devpath with address address */
  
		void setReference(double ref_voltage);				/**< Set the reference voltage for scaling purposes (external reference only) */
		void setReferenceMode(reference_mode_t mode);		/**< Set the reference mode (either internal or external) */
		void setOperationMode(operation_mode_t mode);		/**< Set the operation mode of the ADC */

		void setDisabledMask(uint8_t disabled_mask);		/**< disable channels */

		void setConversionMode(conv_mode_t mode);			/**< Set the conversion mode */

		void begin();										/**< Initialize the sensor. ReferenceMode, OperationMode, and ConversionMode should already be set */

		int16_t read(uint8_t channel);						/**< Read the given channel. Returns -1 if the channel is disabled. */
		
		vector<int16_t> readAll (void);						/**< Returns a vector with all channels. Disabled channels are set to -1 */

		double readScaled(uint8_t channel);					/**< Reads data and scales it according to the reference voltage. Returns NAN if the channel is disabled. */
		
		vector<double> readScaled (void);					/**< Returns a vector with the scaled voltage of all channels. Disabled channels contain NAN. */

		double readTemperatureScaled();						/**< Read the ADC temperature */

	private:
		std::string path;
		int devhandle;
	
		uint8_t addr;
		uint8_t disabled_mask;
		double ref_v;

		reference_mode_t ref_mode;
		operation_mode_t op_mode;
		conv_mode_t conv_mode;

		void setRegisterAddress(uint8_t reg_addr);
		void setRegister(uint8_t reg_addr, uint8_t value);
		uint8_t readCurrentRegister8();
};


#endif
