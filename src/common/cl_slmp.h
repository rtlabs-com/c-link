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

#ifndef CL_SLMP_H
#define CL_SLMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/cl_types.h"

/**
 * Calculate the delay for the node search response.
 *
 * There should be a random delay before the slave responds to a node search
 * request, as there can be a lot of slaves answering.
 *
 * The CC-Link standard says that the delay should be in the range 0-1500 ms.
 *
 * For a large number of slaves, the MAC addresses are pretty random. Note
 * that we need to use the last part of the MAC address, as the first three
 * bytes describes the vendor.
 *
 * Map the last 10 bits of the MAC address to a delay in the range 0-1023 ms.
 * Make sure that the least significant bit has the largest impact on the
 * delay time, to mitigate the effect of consecutive MAC addresses.
 *
 * @param slave_mac_address       Slave MAC address
 * @return the delay in microseconds
 */
uint32_t cl_slmp_calculate_node_search_delay (
   const cl_macaddr_t * slave_mac_address);

/**
 * Parse SLMP request header
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param header           Resulting header
 * @return 0 on success, -1 on failure
 */
int cl_slmp_parse_request_header (
   const uint8_t * buffer,
   size_t recv_len,
   cl_slmp_req_header_t ** header);

/**
 * Parse SLMP response header
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param header           Resulting header
 * @return 0 on success, -1 on failure
 */
int cl_slmp_parse_response_header (
   const uint8_t * buffer,
   size_t recv_len,
   cl_slmp_resp_header_t ** header);

/**
 * Parse SLMP error response
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param response         Resulting response
 * @return 0 on success, -1 on failure
 */
int cl_slmp_parse_error_response (
   const uint8_t * buffer,
   size_t recv_len,
   cl_slmp_error_resp_t ** response);

/**
 * Validate SLMP error response
 *
 * @param response         Response to be validated
 * @return 0 on valid header, -1 on invalid
 */
int cl_slmp_validate_error_response (const cl_slmp_error_resp_t * response);

/**
 * Parse SLMP node search request
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param request          Resulting request
 * @return 0 on success, -1 on failure
 */
int cl_slmp_parse_node_search_request (
   const uint8_t * buffer,
   size_t recv_len,
   cl_slmp_node_search_request_t ** request);

/**
 * Parse SLMP node search response
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param response         Resulting response
 * @return 0 on success, -1 on failure
 */
int cl_slmp_parse_node_search_response (
   const uint8_t * buffer,
   size_t recv_len,
   cl_slmp_node_search_resp_t ** response);

/**
 * Parse SLMP set IP address request
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param request          Resulting request
 * @return 0 on success, -1 on failure
 */
int cl_slmp_parse_set_ip_request (
   const uint8_t * buffer,
   size_t recv_len,
   cl_slmp_set_ipaddr_request_t ** request);

/**
 * Parse SLMP set IP address response
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param response         Resulting response
 * @return 0 on success, -1 on failure
 */
int cl_slmp_parse_set_ip_response (
   const uint8_t * buffer,
   size_t recv_len,
   cl_slmp_set_ipaddr_resp_t ** response);

/**
 * Validate SLMP request header
 *
 * @param header           Header to be validated
 * @param recv_len         UDP payload length (for comparison to header value)
 * @return 0 on valid header, -1 on invalid
 */
int cl_slmp_validate_request_header (
   const cl_slmp_req_header_t * header,
   size_t recv_len);

/**
 * Check if the request header really is a request
 *
 * @param header           Header to be validated
 * @return true if the header field says that it is a request
 */
bool cl_slmp_is_header_request (const cl_slmp_req_header_t * header);

/**
 * Validate SLMP response header
 *
 * @param header           Header to be validated
 * @param recv_len         UDP payload length (for comparison to header value)
 * @return 0 on valid header, -1 on invalid
 */
int cl_slmp_validate_response_header (
   const cl_slmp_resp_header_t * header,
   size_t recv_len);

/**
 * Check if the response header really is a response
 *
 * @param header           Header to be validated
 * @return true if the header field says that it is a response
 */
bool cl_slmp_is_header_response (const cl_slmp_resp_header_t * header);

/**
 * Validate SLMP node search request
 *
 * @param request          Request to be validated
 * @return 0 on valid, -1 on invalid
 */
int cl_slmp_validate_node_search_request (
   const cl_slmp_node_search_request_t * request);

/**
 * Validate SLMP node search response
 *
 * @param response         Response to be validated
 * @return 0 on valid, -1 on invalid
 *
 * @req REQ_CL_UDP_02
 * @req REQ_CL_SLMP_04
 */
int cl_slmp_validate_node_search_response (
   const cl_slmp_node_search_resp_t * response);

/**
 * Validate SLMP set IP address request
 *
 * @param request          Request to be validated
 * @return 0 on valid, -1 on invalid
 */
int cl_slmp_validate_set_ip_request (const cl_slmp_set_ipaddr_request_t * request);

