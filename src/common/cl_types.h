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

#ifndef CL_TYPES_H
#define CL_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cl_options.h"
#include "clm_api.h"
#include "cls_api.h"
#include "common/cl_limiter.h"
#include "common/cl_timer.h"
#include "common/clal.h"

#include "osal.h"

#include <stdint.h>

#define CL_CCIEFB_PORT                          61450
#define CL_SLMP_PORT                            61451
#define CL_CCIEFB_MIN_SUPPORTED_PROTOCOL_VER    1
#define CL_CCIEFB_MAX_SUPPORTED_PROTOCOL_VER    2
#define CL_CCIEFB_MIN_TIMEOUT                   1    /** millisecond */
#define CL_CCIEFB_DEFAULT_TIMEOUT               500  /** milliseconds */
#define CL_CCIEFB_MAX_TIMEOUT_CONSTANT_LINKSCAN 2000 /** milliseconds */
#define CL_CCIEFB_MIN_TIMEOUT_COUNT             1
#define CL_CCIEFB_DEFAULT_TIMEOUT_COUNT         3

/* UDP payload buffer size */
#define CL_BUFFER_LEN 1500

#define CL_INET_ADDRSTR_SIZE 16 /** Incl termination */
#define CL_ETH_ADDRSTR_SIZE  18 /** Incl termination */

#define CL_MAKEU32(a, b, c, d)                                                 \
   (((uint32_t)((a)&0xff) << 24) | ((uint32_t)((b)&0xff) << 16) |              \
    ((uint32_t)((c)&0xff) << 8) | (uint32_t)((d)&0xff))

#define CL_IPADDR_INVALID 0x00000000UL /** 0.0.0.0 */
#define CL_ADDRSIZE_IPV4  4

/** BAP-C2006-ENG-001-J section 7.5
 *
 * Note that for CCIEFB are only CL_SLMP_ENDCODE_SUCCESS and
 * the CL_SLMP_ENDCODE_CCIEFB_* end codes allowed.
 *
 */
typedef enum cl_slmp_error_codes
{
   CL_SLMP_ENDCODE_SUCCESS                               = 0x0000,
   CL_SLMP_ENDCODE_COMMAND_ERROR                         = 0xC059,
   CL_SLMP_ENDCODE_COMMAND_REQUEST_MSG                   = 0xC05C,
   CL_SLMP_ENDCODE_REQUEST_DATA_LENGTH_MISMATCH          = 0xC61C,
   CL_SLMP_ENDCODE_CAN_WRONG_CONDITION                   = 0xCCC7,
   CL_SLMP_ENDCODE_CAN_WRITE_ONLY                        = 0xCCC8,
   CL_SLMP_ENDCODE_CAN_READ_ONLY                         = 0xCCC9,
   CL_SLMP_ENDCODE_CAN_OBJECT_NOT_DEFINED                = 0xCCCA,
   CL_SLMP_ENDCODE_CAN_PDO_MAPPING_NOT_ALLOWED           = 0xCCCB,
   CL_SLMP_ENDCODE_CAN_PDO_DATA_LENGTH_MISMATCH          = 0xCCCC,
   CL_SLMP_ENDCODE_CAN_DATAVALUENUM_MISMATCH             = 0xCCD0,
   CL_SLMP_ENDCODE_CAN_DATAVALUENUM_TOO_LARGE            = 0xCCD1,
   CL_SLMP_ENDCODE_CAN_DATAVALUENUM_TOO_SMALL            = 0xCCD2,
   CL_SLMP_ENDCODE_CAN_SUBINDEX_DOES_NOT_EXIST           = 0xCCD3,
   CL_SLMP_ENDCODE_CAN_INVALID_PARAMETER                 = 0xCCD4,
   CL_SLMP_ENDCODE_CAN_VALUE_TOO_LARGE                   = 0xCCD5,
   CL_SLMP_ENDCODE_CAN_VALUE_TOO_SMALL                   = 0xCCD6,
   CL_SLMP_ENDCODE_CAN_STORING_TRANSMITTING_IMPOSSIBLE   = 0xCCDA,
   CL_SLMP_ENDCODE_CAN_OTHER                             = 0xCCFF,
   CL_SLMP_ENDCODE_REQUEST_BUSY                          = 0xCEE0,
   CL_SLMP_ENDCODE_REQUEST_TOO_LARGE                     = 0xCEE1,
   CL_SLMP_ENDCODE_RESPONSE_TOO_LARGE                    = 0xCEE2,
   CL_SLMP_ENDCODE_GATEWAY_ERROR                         = 0xCF00,
   CL_SLMP_ENDCODE_SERVER_INFO_DOES_NOT_EXIST            = 0xCF10,
   CL_SLMP_ENDCODE_CAN_NOT_BE_SET                        = 0xCF20,
   CL_SLMP_ENDCODE_PARAMETER_DOES_NOT_EXIST              = 0xCF30,
   CL_SLMP_ENDCODE_PARAMETER_WRITING_WRONG_STATE         = 0xCF31,
   CL_SLMP_ENDCODE_DIVIDED_MESSAGE_TIMEOUT               = 0xCF40,
   CL_SLMP_ENDCODE_DIVIDED_MESSAGE_DUPLICATE             = 0xCF41,
   CL_SLMP_ENDCODE_DIVIDED_MESSAGE_DATA_ERROR            = 0xCF42,
   CL_SLMP_ENDCODE_DIVIDED_MESSAGE_LOST                  = 0xCF43,
   CL_SLMP_ENDCODE_DIVIDED_MESSAGE_NOT_SUPPORTED         = 0xCF44,
   CL_SLMP_ENDCODE_COMMUNICATION_RELAY_ERROR             = 0xCF70,
   CL_SLMP_ENDCODE_COMMUNICATION_TIMEOUT                 = 0xCF71,
   CL_SLMP_ENDCODE_CCIEFB_MASTER_DUPLICATION             = 0xCFE0,
   CL_SLMP_ENDCODE_CCIEFB_WRONG_NUMBER_OCCUPIED_STATIONS = 0xCFE1,
   CL_SLMP_ENDCODE_CCIEFB_SLAVE_ERROR                    = 0xCFF0,
   CL_SLMP_ENDCODE_CCIEFB_SLAVE_REQUESTS_DISCONNECT      = 0xCFFF,
} cl_slmp_error_codes_t;

/** BAP-C2006-ENG-003-J section 8.3.3.3 */
typedef enum cl_slmp_dest_proc_no
{
   CL_SLMP_DSTPROCNO_CONTROL          = 0x03D0,
   CL_SLMP_DSTPROCNO_STANDBY          = 0x03D1,
   CL_SLMP_DSTPROCNO_SYSTEM_A         = 0x03D2,
   CL_SLMP_DSTPROCNO_SYSTEM_B         = 0x03D3,
   CL_SLMP_DSTPROCNO_MULTIPROCESSOR_1 = 0x03E0,
   CL_SLMP_DSTPROCNO_MULTIPROCESSOR_2 = 0x03E1,
   CL_SLMP_DSTPROCNO_MULTIPROCESSOR_3 = 0x03E2,
   CL_SLMP_DSTPROCNO_MULTIPROCESSOR_4 = 0x03E3,
   CL_SLMP_DSTPROCNO_DEFAULT          = 0x03FF,
} cl_slmp_dest_proc_no_t;

