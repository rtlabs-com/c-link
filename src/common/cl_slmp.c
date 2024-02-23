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
 * @brief Parsing, validating and building SLMP frames.
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

#include "common/cl_slmp.h"

#include "common/cl_types.h"
#include "common/cl_util.h"

#include "osal_log.h"

#include <string.h>

/************************** Headers ***************************************/

int cl_slmp_parse_request_header (
   const uint8_t * buffer,
   size_t recv_len,
   cl_slmp_req_header_t ** header)
{
   if (sizeof (cl_slmp_req_header_t) > recv_len)
   {
      return -1;
   }
   *header = (cl_slmp_req_header_t *)buffer;

   return 0;
}

int cl_slmp_parse_response_header (
   const uint8_t * buffer,
   size_t recv_len,
   cl_slmp_resp_header_t ** header)
{
   if (sizeof (cl_slmp_resp_header_t) > recv_len)
   {
      return -1;
   }
   *header = (cl_slmp_resp_header_t *)buffer;

   return 0;
}

bool cl_slmp_is_header_request (const cl_slmp_req_header_t * header)
{
   /* Note that sub1 is big endian */
   return CC_FROM_BE16 (header->sub1) == CL_SLMP_REQ_HEADER_SUB1 &&
          CC_FROM_LE16 (header->sub2) == CL_SLMP_REQ_HEADER_SUB2;
}

int cl_slmp_validate_request_header (
   const cl_slmp_req_header_t * header,
   size_t recv_len)
{
   if (cl_slmp_is_header_request (header) == false)
   {
      return -1;
   }

   /* Validate length */
   if (CC_FROM_LE16 (header->length) + CL_SLMP_REQ_HEADER_LENGTH_OFFSET != recv_len)
   {
      return -1;
   }

   if (
      header->network_number != CL_SLMP_HEADER_NETWORK_NUMBER ||
      header->unit_number != CL_SLMP_HEADER_UNIT_NUMBER ||
      CC_FROM_LE16 (header->io_number) != CL_SLMP_HEADER_IO_NUMBER ||
      header->extension != CL_SLMP_HEADER_EXTENSION ||
      CC_FROM_LE16 (header->timer) != CL_SLMP_REQ_HEADER_TIMER)
   {
      return -1;
   }

   /* Do not check these values:
      - serial
      - command
      - sub_command; */

   return 0;
}

bool cl_slmp_is_header_response (const cl_slmp_resp_header_t * header)
{
   /* Note that sub1 is big endian */
   return CC_FROM_BE16 (header->sub1) == CL_SLMP_RESP_HEADER_SUB1 &&
          CC_FROM_LE16 (header->sub2) == CL_SLMP_RESP_HEADER_SUB2;
}

int cl_slmp_validate_response_header (
   const cl_slmp_resp_header_t * header,
   size_t recv_len)
{
   if (cl_slmp_is_header_response (header) == false)
   {
      return -1;
   }

   /* Validate length */
   if (CC_FROM_LE16 (header->length) + CL_SLMP_RESP_HEADER_LENGTH_OFFSET != recv_len)
   {
      return -1;
   }

   if (
      header->network_number != CL_SLMP_HEADER_NETWORK_NUMBER ||
      header->unit_number != CL_SLMP_HEADER_UNIT_NUMBER ||
      CC_FROM_LE16 (header->io_number) != CL_SLMP_HEADER_IO_NUMBER ||
      header->extension != CL_SLMP_HEADER_EXTENSION)
   {
      return -1;
   }

   /* Do not check these values:
      - serial
      - endcode */

   return 0;
}

/************************** Node search ***********************************/

int cl_slmp_parse_node_search_request (
   const uint8_t * buffer,
   size_t recv_len,
   cl_slmp_node_search_request_t ** request)
{
   if (recv_len != sizeof (cl_slmp_node_search_request_t))
   {
      return -1;
   }

   *request = (cl_slmp_node_search_request_t *)buffer;

   return 0;
}

