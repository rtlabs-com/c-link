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
 * @brief Implement utilities
 *
 * Intended to be useful for both master and slave.
 * The functions in this file should not have knowledge on master or slave
 * states or configuration structs.
 *
 * It is preferred that any logging is done when calling these functions
 * (not in this file, if avoidable).
 *
 * No mocking should be necessary for testing these functions.
 */

#include "cl_util.h"

#include "cl_version.h"
#include "common/cl_types.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>

const char * cl_version (void)
{
   return CL_VERSION;
}

void cl_util_copy_mac (cl_macaddr_t * dest, const cl_macaddr_t * src)
{
   uint8_t * byte_dest   = (uint8_t *)dest;
   uint8_t * byte_source = (uint8_t *)src;
   byte_dest[0]          = byte_source[0];
   byte_dest[1]          = byte_source[1];
   byte_dest[2]          = byte_source[2];
   byte_dest[3]          = byte_source[3];
   byte_dest[4]          = byte_source[4];
   byte_dest[5]          = byte_source[5];
}

void cl_util_copy_mac_reverse (cl_macaddr_t * dest, const cl_macaddr_t * src)
{
   uint8_t * byte_dest   = (uint8_t *)dest;
   uint8_t * byte_source = (uint8_t *)src;
   byte_dest[0]          = byte_source[5];
   byte_dest[1]          = byte_source[4];
   byte_dest[2]          = byte_source[3];
   byte_dest[3]          = byte_source[2];
   byte_dest[4]          = byte_source[1];
   byte_dest[5]          = byte_source[0];
}

cl_ipaddr_t cl_util_calc_broadcast_address (cl_ipaddr_t netmask, cl_ipaddr_t ip_addr)
{
   if (ip_addr == CL_IPADDR_INVALID)
   {
      return CL_IPADDR_INVALID;
   }

   /* For algorithm see https://en.wikipedia.org/wiki/Broadcast_address */
   return ip_addr | (~netmask);
}

void cl_util_ip_to_string (cl_ipaddr_t ip, char * outputstring)
{
   (void)clal_snprintf (
      outputstring,
      CL_INET_ADDRSTR_SIZE,
      "%u.%u.%u.%u",
      (uint8_t)((ip >> 24U) & UINT8_MAX),
      (uint8_t)((ip >> 16U) & UINT8_MAX),
      (uint8_t)((ip >> 8U) & UINT8_MAX),
      (uint8_t)(ip & UINT8_MAX));
}

bool cl_utils_is_netmask_valid (cl_ipaddr_t netmask)
{
   if (!(netmask & (~netmask >> 1U)))
   {
      return true;
   }

   return false;
}

bool cl_utils_is_ipaddr_range_valid (cl_ipaddr_t ip_addr)
{
   if (ip_addr == CL_IPADDR_INVALID)
   {
      return false;
   }

   /* Allowed range 0.0.0.1 to 223.255.255.254 */
   if (ip_addr > 0xDFFFFFFE)
   {
      return false;
   }

   return true;
}

void cl_util_buffer_show (const uint8_t * data, int size, int indent_size)
{
   int i;
   int j;
   uint8_t c;

   for (i = 0; i < size; i += 16)
   {
      printf ("%*s%04" PRIX32 ": ", indent_size, "", (uint32_t)i);

      /* Print hex values */
      for (j = 0; j < 16 && (i + j) < size; j++)
      {
         printf ("%02X ", data[i + j]);
      }

      /* Pad short line */
      for (; j < 16; j++)
      {
         printf ("   ");
      }

      printf ("|");

      /* Print ASCII values */
      for (j = 0; j < 16 && (i + j) < size; j++)
      {
         c = data[i + j];
         c = (isprint (c)) ? c : '.';
         printf ("%c", c);
      }

      printf ("|\n");
   }
}
