/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2022 rt-labs AB, Sweden. All rights reserved.
 *
 * See the file LICENSE.md distributed with this software for full
 * license information.
 ********************************************************************/

#ifndef CL_SLMP_UDP_H
#define CL_SLMP_UDP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/cl_types.h"

#include <inttypes.h>
#include <stddef.h>

/**
 * Send a SLMP message via UDP
 *
 * Opens a socket, sends the message and closes the socket.
 *
 * @param socket              Socket
 * @param open_the_socket     Open the socket before use, and
 *                            close it afterwards
 * @param buffer              Buffer with outgoing message
 * @param buffer_size         Buffer size
 * @param local_ip            Local IP address for opening the socket
 * @param remote_ip           Remote (destination) IP address
 * @param remote_port         Remote UDP port
 * @param message_size        Number of bytes to send
 * @return 0 on success, or -1 on error
 */
int cl_slmp_udp_send (
   int * socket,
   bool open_the_socket,
   uint8_t * buffer,
   size_t buffer_size,
   cl_ipaddr_t local_ip,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port,
   size_t message_size);

#ifdef __cplusplus
}
#endif

#endif /* CL_SLMP_UDP_H */
