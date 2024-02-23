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

#ifndef UTILS_FOR_TESTING_H
#define UTILS_FOR_TESTING_H

#include "cl_options.h"
#include "master/clm_iefb.h"
#include "master/clm_master.h"
#include "master/clm_slmp.h"
#include "slave/cls_iefb.h"
#include "slave/cls_slave.h"

#include "mocks.h"

#include <gtest/gtest.h>

#define SIZE_REQUEST_NODE_SEARCH  (sizeof (cl_slmp_node_search_request_t))
#define SIZE_RESPONSE_NODE_SEARCH (sizeof (cl_slmp_node_search_resp_t))
#define SIZE_REQUEST_SET_IP       (sizeof (cl_slmp_set_ipaddr_request_t))
#define SIZE_RESPONSE_SET_IP      (sizeof (cl_slmp_set_ipaddr_resp_t))
#define SIZE_RESPONSE_ERROR       (sizeof (cl_slmp_error_resp_t))
#define SIZE_REQUEST_1_SLAVE      143UL
#define SIZE_REQUEST_3_SLAVES     295UL
#define SIZE_REQUEST_16_SLAVES    1283UL
#define SIZE_RESPONSE_1_SLAVE     131UL
#define SIZE_RESPONSE_2_SLAVES    203UL
#define SIZE_RESPONSE_16_SLAVES   1211UL

inline std::string FormatHexInt (int value)
{
   std::stringstream ss;
   ss << std::hex << std::showbase << value;
   return ss.str();
}

inline std::string FormatByte (uint8_t value)
{
   std::stringstream ss;
   ss << std::setfill ('0') << std::setw (2) << std::hex << std::showbase
      << static_cast<unsigned int> (value);
   return ss.str();
}

inline std::string FormatMacAddress (const cl_macaddr_t * mac)
{
   std::stringstream ss;
   ss << std::setfill ('0') << std::setw (2) << std::hex
      << static_cast<unsigned int> ((*mac)[0]) << ":"
      << static_cast<unsigned int> ((*mac)[1]) << ":"
      << static_cast<unsigned int> ((*mac)[2]) << ":"
      << static_cast<unsigned int> ((*mac)[3]) << ":"
      << static_cast<unsigned int> ((*mac)[4]) << ":"
      << static_cast<unsigned int> ((*mac)[5]);
   return ss.str();
}

template <typename T, size_t size>
::testing::AssertionResult ArraysMatch (
   const T (&expected)[size],
   const T (&actual)[size])
{
   for (size_t i (0); i < size; ++i)
   {
      if (expected[i] != actual[i])
      {
         return ::testing::AssertionFailure()
                << "actual[" << i << "] ("
                << FormatByte (static_cast<int> (actual[i])) << ") != expected["
                << i << "] (" << FormatByte (static_cast<int> (expected[i]))
                << ")";
      }
   }

   return ::testing::AssertionSuccess();
}

inline ::testing::AssertionResult MacAddressMatch (
   const cl_macaddr_t * mac1,
   const cl_macaddr_t * mac2)
{
   for (size_t i (0); i < sizeof (cl_macaddr_t); ++i)
   {
      if ((*mac1)[i] != (*mac2)[i])
      {
         return ::testing::AssertionFailure()
                << "The MAC addresses differs in byte " << i << ". Given "
                << FormatMacAddress (mac1) << " and " << FormatMacAddress (mac2);
      }
   }

   return ::testing::AssertionSuccess();
}

inline ::testing::AssertionResult ReversedMacMatch (
   const cl_macaddr_t * mac1,
   const cl_macaddr_t * mac2)
{
   for (size_t i (0); i < sizeof (cl_macaddr_t); ++i)
   {
      if ((*mac1)[i] != (*mac2)[sizeof (cl_macaddr_t) - 1 - i])
      {
         return ::testing::AssertionFailure()
                << "The MAC addresses do not match. One should be the reverse "
                   "of the other, but they differ in byte "
                << i << ". Given " << FormatMacAddress (mac1) << " and "
                << FormatMacAddress (mac2);
      }
   }

   return ::testing::AssertionSuccess();
}

/************************* Slave callbacks *******************************/

void my_slave_state_ind (cls_t * cls, void * arg, cls_slave_state_t state);

void my_slave_error_ind (
   cls_t * cls,
   void * arg,
   cls_error_message_t error_message,
   cl_ipaddr_t ip_addr,
   uint16_t argument_2);

