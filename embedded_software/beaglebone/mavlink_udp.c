/*****************************************************************************/
/* Hackerboat Mavlink Control
/*
/* Jeremy Ruhland jeremy ( a t ) goopypanther.org
/* Bryan Godbolt godbolt ( a t ) ualberta.ca
/*
/* GNU General Public License blah blah blah
/*
/* This program sends some data to qgroundcontrol using the mavlink protocol.
/*
/*****************************************************************************/
#include "includes.h"

/*****************************************************************************/
/* System defines
/*****************************************************************************/
#define BUFFER_LENGTH 2041 // minimum buffer size that can be used with qnx (I don't know why)

#define LOG_FILE_NAME "./mavlink_udp.log"

#define LOOPTIME 1 // microseconds
#define LOOPS_PER_SECOND 10000 // Fudged for execution time of program

// Identification for our system
#define SYSTEM_ID          2
#define MAV_VEHICLE_TYPE   MAV_TYPE_SURFACE_BOAT
#define MAV_AUTOPILOT_TYPE MAV_AUTOPILOT_GENERIC
#define SENSORS (MAV_SYS_STATUS_SENSOR_3D_GYRO|MAV_SYS_STATUS_SENSOR_3D_ACCEL|MAV_SYS_STATUS_SENSOR_3D_MAG|MAV_SYS_STATUS_SENSOR_GPS|MAV_SYS_STATUS_SENSOR_MOTOR_OUTPUTS)
#define SEC_UNTIL_PANIC 10 // Seconds without heartbeat until failsafe mode
#define NUM_MISSION_ITEMS 256 // Number of mission items (waypoints) we can store

#define ACCEPTABLE_DISTANCE_FROM_WAYPOINT 100 // Meters

/*****************************************************************************/
/* Function prototypes
/*****************************************************************************/
uint64_t microsSinceEpoch(void);
void parseInputParams(int argc, char* argv[]);
void openNetworkSocket(void);
void sendMavlinkPacketOverNetwork(void);
void listParams(void);
void parsePacket(void);
void setMode(void);
void sendNumMissionItems(void);
void sendMissionItem(void);
void receiveMissionCount(void);
void receiveMissionItem(void);
void clearAllMissionItems(void);
void parseCommand(void);
void reachedTargetWaypoint(void);
float getDistanceToWaypoint(void);
float getAngleToWaypoint(void);
void getGpsPosition(void);
void openUarts(void);
void sendPacketToLowLevel(void);
void readLowLevelVoltageLevel(void);
float stringToFloat(position_string_t position);

/*****************************************************************************/
/* Global vars
/*****************************************************************************/

// Param buffers
char target_ip[100];
char gps_serial_device[100];
char low_level_serial_device[100];
char low_level_debug_device[100];

// Network socket vars
int sock;
struct sockaddr_in gcAddr;
struct sockaddr_in locAddr;
uint8_t buf[BUFFER_LENGTH];
ssize_t recsize;
socklen_t fromLen;
int bytes_sent;

// UART vars
FILE *lowLevelControl;
FILE *gpsModule;
FILE *lowLevelDebug;
int lowLevelControlFD;
int lowLevelDebugFD;
int gpsModuleFD;
uint8_t uartBuf[BUFFER_LENGTH];

// Logfile vars
FILE *logFile;
time_t currentTime;

// Msc function call vars
int i;
int functionReturnValue;
float functionReturnValueFloat;
unsigned int temp;

// Mavlink vars
mavlink_message_t msg;
mavlink_status_t status;
uint16_t len;

uint64_t timeOfLastHeartbeat;
uint16_t heartbeatCount;
position_t position = {0, 0, 0, 0, 0, 0, 0, 0};
position_t target;
uint8_t targetWaypoint;
mavlink_command_long_t currentCommand;
uint16_t voltageLevel;

MAV_MODE currentMavMode;
MAV_STATE currentMavState;

mavlink_mission_item_t missionItems[NUM_MISSION_ITEMS];
uint16_t numMissionItems;
uint16_t currentlyInterogatedMissionItem;
uint16_t currentlyActiveMissionItem;