/** BAP-C2006-ENG-003-J section 8.3.6.3, 8.3.10.3 and 8.3.16.3,*/
typedef enum cl_slmp_command
{
   CL_SLMP_COMMAND_REMOTE_TYPENAME_READ             = 0x0101,
   CL_SLMP_COMMAND_FILE_FILEINFO_READ               = 0x0201,
   CL_SLMP_COMMAND_FILE_FILEINFO_DETAIL             = 0x0202,
   CL_SLMP_COMMAND_FILE_SEARCH                      = 0x0203,
   CL_SLMP_COMMAND_FILE_FILEINFO_NO_USAGE           = 0x0204,
   CL_SLMP_COMMAND_DRIVE_STATE_READ                 = 0x0205,
   CL_SLMP_COMMAND_FILE_READ                        = 0x0206,
   CL_SLMP_COMMAND_DEVICE_READ                      = 0x0401,
   CL_SLMP_COMMAND_DEVICE_RANDOM_READ               = 0x0403,
   CL_SLMP_COMMAND_DEVICE_BLOCK_READ                = 0x0406,
   CL_SLMP_COMMAND_DEVICE_ARRAYLABEL_READ           = 0x041A,
   CL_SLMP_COMMAND_DEVICE_LABEL_RANDOM_READ         = 0x041C,
   CL_SLMP_COMMAND_EXTENDUNIT_READ                  = 0x0601,
   CL_SLMP_COMMAND_MEMORY_READ                      = 0x0613,
   CL_SLMP_COMMAND_SELF_TEST                        = 0x0619,
   CL_SLMP_COMMAND_DEVICE_MONITOR_ENTRY             = 0x0801,
   CL_SLMP_COMMAND_DEVICE_MONITOR_EXECUTE           = 0x0802,
   CL_SLMP_COMMAND_FILE_FILELOCK                    = 0x0808,
   CL_SLMP_COMMAND_DEVICE_IDENTIFICATION_GET        = 0x0E28,
   CL_SLMP_COMMAND_NODE_DATA_MONIORING              = 0x0E29,
   CL_SLMP_COMMAND_NODE_SEARCH                      = 0x0E30,
   CL_SLMP_COMMAND_NODE_IPADDRESS_SET               = 0x0E31,
   CL_SLMP_COMMAND_DEVICE_INFO_COMPARE              = 0x0E32,
   CL_SLMP_COMMAND_PARAMETER_GET                    = 0x0E33,
   CL_SLMP_COMMAND_PARAMETER_SET                    = 0x0E34,
   CL_SLMP_COMMAND_PARAMETER_SET_START              = 0x0E35,
   CL_SLMP_COMMAND_PARAMETER_SET_END                = 0x0E36,
   CL_SLMP_COMMAND_PARAMETER_SET_CANCEL             = 0x0E3A,
   CL_SLMP_COMMAND_NODE_STATUS_READ                 = 0x0E44,
   CL_SLMP_COMMAND_NODE_COMMUNICATION_SETTING_GET   = 0x0E45,
   CL_SLMP_COMMAND_NODE_STATUS_READ2                = 0x0E53,
   CL_SLMP_COMMAND_CCIEFB_CYCLIC                    = 0x0E70,
   CL_SLMP_COMMAND_NETWORK_CONFIG                   = 0x0E90,
   CL_SLMP_COMMAND_CCLIETSN_MASTER_CONFIG           = 0x0E91,
   CL_SLMP_COMMAND_CCLIETSN_SLAVE_CONFIG            = 0x0E92,
   CL_SLMP_COMMAND_CCLIETSN_CYCLIC_CONFIG           = 0x0E93,
   CL_SLMP_COMMAND_CCLIETSN_NOTIFICATION            = 0x0E94,
   CL_SLMP_COMMAND_BACKUP_COMMUNICATION_GET         = 0x0EB0,
   CL_SLMP_COMMAND_BACKUP_SUBIDLIST_GET             = 0x0EB1,
   CL_SLMP_COMMAND_BACKUP_DEVICEINFO_GET            = 0x0EB2,
   CL_SLMP_COMMAND_BACKUP_START                     = 0x0EB3,
   CL_SLMP_COMMAND_BACKUP_END                       = 0x0EB4,
   CL_SLMP_COMMAND_BACKUP_REQUEST                   = 0x0EB5,
   CL_SLMP_COMMAND_BACKUP_PRM_GET                   = 0x0EB6,
   CL_SLMP_COMMAND_BACKUP_RESTORE_CHECK             = 0x0EB7,
   CL_SLMP_COMMAND_BACKUP_RESTORE_START             = 0x0EB8,
   CL_SLMP_COMMAND_BACKUP_RESTORE_END               = 0x0EB9,
   CL_SLMP_COMMAND_BACKUP_PRM_SET                   = 0x0EBA,
   CL_SLMP_COMMAND_SLAVE_PRM_CHECK_DELIVERY         = 0x0EBE,
   CL_SLMP_COMMAND_REMOTE_RUN                       = 0x1001,
   CL_SLMP_COMMAND_REMOTE_STOP                      = 0x1002,
   CL_SLMP_COMMAND_REMOTE_PAUSE                     = 0x1003,
   CL_SLMP_COMMAND_REMOTE_LATCH_CLEAR               = 0x1005,
   CL_SLMP_COMMAND_REMOTE_RESET                     = 0x1006,
   CL_SLMP_COMMAND_FILE_NEW_TYPEA                   = 0x1202,
   CL_SLMP_COMMAND_FILE_WRITE                       = 0x1203,
   CL_SLMP_COMMAND_FILE_FILEINFO_CHANGE             = 0x1204,
   CL_SLMP_COMMAND_FILE_DELETE_TYPEA                = 0x1205,
   CL_SLMP_COMMAND_FILE_COPY_TYPEA                  = 0x1206,
   CL_SLMP_COMMAND_DRIVE_DEFRAG                     = 0x1207,
   CL_SLMP_COMMAND_DEVICE_WRITE                     = 0x1401,
   CL_SLMP_COMMAND_DEVICE_RANDOM_WRITE              = 0x1402,
   CL_SLMP_COMMAND_DEVICE_BLOCK_WRITE               = 0x1406,
   CL_SLMP_COMMAND_DEVICE_ARRAYLABEL_WRITE          = 0x141A,
   CL_SLMP_COMMAND_DEVICE_LABEL_RANDOM_WRITE        = 0x141B,
   CL_SLMP_COMMAND_EXTENDUNIT_WRITE                 = 0x1601,
   CL_SLMP_COMMAND_MEMORY_WRITE                     = 0x1613,
   CL_SLMP_COMMAND_CLEAR_ERROR                      = 0x1617,
   CL_SLMP_COMMAND_CLEAR_ERROR_HISTORY              = 0x1619,
   CL_SLMP_COMMAND_EVENT_HISTORY_CLEAR              = 0x161A,
   CL_SLMP_COMMAND_PASSWORD_UNLOCK                  = 0x1630,
   CL_SLMP_COMMAND_PASSWORD_LOCK                    = 0x1631,
   CL_SLMP_COMMAND_FILE_READ_DIRECTORY_FILE         = 0x1810,
   CL_SLMP_COMMAND_FILE_SEARCH_DIRECTORY_FILE       = 0x1811,
   CL_SLMP_COMMAND_FILE_NEW_TYPEB                   = 0x1820,
   CL_SLMP_COMMAND_FILE_DELETE_TYPEB                = 0x1822,
   CL_SLMP_COMMAND_FILE_COPY_TYPEB                  = 0x1824,
   CL_SLMP_COMMAND_FILE_CHANGE_STATE                = 0x1825,
   CL_SLMP_COMMAND_FILE_CHANGE_DATE                 = 0x1826,
   CL_SLMP_COMMAND_FILE_OPEN                        = 0x1827,
   CL_SLMP_COMMAND_FILE_CLOSE                       = 0x182A,
   CL_SLMP_COMMAND_ON_DEMAND                        = 0x2101,
   CL_SLMP_COMMAND_CCLIEF_COMMUNICATION_TEST        = 0x3040,
   CL_SLMP_COMMAND_CCLIEF_CABLE_TEST                = 0x3050,
   CL_SLMP_COMMAND_EVENT_HISTORY_GET_NUM            = 0x3060,
   CL_SLMP_COMMAND_EVENT_HISTORY_GET                = 0x3061,
   CL_SLMP_COMMAND_EVENT_HISTORY_CLOCK_OFFESET_SEND = 0x3062,
   CL_SLMP_COMMAND_REMOTE_CONTROL                   = 0x3070,
   CL_SLMP_COMMAND_COMMUNICATION_SPEED              = 0x3072,
   CL_SLMP_COMMAND_CCLIEF_NODEINFO_GET              = 0x3119,
   CL_SLMP_COMMAND_CYCLIC_OWN_STOP                  = 0x3206,
   CL_SLMP_COMMAND_CYCLIC_OWN_START                 = 0x3207,
   CL_SLMP_COMMAND_CYCLIC_OTHER_STOP                = 0x3208,
   CL_SLMP_COMMAND_CYCLIC_OTHER_START               = 0x3209,
   CL_SLMP_COMMAND_LINKDEVICE_PARAMETER_WRITE       = 0x320A,
   CL_SLMP_COMMAND_LINKDEVICE_PARAMETER_CHECK_REQ   = 0x320B,
   CL_SLMP_COMMAND_LINKDEVICE_PARAMETER_CHECK_RESP  = 0x320C,
   CL_SLMP_COMMAND_RESERVED_STATION_RELEASE         = 0x320D,
   CL_SLMP_COMMAND_RESERVED_STATION_CONFIG          = 0x320E,
   CL_SLMP_COMMAND_WATCHDOG_COUNTER_SET             = 0x3210,
   CL_SLMP_COMMAND_WATCHDOG_COUNTER_OFFSET_CONFIG   = 0x3211,
   CL_SLMP_COMMAND_NODEINFO_ACCEPT                  = 0x3230,
   CL_SLMP_COMMAND_CAN_OBJECT_ACCESS                = 0x4020,
   CL_SLMP_COMMAND_DATA_COLLECTION                  = 0xA445,
} cl_slmp_command_t;

