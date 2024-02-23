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

#ifndef CLAL_H
#define CLAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "clal_sys.h"

#include <stdint.h>

/** Incl termination. The interface name is used for logs, and
 *  will be truncated to fit this size.
 *  On Linux the size is IF_NAMESIZE. On Windows the
 *  "Description" field holds the human readable name, but
 *  the string is often rather long. Use a value to hold most
 *  common interface names. */
#define CLAL_IFNAME_SIZE 50

/**
 * Initialise c-link abstraction layer
 *
 * @return 0, or -1 if an error occurred.
 */
int clal_init (void);

/**
 * Close down c-link abstraction layer
 *
 * You need to manually close any open UDP sockets before calling this
 * function.
 *
 * @return 0, or -1 if an error occurred.
 */
int clal_exit (void);

/**
 * Clear memory
 *
 * Will trigger an assertion if the pointer is NULL.
 *
 * @param dest          Destination
 * @param n             Number of bytes to clear
 */
void clal_clear_memory (void * dest, size_t n);

/**
 * Copy memory
 *
 * Will trigger an assertion if any of the pointers is NULL or if n > dest_size.
 *
 * @param dest          Destination
 * @param dest_size     Buffer size of destination
 * @param src           Source
 * @param n             Number of bytes to copy
 */
void clal_memcpy (void * dest, size_t dest_size, const void * src, size_t n);

/**
 * Copy a string
 *
 * The resulting string is guaranteed to be NULL terminated.
 * This function will not write more than \a dest_size bytes,
 * and copies at most dest_size - 1 characters.
 *
 * The content of the destination string after the NULL termination
 * might have arbitrary values.
 *
 * Will trigger an assertion if any of the pointers is NULL.
 *
 * @param dest          Destination
 * @param src           Source (a NULL terminated string)
 * @param dest_size     Buffer size of destination
 * @return 0 on success, -1 on truncation
 */
int clal_copy_string (char * dest, const char * src, size_t dest_size);

/**
 * Format a string
 *
 * The resulting string is guaranteed to be NULL terminated.
 * Reports an error if any of the pointers are NULL,
 * if \a dest_size is 0, if the destination string will be
 * truncated or on encoding errors.
 *
 * The content of the destination string after the NULL termination
 * might have arbitrary values.
 *
 * Note that the return value is different from some implementations
 * of \a snprintf().
 *
 * @param dest          Destination
 * @param dest_size     Buffer size of destination
 * @param format        Format specification string. Must be NULL terminated.
 * @param ...           Variable arguments
 * @return The number of characters (not counting the NULL terminator) that was
 *         written to \a dest, or -1 on error.
 */
int clal_snprintf (char * dest, size_t dest_size, const char * format, ...);

/**
 * Read current time as UNIX timestamp, with milliseconds
 *
 * @return Timestamp or 0 if not implemented
 *
 * Only used for CC-Link master stack.
 *
 * For example a return value of 1640000000000 corresponds to
 * Dec 20 2021 11:33:20 GMT+0000
 */
uint64_t clal_get_unix_timestamp_ms();

/**
 * Get the interface index from the operating system for a given IP address
 *
 * A valid interface index has value 1 or larger.
 *
 * For example:
 *
 * IP address        Represented by
 * 1.0.0.0           0x01000000 = 16777216
 * 0.0.0.1           0x00000001 = 1
 *
 * @param ip_address       IP address (IPv4)
 * @param ifindex          Resulting Network interface index
 * @return 0 on success and -1 if no interface was found
 */
int clal_get_ifindex (uint32_t ip_address, int * ifindex);

/**
 * Read the netmask as an integer. For IPv4.
 *
 * @param ifindex          Network interface interface
 * @param netmask          Resulting netmask
 * @return 0 on success, -1 on error
 * Returns -1 when no link is available.
 */
int clal_get_netmask (int ifindex, uint32_t * netmask);

/**
 * Set the IP address and netmask. For IPv4.
 *
 * For example:
 *
 * IP address        Represented by
 * 1.0.0.0           0x01000000 = 16777216
 * 0.0.0.1           0x00000001 = 1
 *
 * Netmask           Represented by
 * 255.255.255.0     0xFFFFFF00 = 4294967040
 *
 * Only used for CC-Link slave stack.
 *
 * @param ifindex          Network interface index
 * @param ip_address       IP address
 * @param netmask          Netmask
 * @return 0 on success and -1 on error
 */