int cl_slmp_parse_node_search_response (
   const uint8_t * buffer,
   size_t recv_len,
   cl_slmp_node_search_resp_t ** response)
{
   if (recv_len != sizeof (cl_slmp_node_search_resp_t))
   {
      return -1;
   }

   *response = (cl_slmp_node_search_resp_t *)buffer;

   return 0;
}

int cl_slmp_validate_node_search_request (
   const cl_slmp_node_search_request_t * request)
{
   /* Note that the cl_slmp_req_header_t header is already validated */

   if (request->header.length != sizeof (cl_slmp_node_search_request_t) - CL_SLMP_RESP_HEADER_LENGTH_OFFSET)
   {
      return -1;
   }

   /* IPv4 only */
   if (request->master_ip_addr_size != CL_ADDRSIZE_IPV4)
   {
      return -1;
   }

   /* Master IP address must be valid */
   if (CC_FROM_LE32 (request->master_ip_addr) == CL_IPADDR_INVALID)
   {
      return -1;
   }

   /* Do not check these values:
       - master_mac_addr */

   return 0;
}

int cl_slmp_validate_node_search_response (
   const cl_slmp_node_search_resp_t * response)
{
   /* Note that the cl_slmp_resp_header_t header is already validated */

   if (response->header.length != sizeof (cl_slmp_node_search_resp_t) - CL_SLMP_RESP_HEADER_LENGTH_OFFSET)
   {
      return -1;
   }

   /* IPv4 only */
   if (response->slave_ip_addr_size != CL_ADDRSIZE_IPV4)
   {
      return -1;
   }

   if (response->master_ip_addr_size != CL_ADDRSIZE_IPV4)
   {
      return -1;
   }

   /* TODO (rtljobe): Remove this check of fixed value */
   if (response->target_ip_addr_size != CL_ADDRSIZE_IPV4)
   {
      return -1;
   }

   /* Slave IP address and netmask must be valid */
   if (CC_FROM_LE32 (response->slave_ip_addr) == CL_IPADDR_INVALID)
   {
      return -1;
   }

   if (cl_utils_is_netmask_valid (CC_FROM_LE32 (response->slave_netmask)) == false)
   {
      return -1;
   }

   /* Master IP address must be valid */
   if (CC_FROM_LE32 (response->master_ip_addr) == CL_IPADDR_INVALID)
   {
      return -1;
   }

   /* Do not check these values:
      - vendor_code
      - model_code
      - equipment_ver
      - slave_status
      - slave_mac_addr
      - master_mac_addr
      - slave_port
      - slave_protocol_settings
      - target_port
      - target_ip_addr
      - slave_hostname_size
      - slave_default_gateway */

   return 0;
}

/************************** Set IP **************************************/

int cl_slmp_parse_set_ip_request (
   const uint8_t * buffer,
   size_t recv_len,
   cl_slmp_set_ipaddr_request_t ** request)
{
   if (recv_len != sizeof (cl_slmp_set_ipaddr_request_t))
   {
      return -1;
   }

   *request = (cl_slmp_set_ipaddr_request_t *)buffer;

   return 0;
}

int cl_slmp_parse_set_ip_response (
   const uint8_t * buffer,
   size_t recv_len,
   cl_slmp_set_ipaddr_resp_t ** response)
{
   if (recv_len != sizeof (cl_slmp_set_ipaddr_resp_t))
   {
      return -1;
   }

   *response = (cl_slmp_set_ipaddr_resp_t *)buffer;

   return 0;
}