#define CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC      0x0000
#define CL_SLMP_SUBCOMMAND_NODE_SEARCH        0x0000
#define CL_SLMP_SUBCOMMAND_NODE_IPADDRESS_SET 0x0000

/** BAP-C2006-ENG-003-J section 8.3.6.5.24.2 */
typedef enum cl_slmp_drive_name
{
   CL_SLMP_DRIVE_NAME_INTERNAL_RAM           = 0x0000,
   CL_SLMP_DRIVE_NAME_MEMORY_CARD_A_RAM      = 0x0001,
   CL_SLMP_DRIVE_NAME_MEMORY_CARD_A_ROM      = 0x0002,
   CL_SLMP_DRIVE_NAME_MEMORY_CARD_B_RAM      = 0x0003,
   CL_SLMP_DRIVE_NAME_MEMORY_CARD_B_ROM      = 0x0004,
   CL_SLMP_DRIVE_WITH_CURRENT_PARAMETER_FILE = 0x00F,
} cl_slmp_drive_name_t;

/** See also cl_literals_get_slave_event() */
typedef enum cls_slave_event
{
   CLS_SLAVE_EVENT_NONE,
   CLS_SLAVE_EVENT_STARTUP,
   CLS_SLAVE_EVENT_CYCLIC_NEW_MASTER,
   CLS_SLAVE_EVENT_CYCLIC_CORRECT_MASTER,
   CLS_SLAVE_EVENT_CYCLIC_WRONG_MASTER,
   CLS_SLAVE_EVENT_CYCLIC_WRONG_STATIONCOUNT,
   CLS_SLAVE_EVENT_CYCLIC_INCOMING_WHEN_DISABLED,
   CLS_SLAVE_EVENT_TIMEOUT_MASTER,
   CLS_SLAVE_EVENT_REENABLE_SLAVE,
   CLS_SLAVE_EVENT_DISABLE_SLAVE,
   CLS_SLAVE_EVENT_DISABLE_SLAVE_WAIT_ENDED,
   CLS_SLAVE_EVENT_IP_UPDATED,

   /** Dummy event */
   CLS_SLAVE_EVENT_LAST
} cls_slave_event_t;

/** See also cl_literals_get_group_event() */
typedef enum clm_group_event
{
   /** No more event will be triggered */
   CLM_GROUP_EVENT_NONE,

   /** Startup of the group
    *  Event sent to all groups simultaneously. */
   CLM_GROUP_EVENT_STARTUP,

   /** Master parameters changed. Known as IsParamSet() in spec
    *  Event sent to all groups simultaneously. */
   CLM_GROUP_EVENT_PARAMETER_CHANGE,

   /** Configuration has been updated.
    *  Event sent to all groups simultaneously. */
   CLM_GROUP_EVENT_NEW_CONFIG,

   /** Arbitration is finished.
    *  Event sent to all groups simultaneously. */
   CLM_GROUP_EVENT_ARBITRATION_DONE,

   /** We received request from other master.
    *  Event sent to all groups simultaneously.
    *  Known as masterID!=MyMasterId */
   CLM_GROUP_EVENT_REQ_FROM_OTHER,

   /** Masterduplication alarm received from slave.
    *  Event sent to all groups simultaneously. */
   CLM_GROUP_EVENT_MASTERDUPL_ALARM,

   /** Beginning of link scan. Known as IsLinkScanStart() in spec */
   CLM_GROUP_EVENT_LINKSCAN_START,

   /** Link scan times out. Some slave has not responded */
   CLM_GROUP_EVENT_LINKSCAN_TIMEOUT,

   /** We have received response from all slaves
    *  Known as IsLinkScanComp() in spec. */
   CLM_GROUP_EVENT_LINKSCAN_COMPLETE,

   /** Dummy event */
   CLM_GROUP_EVENT_LAST
} clm_group_event_t;