void my_slave_master_running_ind (
   cls_t * cls,
   void * arg,
   bool connected_to_master,
   bool connected_and_running,
   bool stopped_by_user,
   uint16_t protocol_ver,
   uint16_t master_application_status);

void my_slave_connect_ind (
   cls_t * cls,
   void * arg,
   cl_ipaddr_t master_ip_addr,
   uint16_t group_no,
   uint16_t slave_station_no);

void my_slave_disconnect_ind (cls_t * cls, void * arg);

void my_slave_node_search_ind (
   cls_t * cls,
   void * arg,
   cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr);

void my_slave_set_ip_ind (
   cls_t * cls,
   void * arg,
   cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr,
   cl_ipaddr_t new_ip_addr,
   cl_ipaddr_t new_netmask,
   bool ip_setting_allowed,
   bool did_set_ip);

/************************* Master callbacks *******************************/

void my_master_state_ind (clm_t * clm, void * arg, clm_master_state_t state);

void my_master_connect_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   cl_ipaddr_t slave_id);

void my_master_disconnect_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   cl_ipaddr_t slave_id);

void my_master_linkscan_complete_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   bool success);

void my_master_alarm_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t end_code,
   uint16_t slave_err_code,
   uint32_t local_management_info);

void my_master_error_ind (
   clm_t * clm,
   void * arg,
   clm_error_message_t error_message,
   cl_ipaddr_t ip_addr,
   uint16_t argument_2);

void my_master_changed_slave_info_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t end_code,
   uint16_t slave_err_code,
   uint32_t local_management_info);

void my_master_node_search_result_cfm (
   clm_t * clm,
   void * arg,
   clm_node_search_db_t * db);

void my_master_set_ip_cfm (clm_t * clm, void * arg, clm_master_setip_status_t status);

/************************* Data frames **************************************/

inline uint8_t request_node_search[] = {

   // clang-format off

   /* Fixed header. Pos 0 */
   0x54, 0x00,

   /* Serial number. Pos 2 */
   0x01, 0x23,

   /* Fixed value. Pos 4 */
   0x00, 0x00,

   /* Network number. Pos 6 */
   0x00,

   /* Unit number. Pos 7 */
   0xFF,

   /* IO number. Pos 8 */
   0xFF, 0x03,

   /* Extension. Pos 10 */
   0x00,

   /* Length = 17. UDP payload 30 bytes */
   0x11, 0x00,

   /* Timer. Pos 13 */
   0x00, 0x00,

   /* Command. Pos 15 */
   0x30, 0x0E,

   /* Subcommand. Pos 17 */
   0x00, 0x00,

   /* Master MAC address. Pos 19 */
   0x26, 0x25, 0x24, 0x23, 0x22, 0x21,

   /* Master IP address size. Pos 25 */
   0x04,

   /* Master IP address. Pos 26 */
   0x04, 0x03, 0x02, 0x01,

   // clang-format on
};
CC_STATIC_ASSERT (sizeof (request_node_search) == SIZE_REQUEST_NODE_SEARCH);

inline uint8_t response_node_search[] = {
   // clang-format off

   /* Fixed header. Pos 0 */
   0xD4, 0x00,

   /* Serial number. Pos 2 */
   0x01, 0x23,

   /* Fixed value. Pos 4 */
   0x00, 0x00,

   /* Network number. Pos 6 */
   0x00,

   /* Unit number. Pos 7 */
   0xFF,

   /* IO number. Pos 8 */
   0xFF, 0x03,

   /* Extension. Pos 10 */
   0x00,

   /* Length = 53. UDP payload 66 bytes */
   0x35, 0x00,

   /* Endcode. Pos 13 */
   0x00, 0x00,

   /* Master MAC address. Pos 15 */
   0x26, 0x25, 0x24, 0x23, 0x22, 0x21,

   /* Master IP address size. Pos 21 */
   0x04,

   /* Master IP address. Pos 22 */
   0x04, 0x03, 0x02, 0x01,

   /* Slave MAC address. Pos 26 */
   0x56, 0x55, 0x54, 0x53, 0x52, 0x51,

   /* Slave IP address size. Pos 32 */
   0x04,

   /* Slave IP address. Pos 33 */
   0x06, 0x03, 0x02, 0x01,

   /* Slave netmask. Pos 37 */
   0x00, 0x00, 0xFF, 0xFF,

   /* Slave default gateway. Pos 41 */
   0xFF, 0xFF, 0xFF, 0xFF,

   /* Slave hostname size. Pos 45 */
   0x00,

   /* Vendor code. Pos 46 */
   0x56, 0x34,

   /* Model code. Pos 48 */
   0xDE, 0xBC, 0x9A, 0x78,

   /* Equipment version. Pos 52 */
   0x12, 0xF0,

   /* Target IP address size. Pos 54 */
   0x04,

   /* Target IP address. Pos 55 */
   0xFF, 0xFF, 0xFF, 0xFF,

   /* Target port. Pos 59 */
   0xFF, 0xFF,

   /* Slave status. Pos 61 */
   0x00, 0x00,

   /* Slave port. Pos 63 */
   0x0B, 0xF0,

   /* Slave protocol. Pos 65 */
   0x01

   // clang-format on
};
CC_STATIC_ASSERT (sizeof (response_node_search) == SIZE_RESPONSE_NODE_SEARCH);

