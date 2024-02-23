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

#ifndef CL_UTIL_H
#define CL_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/cl_types.h"

/**
 * Calculate UDP broadcast address
 *
 * @param netmask          Netmask
 * @param ip_addr          IP address
 * @return Resulting broadcast address
 *
 * If the IP address is invalid (0.0.0.0), the resulting broadcast address will
 * also be invalid (0.0.0.0)
 */
cl_ipaddr_t cl_util_calc_broadcast_address (
   cl_ipaddr_t netmask,
   cl_ipaddr_t ip_addr);

/**
 * Convert IP address to string
 *
 * @param ip               IP address
 * @param outputstring     Resulting string buffer. Should have size
 *                         CL_INET_ADDRSTR_SIZE.
 */
void cl_util_ip_to_string (cl_ipaddr_t ip, char * outputstring);

/**
 * Copy a MAC address
 *
 * @param dest       Destination
 * @param src        Source
 */
void cl_util_copy_mac (cl_macaddr_t * dest, const cl_macaddr_t * src);

/**
 * Copy a MAC address in reverse byte order
 *
 * @param dest       Destination
 * @param src        Source
 */
void cl_util_copy_mac_reverse (cl_macaddr_t * dest, const cl_macaddr_t * src);

/**
 * Validate netmask
 *
 * @param netmask          Netmask to be validated
 * @return true if the netmask is valid, and false if invalid
 */
bool cl_utils_is_netmask_valid (cl_ipaddr_t netmask);

/**
 * Validate that the IP address is in the correct range
 *
 * Allowed range is 0.0.0.1 to 223.255.255.254
 *
 * @param ip_addr          IP address to be validated
 * @return true if the IP address is in correct range, and false if not
 *
 * @req REQ_CL_PROTOCOL_19
 * @req REQ_CL_PROTOCOL_36
 *
 */
bool cl_utils_is_ipaddr_range_valid (cl_ipaddr_t ip_addr);

/**
 * Display buffer contents
 *
 * For example:
 *
 *    uint8_t mybuffer[18] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
 *                            'J', 'K', 'L',   0,   1,   2,   3,   4,   5};
 *    cl_util_buffer_show (mybuffer, 18);
 *
 * will be displayed as:
 *
 *    0000: 41 42 43 44 45 46 47 48 49 4a 4b 4c 00 01 02 03 |ABCDEFGHIJKL....|
 *    0010: 04 05                                           |..|
 *
 * @param data          Buffer contents to be displayed
 * @param size          Buffer size (or smaller, to display only the beginning)
 * @param indent_size   Indentation of resulting text
 */
void cl_util_buffer_show (const uint8_t * data, int size, int indent_size);

#ifdef __cplusplus
}
#endif

#endif /* CL_UTIL_H */