/** See also cl_literals_get_device_event() */
typedef enum clm_device_event
{
   CLM_DEVICE_EVENT_NONE,

   /** Group startup
    *  Known as "Master station startup" in spec
    *  Sent to all device state machines in a group */
   CLM_DEVICE_EVENT_GROUP_STARTUP,

   /** Group returns to standby.
    *  For IsParameterSet() and IsMasterOverlap()
    *  Sent to all device state machines in a group */
   CLM_DEVICE_EVENT_GROUP_STANDBY,

   /** Not all devices in the group has responded.
    *  Known as ResWaitTimer timeout.
    *  Sent to all device state machines in a group */
   CLM_DEVICE_EVENT_GROUP_TIMEOUT,

   /** All enabled devices in the group have responded.
    *  Known as IsOtherSlaveCyclicComp()
    *  Sent to all device state machines in a group */
   CLM_DEVICE_EVENT_GROUP_ALL_RESPONDED,

   /** Received response without error.
    *  Known as IsRecvCyclicRes() with endCode==0
    *  and possibly CmpFrameSequenceNo()==TRUE */
   CLM_DEVICE_EVENT_RECEIVE_OK,

   /** Received response with error.
    *  Known as IsRecvCyclicRes() with endCode!=0 */
   CLM_DEVICE_EVENT_RECEIVE_ERROR,

   /** "Scan start" from group, this device should be running
    *  Known as IsLinkScanStart() with StartCyclicFlag==TRUE */
   CLM_DEVICE_EVENT_SCAN_START_DEVICE_START,

   /** "Scan start" from group, this device should be stopped.
    *  Known as IsLinkScanStart() with StartCyclicFlag==FALSE */
   CLM_DEVICE_EVENT_SCAN_START_DEVICE_STOP,

   /** Internal state for handling timeout counter. Full timeout counter. */
   CLM_DEVICE_EVENT_TIMEOUTCOUNTER_FULL,

   /** Internal state for handling timeout counter. Not yet full.  */
   CLM_DEVICE_EVENT_TIMEOUTCOUNTER_NOT_FULL,

   /** Slave duplication detected.
    *  Known as IsSlaveOverlap()
    *  Typically sent to a single device representation state machine. */
   CLM_DEVICE_EVENT_SLAVE_DUPLICATION,

   /** Dummy event */
   CLM_DEVICE_EVENT_LAST
} clm_device_event_t;

/************************ CCIEFB frames *********************************/

#define CL_CCIEFB_MIN_GROUP_NO                         1
#define CL_CCIEFB_MAX_GROUP_NO                         64
#define CL_CCIEFB_MIN_OCCUPIED_STATIONS_PER_GROUP      1
#define CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP      16
#define CL_CCIEFB_MAX_OCCUPIED_STATIONS_FOR_ALL_GROUPS 64

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_cciefb_req_header
{
   uint16_t reserved1;   /** Always 0x5000. Big endian */
   uint8_t reserved2;    /** Always 0x00. */
   uint8_t reserved3;    /** Always 0xFF. */
   uint16_t reserved4;   /** Always 0x03FF Little endian */
   uint8_t reserved5;    /** Always 0x00. */
   uint16_t dl;          /** Length, little endian */
   uint16_t reserved6;   /** Always 0x0000. Little endian */
   uint16_t command;     /** Little endian */
   uint16_t sub_command; /** Little endian */
} cl_cciefb_req_header_t;
CC_PACKED_END
CC_STATIC_ASSERT (sizeof (cl_cciefb_req_header_t) == 15);

/** Bytes up to and including 'dl' field */
#define CL_CCIEFB_REQ_HEADER_DL_OFFSET 9U

#define CL_CCIEFB_REQ_HEADER_RESERVED1 0x5000
#define CL_CCIEFB_REQ_HEADER_RESERVED2 0x00
#define CL_CCIEFB_REQ_HEADER_RESERVED3 0xFF
#define CL_CCIEFB_REQ_HEADER_RESERVED4 0x03FF
#define CL_CCIEFB_REQ_HEADER_RESERVED5 0x00
#define CL_CCIEFB_REQ_HEADER_RESERVED6 0x0000

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_cciefb_resp_header
{
   uint16_t reserved1; /** Always 0xD000. Big endian */
   uint8_t reserved2;  /** Always 0x00. */
   uint8_t reserved3;  /** Always 0xFF. */
   uint16_t reserved4; /** Always 0x03FF. Little endian */
   uint8_t reserved5;  /** Always 0x00. */
   uint16_t dl;        /** Length, little endian */
   uint16_t reserved6; /** Always 0x0000. Little endian */
} cl_cciefb_resp_header_t;
CC_PACKED_END
CC_STATIC_ASSERT (sizeof (cl_cciefb_resp_header_t) == 11);

/** Bytes up to and including 'dl' field */
#define CL_CCIEFB_RESP_HEADER_DL_OFFSET 9U

#define CL_CCIEFB_RESP_HEADER_RESERVED1 0xD000
#define CL_CCIEFB_RESP_HEADER_RESERVED2 0x00
#define CL_CCIEFB_RESP_HEADER_RESERVED3 0xFF
#define CL_CCIEFB_RESP_HEADER_RESERVED4 0x03FF
#define CL_CCIEFB_RESP_HEADER_RESERVED5 0x00
#define CL_CCIEFB_RESP_HEADER_RESERVED6 0x0000

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_cciefb_cyclic_req_header
{
   uint16_t protocol_ver;            /** Little endian */
   uint16_t reserved1;               /** Always 0x0000. Little endian */
   uint16_t cyclic_info_offset_addr; /** Little endian */
   uint8_t reserved2[14];            /** Always 0x00. */
} cl_cciefb_cyclic_req_header_t;
CC_PACKED_END
CC_STATIC_ASSERT (sizeof (cl_cciefb_cyclic_req_header_t) == 20);

#define CL_CCIEFB_CYCLIC_REQ_HEADER_RESERVED1 0x0000
#define CL_CCIEFB_CYCLIC_REQ_HEADER_RESERVED2 0x00

/** Valid for protocol version 1 and 2 */
#define CL_CCIEFB_CYCLIC_REQ_CYCLIC_OFFSET 36

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_cciefb_cyclic_resp_header
{
   uint16_t protocol_ver;            /** Little endian */
   uint16_t end_code;                /** Little endian. cl_slmp_error_codes_t */
   uint16_t cyclic_info_offset_addr; /** Little endian */
   uint8_t reserved1[14];            /** Always 0x00. */
} cl_cciefb_cyclic_resp_header_t;
CC_PACKED_END
CC_STATIC_ASSERT (sizeof (cl_cciefb_cyclic_resp_header_t) == 20);

#define CL_CCIEFB_CYCLIC_RESP_HEADER_RESERVED1 0x00

/** Valid for protocol version 1 and 2 */
#define CL_CCIEFB_CYCLIC_RESP_CYCLIC_OFFSET 40

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_cciefb_master_station_notification
{
   /** Little endian. Master application status
       See CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_xxx */
   uint16_t master_local_unit_info;
   uint16_t reserved; /** Always 0x0000. Little endian */
   /** Little endian. Unix timestamp with milliseconds */
   uint64_t clock_info;
} cl_cciefb_master_station_notification_t;
CC_PACKED_END
CC_STATIC_ASSERT (sizeof (cl_cciefb_master_station_notification_t) == 12);