/**
 * Validate SLMP set IP address response
 *
 * @param response         Response to be validated
 * @param my_mac_addr      Our MAC address, to verify that the request is for
 *                         us.
 * @return 0 on valid, -1 on invalid
 *
 * @req REQ_CL_SLMP_04
 *
 */
int cl_slmp_validate_set_ip_response (
   const cl_slmp_set_ipaddr_resp_t * response,
   const cl_macaddr_t * my_mac_addr);

/**
 * Prepare an outgoing node search request frame, in a buffer.
 *
 * No validation of arguments is done.
 *
 * @param buffer              Output buffer
 * @param buf_size            Buffer size
 * @param master_mac_addr     Master MAC address
 * @param master_ip_addr      Master IP address
 * @param request_serial      Serial number of the request
 *
 * @req REQ_CL_SLMP_02
 *
 */
void cl_slmp_prepare_node_search_request_frame (
   uint8_t * buffer,
   uint32_t buf_size,
   const cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr,
   uint16_t request_serial);

/**
 * Prepare an outgoing node search response frame, in a buffer.
 *
 * No validation of arguments is done.
 *
 * @param buffer              Output buffer
 * @param buf_size            Buffer size
 * @param master_mac_addr     Master MAC address
 * @param master_ip_addr      Master IP address
 * @param slave_mac_addr      Our MAC address
 * @param slave_ip_addr       Our IP address
 * @param slave_netmask       Our netmask
 * @param slave_status        Slave status. Normal status is 0x0000. Other
 *                            values are manufacturer defined.
 * @param vendor_code         Vendor code
 * @param model_code          Model code
 * @param equipment_ver       Equipment version
 * @param request_serial      Serial number of the request
 *
 * @req REQ_CL_SLMP_05
 * @req REQ_CL_SLMP_06
 * @req REQ_CL_SLMP_07
 * @req REQ_CL_SLMP_08
 * @req REQ_CL_SLMP_09
 * @req REQ_CL_SLMP_10
 * @req REQ_CL_SLMP_11
 *
 */
void cl_slmp_prepare_node_search_response_frame (
   uint8_t * buffer,
   uint32_t buf_size,
   const cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr,
   const cl_macaddr_t * slave_mac_addr,
   cl_ipaddr_t slave_ip_addr,
   cl_ipaddr_t slave_netmask,
   uint16_t slave_status,
   uint16_t vendor_code,
   uint32_t model_code,
   uint16_t equipment_ver,
   uint16_t request_serial);

/**
 * Prepare an outgoing set IP request frame.
 *
 * No validation of arguments is done.
 *
 * @param buffer              Output buffer
 * @param buf_size            Buffer size
 * @param master_mac_addr     Master MAC address
 * @param master_ip_addr      Master IP address
 * @param request_serial      Serial number of the request
 * @param slave_mac_addr      Slave MAC address
 * @param slave_new_ip_addr   New IP address for the slave
 * @param slave_new_netmask   New netmask for the slave
 *
 * @req REQ_CL_SLMP_03
 *
 */
void cl_slmp_prepare_set_ip_request_frame (
   uint8_t * buffer,
   uint32_t buf_size,
   const cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr,
   uint16_t request_serial,
   const cl_macaddr_t * slave_mac_addr,
   cl_ipaddr_t slave_new_ip_addr,
   cl_ipaddr_t slave_new_netmask);

/**
 * Prepare an outgoing set IP address response frame, in a buffer.
 *
 * No validation of arguments is done.
 *
 * @param buffer              Output buffer
 * @param buf_size            Buffer size
 * @param master_mac_addr     Master MAC address
 * @param request_serial      Serial number of the request
 */
void cl_slmp_prepare_set_ip_response_frame (
   uint8_t * buffer,
   uint32_t buf_size,
   const cl_macaddr_t * master_mac_addr,
   uint16_t request_serial);

/**
 * Prepare an outgoing error response frame, in a buffer.
 *
 * No validation of arguments is done.
 *
 * @param buffer              Output buffer
 * @param buf_size            Buffer size
 * @param command             Command
 * @param sub_command         Subcommand
 * @param endcode             End code
 * @param request_serial      Serial number of the request
 */
void cl_slmp_prepare_error_response_frame (
   uint8_t * buffer,
   uint32_t buf_size,
   uint16_t command,
   uint16_t sub_command,
   uint16_t endcode,
   uint16_t request_serial);

/************ Internal functions made available for tests *******************/

void cl_slmp_prepare_request_header (
   uint16_t serial,
   uint16_t command,
   uint16_t sub_command,
   uint16_t udp_payload_length,
   cl_slmp_req_header_t * header);

void cl_slmp_prepare_response_header (
   uint16_t serial,
   uint16_t endcode,
   size_t udp_payload_len,
   cl_slmp_resp_header_t * header);

#ifdef __cplusplus
}
#endif

#endif /* CL_SLMP_H */