inline uint8_t request_set_ip[] = {
   // clang-format off

   /* Fixed header. Pos 0 */
   0x54, 0x00,

   /* Serial number. Pos 2 */
   0x01, 0x23,

   /* Fixed value. Pos 4 */
   0x00, 0x00,

   /* Network number. Pos 6 */
   0x00,

   /* Unit number. Pos 7 */
   0xFF,

   /* IO number. Pos 8 */
   0xFF, 0x03,

   /* Extension. Pos 10 */
   0x00,

   /* Length = 45. UDP payload 58 bytes */
   0x2D, 0x00,

   /* Timer. Pos 13 */
   0x00, 0x00,

   /* Command. Pos 15 */
   0x31, 0x0E,

   /* Subcommand. Pos 17 */
   0x00, 0x00,

   /* Master MAC address. Pos 19 */
   0x26, 0x25, 0x24, 0x23, 0x22, 0x21,

   /* Master IP address size. Pos 25 */
   0x04,

   /* Master IP address. Pos 26 */
   0x04, 0x03, 0x02, 0x01,

   /* Slave MAC address. Pos 30 */
   0x56, 0x55, 0x54, 0x53, 0x52, 0x51,

   /* Slave IP address size. Pos 36 */
   0x04,

   /* New slave IP address. Pos 37 */
   0x09, 0x03, 0x02, 0x01,

   /* New slave netmask. Pos 41 */
   0x00, 0xFF, 0xFF, 0xFF,

   /* New slave default gateway. Pos 45 */
   0xFF, 0xFF, 0xFF, 0xFF,

   /* New slave hostname size. Pos 49 */
   0x00,

   /* Target IP address size. Pos 50 */
   0x04,

   /* New target IP address. Pos 51 */
   0xFF, 0xFF, 0xFF, 0xFF,

   /* New target port. Pos 55 */
   0xFF, 0xFF,

   /* Slave protocol settings. Pos 57 */
   0x01,

   // clang-format on
};
CC_STATIC_ASSERT (sizeof (request_set_ip) == SIZE_REQUEST_SET_IP);

inline uint8_t response_set_ip[] = {
   // clang-format off

   /* Fixed header. Pos 0 */
   0xD4, 0x00,

   /* Serial number. Pos 2 */
   0x01, 0x23,

   /* Fixed value. Pos 4 */
   0x00, 0x00,

   /* Network number. Pos 6 */
   0x00,

   /* Unit number. Pos 7 */
   0xFF,

   /* IO number. Pos 8 */
   0xFF, 0x03,

   /* Extension. Pos 10 */
   0x00,

   /* Length = 8. UDP payload 21 bytes */
   0x08, 0x00,

   /* Endcode. Pos 13 */
   0x00, 0x00,

   /* Master MAC address. Pos 15 */
   0x26, 0x25, 0x24, 0x23, 0x22, 0x21,

   // clang-format on
};
CC_STATIC_ASSERT (sizeof (response_set_ip) == SIZE_RESPONSE_SET_IP);