#define CL_CCIEFB_MASTER_STATION_NOTIFICATION_RESERVED       0x0000
#define CL_CCIEFB_MASTER_STATION_NOTIFICATION_MASK_BITS_VER1 0xFFFEU
#define CL_CCIEFB_MASTER_STATION_NOTIFICATION_MASK_BITS_VER2 0xFFFCU
#define CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED             0x0000
#define CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING             0x0001
#define CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED_BY_USER     0x0002

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_cciefb_slave_station_notification
{
   uint16_t vendor_code;   /** Little endian */
   uint16_t reserved1;     /** Always 0x0000. Little endian */
   uint32_t model_code;    /** Little endian */
   uint16_t equipment_ver; /** Little endian */
   uint16_t reserved2;     /** Always 0x0000. Little endian */
   /** Little endian. cl_slave_appl_operation_status_t */
   uint16_t slave_local_unit_info;
   uint16_t slave_err_code;        /** Little endian */
   uint32_t local_management_info; /** Little endian */
} cl_cciefb_slave_station_notification_t;
CC_PACKED_END
CC_STATIC_ASSERT (sizeof (cl_cciefb_slave_station_notification_t) == 20);

#define CL_CCIEFB_SLAVE_STATION_NOTIFICATION_RESERVED1              0x0000
#define CL_CCIEFB_SLAVE_STATION_NOTIFICATION_RESERVED2              0x0000
#define CL_CCIEFB_SLAVE_STATION_NOTIFICATION_MASK_BITS              0xFFFEU
#define CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE 0x0000
#define CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO          \
   0x00000000U

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_cciefb_cyclic_req_data_header
{
   uint32_t master_id; /** Little endian. Master IP address. */
   uint8_t group_no;
   uint8_t reserved3;                   /** Always 0x00. */
   uint16_t frame_sequence_no;          /** Little endian */
   uint16_t timeout_value;              /** Little endian, in milliseconds */
   uint16_t parallel_off_timeout_count; /** Little endian */
   uint16_t parameter_no;               /** Little endian */
   /** Little endian. Total number of occupied stations in this group */
   uint16_t slave_total_occupied_station_count;
   uint16_t cyclic_transmission_state; /** Little endian */
   uint16_t reserved4;                 /** Always 0x0000. Little endian */
} cl_cciefb_cyclic_req_data_header_t;
CC_PACKED_END
CC_STATIC_ASSERT (sizeof (cl_cciefb_cyclic_req_data_header_t) == 20);

#define CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_RESERVED3               0x00
#define CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_RESERVED4               0x0000
#define CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF 0x0000

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_cciefb_cyclic_resp_data_header
{
   uint32_t slave_id; /** Little endian. Slave IP address */
   uint8_t group_no;
   uint8_t reserved2;          /** Always 0x00. */
   uint16_t frame_sequence_no; /** Little endian */
} cl_cciefb_cyclic_resp_data_header_t;
CC_PACKED_END
CC_STATIC_ASSERT (sizeof (cl_cciefb_cyclic_resp_data_header_t) == 8);

#define CL_CCIEFB_CYCLIC_RESP_DATA_HEADER_RESERVED2 0x00

typedef struct cls_memory_area
{
   cl_ry_t ry[CLS_MAX_OCCUPIED_STATIONS];
   cl_rww_t rww[CLS_MAX_OCCUPIED_STATIONS];
} cls_memory_area_t;

typedef struct clm_group_memory_area
{
   cl_rx_t rx[CLM_MAX_OCCUPIED_STATIONS_PER_GROUP];
   cl_ry_t ry[CLM_MAX_OCCUPIED_STATIONS_PER_GROUP];
   cl_rwr_t rwr[CLM_MAX_OCCUPIED_STATIONS_PER_GROUP];
   cl_rww_t rww[CLM_MAX_OCCUPIED_STATIONS_PER_GROUP];

} clm_group_memory_area_t;

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_cciefb_cyclic_req_full_headers
{
   cl_cciefb_req_header_t req_header;
   cl_cciefb_cyclic_req_header_t cyclic_header;
   cl_cciefb_master_station_notification_t master_station_notification;
   cl_cciefb_cyclic_req_data_header_t cyclic_data_header;
   uint8_t data[];
} cl_cciefb_cyclic_req_full_headers_t;
CC_PACKED_END

typedef struct cls_cciefb_cyclic_request_info
{
   cl_ipaddr_t remote_ip;
   uint16_t remote_port;
   cl_ipaddr_t slave_ip_addr;
   cl_cciefb_cyclic_req_full_headers_t * full_headers;
   uint32_t * first_slave_id; /** First slave ID in request (Little endian) */
   cl_rww_t * first_rww;      /** First RWw of all in the request */
   cl_ry_t * first_ry;        /** First RY of all in the request */
} cls_cciefb_cyclic_request_info_t;

typedef struct clm_cciefb_cyclic_request_info
{
   uint8_t * buffer;
   size_t buf_size;
   size_t udp_payload_len;
   cl_cciefb_cyclic_req_full_headers_t * full_headers;
   uint32_t * first_slave_id; /** First slave ID in request (Little endian) */
   cl_rww_t * first_rww;      /** First RWw of all in the request */
   cl_ry_t * first_ry;        /** First RY of all in the request */
} clm_cciefb_cyclic_request_info_t;

#define CL_CCIEFB_MULTISTATION_INDICATOR UINT32_MAX

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_cciefb_cyclic_resp_full_headers
{
   cl_cciefb_resp_header_t resp_header;
   cl_cciefb_cyclic_resp_header_t cyclic_header;
   cl_cciefb_slave_station_notification_t slave_station_notification;
   cl_cciefb_cyclic_resp_data_header_t cyclic_data_header;
   uint8_t data[];
} cl_cciefb_cyclic_resp_full_headers_t;
CC_PACKED_END

typedef struct cls_cciefb_cyclic_response_info
{
   uint8_t * buffer;
   size_t buf_size;
   size_t udp_payload_len;
   cl_cciefb_cyclic_resp_full_headers_t * full_headers;
   cl_rwr_t * first_rwr;
   cl_rx_t * first_rx;
} cls_cciefb_cyclic_response_info_t;

typedef struct cls_addr_info
{
   uint32_t remote_ip;
   uint16_t remote_port;
   uint32_t local_ip;
   uint32_t local_netmask;
   uint8_t local_mac_address[6];
   int ifindex;
} cls_addr_info_t;

typedef struct clm_cciefb_cyclic_response_info
{
   uint16_t remote_port;
   uint32_t remote_ip;
   uint16_t number_of_occupied;
   uint32_t reception_timestamp;
   uint16_t device_index; /** Within group. Assigned in master after parsing */
   cl_cciefb_cyclic_resp_full_headers_t * full_headers;
   cl_rwr_t * first_rwr;
   cl_rx_t * first_rx;
} clm_cciefb_cyclic_response_info_t;