int cl_slmp_validate_set_ip_request (const cl_slmp_set_ipaddr_request_t * request)
{
   /* Note that the cl_slmp_req_header_t header is already validated */

   if (request->header.length != sizeof (cl_slmp_set_ipaddr_request_t) - CL_SLMP_RESP_HEADER_LENGTH_OFFSET)
   {
      return -1;
   }

   cl_ipaddr_t master_ip_addr    = CC_FROM_LE32 (request->master_ip_addr);
   cl_ipaddr_t slave_new_ip_addr = CC_FROM_LE32 (request->slave_new_ip_addr);
   cl_ipaddr_t slave_new_netmask = CC_FROM_LE32 (request->slave_new_netmask);

   if (
      request->master_ip_addr_size != CL_ADDRSIZE_IPV4 ||
      request->slave_ip_addr_size != CL_ADDRSIZE_IPV4 ||
      /* TODO (rtljobe): Remove this check of fixed value */
      request->target_ip_addr_size != CL_ADDRSIZE_IPV4)
   {
      return -1;
   }

   /* TODO (rtljobe): Remove this check of fixed value */
   if (request->slave_protocol_settings != CL_SLMP_PROTOCOL_IDENTIFIER_UDP)
   {
      return -1;
   }

   /* TODO (rtljobe): Remove this check of fixed value */
   if (request->slave_hostname_size != CL_SLMP_SET_IP_REQ_SLAVE_HOSTNAME_SIZE)
   {
      return -1;
   }

   if (master_ip_addr == CL_IPADDR_INVALID || slave_new_ip_addr == CL_IPADDR_INVALID)
   {
      return -1;
   }

   if (cl_utils_is_netmask_valid (slave_new_netmask) == false)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): The incoming netmask is invalid: 0x%08X\n",
         __LINE__,
         (unsigned int)slave_new_netmask);
      return -1;
   }

   /* TODO (rtljobe): Remove this check of fixed value */
   if (CC_FROM_LE32 (request->slave_default_gateway) != CL_SLMP_SET_IP_REQ_SLAVE_DEFAULT_GATEWAY)
   {
      return -1;
   }

   /* TODO (rtljobe): Remove this check of fixed value */
   if (CC_FROM_LE32 (request->target_ip_addr) != CL_SLMP_SET_IP_REQ_TARGET_IP_ADDR)
   {
      return -1;
   }

   /* TODO (rtljobe): Remove this check of fixed value */
   if (CC_FROM_LE16 (request->target_port) != CL_SLMP_SET_IP_REQ_TARGET_PORT)
   {
      return -1;
   }

   /* Do not check these values:
      - slave_mac_addr
      - master_mac_addr */

   return 0;
}

int cl_slmp_validate_set_ip_response (
   const cl_slmp_set_ipaddr_resp_t * response,
   const cl_macaddr_t * my_mac_addr)
{
   cl_macaddr_t master_mac_addr = {0};

   /* Note that the cl_slmp_resp_header_t header is already validated */

   if (response->header.length != sizeof (cl_slmp_set_ipaddr_resp_t) - CL_SLMP_RESP_HEADER_LENGTH_OFFSET)
   {
      return -1;
   }

   /* MAC address in the payload part of the request has reversed byte order */
   cl_util_copy_mac_reverse (&master_mac_addr, &response->master_mac_addr);

   if (memcmp (&master_mac_addr, my_mac_addr, sizeof (master_mac_addr)) != 0)
   {
      return -1;
   }

   return 0;
}

int cl_slmp_parse_error_response (
   const uint8_t * buffer,
   size_t recv_len,
   cl_slmp_error_resp_t ** response)
{
   if (recv_len != sizeof (cl_slmp_error_resp_t))
   {
      return -1;
   }

   *response = (cl_slmp_error_resp_t *)buffer;

   return 0;
}

int cl_slmp_validate_error_response (const cl_slmp_error_resp_t * response)
{
   /* Note that the cl_slmp_resp_header_t header is already validated */

   if (response->header.length != sizeof (cl_slmp_error_resp_t) - CL_SLMP_RESP_HEADER_LENGTH_OFFSET)
   {
      return -1;
   }

   if (response->error_network_number != CL_SLMP_HEADER_NETWORK_NUMBER)
   {
      return -1;
   }

   if (response->error_unit_number != CL_SLMP_HEADER_UNIT_NUMBER)
   {
      return -1;
   }

   if (CC_FROM_LE16 (response->error_io_number) != CL_SLMP_HEADER_IO_NUMBER)
   {
      return -1;
   }

   if (response->error_extension != CL_SLMP_HEADER_EXTENSION)
   {
      return -1;
   }

   /* Do not check these values:
       - command
       - sub_command */

   return 0;
}

