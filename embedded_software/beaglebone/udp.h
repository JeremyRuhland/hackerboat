/**
 * @file udp.h
 * @brief UDP connection functions
 *
 * @author Jeremy Ruhland <jeremy ( a t ) goopypanther.org>
 * @author Bryan Godbolt godbolt ( a t ) ualberta.ca
 * @author Wim Lewis wiml ( a t ) hhhh.org
 *
 * @license GPL 3.0
 * @version 1.0
 * @since Jun 16, 2015
 */

#ifndef UDP_H_
#define UDP_H_

extern void udpOpenSocket(const char *host);
extern void udpCloseSocket(void);
extern void udpSend(const uint8_t *data, uint32_t dataLength);
extern uint32_t udpGetMessage(mavlink_message_t *message, mavlink_status_t *messageStatus);

#endif /* UDP_H_ */