/** Runtime data for one group, including transmission buffer */
typedef struct clm_group_data
{
   uint16_t group_index;       /** Group number, allowed 0..63 */
   uint16_t total_occupied;    /** Number of occupied slave stations in group */
   uint16_t frame_sequence_no; /** Frame sequence counter */
   uint16_t cyclic_transmission_state; /** One bit per slave */
   uint32_t timestamp_link_scan_start;
   clm_group_state_t group_state;
   cl_timer_t response_wait_timer;
   cl_timer_t constant_linkscan_timer; /** Also known as ListenTimer */

   /** Frame for CCIEFB requests, for sending outgoing RY and RWw data */
   uint8_t sendbuf[CL_BUFFER_LEN];
   clm_cciefb_cyclic_request_info_t req_frame;

   clm_slave_device_data_t slave_devices[CLM_MAX_OCCUPIED_STATIONS_PER_GROUP];

   /** Memory area for user data. RX, RY, RWr and RWw. */
   clm_group_memory_area_t memory_area;
} clm_group_data_t;

/************************** Slave state machine ***************************/

typedef struct cls_slave_fsm
{
   cls_slave_state_t next;
   cls_slave_event_t (*action) (
      cls_t * cls,
      uint32_t now,
      const cls_cciefb_cyclic_request_info_t * request,
      cls_slave_event_t event);
} cls_slave_fsm_t;

typedef struct cls_slave_fsm_entry_exit
{
   cls_slave_event_t (*on_entry) (
      cls_t * cl,
      uint32_t now,
      const cls_cciefb_cyclic_request_info_t * request,
      cls_slave_event_t event);
   cls_slave_event_t (*on_exit) (
      cls_t * cls,
      uint32_t now,
      const cls_cciefb_cyclic_request_info_t * request,
      cls_slave_event_t event);
} cls_slave_fsm_entry_exit_t;

typedef struct cls_slave_fsm_transition
{
   cls_slave_state_t state;
   cls_slave_event_t event;
   cls_slave_state_t next;
   cls_slave_event_t (*action) (
      cls_t * cls,
      uint32_t now,
      const cls_cciefb_cyclic_request_info_t * request,
      cls_slave_event_t event);
} cls_slave_fsm_transition_t;

typedef struct cls_slave_fsm_on_entry_exit_table
{
   cls_slave_state_t state;
   cls_slave_event_t (*on_entry) (
      cls_t * cls,
      uint32_t now,
      const cls_cciefb_cyclic_request_info_t * request,
      cls_slave_event_t event);
   cls_slave_event_t (*on_exit) (
      cls_t * cls,
      uint32_t now,
      const cls_cciefb_cyclic_request_info_t * request,
      cls_slave_event_t event);
} cls_slave_fsm_on_entry_exit_table_t;

/************************** Device state machine ***************************/

typedef struct clm_device_fsm
{
   clm_device_state_t next;
   clm_device_event_t (*action) (
      clm_t * clm,
      uint32_t now,
      clm_group_data_t * group_data,
      clm_slave_device_data_t * slave_device_data,
      clm_device_event_t event,
      clm_device_state_t new_state);
} clm_device_fsm_t;

typedef struct clm_device_fsm_entry_exit
{
   clm_device_event_t (*on_entry) (
      clm_t * clm,
      uint32_t now,
      clm_group_data_t * group_data,
      clm_slave_device_data_t * slave_device_data,
      clm_device_event_t event,
      clm_device_state_t new_state);
   clm_device_event_t (*on_exit) (
      clm_t * clm,
      uint32_t now,
      clm_group_data_t * group_data,
      clm_slave_device_data_t * slave_device_data,
      clm_device_event_t event,
      clm_device_state_t new_state);
} clm_device_fsm_entry_exit_t;

typedef struct clm_device_fsm_transition
{
   clm_device_state_t state;
   clm_device_event_t event;
   clm_device_state_t next;
   clm_device_event_t (*action) (
      clm_t * clm,
      uint32_t now,
      clm_group_data_t * group_data,
      clm_slave_device_data_t * slave_device_data,
      clm_device_event_t event,
      clm_device_state_t new_state);
} clm_device_fsm_transition_t;

typedef struct clm_device_fsm_on_entry_exit_table
{
   clm_device_state_t state;
   clm_device_event_t (*on_entry) (
      clm_t * clm,
      uint32_t now,
      clm_group_data_t * group_data,
      clm_slave_device_data_t * slave_device_data,
      clm_device_event_t event,
      clm_device_state_t new_state);
   clm_device_event_t (*on_exit) (
      clm_t * clm,
      uint32_t now,
      clm_group_data_t * group_data,
      clm_slave_device_data_t * slave_device_data,
      clm_device_event_t event,
      clm_device_state_t new_state);
} clm_device_fsm_on_entry_exit_table_t;

/************************** Group state machine ***************************/

typedef struct clm_group_fsm
{
   clm_group_state_t next;
   clm_group_event_t (*action) (
      clm_t * clm,
      uint32_t now,
      clm_group_data_t * group_data,
      clm_group_event_t event,
      clm_group_state_t new_state);
} clm_group_fsm_t;

typedef struct clm_group_fsm_entry_exit
{
   clm_group_event_t (*on_entry) (
      clm_t * clm,
      uint32_t now,
      clm_group_data_t * group_data,
      clm_group_event_t event,
      clm_group_state_t new_state);
   clm_group_event_t (*on_exit) (
      clm_t * clm,
      uint32_t now,
      clm_group_data_t * group_data,
      clm_group_event_t event,
      clm_group_state_t new_state);
} clm_group_fsm_entry_exit_t;

typedef struct clm_group_fsm_transition
{
   clm_group_state_t state;
   clm_group_event_t event;
   clm_group_state_t next;
   clm_group_event_t (*action) (
      clm_t * clm,
      uint32_t now,
      clm_group_data_t * group_data,
      clm_group_event_t event,
      clm_group_state_t new_state);
} clm_group_fsm_transition_t;

typedef struct clm_group_fsm_on_entry_exit_table
{
   clm_group_state_t state;
   clm_group_event_t (*on_entry) (
      clm_t * clm,
      uint32_t now,
      clm_group_data_t * group_data,
      clm_group_event_t event,
      clm_group_state_t new_state);
   clm_group_event_t (*on_exit) (
      clm_t * clm,
      uint32_t now,
      clm_group_data_t * group_data,
      clm_group_event_t event,
      clm_group_state_t new_state);
} clm_group_fsm_on_entry_exit_table_t;

/*************************** SLMP frames ********************************/

#define CLM_SLMP_SERIAL_NONE (-1)

/* Common header field values */
#define CL_SLMP_HEADER_NETWORK_NUMBER 0x00
#define CL_SLMP_HEADER_UNIT_NUMBER    0xFF
#define CL_SLMP_HEADER_IO_NUMBER      CL_SLMP_DSTPROCNO_DEFAULT
#define CL_SLMP_HEADER_EXTENSION      0x00