/*****************************************************************************/
/* Main
/*****************************************************************************/
int main(int argc, char* argv[]) {
    parseInputParams(argc, argv); // Parse input params

    openNetworkSocket(); // Open network socket

    openUarts();

    logFile = fopen(LOG_FILE_NAME, "a");

    timeOfLastHeartbeat = microsSinceEpoch(); // Prevent failsafe on first loop
    heartbeatCount = 0;
    currentMavState = MAV_STATE_BOOT; // Put us on standby state
    currentMavMode = MAV_MODE_MANUAL_DISARMED; // Put us in manual mode

    // Set mission item index to 0
    numMissionItems = 0;
    currentlyActiveMissionItem = 0;
    targetWaypoint = 0;
    voltageLevel = 12000;

    // Main loop
    for (;;) {
        if (heartbeatCount == LOOPS_PER_SECOND) {
            currentTime = time(0);
            printf("\n%s", ctime(&currentTime));
            fprintf(logFile, "\n%s", ctime(&currentTime));

            // Send Heartbeat
            mavlink_msg_heartbeat_pack(SYSTEM_ID,                  // System ID
                                       MAV_COMP_ID_SYSTEM_CONTROL, // Component ID
                                       &msg,                       // Message buffer
                                       MAV_VEHICLE_TYPE,           // MAV vehicle type
                                       MAV_AUTOPILOT_TYPE,         // Autopilot type
                                       currentMavMode,             // System mode
                                       0,                          // Custom mode (empty)
                                       currentMavState);           // System status

            sendMavlinkPacketOverNetwork();

            // Transmitted packet
            printf("\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d heartbeat", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);
            fprintf(logFile, "\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d heartbeat", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);

            // Send Status
            mavlink_msg_sys_status_pack(SYSTEM_ID,                  // System ID
                                        MAV_COMP_ID_SYSTEM_CONTROL, // Component ID
                                        &msg,                       // Message buffer
                                        SENSORS,                    // Sensors present
                                        SENSORS,                    // Sensors enabled
                                        SENSORS,                    // Sensor health
                                        500,                        // System load
                                        voltageLevel,               // Batt voltage
                                        -1,                         // Batt current
                                        -1,                         // Batt remaining
                                        0,                          // Comm drop percentage
                                        0,                          // Comm errors
                                        0,                          // Custom error 1
                                        0,                          // Custom error 2
                                        0,                          // Custom error 3
                                        0);                         // Custom error 4

            sendMavlinkPacketOverNetwork();

            // Transmitted packet
            printf("\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d status", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);
            fprintf(logFile, "\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d status", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);

            // Send GPS Position
            mavlink_msg_global_position_int_pack(SYSTEM_ID,                             // System ID
                                                 MAV_COMP_ID_GPS,                       // Component ID
                                                 &msg,                                  // Message buffer
                                                 microsSinceEpoch(),                    // Current time
                                                 ((int32_t) (position.lat * 10000000)), // Lat
                                                 ((int32_t) (position.lon * 10000000)), // Long
                                                 position.alt,                          // Alt
                                                 position.alt,                          // Relative alt (assume same)
                                                 position.vx,                           // X vel
                                                 position.vy,                           // Y vel
                                                 position.vz,                           // Z vel
                                                 position.hdg);                         // Heading

            sendMavlinkPacketOverNetwork();

            // Transmitted packet
            printf("\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d GPS", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);
            fprintf(logFile, "\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d GPS", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);

            // Send current waypoint if in auto mode
            if (currentMavMode == MAV_MODE_AUTO_ARMED) {
                mavlink_msg_mission_current_pack(SYSTEM_ID,
                                                 MAV_COMP_ID_SYSTEM_CONTROL,
                                                 &msg,
                                                 targetWaypoint);

                sendMavlinkPacketOverNetwork();

                // Transmitted packet
                printf("\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d current mission", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);
                fprintf(logFile, "\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d current mission", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);
            } else {}

            // Update GPS position
#if 0
            getGpsPosition();
            if (position.lock) {
                currentMavState = MAV_STATE_ACTIVE; // GPS lock, go to active state
            } else {
                currentMavState = MAV_STATE_BOOT;
            }
#endif
            functionReturnValueFloat = getDistanceToWaypoint();
            if (functionReturnValueFloat <= ACCEPTABLE_DISTANCE_FROM_WAYPOINT) {
                reachedTargetWaypoint();
            } else {}

            functionReturnValueFloat = getDistanceToWaypoint();
            functionReturnValueFloat = getAngleToWaypoint();

            // Update compass

            heartbeatCount = 0;
        } else {
            heartbeatCount++;
        }

        // Receive incoming packets from network interface
        memset(buf, 0, BUFFER_LENGTH);
        recsize = recvfrom(sock,                         // Socket device
                           (void *) buf,                 // Receive buffer
                           BUFFER_LENGTH,                // Length of buffer
                           0,                            // Extra settings (none)
                           (struct sockaddr *) &gcAddr, // Receive from address
                           &fromLen);                    // Receive address length
        if (recsize > 0) {
            // Something received - print out all bytes and parse packet
            //printf("Bytes Received: %d\nDatagram: ", (int)recsize);
            for (i = 0; i < recsize; ++i) {
                temp = buf[i];
                //printf("%02x ", (unsigned char)temp);
                functionReturnValue = mavlink_parse_char(MAVLINK_COMM_0, // Channel
                                                         buf[i],         // Char to parse
                                                         &msg,           // Message buffer
                                                         &status);       // Message status
                if (functionReturnValue) {
                    // Packet received
                    printf("\nReceived packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d ", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);
                    fprintf(logFile, "\nReceived packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d ", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);

                    sendPacketToLowLevel();
                    parsePacket();
                } else {}
            }
        } else {}
        memset(buf, 0, BUFFER_LENGTH);

        // Read voltage level from low level device
        readLowLevelVoltageLevel();

        // Watchdog return, if out of contact for 10 seconds, beach self
        if ((microsSinceEpoch()-timeOfLastHeartbeat) > (SEC_UNTIL_PANIC * 1000000)) {
            if (heartbeatCount >= LOOPS_PER_SECOND) {
                printf("\n*** FAILSAFE ***");
                fprintf(logFile, "\n*** FAILSAFE ***");
            } else {}
            // Head for beach
        } else {}

        // Flush buffers to force output
        fflush(NULL);

        usleep(LOOPTIME); // Sleep 1 microsecond
    }
}

