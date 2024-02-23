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

/**
 * @file
 * @brief Functions for sending SLMP UDP frames
 */

#ifdef UNIT_TEST
#define clal_udp_open   mock_clal_udp_open
#define clal_udp_sendto mock_clal_udp_sendto
#define clal_udp_close  mock_clal_udp_close
#endif

#include "common/cl_slmp_udp.h"

#include "common/cl_util.h"
#include "common/clal.h"

#include "osal_log.h"

#include <stdio.h>
#include <string.h>

int cl_slmp_udp_send (
   int * socket,
   bool open_the_socket,
   uint8_t * buffer,
   size_t buffer_size,
   cl_ipaddr_t local_ip,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port,
   size_t message_size)
{
   ssize_t sent_size = 0;
#if LOG_DEBUG_ENABLED(CL_SLMP_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
#endif

   if (message_size > buffer_size)
   {
      return -1;
   }

   if (open_the_socket)
   {
#if LOG_DEBUG_ENABLED(CL_SLMP_LOG)
      cl_util_ip_to_string (local_ip, ip_string);
      LOG_DEBUG (
         CL_SLMP_LOG,
         "SLMP(%d): Opening separate socket for sending SLMP. Binding to IP "
         "address %s port %u\n",
         __LINE__,
         ip_string,
         CL_SLMP_PORT);
#endif
#ifndef FUZZ_TEST
      *socket = clal_udp_open (local_ip, CL_SLMP_PORT);
#endif
   }
   if (*socket == -1)
   {
      LOG_ERROR (
         CL_SLMP_LOG,
         "SLMP(%d): The socket for sending SLMP messages is not open.\n ",
         __LINE__);
      return -1;
   }

#if LOG_DEBUG_ENABLED(CL_SLMP_LOG)
   cl_util_ip_to_string (remote_ip, ip_string);
   LOG_DEBUG (
      CL_SLMP_LOG,
      "SLMP(%d): Sending %u bytes SLMP to IP address %s port %u on socket %d\n",
      __LINE__,
      (unsigned)message_size,
      ip_string,
      remote_port,
      *socket);
#endif
   sent_size =
      clal_udp_sendto (*socket, remote_ip, remote_port, buffer, message_size);

   if (open_the_socket)
   {
      LOG_DEBUG (
         CL_SLMP_LOG,
         "SLMP(%d): Closing SLMP send socket %d (after sending %d bytes).\n",
         __LINE__,
         *socket,
         (int)sent_size);
      clal_udp_close (*socket);
      *socket = -1;
   }

   if (sent_size < 0 || (size_t)sent_size != message_size)
   {
      return -1;
   }

   return 0;
}
