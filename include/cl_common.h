/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2022 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#ifndef CL_COMMON_H
#define CL_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cl_export.h"
#include "cl_options.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Max filename size, including termination  */
#define CL_MAX_FILENAME_SIZE 30

/** Including separator and one termination */
#define CL_MAX_FILE_FULLPATH_SIZE                                              \
   (CL_MAX_DIRECTORYPATH_SIZE + CL_MAX_FILENAME_SIZE)

/** IP address as an uint32_t
 *
 * IP address        Represented by
 * 1.0.0.0           0x01000000 = 16777216
 * 0.0.0.1           0x00000001 = 1
 *
 * This is a.b.c.d
 */
typedef uint32_t cl_ipaddr_t;

typedef uint8_t cl_macaddr_t[6];

#define CL_IPADDR_ANY             0x00000000UL /** 0.0.0.0 */
#define CL_IPADDR_LOCAL_BROADCAST 0xFFFFFFFFUL /** 255.255.255.255 */

/** Slave application is running or stopped.
    This enum is used by both slave and master.  */
typedef enum cl_slave_appl_operation_status
{
   CL_SLAVE_APPL_OPERATION_STATUS_STOPPED   = 0,
   CL_SLAVE_APPL_OPERATION_STATUS_OPERATING = 1,
} cl_slave_appl_operation_status_t;

/** Number of words (16-bit registers) in one RWr or RWw area */
#define CL_WORDSIGNALS_PER_AREA 32

/** Number of bytes required to store one RX or RY area */
#define CL_BYTES_PER_BITAREA 8

/** Number of bits in one RX or RY area */
#define CL_BITSIGNALS_PER_AREA (CL_BYTES_PER_BITAREA * 8)

/** RWw data area with 32 words (each 16 bits) sent from master to slave.
    The slave has \a num_occupied_stations of these areas.
    See REQ_CLM_CAPACITY_02 and REQ_CLS_CAPACITY_02. */
typedef struct cl_rww
{
   /** Words */
   uint16_t words[CL_WORDSIGNALS_PER_AREA];
} cl_rww_t;

/** RY data area with 64 individual bits sent from master to slave.
    The slave has \a num_occupied_stations of these areas.
    See REQ_CLM_CAPACITY_04 and REQ_CLS_CAPACITY_04. */
typedef struct cl_ry
{
   /** Bits */
   uint8_t bytes[CL_BYTES_PER_BITAREA];
} cl_ry_t;

/** RWr data area with 32 words (each 16 bits) sent from slave to master.
    The slave has \a num_occupied_stations of these areas.
    See REQ_CLM_CAPACITY_01 and REQ_CLS_CAPACITY_01. */
typedef struct cl_rwr
{
   /** Words */
   uint16_t words[CL_WORDSIGNALS_PER_AREA];
} cl_rwr_t;

/** RX data area with 64 individual bits sent from slave to master.
    The slave has \a num_occupied_stations of these areas.
    See REQ_CLM_CAPACITY_03 and REQ_CLS_CAPACITY_03. */

typedef struct cl_rx
{
   /** Bits */
   uint8_t bytes[CL_BYTES_PER_BITAREA];
} cl_rx_t;

/**
 * Get c-link stack version
 *
 * @return version of c-link stack, as a null terminated string.
 */
CL_EXPORT const char * cl_version (void);

#ifdef __cplusplus
}
#endif

#endif /* CL_COMMON_H */