uint64_t microsSinceEpoch(void) {

    struct timeval tv;

    uint64_t micros = 0;

    gettimeofday(&tv, NULL);
    micros =  ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;

    return micros;
}

void parseInputParams(int argc, char* argv[]) {
    strcpy(target_ip, "127.0.0.1"); // Set default ip
    if (argc == 4) {
        strcpy(target_ip, argv[1]); // Change the target ip if parameter was given
        strcpy(gps_serial_device, argv[2]);
        strcpy(low_level_serial_device, argv[3]);
    } else {
        printf("\n");
        printf("\tUsage:\n\n");
        printf("\t");
        printf("%s", argv[0]);
        printf(" <host ip> <GPS serial device> <low level serial device>\n");
        printf("\tDefault for localhost: udp-server 127.0.0.1 /dev/tty1 /dev/tty2\n\n");
        exit(EXIT_FAILURE);
    }
}

void openNetworkSocket(void) {
    memset(&locAddr, 0, sizeof(locAddr));
    locAddr.sin_family = AF_INET;
    locAddr.sin_addr.s_addr = INADDR_ANY;
    locAddr.sin_port = htons(14551);

    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    // Bind the socket to port 14551 - necessary to receive packets from qgroundcontrol
    functionReturnValue = bind(sock,                         // Socket identifier
                               (struct sockaddr *) &locAddr, // Bind location & address
                               sizeof(struct sockaddr));     // Etc.
    if (functionReturnValue == -1) { // Warn us if bind fails
        perror("error bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    } else {}

    // Attempt to make socket non blocking
    functionReturnValue = fcntl(sock, F_SETFL, O_NONBLOCK);
    if (functionReturnValue < 0) {
        fprintf(stderr, "error setting nonblocking: %s\n", strerror(errno));
        close(sock);
        exit(EXIT_FAILURE);
    } else {}

    memset(&gcAddr, 0, sizeof(gcAddr));
    gcAddr.sin_family = AF_INET;
    gcAddr.sin_addr.s_addr = inet_addr(target_ip);
    gcAddr.sin_port = htons(14550);
}

void sendMavlinkPacketOverNetwork(void) {
    len = mavlink_msg_to_send_buffer(buf, &msg);

    bytes_sent = sendto(sock,                         // Outgoing device
                        buf,                          // Buffer
                        len,                          // Buffer size
                        0,                            // Flags
                        (struct sockaddr*)&gcAddr,    // Address
                        sizeof(struct sockaddr_in));  // Address length
}

void parsePacket(void) {
    //printf("\nWe got a packet!\n");
    switch (msg.msgid) { // Decide what to do with packet
    case MAVLINK_MSG_ID_HEARTBEAT: // Record time of last heartbeat for watchdog
        printf("heartbeat");
        fprintf(logFile, "heartbeat");
        timeOfLastHeartbeat = microsSinceEpoch();
        break;
    case MAVLINK_MSG_ID_PARAM_REQUEST_LIST:
    case MAVLINK_MSG_ID_PARAM_REQUEST_READ:
        printf("param request");
        fprintf(logFile, "param request");
        listParams();
        break;
    case MAVLINK_MSG_ID_MISSION_REQUEST_LIST:
        printf("mission list request");
        fprintf(logFile, "mission list request");
        sendNumMissionItems();
        break;
    case MAVLINK_MSG_ID_MISSION_REQUEST:
        printf("mission item request %d", mavlink_msg_mission_request_get_seq(&msg));
        fprintf(logFile, "mission item request %d", mavlink_msg_mission_request_get_seq(&msg));
        sendMissionItem();
        break;
    case MAVLINK_MSG_ID_MISSION_ACK:
        printf("mission ack");
        fprintf(logFile, "mission ack");
        break;
    case MAVLINK_MSG_ID_MISSION_COUNT:
        printf("mission count %d", mavlink_msg_mission_count_get_count(&msg));
        fprintf(logFile, "mission count %d", mavlink_msg_mission_count_get_count(&msg));
        receiveMissionCount();
        break;
    case MAVLINK_MSG_ID_MISSION_ITEM:
        printf("mission item %d", mavlink_msg_mission_item_get_seq(&msg));
        fprintf(logFile, "mission item %d", mavlink_msg_mission_item_get_seq(&msg));
        receiveMissionItem();
        break;
    case MAVLINK_MSG_ID_MISSION_CLEAR_ALL:
        printf("clear all mission items");
        fprintf(logFile, "clear all mission items");
        clearAllMissionItems();
        break;
    case MAVLINK_MSG_ID_SET_MODE:
        printf("set mode");
        fprintf(logFile, "set mode");
        setMode();
        break;
    case MAVLINK_MSG_ID_COMMAND_LONG:
        printf("command");
        fprintf(logFile, "command");
        parseCommand();
        break;
    default:
        printf("other packet type");
        fprintf(logFile, "other packet type");
        break;
    }
}

void listParams(void) {
    mavlink_msg_param_value_pack(SYSTEM_ID,                  // System ID
                                 MAV_COMP_ID_SYSTEM_CONTROL, // Component
                                 &msg,                       // Message buffer
                                 "NOPARAMS",                 // Param name
                                 0,                          // Param value (dummy)
                                 MAVLINK_TYPE_FLOAT,         // Param type
                                 1,                          // Total params on system
                                 0);                         // Current param index

    sendMavlinkPacketOverNetwork();

    // Transmitted packet
    printf("\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d params list", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);
    fprintf(logFile, "\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d params list", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);
}

void setMode(void) {
    currentMavMode = mavlink_msg_set_mode_get_base_mode(&msg);
}

void sendNumMissionItems(void) {
    mavlink_msg_mission_count_pack(SYSTEM_ID,                  // System ID
                                   MAV_COMP_ID_SYSTEM_CONTROL, // Component ID
                                   &msg,                       // Message buffer
                                   msg.sysid,                  // SysID of system making request
                                   msg.compid,                 // CompID of system making request
                                   numMissionItems);           // Number of currently programmed mission items

    sendMavlinkPacketOverNetwork();

    // Transmitted packet
    printf("\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d mission item count %d", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid, numMissionItems);
    fprintf(logFile, "\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d mission item count %d", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid, numMissionItems);
}

void sendMissionItem(void) {
    currentlyInterogatedMissionItem = mavlink_msg_mission_request_get_seq(&msg);

    if (currentlyInterogatedMissionItem < numMissionItems) {
        mavlink_msg_mission_item_encode(SYSTEM_ID,                                       // System ID
                                        MAV_COMP_ID_SYSTEM_CONTROL,                      // Component ID
                                        &msg,                                            // Message buffer
                                        &missionItems[currentlyInterogatedMissionItem]); // Mission item structure from array of mission items

        sendMavlinkPacketOverNetwork();

        // Transmitted packet
        printf("\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d mission item %d", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid, currentlyInterogatedMissionItem);
        fprintf(logFile, "\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d mission item %d", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid, currentlyInterogatedMissionItem);
    } else {
        printf("\nWe don't have that mission item");
        fprintf(logFile, "\nWe don't have that mission item");
        mavlink_msg_mission_ack_pack(SYSTEM_ID,                  // System ID
                                     MAV_COMP_ID_SYSTEM_CONTROL, // Component ID
                                     &msg,                       // Message buffer
                                     msg.sysid,                  // SysID of requesting system
                                     msg.compid,                 // CompID of requesting system
                                     MAV_MISSION_ERROR);         // We must be out of space

        sendMavlinkPacketOverNetwork();
    }
}

void receiveMissionCount(void) {
    currentlyInterogatedMissionItem = 0;
    numMissionItems = mavlink_msg_mission_count_get_count(&msg);

    mavlink_msg_mission_request_pack(SYSTEM_ID,                        // System ID
                                     MAV_COMP_ID_SYSTEM_CONTROL,       // Component ID
                                     &msg,                             // Message buffer
                                     msg.sysid,                        // SysID of requesting system
                                     msg.compid,                       // CompID of requesting system
                                     currentlyInterogatedMissionItem); // ID of mission item requested

    sendMavlinkPacketOverNetwork();

    // Transmitted packet
    printf("\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d request mission item %d", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid, currentlyInterogatedMissionItem);
    fprintf(logFile, "\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d request mission item %d", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid, currentlyInterogatedMissionItem);
}

void receiveMissionItem(void) {
    currentlyInterogatedMissionItem = mavlink_msg_mission_item_get_seq(&msg);

    if (currentlyInterogatedMissionItem < (numMissionItems-1)) {
        mavlink_msg_mission_item_decode(&msg, &missionItems[currentlyInterogatedMissionItem]);
        missionItems[currentlyInterogatedMissionItem].target_system = msg.sysid;

        mavlink_msg_mission_request_pack(SYSTEM_ID,                            // System ID
                                         MAV_COMP_ID_SYSTEM_CONTROL,           // Component ID
                                         &msg,                                 // Message buffer
                                         msg.sysid,                            // SysID of requesting system
                                         msg.compid,                           // CompID of requesting system
                                         (currentlyInterogatedMissionItem+1)); // ID of mission item requested

        sendMavlinkPacketOverNetwork();

        // Transmitted packet
        printf("\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d request mission item %d", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid, (currentlyInterogatedMissionItem+1));
        fprintf(logFile, "\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d request mission item %d", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid, (currentlyInterogatedMissionItem+1));
    } else if (currentlyInterogatedMissionItem == (numMissionItems-1)) {
        mavlink_msg_mission_item_decode(&msg, &missionItems[currentlyInterogatedMissionItem]);
        missionItems[currentlyInterogatedMissionItem].target_system = msg.sysid;

        mavlink_msg_mission_ack_pack(SYSTEM_ID,                  // System ID
                                     MAV_COMP_ID_SYSTEM_CONTROL, // Component ID
                                     &msg,                       // Message buffer
                                     msg.sysid,                  // SysID of requesting system
                                     msg.compid,                 // CompID of requesting system
                                     MAV_MISSION_ACCEPTED);      // Accepting waypoint list

        sendMavlinkPacketOverNetwork();

        // Transmitted packet
        printf("\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d mission item list ack", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);
        fprintf(logFile, "\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d mission item list ack", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);
    } else {
        printf("\nCant store that many mission items");
        fprintf(logFile, "\nCant store that many mission items");
        mavlink_msg_mission_ack_pack(SYSTEM_ID,                  // System ID
                                     MAV_COMP_ID_SYSTEM_CONTROL, // Component ID
                                     &msg,                       // Message buffer
                                     msg.sysid,                  // SysID of requesting system
                                     msg.compid,                 // CompID of requesting system
                                     MAV_MISSION_NO_SPACE);      // We must be out of space

        sendMavlinkPacketOverNetwork();

    }
}

void clearAllMissionItems(void) {
    numMissionItems = 0;

    mavlink_msg_mission_ack_pack(SYSTEM_ID,                  // System ID
                                 MAV_COMP_ID_SYSTEM_CONTROL, // Component ID
                                 &msg,                       // Message buffer
                                 msg.sysid,                  // SysID of requesting system
                                 msg.compid,                 // CompID of requesting system
                                 MAV_MISSION_ACCEPTED);      // We must be out of space

    sendMavlinkPacketOverNetwork();

    // Transmitted packet
    printf("\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d mission item list ack", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);
    fprintf(logFile, "\nTransmitted packet: SEQ: %d, SYS: %d, COMP: %d, LEN: %d, MSG ID: %d mission item list ack", msg.seq, msg.sysid, msg.compid, msg.len, msg.msgid);
}

void parseCommand(void) {
    mavlink_msg_command_long_decode(&msg, &currentCommand);

    if (currentCommand.target_system == SYSTEM_ID) {
        switch (currentCommand.command) {
        case MAV_CMD_NAV_WAYPOINT:
            currentMavMode = MAV_MODE_MANUAL_ARMED;
            target.lat = currentCommand.param5;
            target.lon = currentCommand.param6;
            break;
        case MAV_CMD_NAV_RETURN_TO_LAUNCH:
            break;
        case MAV_CMD_NAV_TAKEOFF:
            position.lat = missionItems[currentlyActiveMissionItem].x;
            position.lon = missionItems[currentlyActiveMissionItem].y;
            break;
        case MAV_CMD_MISSION_START:
            break;
        default:
            break;
        }
    } else {}
}

void reachedTargetWaypoint(void) {
    if (missionItems[currentlyActiveMissionItem].command == MAV_CMD_DO_JUMP) {
        currentlyActiveMissionItem = missionItems[currentlyActiveMissionItem].param1;
    } else if (missionItems[currentlyActiveMissionItem].autocontinue) {
        currentlyActiveMissionItem++;
    } else if (missionItems[currentlyActiveMissionItem].autocontinue == 0) {
        currentMavMode = MAV_MODE_MANUAL_DISARMED;
    } else {}
}

float getDistanceToWaypoint(void) {
    float a;
    float c;
    float d;
    float targetLat;
    float targetLon;
    float currentLat;
    float currentLon;

    currentLat = (position.lat/(180/M_PI));
    currentLon = (position.lon/(180/M_PI));
    targetLat = (missionItems[currentlyActiveMissionItem].x/(180/M_PI));
    targetLon = (missionItems[currentlyActiveMissionItem].y/(180/M_PI));

    a = powf(sin((targetLat-currentLat)/2), 2)+cosf(currentLat)*cosf(targetLat)*powf(sinf((targetLon-targetLat)/2), 2);
    c = 2*atan2f(sqrtf(a), sqrtf(1-a));
    d = (c*6371)/1000;

    return (d);
}

float getAngleToWaypoint(void) {
    float angle;
    float targetLat;
    float targetLon;
    float currentLat;
    float currentLon;

    currentLat = (position.lat/(180/M_PI));
    currentLon = (position.lon/(180/M_PI));
    targetLat = (missionItems[currentlyActiveMissionItem].x/(180/M_PI));
    targetLon = (missionItems[currentlyActiveMissionItem].y/(180/M_PI));

    angle = atan2f((sinf(targetLon-currentLon)*cosf(targetLat)), (cosf(currentLat)*sinf(targetLat)-sinf(currentLat)*cosf(targetLat)*cosf(targetLon-currentLon)));
    angle *= (180/M_PI);

    return (angle);
}

void openUarts(void) {
    lowLevelControl = fopen(low_level_serial_device, "r+");
    lowLevelControlFD = fileno(lowLevelControl);
    lowLevelDebug = fopen(low_level_debug_device, "r");
    lowLevelDebugFD = fileno(lowLevelDebug);
    gpsModule = fopen(gps_serial_device, "r");
}

void sendPacketToLowLevel(void) {
    len = mavlink_msg_to_send_buffer(uartBuf, &msg);

    write(lowLevelControlFD, uartBuf, len);
    fflush(lowLevelControl);
}

void readLowLevelVoltageLevel(void) {
    int incomingSize;

    memset(uartBuf, 0, BUFFER_LENGTH);
    incomingSize = read(lowLevelControlFD, &uartBuf, BUFFER_LENGTH);

    if (incomingSize) {
        for (i = 0; i < incomingSize; ++i) {
            temp = uartBuf[i];
            functionReturnValue = mavlink_parse_char(MAVLINK_COMM_0,
                                                     uartBuf[i],
                                                     &msg,
                                                     &status);

            if (functionReturnValue && (msg.msgid == MAVLINK_MSG_ID_POWER_STATUS)) {
                voltageLevel = mavlink_msg_power_status_get_Vservo(&msg);
            } else {}
        }
    } else {}

    memset(uartBuf, 0, BUFFER_LENGTH);
}

void getUartDebug(void) {
    memset(uartBuf, 0, BUFFER_LENGTH);
    read(lowLevelDebugFD, &uartBuf, BUFFER_LENGTH);
    printf("\nArduino message", uartBuf);
    fprintf(logFile, "\nArduino message", uartBuf);
    memset(uartBuf, 0, BUFFER_LENGTH);
}

void getGpsPosition(void) {
    position_string_t latString;
    position_string_t lonString;
    position_string_t velocity;
    uint8_t gpsBuff[500];
    uint16_t gpsCount;

    memset(gpsBuff, '\0', 500);
    gpsCount = read(gpsModuleFD, gpsBuff, 500);

functionReturnValue = strncmp(gpsBuff, "$GPRMC", 6);
    while (functionReturnValue) {
        functionReturnValue = strncmp(&gpsBuff[i], "$GPRMC", 6);
        i++;
    }

    for (; gpsBuff[i] != ','; i++) {}
    i++;
    for (; gpsBuff[i] != ','; i++) {}
    i++;
    for (; gpsBuff[i] != ','; i++) {}
    i++;

    latString.c = &gpsBuff[i];

    for (; gpsBuff[i] != ','; i++) {}
    i++;

    if (gpsBuff[i] == 'N') {
        latString.dir = 1;
    } else {
        latString.dir = -1;
    }

    for (; gpsBuff[i] != ','; i++) {}
    i++;

    lonString.c = &gpsBuff[i];
    for (; gpsBuff[i] != ','; i++) {}
    i++;
    if (gpsBuff[i] == 'E') {
        lonString.dir = 1;
    } else {
        lonString.dir = -1;
    }

    for (; gpsBuff[i] != ','; i++) {}
    i++;

    velocity.c = &gpsBuff[i];

    position.lat = ((stringToFloat(latString) * latString.dir)/100);
    position.lon = ((stringToFloat(lonString) * lonString.dir)/100);
    position.vx = ((uint16_t) stringToFloat(velocity));

}

float stringToFloat(position_string_t pos) { // char* s
	long  integer_part = 0;
	float decimal_part = 0.0;
	float decimal_pivot = 0.1;
	uint8_t isdecimal = 0;
    uint8_t isnegative = 0;
	char *c;

    c = pos.c;
	while (*c != ',')  { // ( c = *s++)
		// skip special/sign chars
		if (*c == '-') {
			isnegative = 1;
			c++;
			continue;
		} else {}
		if (*c == '+') {
			c++;
			continue;
		} else {}
		if (*c == '.') {
			isdecimal = 1;
			c++;
			continue;
		} else {}
		
		if (!isdecimal) {
			integer_part = (10 * integer_part) + (*c - 48);
		}
		else {
			decimal_part += decimal_pivot * (float)(*c - 48);
			decimal_pivot /= 10.0;
		}

        c++;
	}
	// add integer part
	decimal_part += (float)integer_part;
	
	// check negative
	if (isnegative)  decimal_part = - decimal_part;

	return decimal_part;
}