inline uint8_t response_set_ip_error[] = {
   // clang-format off

   /* Fixed header. Pos 0 */
   0xD4, 0x00,

   /* Serial number. Pos 2 */
   0x01, 0x23,

   /* Fixed value. Pos 4 */
   0x00, 0x00,

   /* Network number. Pos 6 */
   0x00,

   /* Unit number. Pos 7 */
   0xFF,

   /* IO number. Pos 8 */
   0xFF, 0x03,

   /* Extension. Pos 10 */
   0x00,

   /* Length = 11. UDP payload 24 bytes */
   0x0B, 0x00,

   /* Endcode. Pos 13 */
   0x5C, 0xC0,

   /* Error network number. Pos 15 */
   0x00,

   /* Error unit number. Pos 16 */
   0xFF,

   /* Error IO number. Pos 17 */
   0xFF, 0x03,

   /* Error extension. Pos 19 */
   0x00,

   /* Command. Pos 20 */
   0x31, 0x0E,

   /* Subcommand. Pos 22 */
   0x00, 0x00,

   // clang-format on
};
CC_STATIC_ASSERT (sizeof (response_set_ip_error) == SIZE_RESPONSE_ERROR);

inline uint8_t request_payload_running[] = {
   // clang-format off

   /* Fixed header. Pos 0 */
   0x50, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00,

   /* Length = 286 for 3 occupied slaves. UDP payload 295 bytes */
   0x1E, 0x01,

   /* Fixed value. Pos 9 */
   0x00, 0x00,

   /* Command, subcommand. Pos 11 */
   0x70, 0x0E, 0x00, 0x00,

   /* Protocol version. Pos 15 */
   0x02, 0x00,

   /* Reserved. Pos 17 */
   0x00, 0x00,

   /* Cyclic offset = 36 for protocol v1 and 2 */
   0x24, 0x00,

   /* Reserved. Pos 21 */
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,

   /* Master station running. Pos 35 */
   0x01, 0x00,

   /* Reserved. Pos 37 */
   0x00, 0x00,

   /* Master time (Unix timestamp with milliseconds). Pos 39 */
   0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF,

   /* Master ID = IP 1.2.3.4   Pos 47 */
   0x04, 0x03, 0x02, 0x01,

   /* Group. Pos 51 */
   0x01,

   /* Reserved. Pos 52 */
   0x00,

   /* Frame sequence number. Pos 53 */
   0x11, 0x22,

   /* Timeout period. Pos 55*/
   0x33, 0x00,

   /* Timeout multiplier. Pos 57 */
   0x55, 0x00,

   /* Parameter identifier. Pos 59 */
   0x77, 0x88,

   /* Total number of slave stations. Pos 61 */
   0x03, 0x00,

   /* Cyclic transmission status. Slave 1 and 2 running. Pos 63 */
   0x03, 0x00,

   /* Reserved. Pos 65 */
   0x00, 0x00,

   /* IP slave 1 = 1.2.3.5   Pos 67 */
   0x05, 0x03, 0x02, 0x01,

   /* IP slave 2 = 1.2.3.6, Occupies 2 slave stations. Ego node. Pos 71 */
   0x06, 0x03, 0x02, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,

   /* RWw slave 1. Pos 79 */
   0x11, 0x00, 0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,

   /* RWw slave 2 */
   0x22, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,

   /* RWw slave 3 */
   0x44, 0x00, 0x00, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x66,

   /* RY slave 1 */
   0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

   /* RY slave 2 */
   0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

   /* RY slave 3 */
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,

   // clang-format on
};
CC_STATIC_ASSERT (sizeof (request_payload_running) == SIZE_REQUEST_3_SLAVES);

/** Cyclic response frame. This is from IP 1.2.3.5, which is group 1 (group
    index 0) and device index 0. Occupies one slave station */
inline uint8_t response_payload_di0[] = {
   // clang-format off

   /* Fixed header. Pos 0 */
   0xD0, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00,

   /* Length = 122 for 1 occupied slave. UDP payload 131 bytes */
   0x7A, 0x00,

   /* Fixed value. Pos 9 */
   0x00, 0x00,

   /* Protocol version. Pos 11 */
   0x02, 0x00,

   /* End code. Pos 13 */
   0x00, 0x00,

   /* Cyclic offset = 40 for protocol v1 and 2 */
   0x28, 0x00,

   /* Reserved. Pos 17 */
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,

   /* Vendor code. Pos 31 */
   0x89, 0x67,

   /* Reserved1. Pos 33 */
   0x00, 0x00,

   /* Model code. Pos 35 */
   0x9A, 0x78, 0x56, 0x34,

   /* Equipment version. Pos 39 */
   0xDE, 0xBC,

   /* Reserved2. Pos 41 */
   0x00, 0x00,

   /* Slave application running. Pos 43 */
   0x01, 0x00,

   /* Slave error code. Pos 45 */
   0x62, 0x61,

   /* Local management info. Pos 47 */
   0x36, 0x35, 0x34, 0x33,

   /* Slave ID = IP 1.2.3.5   Pos 51 */
   0x05, 0x03, 0x02, 0x01,

   /* Group. Pos 55 */
   0x01,

   /* Reserved2. Pos 56 */
   0x00,

   /* Frame sequence number. Pos 57 */
   0x00, 0x00,

   /* RWr occupied 1. Pos 59  */
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
   0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
   0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,

   /* RX occupied 1 */
   0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,

   // clang-format on
};
CC_STATIC_ASSERT (sizeof (response_payload_di0) == SIZE_RESPONSE_1_SLAVE);