/** MT type header: rdReqMT-PDU and wrReqMT-PDU */
CC_PACKED_BEGIN
typedef struct CC_PACKED cl_slmp_req_header
{
   uint16_t sub1;   /** Frame type, always 0x5400 for CCIEFB. Big endian */
   uint16_t serial; /** Little endian */
   uint16_t sub2;   /** Always 0x0000 for CCIEFB. Little endian */
   uint8_t network_number; /** Always 0x00 for CCIEFB. */
   uint8_t unit_number;    /** Always 0xFF for CCIEFB. */
   uint16_t io_number;     /** Always 0x03FF for CCIEFB. Little endian */
   uint8_t extension;      /** Always 0x00 for CCIEFB. */
   uint16_t length;        /** Little endian */
   uint16_t timer;         /** Always 0x0000 for CCIEFB. Little endian */
   uint16_t command;       /** Little endian */
   uint16_t sub_command;   /** Little endian */
} cl_slmp_req_header_t;
CC_PACKED_END
CC_STATIC_ASSERT (sizeof (cl_slmp_req_header_t) == 19);

/** Bytes up to and including 'length' field */
#define CL_SLMP_REQ_HEADER_LENGTH_OFFSET 13U

#define CL_SLMP_REQ_HEADER_SUB1  0x5400
#define CL_SLMP_REQ_HEADER_SUB2  0x0000
#define CL_SLMP_REQ_HEADER_TIMER 0

/** MT type header: rdResMT-PDU and wrResMT-PDU */
CC_PACKED_BEGIN
typedef struct CC_PACKED cl_slmp_resp_header
{
   uint16_t sub1;   /** Frame type, always 0xD400 for CCIEFB. Big endian */
   uint16_t serial; /** Little endian */
   uint16_t sub2;   /** Always 0x0000 for CCIEFB. Little endian */
   uint8_t network_number; /** Always 0x00 for CCIEFB. */
   uint8_t unit_number;    /** Always 0xFF for CCIEFB. */
   uint16_t io_number;     /** Always 0x03FF for CCIEFB. Little endian */
   uint8_t extension;      /** Always 0x00 for CCIEFB. */
   uint16_t length;        /** Little endian */
   uint16_t endcode;       /** Little endian */
} cl_slmp_resp_header_t;
CC_PACKED_END
CC_STATIC_ASSERT (sizeof (cl_slmp_resp_header_t) == 15);

/** Bytes up to and including 'length' field */
#define CL_SLMP_RESP_HEADER_LENGTH_OFFSET 13U

#define CL_SLMP_RESP_HEADER_SUB1 0xD400
#define CL_SLMP_RESP_HEADER_SUB2 0x0000

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_slmp_node_search_request
{
   cl_slmp_req_header_t header;
   uint8_t master_mac_addr[6];  /** Little endian = reversed order */
   uint8_t master_ip_addr_size; /** 4, as IPv4 is used */
   uint32_t master_ip_addr;     /** Little endian */
} cl_slmp_node_search_request_t;
CC_PACKED_END
CC_STATIC_ASSERT (
   sizeof (cl_slmp_node_search_request_t) == sizeof (cl_slmp_req_header_t) + 11);

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_slmp_node_search_resp
{
   cl_slmp_resp_header_t header;
   uint8_t master_mac_addr[6];     /** Little endian = reversed order */
   uint8_t master_ip_addr_size;    /** 4, as IPv4 is used */
   uint32_t master_ip_addr;        /** Little endian */
   uint8_t slave_mac_addr[6];      /** Little endian = reversed order */
   uint8_t slave_ip_addr_size;     /** 4, as IPv4 is used */
   uint32_t slave_ip_addr;         /** Little endian */
   uint32_t slave_netmask;         /** Little endian */
   uint32_t slave_default_gateway; /** Little endian, always OxFFFFFFFF */
   uint8_t slave_hostname_size;    /** Always 0, so no hostname field */
   uint16_t vendor_code;           /** Little endian */
   uint32_t model_code;            /** Little endian */
   uint16_t equipment_ver;         /** Little endian */
   uint8_t target_ip_addr_size;    /** 4, as IPv4 is used */
   uint32_t target_ip_addr;        /** Little endian, always OxFFFFFFFF */
   uint16_t target_port;           /** Little endian, always 0xFFFF */
   /** Little endian.
       Normal status = 0x0000 = CL_SLMP_NODE_SEARCH_RESP_SERVER_STATUS_NORMAL
       Other values are manufacturer defined
       See SLMP protocol section 8.3.11.3.1.17 */
   uint16_t slave_status;
   uint16_t slave_port;             /** Little endian, always 61451 */
   uint8_t slave_protocol_settings; /** Always 0x01 = UDP */
} cl_slmp_node_search_resp_t;
CC_PACKED_END
CC_STATIC_ASSERT (
   sizeof (cl_slmp_node_search_resp_t) == sizeof (cl_slmp_resp_header_t) + 51);

#define CL_SLMP_NODE_SEARCH_RESP_SLAVE_DEFAULT_GATEWAY UINT32_MAX
#define CL_SLMP_NODE_SEARCH_RESP_SLAVE_HOSTNAME_SIZE   0x00
#define CL_SLMP_NODE_SEARCH_RESP_TARGET_IP_ADDR        UINT32_MAX
#define CL_SLMP_NODE_SEARCH_RESP_TARGET_PORT           UINT16_MAX
#define CL_SLMP_NODE_SEARCH_RESP_SERVER_STATUS_NORMAL  0x0000
#define CL_SLMP_PROTOCOL_IDENTIFIER_UDP                0x01

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_slmp_set_ipaddr_request
{
   cl_slmp_req_header_t header;
   uint8_t master_mac_addr[6];      /** Little endian = reversed order */
   uint8_t master_ip_addr_size;     /** 4, as IPv4 is used */
   uint32_t master_ip_addr;         /** Little endian */
   uint8_t slave_mac_addr[6];       /** Little endian = reversed order */
   uint8_t slave_ip_addr_size;      /** 4, as IPv4 is used */
   uint32_t slave_new_ip_addr;      /** Little endian */
   uint32_t slave_new_netmask;      /** Little endian */
   uint32_t slave_default_gateway;  /** Little endian, always OxFFFFFFFF */
   uint8_t slave_hostname_size;     /** Always 0, so no hostname field */
   uint8_t target_ip_addr_size;     /** 4, as IPv4 is used */
   uint32_t target_ip_addr;         /** Little endian, always OxFFFFFFFF */
   uint16_t target_port;            /** Little endian, always 0xFFFF */
   uint8_t slave_protocol_settings; /** Always 0x01 = UDP */
} cl_slmp_set_ipaddr_request_t;
CC_PACKED_END
CC_STATIC_ASSERT (
   sizeof (cl_slmp_set_ipaddr_request_t) == sizeof (cl_slmp_req_header_t) + 39);

#define CL_SLMP_SET_IP_REQ_SLAVE_DEFAULT_GATEWAY UINT32_MAX
#define CL_SLMP_SET_IP_REQ_SLAVE_HOSTNAME_SIZE   0x00
#define CL_SLMP_SET_IP_REQ_TARGET_IP_ADDR        UINT32_MAX
#define CL_SLMP_SET_IP_REQ_TARGET_PORT           UINT16_MAX

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_slmp_set_ipaddr_resp
{
   cl_slmp_resp_header_t header;
   uint8_t master_mac_addr[6]; /** Little endian = reversed order */
} cl_slmp_set_ipaddr_resp_t;
CC_PACKED_END
CC_STATIC_ASSERT (
   sizeof (cl_slmp_set_ipaddr_resp_t) == sizeof (cl_slmp_resp_header_t) + 6);