/************************** Create frames ********************************/

/**
 * Prepare a SLMP request header
 *
 * No validation of the arguments is done.
 *
 * @param serial                 Request serial number
 * @param command                Command
 * @param sub_command            Subcommand
 * @param udp_payload_length     Size of full response (UDP payload)
 * @param header                 Header to be updated
 */
void cl_slmp_prepare_request_header (
   uint16_t serial,
   uint16_t command,
   uint16_t sub_command,
   uint16_t udp_payload_length,
   cl_slmp_req_header_t * header)
{
   clal_clear_memory (header, sizeof (*header));

   /* Note that sub1 is big-endian, while the others are little-endian */
   header->sub1           = CC_TO_BE16 (CL_SLMP_REQ_HEADER_SUB1);
   header->serial         = CC_TO_LE16 (serial);
   header->sub2           = CC_TO_LE16 (CL_SLMP_REQ_HEADER_SUB2);
   header->network_number = CL_SLMP_HEADER_NETWORK_NUMBER;
   header->unit_number    = CL_SLMP_HEADER_UNIT_NUMBER;
   header->io_number      = CC_TO_LE16 (CL_SLMP_HEADER_IO_NUMBER);
   header->extension      = CL_SLMP_HEADER_EXTENSION;
   header->length =
      CC_TO_LE16 (udp_payload_length - CL_SLMP_REQ_HEADER_LENGTH_OFFSET);
   header->timer       = CL_SLMP_REQ_HEADER_TIMER;
   header->command     = CC_TO_LE16 (command);
   header->sub_command = CC_TO_LE16 (sub_command);
}

/**
 * Prepare a SLMP response header
 *
 * No validation of the arguments is done.
 *
 * @param serial                 Request serial number
 * @param endcode                Endcode
 * @param udp_payload_len        Size of full response (UDP payload)
 * @param header                 Header to be updated
 */