/** Cyclic response frame. This is from IP 1.2.3.6, which is group 1 (group
    index 0) and device index 1. Occupies two slave stations. */
inline uint8_t response_payload_di1[] = {
   // clang-format off

   /* Fixed header. Pos 0 */
   0xD0, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00,

   /* Length = 194 for 2 occupied slaves. UDP payload 203 bytes. Pos 7 */
   0xC2, 0x00,

   /* Fixed value. Pos 9 */
   0x00, 0x00,

   /* Protocol version. Pos 11 */
   0x02, 0x00,

   /* End code. Pos 13 */
   0x00, 0x00,

   /* Cyclic offset = 40 for protocol v1 and 2 */
   0x28, 0x00,

   /* Reserved. Pos 17 */
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,

   /* Vendor code. Pos 31 */
   0x56, 0x34,

   /* Reserved1. Pos 33 */
   0x00, 0x00,

   /* Model code. Pos 35 */
   0xDE, 0xBC, 0x9A, 0x78,

   /* Equipment version. Pos 39 */
   0x12, 0xF0,

   /* Reserved2. Pos 41 */
   0x00, 0x00,

   /* Slave application running. Pos 43 */
   0x01, 0x00,

   /* Slave error code. Pos 45 */
   0x39, 0x38,

   /* Local management info. Pos 47 */
   0x26, 0x25, 0x24, 0x23,

   /* Slave ID = IP 1.2.3.6   Pos 51 */
   0x06, 0x03, 0x02, 0x01,

   /* Group. Pos 55 */
   0x01,

   /* Reserved2. Pos 56 */
   0x00,

   /* Frame sequence number at startup. Pos 57 */
   0x00, 0x00,

   /* RWr occupied 1. Pos 59 */
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
   0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
   0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,

   /* RWr occupied 2 */
   0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
   0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
   0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
   0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,

   /* RX occupied 1 */
   0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,

   /* RX occupied 2 */
   0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,

   // clang-format on
};
CC_STATIC_ASSERT (sizeof (response_payload_di1) == SIZE_RESPONSE_2_SLAVES);

/************************* Test fixtures ************************************/

class UnitTest : public ::testing::Test
{
 protected:
   void SetUp() override
   {
      /* Reset mock call counters, set mocked MAC address etc for slave */
      mock_clear();
   }
};

/**
 * Test fixture with initialised structures etc, for slave testing
 */