/** MT type error response frame: rdErrMT-PDU and wrErrMT-PDU */
CC_PACKED_BEGIN
typedef struct CC_PACKED cl_slmp_error_resp
{
   cl_slmp_resp_header_t header;
   uint8_t error_network_number; /** Always 0x00 for CCIEFB. */
   uint8_t error_unit_number;    /** Always 0xFF for CCIEFB. */
   uint16_t error_io_number;     /** Always 0x03FF for CCIEFB. Little endian */
   uint8_t error_extension;      /** Always 0x00 for CCIEFB. */
   uint16_t command;             /** Little endian */
   uint16_t sub_command;         /** Little endian */
} cl_slmp_error_resp_t;
CC_PACKED_END
CC_STATIC_ASSERT (
   sizeof (cl_slmp_error_resp_t) == sizeof (cl_slmp_resp_header_t) + 9);

/*************************** Files ****************************************/

CC_PACKED_BEGIN
typedef struct CC_PACKED cl_file_header
{
   uint32_t magic;   /** File magic number */
   uint32_t version; /** File version number */
} cl_file_header_t;
CC_PACKED_END
CC_STATIC_ASSERT (sizeof (cl_file_header_t) == 8);

/**************************************************************************/

typedef enum cls_cciefb_logwarning_message
{
   CLS_LOGLIMITER_MASTER_DUPLICATION,
   CLS_LOGLIMITER_WRONG_NUMBER_OCCUPIED
} cls_cciefb_logwarning_message_t;

/**************************************************************************/

struct cls
{
   /* Slave settings */
   cls_cfg_t config;

   /** Which endcode to use if slave is disabled */
   cl_slmp_error_codes_t endcode_slave_disabled;

   /** Slave application status. slave_local_unit_info */
   cl_slave_appl_operation_status_t slave_application_status;

   /** For limiting warnings, to avoid spamming logfiles */
   cl_limiter_t loglimiter;

   /** To avoid repeated error callbacks for the same messagetype */
   cl_limiter_t errorlimiter;

   /** User defined values for sending to the PLC */
   uint32_t local_management_info;
   uint16_t slave_err_code;

   /** Data area for incoming RY and RWw from master
       Note that the content is little-endian (copied directly from
       incoming frame). */
   cls_memory_area_t cyclic_data_area;

   /* Receive and send buffers */

   int cciefb_socket;
   int slmp_send_socket;
   int slmp_receive_socket;
   uint8_t cciefb_receivebuf[CL_BUFFER_LEN];
   uint8_t slmp_receivebuf[CL_BUFFER_LEN];
   uint8_t slmp_sendbuf[CL_BUFFER_LEN];

   /** Frame for CCIEFB normal responses. Holds outgoing RX and RWr data.
       Note that the cyclic data is little-endian. */
   uint8_t cciefb_sendbuf_normal[CL_BUFFER_LEN];
   cls_cciefb_cyclic_response_info_t cciefb_resp_frame_normal;

   /** Frame for CCIEFB error responses. RX and RWr data is zero */
   uint8_t cciefb_sendbuf_error[CL_BUFFER_LEN];
   cls_cciefb_cyclic_response_info_t cciefb_resp_frame_error;

   /** Connection data from master */
   cls_master_connection_t master;

   /* Slave state machine */

   cls_slave_state_t state;
   cls_slave_fsm_t fsm_matrix[CLS_SLAVE_STATE_LAST][CLS_SLAVE_EVENT_LAST];
   cls_slave_fsm_entry_exit_t fsm_on_entry_exit[CLS_SLAVE_STATE_LAST];
   cl_timer_t receive_timer;             /** Cyclic data receive timeout */
   cl_timer_t timer_for_disabling_slave; /** For graceful slave disconnect */

   /** Data for triggering master state change callback */
   struct
   {
      bool executed_before;
      bool previous_connected_to_master;
      uint16_t previous_protocol_ver;
      uint16_t previous_master_application_status;
   } master_state_callback_trigger_data;

   /** Data for delayed node search response */
   struct
   {
      cl_timer_t response_timer;
      cl_macaddr_t master_mac_address;
      cl_ipaddr_t master_ip_addr;
      uint16_t master_port;
      uint16_t request_serial;
      cl_macaddr_t slave_mac_address;
      cl_ipaddr_t slave_ip_address;
      cl_ipaddr_t slave_netmask;
   } node_search;
};

struct clm
{
   /* ****** Master settings ****** */

   /** IP address that the master broadcasts SLMP UDP to */
   cl_ipaddr_t slmp_broadcast_ip;

   /** IP address that the master broadcasts IEFB UDP to */
   cl_ipaddr_t iefb_broadcast_ip;

   /** Master netmask */
   cl_ipaddr_t master_netmask;

   /** Master MAC address */
   cl_macaddr_t mac_address;

   /** Ethernet interface name, for debugging */
   char ifname[CLAL_IFNAME_SIZE];

   /** Ethernet interface index */
   int ifindex;

   /** IP address of conflicting master.
    *  Parsed from incoming request. */
   cl_ipaddr_t latest_conflicting_master_ip;

   clm_cfg_t config;

   /* ****** State machine transition tables ****** */

   clm_device_fsm_t device_fsm_matrix[CLM_DEVICE_STATE_LAST][CLM_DEVICE_EVENT_LAST];
   clm_device_fsm_entry_exit_t device_fsm_on_entry_exit[CLM_DEVICE_STATE_LAST];
   clm_group_fsm_t group_fsm_matrix[CLM_GROUP_STATE_LAST][CLM_GROUP_EVENT_LAST];
   clm_group_fsm_entry_exit_t group_fsm_on_entry_exit[CLM_GROUP_STATE_LAST];

   /* ****** Master runtime data ****** */

   /** Current outgoing SLMP request serial number */
   uint16_t slmp_request_serial;
   /** Simplified states for informing the application */
   clm_master_state_t master_state;
   uint16_t parameter_no;           /** Parameter setting version number */
   uint16_t master_local_unit_info; /** Master application running or not. See
                                       CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_xxx */
   cl_timer_t arbitration_timer;
   /** Value of slmp_request_serial when latest node search was done, or
    * -1 if no node search is pending. */
   int node_search_serial;
   cl_timer_t node_search_timer;
   /** Value of slmp_request_serial when latest set IP request was done, or
    * -1 if no set IP request is pending. */
   int set_ip_request_serial;
   cl_timer_t set_ip_request_timer;

   /** Node search database */
   clm_node_search_db_t node_search_db;

   /** Group runtime data */
   clm_group_data_t groups[CLM_MAX_GROUPS];

   /** To avoid repeated error callbacks for the same messagetype */
   cl_limiter_t errorlimiter;

   /* ****** Sockets and receive buffers ****** */

   int cciefb_socket;
   int cciefb_arbitration_socket;
   int slmp_send_socket;
   int slmp_receive_socket;
   uint8_t cciefb_receivebuf[CL_BUFFER_LEN];
   uint8_t slmp_receivebuf[CL_BUFFER_LEN];
   uint8_t slmp_sendbuf[CL_BUFFER_LEN];
};

#ifdef __cplusplus
}
#endif

#endif /* CL_TYPES_H */
