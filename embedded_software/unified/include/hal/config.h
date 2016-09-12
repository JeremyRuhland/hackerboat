#ifndef CONFIG_H
#define CONFIG_H

#include <chrono>

#define ARDUINO_BUF_LEN 	4096
#define LOCAL_BUF_LEN		32768
#define MAX_URI_TOKENS		8
#define REST_ID				254
#define REST_NAME			"BoneHackerBoat"
#define MAX_TOKENS			5
#define MAX_TOKEN_LEN		64
#define HASHSEED			0xdeadbeef
#define GNSS_TTY			"/dev/ttyS4"
#define GNSS_UART			UART4
#define GNSS_BPS			B9600
#define GNSS_BAUD			Baud9600
#define GNSS_TIMEOUT		(180)
#define FRAME_LEN_NS		(500000000)		/// State machine execution frame length, in nanoseconds
#define UART_TIMEOUT		(100)		/// UART contention timeout, in milliseconds
#define UART_READ_TIMEOUT	(15)		/// UART read timeout, in milliseconds
#define SHORE_TIMEOUT		(60)
#define RETURN_TIMEOUT		(180)
#define LAUNCH_WAYPOINT		(0)

// New hardware declarations

#define ADC_UPPER_ADDR				(0x1f)
#define ADC_LOWER_ADDR				(0x1d)
#define ADC_I2C_BUS					(I2CBus::BUS_2)
#define IMU_I2C_BUS					(I2CBus::BUS_1)
#define THROTTLE_MAX				(5)
#define THROTTLE_MIN				(-5)
#define LIGHTS_COUNT				(72)
#define AIS_MAX_TIME				(600s)
#define AIS_MAX_DISTANCE			(10)
#define CONFIG_PIN_PATH				"/usr/local/bin/config-pin"
#define SYSTEM_DISARM_INPUT_PORT	(8)
#define SYSTEM_DISARM_INPUT_PIN		(22)
#define SYSTEM_ARM_INPUT_PORT		(8)
#define SYSTEM_ARM_INPUT_PIN		(20)
#define	SYSTEM_SERVO_PORT			(8)
#define SYSTEM_SERVO_PIN			(19)

// Timeouts

#define GPS_SENSE_TIMEOUT	(5s)
#define RC_SENSE_TIMEOUT	(500ms)
#define IMU_SENSE_TIMEOUT	(500ms)
#define IMU_LOCK_TIMEOUT	(300us)
#define SELFTEST_DELAY		(30s)
#define ADC_START_COUNT		(30)  
#define ADC_START_PERIOD	(500us)
#define ADC_LOCK_TIMEOUT	(500us)
#define DISARM_PULSE_LEN	(50ms)
#define ARM_PULSE_LEN		(50ms)
#define HORN_TIME			(2s)

// Voltage/current limits

#define SYSTEM_START_BATTERY_MIN	(12.0)

// Log file names
#define MAIN_LOGFILE		"main.log"
#define HARDWARE_LOGFILE	"hardware.log"

// Relay map initializer
#define RELAY_MAP_INITIALIZER	{ { "RED", Relay("RED", Pin(8, 3, true), Pin(8, 4, false)) }, \
								  { "WHITE", Relay("WHITE", Pin(8, 5, true), Pin(8, 6, false)) }, \
								  { "YELLOW", Relay("YELLOW", Pin(8, 7, true), Pin(8, 8, false)) }, \
								  { "REDWHT", Relay("REDWHT", Pin(8, 9, true), Pin(8, 10, false)) }, \
								  { "YLWWHT", Relay("YLWWHT", Pin(8, 11, true), Pin(8, 12, false)) }, \
								  { "DIR", Relay("DIR", Pin(8, 13, true), Pin(8, 14, false)) }, \
								  { "DISARM", Relay("DISARM", Pin(8, 15, true), Pin(8, 16, false)) }, \
								  { "HORN", Relay("HORN", Pin(8, 17, true), Pin(8, 18, false)) }, \
								  { "ENABLE", Relay("ENABLE", Pin(8, 24, true), Pin(8, 26, false)) } }

// Analog initializers
#define ADC_UPPER_INITIALIZER	{"RED", "WHITE", "YELLOW", "REDWHT", "YLWWHT", "DIR", "DISARM", "ENABLE"}
#define ADC_LOWER_INITIALIZER	{"HORN", "mot_i", "mot_v", "charge_v", "charge_i", "aux_0", "aux_1", "servo_i"}								  
#define ADC128D818_EXTERNAL_REF	(5.0)
#define ADC_BATMON_PATH			""
#define ADC_BATMON_NAME			"battery_mon"

// Database names
#define DB_DIRECTORY		"/tmp" /* Or override with DB_DIRECTORY environment variable */
#define GPS_DB_FILE			"gps.db"
#define AIS_DB_FILE			"ais.db"
#define STATE_DB_FILE		"state.db"
#define HEALTH_DB_FILE		"health.db"

#endif /* CONFIG_H */