int clal_set_ip_address_netmask (int ifindex, uint32_t ip_address, uint32_t netmask);

/**
 * Get the network interface name from the operating system
 *
 * Intended for diagnostic log output.
 * The resulting string will be truncated to fit in CLAL_IFNAME_SIZE.
 *
 * It is optional to implement this function. (Return -1
 * if not implemented).
 *
 * @param ifindex          Network interface index
 * @param ifname           Resulting name. Should be a buffer
 *                         with size CLAL_IFNAME_SIZE.
 * @return 0 on success and -1 on error or not implemented
 */
int clal_get_ifname (int ifindex, char * ifname);

/**
 * Get the MAC address from the operating system
 *
 * @param ifindex          Network interface index
 * @param mac_address      Resulting MAC address
 * @return 0 on success and -1 on error
 */
int clal_get_mac_address (int ifindex, uint8_t (*mac_address)[6]);

/**
 * Open an UDP socket
 *
 * It should enable reuseaddr, broadcast and packetinfo
 *
 * @param ip               IP address to listen on
 * @param port             UDP port to listen on
 * @return socket handle, or -1 if an error occurred.
 */
int clal_udp_open (uint32_t ip, uint16_t port);

/**
 * Send UDP data
 *
 * @param handle           Socket handle
 * @param ip               Destination IP address
 * @param port             Destination UDP port
 * @param data             Data to be sent
 * @param size             Size of data
 * @return the number of bytes sent, or -1 if an error occurred.
 */
ssize_t clal_udp_sendto (
   int handle,
   uint32_t ip,
   uint16_t port,
   const void * data,
   size_t size);

/**
 * Receive UDP data.
 *
 * This is a nonblocking function, and it returns 0 immediately if no
 * data is available.
 *
 * Only used for CC-Link master stack.
 *
 * @param handle           Socket handle
 * @param remote_ip        Resulting source (remote) IP address
 * @param remote_port      Resulting source (remote) UDP port
 * @param data             Resulting received data
 * @param size             Size of buffer for received data
 * @return the number of bytes received, or -1 if an error occurred.
 */
ssize_t clal_udp_recvfrom (
   int handle,
   uint32_t * remote_ip,
   uint16_t * remote_port,
   void * data,
   size_t size);

/**
 * Receive UDP data, and read interface index and own IP address.
 *
 * This is a nonblocking function, and it returns 0 immediately if no
 * data is available.
 *
 * @param handle           Socket handle
 * @param remote_ip        Resulting source (remote) IP address
 * @param remote_port      Resulting source (remote) UDP port
 * @param local_ip         Resulting local (own) IP address
 * @param ifindex          Resulting network interface index
 * @param data             Resulting received data
 * @param size             Size of buffer for received data
 * @return the number of bytes received, or -1 if an error occurred.
 */
ssize_t clal_udp_recvfrom_with_ifindex (
   int handle,
   uint32_t * remote_ip,
   uint16_t * remote_port,
   uint32_t * local_ip,
   int * ifindex,
   void * data,
   size_t size);

/**
 * Close an UDP socket
 *
 * @param id               Socket handle
 */
void clal_udp_close (int handle);

/**
 * Load a binary file.
 *
 * Can load the data into two buffers.
 *
 * Will report an error if size_1 + size_2 bytes are not available.
 *
 * @param fullpath         Full path to the file
 * @param object_1         Data to load, or NULL. Mandatory if size_1 > 0
 * @param size_1           Size of object_1.
 * @param object_2         Data to load, or NULL. Mandatory if size_2 > 0
 * @param size_2           Size of object_2.
 * @return  0  if the operation succeeded.
 *          -1 if not found or an error occurred.
 */
int clal_load_file (
   const char * fullpath,
   void * object_1,
   size_t size_1,
   void * object_2,
   size_t size_2);

/**
 * Save a binary file.
 *
 * Can handle two output buffers.
 *
 * @param fullpath         Full path to the file
 * @param object_1         Data to save, or NULL. Mandatory if size_1 > 0
 * @param size_1           Size of object_1.
 * @param object_2         Data to save, or NULL. Mandatory if size_2 > 0
 * @param size_2           Size of object_2.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int clal_save_file (
   const char * fullpath,
   const void * object_1,
   size_t size_1,
   const void * object_2,
   size_t size_2);

/**
 * Clear a binary file.
 *
 * @param fullpath         Full path to the file
 */
void clal_clear_file (const char * fullpath);

#ifdef __cplusplus
}
#endif

#endif /* CLAL_H */