void cl_slmp_prepare_response_header (
   uint16_t serial,
   uint16_t endcode,
   size_t udp_payload_len,
   cl_slmp_resp_header_t * header)
{
   clal_clear_memory (header, sizeof (*header));

   /* Note that sub1 is big-endian, while the others are little-endian */
   header->sub1           = CC_TO_BE16 (CL_SLMP_RESP_HEADER_SUB1);
   header->serial         = CC_TO_LE16 (serial);
   header->sub2           = CC_TO_LE16 (CL_SLMP_RESP_HEADER_SUB2);
   header->network_number = CL_SLMP_HEADER_NETWORK_NUMBER;
   header->unit_number    = CL_SLMP_HEADER_UNIT_NUMBER;
   header->io_number      = CC_TO_LE16 (CL_SLMP_HEADER_IO_NUMBER);
   header->extension      = CL_SLMP_HEADER_EXTENSION;
   header->length         = CC_TO_LE16 (
      (udp_payload_len & UINT16_MAX) - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   header->endcode = CC_TO_LE16 (endcode);
}

void cl_slmp_prepare_node_search_request_frame (
   uint8_t * buffer,
   uint32_t buf_size,
   const cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr,
   uint16_t request_serial)
{
   cl_slmp_node_search_request_t * request;

   CC_ASSERT (buf_size >= sizeof (cl_slmp_node_search_request_t));

   clal_clear_memory (buffer, buf_size);
   request = (cl_slmp_node_search_request_t *)buffer;

   cl_slmp_prepare_request_header (
      request_serial,
      CL_SLMP_COMMAND_NODE_SEARCH,
      CL_SLMP_SUBCOMMAND_NODE_SEARCH,
      sizeof (cl_slmp_node_search_request_t),
      &request->header);

   /* MAC addresses in the payload part of the request should have reversed
      byte order */
   cl_util_copy_mac_reverse (&request->master_mac_addr, master_mac_addr);
   request->master_ip_addr_size = CL_ADDRSIZE_IPV4;
   request->master_ip_addr      = CC_TO_LE32 (master_ip_addr);
}

void cl_slmp_prepare_node_search_response_frame (
   uint8_t * buffer,
   uint32_t buf_size,
   const cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr,
   const cl_macaddr_t * slave_mac_addr,
   cl_ipaddr_t slave_ip_addr,
   cl_ipaddr_t slave_netmask,
   uint16_t slmp_slave_status,
   uint16_t vendor_code,
   uint32_t model_code,
   uint16_t equipment_ver,
   uint16_t request_serial)
{
   cl_slmp_node_search_resp_t * response;

   CC_ASSERT (buf_size >= sizeof (cl_slmp_node_search_resp_t));

   clal_clear_memory (buffer, buf_size);
   response = (cl_slmp_node_search_resp_t *)buffer;

   cl_slmp_prepare_response_header (
      request_serial,
      CL_SLMP_ENDCODE_SUCCESS,
      sizeof (cl_slmp_node_search_resp_t),
      &response->header);

   /* MAC addresses in the payload part of the response should have reversed
      byte order */
   cl_util_copy_mac_reverse (&response->master_mac_addr, master_mac_addr);
   response->master_ip_addr_size = CL_ADDRSIZE_IPV4;
   response->master_ip_addr      = CC_TO_LE32 (master_ip_addr);
   cl_util_copy_mac_reverse (&response->slave_mac_addr, slave_mac_addr);
   response->slave_ip_addr_size = CL_ADDRSIZE_IPV4;
   response->slave_ip_addr      = CC_TO_LE32 (slave_ip_addr);
   response->slave_netmask      = CC_TO_LE32 (slave_netmask);
   response->slave_default_gateway =
      CC_TO_LE32 (CL_SLMP_NODE_SEARCH_RESP_SLAVE_DEFAULT_GATEWAY);
   response->slave_hostname_size = CL_SLMP_NODE_SEARCH_RESP_SLAVE_HOSTNAME_SIZE;
   response->vendor_code         = CC_TO_LE16 (vendor_code);
   response->model_code          = CC_TO_LE32 (model_code);
   response->equipment_ver       = CC_TO_LE16 (equipment_ver);
   response->target_ip_addr_size = CL_ADDRSIZE_IPV4;
   response->target_ip_addr =
      CC_TO_LE32 (CL_SLMP_NODE_SEARCH_RESP_TARGET_IP_ADDR);
   response->target_port  = CC_TO_LE16 (CL_SLMP_NODE_SEARCH_RESP_TARGET_PORT);
   response->slave_status = CC_TO_LE16 (slmp_slave_status);
   response->slave_port   = CC_TO_LE16 (CL_SLMP_PORT);
   response->slave_protocol_settings = CL_SLMP_PROTOCOL_IDENTIFIER_UDP;
}

void cl_slmp_prepare_set_ip_request_frame (
   uint8_t * buffer,
   uint32_t buf_size,
   const cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr,
   uint16_t request_serial,
   const cl_macaddr_t * slave_mac_addr,
   cl_ipaddr_t slave_new_ip_addr,
   cl_ipaddr_t slave_new_netmask)
{
   cl_slmp_set_ipaddr_request_t * request;

   CC_ASSERT (buf_size >= sizeof (cl_slmp_set_ipaddr_request_t));

   clal_clear_memory (buffer, buf_size);
   request = (cl_slmp_set_ipaddr_request_t *)buffer;

   cl_slmp_prepare_request_header (
      request_serial,
      CL_SLMP_COMMAND_NODE_IPADDRESS_SET,
      CL_SLMP_SUBCOMMAND_NODE_IPADDRESS_SET,
      sizeof (cl_slmp_set_ipaddr_request_t),
      &request->header);

   /* MAC addresses in the payload part of the request should have reversed
      byte order */
   cl_util_copy_mac_reverse (&request->master_mac_addr, master_mac_addr);
   request->master_ip_addr_size = CL_ADDRSIZE_IPV4;
   request->master_ip_addr      = CC_TO_LE32 (master_ip_addr);
   cl_util_copy_mac_reverse (&request->slave_mac_addr, slave_mac_addr);
   request->slave_ip_addr_size = CL_ADDRSIZE_IPV4;
   request->slave_new_ip_addr  = CC_TO_LE32 (slave_new_ip_addr);
   request->slave_new_netmask  = CC_TO_LE32 (slave_new_netmask);
   request->slave_default_gateway =
      CC_TO_LE32 (CL_SLMP_SET_IP_REQ_SLAVE_DEFAULT_GATEWAY);
   request->slave_hostname_size = CL_SLMP_SET_IP_REQ_SLAVE_HOSTNAME_SIZE;
   request->target_ip_addr_size = CL_ADDRSIZE_IPV4;
   request->target_ip_addr = CC_TO_LE32 (CL_SLMP_SET_IP_REQ_TARGET_IP_ADDR);
   request->target_port    = CC_TO_LE16 (CL_SLMP_SET_IP_REQ_TARGET_PORT);
   request->slave_protocol_settings = CL_SLMP_PROTOCOL_IDENTIFIER_UDP;
}

void cl_slmp_prepare_set_ip_response_frame (
   uint8_t * buffer,
   uint32_t buf_size,
   const cl_macaddr_t * master_mac_addr,
   uint16_t request_serial)
{
   cl_slmp_set_ipaddr_resp_t * response;

   CC_ASSERT (buf_size >= sizeof (cl_slmp_set_ipaddr_resp_t));

   clal_clear_memory (buffer, buf_size);
   response = (cl_slmp_set_ipaddr_resp_t *)buffer;

   cl_slmp_prepare_response_header (
      request_serial,
      CL_SLMP_ENDCODE_SUCCESS,
      sizeof (cl_slmp_set_ipaddr_resp_t),
      &response->header);

   /* MAC address in the payload part of the response should have reversed
   byte order */
   cl_util_copy_mac_reverse (&response->master_mac_addr, master_mac_addr);
}

void cl_slmp_prepare_error_response_frame (
   uint8_t * buffer,
   uint32_t buf_size,
   uint16_t command,
   uint16_t sub_command,
   uint16_t endcode,
   uint16_t request_serial)
{
   cl_slmp_error_resp_t * response;

   CC_ASSERT (buf_size >= sizeof (cl_slmp_error_resp_t));

   clal_clear_memory (buffer, buf_size);
   response = (cl_slmp_error_resp_t *)buffer;

   cl_slmp_prepare_response_header (
      request_serial,
      endcode,
      sizeof (cl_slmp_error_resp_t),
      &response->header);

   response->error_network_number = CL_SLMP_HEADER_NETWORK_NUMBER;
   response->error_unit_number    = CL_SLMP_HEADER_UNIT_NUMBER;
   response->error_io_number      = CC_TO_LE16 (CL_SLMP_HEADER_IO_NUMBER);
   response->error_extension      = CL_SLMP_HEADER_EXTENSION;
   response->command              = CC_TO_LE16 (command);
   response->sub_command          = CC_TO_LE16 (sub_command);
}

/************************** Numerical utilities *************************/

uint32_t cl_slmp_calculate_node_search_delay (const cl_macaddr_t * slave_mac_address)
{
   uint32_t milliseconds = 0;
   uint16_t multiplier   = 512;
   uint16_t bitposition;

   for (bitposition = 0; bitposition < 8; bitposition++)
   {
      milliseconds +=
         multiplier * (((*slave_mac_address)[5] >> bitposition) & 0x01U);
      multiplier >>= 1U;
   }
   milliseconds += 2 * ((*slave_mac_address)[4] & 0x01U);
   milliseconds += (((*slave_mac_address)[4] >> 1) & 0x01U);

   return CL_TIMER_MICROSECONDS_PER_MILLISECOND * milliseconds;
}