class SlaveUnitTest : public UnitTest
{
 protected:
   /* Note: See also the values given in mocks.cpp */
   cls_t cls                                              = {};
   cls_cfg_t config                                       = {};
   uint8_t request_payload_initial[SIZE_REQUEST_3_SLAVES] = {0};
   uint32_t now                             = 0;          /* microseconds */
   const uint32_t tick_size                 = 1000;       /* microseconds */
   const uint32_t longer_than_nodesearch_us = 2000000;    /* 2 seconds */
   const uint32_t longer_than_timeout_us    = 10000000;   /* 10 seconds */
   const cl_ipaddr_t my_ip                  = 0x01020306; /* IP 1.2.3.6 */
   const cl_ipaddr_t new_ip                 = 0x01020309; /* IP 1.2.3.9 */
   const cl_ipaddr_t my_broadcast_ip        = 0x0102FFFF; /* IP 1.2.255.255 */
   const cl_ipaddr_t my_netmask             = 0xFFFF0000; /* 255.255.0.0 */
   const int my_ifindex                     = 4;
   const uint16_t my_group_no               = 1;
   const uint16_t my_slave_station_no       = 2;
   const uint16_t total_occupied_stations   = 3;          /* Within out group */
   const cl_ipaddr_t new_netmask            = 0xFFFFFF00; /* 255.255.255.0 */
   const cl_ipaddr_t remote_ip              = 0x01020304; /* IP 1.2.3.4 */
   const cl_macaddr_t remote_mac_addr = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26};
   const uint16_t parameter_no        = 0x8877;
   const uint16_t frame_sequence_no   = 0x2211;
   const uint16_t slmp_serial         = 0x2301;
   const uint16_t remote_protocol_ver = 2;
   const uint16_t my_protocol_ver     = 2;
   const uint64_t clock_info          = 0xEFCDAB9078563407;
   const uint64_t new_clock_info      = 0xEFCDAB9078563412;
   const uint16_t timeout_ms          = 0x0033;
   const uint16_t timeout_count       = 0x0055;
   cl_mock_network_interface_t * mock_interface = &mock_data.interfaces[0];
   cl_mock_udp_port_t * mock_cciefb_port        = &mock_data.udp_ports[0];
   cl_mock_udp_port_t * mock_slmp_port          = &mock_data.udp_ports[1];
   cl_mock_udp_port_t * mock_slmp_send_port     = &mock_data.udp_ports[2];

   /* 3 slaves in total, we occupy 2 slave stations */

   void SetUp() override
   {
      UnitTest::SetUp();

      /* Set up configuration */
      clal_clear_memory (&config, sizeof (config));
      config.num_occupied_stations       = 2;
      config.ip_setting_allowed          = true;
      config.vendor_code                 = 0x3456;
      config.model_code                  = 0x789ABCDE;
      config.equipment_ver               = 0xF012;
      config.connect_cb                  = my_slave_connect_ind;
      config.error_cb                    = my_slave_error_ind;
      config.disconnect_cb               = my_slave_disconnect_ind;
      config.state_cb                    = my_slave_state_ind;
      config.master_running_cb           = my_slave_master_running_ind;
      config.node_search_cb              = my_slave_node_search_ind;
      config.set_ip_cb                   = my_slave_set_ip_ind;
      config.cb_arg                      = &mock_data;
      config.use_slmp_directed_broadcast = false;

      /* Useful for tests without cls_slave_init()
         Note that cls_slave_init() will clear cls */
      clal_memcpy (&cls.config, sizeof (cls.config), &config, sizeof (config));

      /* Prepare request frame with cyclic transmission off for all slaves,
         and earlier timestamp */
      clal_memcpy (
         request_payload_initial,
         sizeof (request_payload_initial),
         request_payload_running,
         SIZE_REQUEST_3_SLAVES);
      request_payload_initial[63] = 0x00;
      request_payload_initial[39] = 0x07;
   };
};

/**
 * Test fixture with initialised structures etc, for master testing
 */
class MasterUnitTest : public UnitTest
{
 protected:
   /* Note: See also the values given in mocks.cpp */
   clm_t clm                                  = {};
   clm_cfg_t config                           = {};
   uint32_t now                               = 0;       /* microseconds */
   const uint32_t tick_size                   = 1000;    /* microseconds */
   const uint32_t longer_than_arbitration_us  = 2600000; /* 2.6 seconds */
   const uint32_t long_enough_for_nodesearch  = 1100000; /* 1.1 seconds */
   const uint32_t longer_than_timeout_us      = 1000000; /* 1 second */
   const uint32_t almost_timeout_us           = 497000;  /* 0.497 seconds */
   const uint16_t gi                          = 0;       /* Group index */
   const uint16_t group_number                = gi + 1;
   const uint16_t sdi0                        = 0; /* Slave device index */
   const uint16_t sdi                         = 1; /* Slave device index */
   const uint16_t timeout_value               = 500;
   const uint16_t arbitration_time_ms         = 2500;
   const uint16_t callback_time_nodesearch_ms = 2000;
   const uint16_t callback_time_setip_ms      = 500;
   const uint16_t slave_vendor_code           = 0x3456;
   const uint32_t slave_model_code            = 0x789ABCDE;
   const uint16_t slave_equipment_ver         = 0xF012;
   const uint16_t slaves_in_group             = 3; /* Occupied slaves */
   const uint16_t alarm_end_code              = 0x0004;
   const uint16_t alarm_slave_err_code        = 0x3839;
   const uint32_t alarm_local_management_info = 0x23242526;
   const uint16_t parameter_no                = 501; /* See also mocks.cpp */
   const size_t expected_filesize             = 10U;
   const cl_ipaddr_t my_ip                    = 0x01020304; /* IP 1.2.3.4 */
   const int my_ifindex                       = 5;
   const cl_ipaddr_t new_ip                   = 0x01020309; /* IP 1.2.3.9 */
   const cl_ipaddr_t broadcast_ip             = 0x0102FFFF; /* IP 1.2.255.255 */
   const cl_ipaddr_t my_netmask               = 0xFFFF0000; /* 255.255.0.0 */
   const cl_ipaddr_t new_netmask              = 0xFFFFFF00; /* 255.255.255.0 */
   const cl_ipaddr_t remote_ip_di0            = 0x01020305; /* IP 1.2.3.5 */
   const cl_ipaddr_t remote_ip                = 0x01020306; /* IP 1.2.3.6 */
   const cl_ipaddr_t remote_netmask           = 0xFFFF0000; /* 255.255.0.0 */
   const cl_macaddr_t remote_mac_addr = {0x51, 0x52, 0x53, 0x54, 0x55, 0x56};
   cl_mock_master_file_info_t * storage_file = &mock_data.storage_files[0];
   cl_mock_master_callback_counters_t * cb_counters =
      &mock_data.master_cb_counters[0];
   cl_mock_network_interface_t * mock_interface = &mock_data.interfaces[0];
   cl_mock_udp_port_t * mock_cciefb_port        = &mock_data.udp_ports[0];
   cl_mock_udp_port_t * mock_slmp_port          = &mock_data.udp_ports[1];
   cl_mock_udp_port_t * mock_slmp_send_port     = &mock_data.udp_ports[2];
   const uint16_t frame_sequenceno_startup      = 0; /* CCIEFB */
   const uint16_t pending_slmp_serial           = 0x2301;
   uint8_t response_di1_next_sequence_number[SIZE_RESPONSE_2_SLAVES]     = {};
   uint8_t response_di0_next_sequence_number[SIZE_RESPONSE_1_SLAVE]      = {};
   uint8_t response_di0_next_next_sequence_number[SIZE_RESPONSE_1_SLAVE] = {};

   /* 3 slaves in total, response from device occupying 2 slave stations */

   void SetUp() override
   {
      /* Reset mock call counters, set mocked MAC address etc for master */
      mock_clear_master();

      /* Set up configuration */
      clal_clear_memory (&config, sizeof (config));
      config.state_cb                     = my_master_state_ind;
      config.linkscan_cb                  = my_master_linkscan_complete_ind;
      config.connect_cb                   = my_master_connect_ind;
      config.disconnect_cb                = my_master_disconnect_ind;
      config.alarm_cb                     = my_master_alarm_ind;
      config.error_cb                     = my_master_error_ind;
      config.changed_slave_info_cb        = my_master_changed_slave_info_ind;
      config.node_search_cfm_cb           = my_master_node_search_result_cfm;
      config.set_ip_cfm_cb                = my_master_set_ip_cfm;
      config.cb_arg                       = cb_counters;
      config.use_slmp_directed_broadcast  = false;
      config.protocol_ver                 = 2;
      config.arbitration_time             = arbitration_time_ms;
      config.callback_time_node_search    = callback_time_nodesearch_ms;
      config.callback_time_set_ip         = callback_time_setip_ms;
      config.max_statistics_samples       = 1000;
      config.master_id                    = my_ip;
      config.hier.number_of_groups        = 1;
      config.hier.groups[0].timeout_value = timeout_value;
      config.hier.groups[0].parallel_off_timeout_count = 3;
      config.hier.groups[0].num_slave_devices          = 2;
      config.hier.groups[0].slave_devices[0].slave_id  = remote_ip_di0;
      config.hier.groups[0].slave_devices[0].num_occupied_stations = 1;
      config.hier.groups[0].slave_devices[1].slave_id              = remote_ip;
      config.hier.groups[0].slave_devices[1].num_occupied_stations = 2;

      (void)clal_copy_string (
         config.file_directory,
         "my_directory",
         sizeof (config.file_directory));

      /* Prepare response with next sequence number */
      clal_memcpy (
         response_di1_next_sequence_number,
         sizeof (response_di1_next_sequence_number),
         (uint8_t *)&response_payload_di1,
         SIZE_RESPONSE_2_SLAVES);
      response_di1_next_sequence_number[57] = 0x01;
      clal_memcpy (
         response_di0_next_sequence_number,
         sizeof (response_di0_next_sequence_number),
         (uint8_t *)&response_payload_di0,
         SIZE_RESPONSE_1_SLAVE);
      response_di0_next_sequence_number[57] = 0x01;
      clal_memcpy (
         response_di0_next_next_sequence_number,
         sizeof (response_di0_next_next_sequence_number),
         (uint8_t *)&response_payload_di0,
         SIZE_RESPONSE_1_SLAVE);
      response_di0_next_next_sequence_number[57] = 0x02;
   };
};

/**
 * Test fixture where our slave is initialised, but no master has connected
 */
class SlaveIntegrationTestNotConnected : public SlaveUnitTest
{
 protected:
   void SetUp() override
   {
      SlaveUnitTest::SetUp();

      ASSERT_GE (CLS_MAX_OCCUPIED_STATIONS, 3);

      (void)cls_slave_init (&cls, &config, now);
      cls_iefb_periodic (&cls, now);

      now += tick_size;
      cls_iefb_periodic (&cls, now);
   };
};

/**
 * Test fixture intended for integration tests. No master initialised yet.
 */
class MasterIntegrationTestNotInitialised : public MasterUnitTest
{
};

/**
 * Test fixture where our master is initialised, but no slave has responded
 */
class MasterIntegrationTestNoResponseYet : public MasterUnitTest
{
 protected:
   const clm_slave_device_data_t * slave_device_connection_details = nullptr;
   const clm_slave_device_statistics_t * statistics                = nullptr;

   void SetUp() override
   {
      int result = 0;
      MasterUnitTest::SetUp();

      result = clm_master_init (&clm, &config, now);
      CC_ASSERT (result == 0);
      clm_iefb_periodic (&clm, now);

      slave_device_connection_details =
         clm_iefb_get_device_connection_details (&clm, gi, sdi);
      statistics = &slave_device_connection_details->statistics;

      now += longer_than_arbitration_us;
      clm_iefb_periodic (&clm, now);

      now += tick_size;
      clm_iefb_periodic (&clm, now);
   };
};

/**
 * Test fixture where our master is initialised and slave device index 0 has
 * responded
 */
class MasterIntegrationTestDevice0Responded
    : public MasterIntegrationTestNoResponseYet
{
 protected:
   void SetUp() override
   {
      MasterIntegrationTestNoResponseYet::SetUp();

      mock_set_udp_fakedata (
         mock_cciefb_port,
         remote_ip_di0,
         CL_CCIEFB_PORT,
         (uint8_t *)&response_payload_di0,
         SIZE_RESPONSE_1_SLAVE);
      now += tick_size;
      clm_iefb_periodic (&clm, now);
   };
};

/**
 * Test fixture where our master is initialised and slave device index 0 and 1
 * have responded
 */
class MasterIntegrationTestBothDevicesResponded
    : public MasterIntegrationTestDevice0Responded
{
 protected:
   void SetUp() override
   {
      MasterIntegrationTestDevice0Responded::SetUp();

      mock_set_udp_fakedata (
         mock_cciefb_port,
         remote_ip,
         CL_CCIEFB_PORT,
         (uint8_t *)&response_payload_di1,
         SIZE_RESPONSE_2_SLAVES);
      now += tick_size;
      clm_iefb_periodic (&clm, now);
   };
};

/**
 * Test fixture where the master has connected to our slave
 */
class SlaveIntegrationTestConnected : public SlaveIntegrationTestNotConnected
{
 protected:
   void SetUp() override
   {
      SlaveIntegrationTestNotConnected::SetUp();

      /* Master connects */
      mock_set_udp_fakedata_with_local_ipaddr (
         mock_cciefb_port,
         my_ip,
         my_ifindex,
         remote_ip,
         CL_CCIEFB_PORT,
         (uint8_t *)&request_payload_initial,
         SIZE_REQUEST_3_SLAVES);

      now += tick_size;
      cls_iefb_periodic (&cls, now);

      /* Cyclic data from master */
      mock_set_udp_fakedata_with_local_ipaddr (
         mock_cciefb_port,
         my_ip,
         my_ifindex,
         remote_ip,
         CL_CCIEFB_PORT,
         (uint8_t *)&request_payload_running,
         SIZE_REQUEST_3_SLAVES);

      now += tick_size;
      cls_iefb_periodic (&cls, now);
   };
};

/**
 * Test fixture where our master is initialised
 */
class MasterIntegrationTestNotConnected : public MasterUnitTest
{
 protected:
   void SetUp() override
   {
      MasterUnitTest::SetUp();

      (void)clm_master_init (&clm, &config, now);
      clm_slmp_periodic (&clm, now);
      now += tick_size;
      clm_slmp_periodic (&clm, now);
   };
};

#endif /* UTILS_FOR_TESTING_H */
