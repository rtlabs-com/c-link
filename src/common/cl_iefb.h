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

#ifndef CL_CCIEFB_H
#define CL_CCIEFB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/cl_types.h"

/**
 * Prepare an outgoing cyclic request frame, in a buffer.
 *
 * No validation of inputs is done.
 *
 * @param buffer                       Output buffer to be initialized
 * @param buf_size                     Buffer size
 * @param protocol_ver                 Protocol version
 * @param timeout_value                Timeout value
 * @param parallel_off_timeout_count   Timeout count
 * @param master_id                    Master IP address
 * @param group_no                     Group number
 * @param occupied_stations            Number of occupied stations in group
 * @param parameter_no                 Parameter update number
 * @param frame_info                   Frame information to be updated.
 *                                     Will also keep a reference to the buffer.
 *
 * @req REQ_CL_PROTOCOL_01
 * @req REQ_CL_PROTOCOL_02
 * @req REQ_CL_PROTOCOL_03
 * @req REQ_CL_PROTOCOL_04
 * @req REQ_CL_PROTOCOL_05
 * @req REQ_CL_PROTOCOL_06
 * @req REQ_CL_PROTOCOL_07
 * @req REQ_CL_PROTOCOL_08
 * @req REQ_CL_PROTOCOL_09
 * @req REQ_CL_PROTOCOL_10
 * @req REQ_CL_PROTOCOL_11
 * @req REQ_CL_PROTOCOL_12
 * @req REQ_CL_PROTOCOL_17
 * @req REQ_CL_PROTOCOL_21
 * @req REQ_CL_PROTOCOL_23
 * @req REQ_CL_PROTOCOL_32
 * @req REQ_CL_PROTOCOL_38
 * @req REQ_CL_PROTOCOL_39
 * @req REQ_CL_PROTOCOL_56
 *
 *
 */
void cl_iefb_initialise_request_frame (
   uint8_t * buffer,
   uint32_t buf_size,
   uint16_t protocol_ver,
   uint16_t timeout_value,
   uint16_t parallel_off_timeout_count,
   cl_ipaddr_t master_id,
   uint16_t group_no,
   uint16_t occupied_stations,
   uint16_t parameter_no,
   clm_cciefb_cyclic_request_info_t * frame_info);

/**
 * Update headers of an outgoing cyclic request frame in a buffer.
 *
 * No validation of inputs is done.
 *
 * @param frame_info                Frame to be updated
 * @param frame_sequence_no         Frame sequence number
 * @param clock_info                Unix timestamp in milliseconds, or 0 if
 *                                  not available.
 * @param master_local_unit_info    Master application status, see
 *                                  CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_xxx
 * @param cyclic_transmission_state Cyclic transmission state. One bit per slave
 *
 * @req REQ_CL_PROTOCOL_18
 *
 */
void cl_iefb_update_request_frame_headers (
   clm_cciefb_cyclic_request_info_t * frame_info,
   uint16_t frame_sequence_no,
   uint64_t clock_info,
   uint16_t master_local_unit_info,
   uint16_t cyclic_transmission_state);

/**
 * Calculate the UDP payload size of a cyclic data request frame.
 *
 * No validation is done on input data.
 *
 * @param slave_total_occupied_station_count Total number of occupied slaves for
 *                                           the relevant group in the master
 * @return the number of bytes in the UDP payload
 *
 * @req REQ_CL_PROTOCOL_38
 * @req REQ_CL_PROTOCOL_39
 *
 */
size_t cl_calculate_cyclic_request_size (
   uint16_t slave_total_occupied_station_count);

/**
 * Validate slave endcode
 *
 * @param end_code      Slave end code
 * @return true if the endcode is valid
 *
 * @req REQ_CLS_ERROR_04
 */
bool cl_is_slave_endcode_valid (cl_slmp_error_codes_t end_code);

/**
 * Calculate the UDP payload size of a cyclic data response frame.
 *
 * No validation is done on input data.
 *
 * @param occupied_stations      Number of slave stations occupied by a slave.
 * @return the number of bytes in the UDP payload
 */
size_t cl_calculate_cyclic_response_size (uint16_t occupied_stations);

/**
 * Calculate the number of occupied stations in a response frame.
 *
 * @param udp_payload_size      UDP payload size of the CCIEFB response frame
 * @return the number of occupied stations, or 0 at error.
 */
uint16_t cl_calculate_number_of_occupied_stations (size_t udp_payload_size);

/**
 * Validate size etc for a cyclic data request frame.
 *
 * @param protocol_ver                       CCIEFB protocol version.
 *                                           Typically 1 or 2
 * @param recv_len                           UDP payload length
 * @param dl                                 Length given in request header.
 * @param slave_total_occupied_station_count Total number of slaves for the
 *                                           master
 * @param cyclic_info_offset_addr            Offset of cyclic info
 * @return 0 for valid values, -1 on error
 */
int cl_iefb_validate_req_cyclic_frame_size (
   uint16_t protocol_ver,
   size_t recv_len,
   uint16_t dl,
   uint16_t slave_total_occupied_station_count,
   uint16_t cyclic_info_offset_addr);

/**
 * Parse CCIEFB request header
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param header           Resulting header
 *
 * @return 0 on success, -1 on failure
 */
int cl_iefb_parse_request_header (
   uint8_t * buffer,
   size_t recv_len,
   cl_cciefb_req_header_t ** header);

/**
 * Validate CCIEFB request header
 *
 * @param header           Header to be validated
 * @param recv_len         UDP payload length (for comparison to header value)
 *
 * @return 0 on valid header, -1 on invalid
 */
int cl_iefb_validate_request_header (
   const cl_cciefb_req_header_t * header,
   size_t recv_len);

/**
 * Set cyclic transmission state bit for one slave station
 *
 * Will only have an effect for valid slave station numbers.
 *
 * @param cyclic_transmission_state    Integer to be updated
 * @param slave_station_no             Slave station number, 1..16
 * @param state                        True if cyclic transmission is performed
 */
void cl_iefb_set_cyclic_transmission_state (
   uint16_t * cyclic_transmission_state,
   uint16_t slave_station_no,
   bool state);

/**
 * Extract the transmission state for my slave station (in message from master)
 *
 * @param cyclic_transmission_state Transmission state for all slaves in group
 * @param my_slave_station_no       My slave station number
 * @return true when I should be transmitting. False otherwise (including
 *         when wrong station number is given)
 */
bool cl_iefb_extract_my_transmission_state (
   uint16_t cyclic_transmission_state,
   uint16_t my_slave_station_no);

/**
 * Calculate area number for RX and RY.
 *
 * Find which byte the bit is located in, and generate bit mask.
 *
 * RX 0-63        Area 0      At least 1 occupied slave station
 * RX 64-127      Area 1      At least 2 occupied slave stations
 * RX 128-191     Area 2      At least 3 occupied slave stations
 * etc
 *
 *       binary       areanumber   byte_in_area   bitmask
 * RX0   00000000     0            0              0x01
 * RX1   00000001     0            0              0x02
 * RX2   00000010     0            0              0x04
 * RX6   00000110     0            0              0x40
 * RX7   00000111     0            0              0x80
 * RX8   00001000     0            1              0x01
 * RX9   00001001     0            1              0x02
 * RX61  00111101     0            7              0x20
 * RX62  00111110     0            7              0x40
 * RX63  00111111     0            7              0x80
 * RX64  01000000     1            0              0x01
 * RX65  01000001     1            0              0x02
 *
 * In summary:  Areanumber is bit 6 and higher
 *              byte_in_area is bit 5-3
 *              Calculate bitmask from input bit 2-0
 *
 * @param number           RX or RY number
 * @param byte_in_area     Resulting byte number, will be 0..7
 * @param bitmask          Resulting bit mask. A single bit is set.
 * @return area number
 */
uint16_t cl_iefb_bit_calculate_areanumber (
   uint16_t number,
   uint16_t * byte_in_area,
   uint8_t * bitmask);

/**
 * Calculate area number for RWr and RWw.
 *
 * Also calculate register number within an area.
 *
 * RWr 0-31       Area 0      At least 1 occupied slave station
 * RWr 32-63      Area 1      At least 2 occupied slave stations
 * RWr 63-95      Area 2      At least 3 occupied slave stations
 * etc
 *
 *          binary       areanumber   register_in_area
 * RWr 0    00000000     0             0
 * RWr 1    00000001     0             1
 * RWr 30   00011110     0            30
 * RWr 31   00011111     0            31
 * RWr 32   00100000     1             0
 * RWr 33   00100001     1             1
 *
 * In summary:  Areanumber is bit 5 and higher
 *              register_in_area is bit 4-0
 *
 * @param number           RWr or RWw number
 * @param register_in_area Resulting register within the area
 * @return area number
 */
uint16_t cl_iefb_register_calculate_areanumber (
   uint16_t number,
   uint16_t * register_in_area);

/**
 * Validate CCIEFB cyclic request header
 *
 * @param cyclic_header           Header to be validated
 *
 * @return 0 on valid header, -1 on invalid
 */
int cl_iefb_validate_cyclic_request_header (
   const cl_cciefb_cyclic_req_header_t * cyclic_header);

/**
 * Validate master station notification
 *
 * @param master_station_notification   Struct to be validated
 * @param protocol_ver                  Protocol version
 * @return 0 on valid, -1 on invalid
 */
int cl_iefb_validate_master_station_notification (
   const cl_cciefb_master_station_notification_t * master_station_notification,
   uint16_t protocol_ver);

/**
 * Validate CCIEFB cyclic request data header
 *
 * @param cyclic_data_header      Header to be validated
 * @param remote_ip               Remote IP, for comparison to header value
 *
 * @return 0 on valid header, -1 on invalid
 */
int cl_iefb_validate_req_cyclic_data_header (
   const cl_cciefb_cyclic_req_data_header_t * cyclic_data_header,
   cl_ipaddr_t remote_ip);

/**
 * Calculate the total timeout value.
 *
 * Takes into account the default values when arguments are 0.
 *
 * @param timeout_value               Timeout value in milliseconds
 * @param parallel_off_timeout_count  Count
 * @return the total timeout value in microseconds
 *
 * @req REQ_CLS_TIMING_01
 * @req REQ_CL_PROTOCOL_25
 * @req REQ_CL_PROTOCOL_28
 * @req REQ_CL_PROTOCOL_29
 *
 */
uint64_t cl_calculate_total_timeout_us (
   uint16_t timeout_value,
   uint16_t parallel_off_timeout_count);

/**
 * Get slave ID (IP address) for a slave station from a little-endian buffer
 *
 * @param first_slave_id      Pointer to buffer with little-endian IP addresses.
 * @param abs_slave_station   Which slave station to read the IP address for.
 *                            Allowed value 1..16 (but not larger than
 *                            total_occupied)
 * @param total_occupied      Total number of slave stations in use by the
 *                            master. For error checking. Allowed value 1..16
 * @param slave_id            Resulting slave ID
 * @return 0 on success, -1 on error
 */
int cl_iefb_request_get_slave_id (
   const uint32_t * first_slave_id,
   uint16_t abs_slave_station,
   uint16_t total_occupied,
   cl_ipaddr_t * slave_id);

/**
 * Analyze a list of slave IDs (IP addresses), and compare to my slave ID
 *
 * Note that an IP of 0.0.0.0 indicates that the slave that previously did
 * occupy the position should stop sending.
 *
 * An IP address of 255.255.255.255 indicates that the previous IP address
 * in the list should use also this slave station number (when occupying
 * several slave stations).
 *
 * Returns -1 if \a my_slave_id is invalid (0.0.0.0)
 *
 * @param my_slave_id              My slave ID (IP address)
 * @param total_occupied           Number of entries in the list
 * @param request_first_slave_id   Buffer with list of little-endian slave IDs
 * @param found_my_slave_id        True if my slave ID is found
 * @param my_slave_station_no      Where my slaveID is in the list. Will be
 *                                 set to 1 if it is first in the list.
 * @param implied_occupation_count Number of slave stations I should
 *                                 occupy according to list.
 * @return 0 on successful list parsing, -1 on error
 *
 * @req REQ_CLS_STATIONNUMBER_01
 * @req REQ_CLS_STATIONNUMBER_02
 * @req REQ_CLS_STATIONNUMBER_03
 * @req REQ_CL_PROTOCOL_34
 * @req REQ_CL_PROTOCOL_37
 * @req REQ_CL_PROTOCOL_52
 *
 */
int cl_iefb_analyze_slave_ids (
   cl_ipaddr_t my_slave_id,
   uint16_t total_occupied,
   uint32_t * request_first_slave_id,
   bool * found_my_slave_id,
   uint16_t * my_slave_station_no,
   uint16_t * implied_occupation_count);

/**
 * Parse CCIEFB cyclic request message headers
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param full_headers     Resulting header
 *
 * @return 0 on success, -1 on failure
 */
int cl_iefb_parse_req_full_cyclic_headers (
   uint8_t * buffer,
   size_t recv_len,
   cl_cciefb_cyclic_req_full_headers_t ** full_headers);

/**
 * Validate CCIEFB cyclic request headers
 *
 * @param full_headers            Headers to be validated
 * @param recv_len                UDP payload length, for comparison to content
 * @param remote_ip               Remote IP, for comparison to header value
 *
 * @return 0 on valid, -1 on invalid
 */
int cl_iefb_validate_req_full_cyclic_headers (
   const cl_cciefb_cyclic_req_full_headers_t * full_headers,
   size_t recv_len,
   cl_ipaddr_t remote_ip);

/**
 * Parse CCIEFB request cyclic data
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param slave_total_occupied_station_count Total number of occupied slave
 *                                           stations in request
 * @param first_slave_id   Resulting pointer to first slave ID in request (LE)
 * @param first_rww        Resulting pointer to first RWw in request (LE)
 * @param first_ry         Resulting pointer to first RY in request (LE)
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CL_PROTOCOL_31
 */
int cl_iefb_parse_request_cyclic_data (
   uint8_t * buffer,
   size_t recv_len,
   uint16_t slave_total_occupied_station_count,
   uint32_t ** first_slave_id,
   cl_rww_t ** first_rww,
   cl_ry_t ** first_ry);

/**
 * Get the corresponding RWw area for a slave station, from a request message
 *
 * @param request                 Request
 * @param abs_slave_station       Absolute slave station number to use in the
 *                                request.
 *                                Allowed values 1..slave_total_occupied
 *                                as given in the request.
 * @return Pointer to corresponding RWw area, or NULL on error
 */
const cl_rww_t * cl_iefb_request_get_rww (
   const cls_cciefb_cyclic_request_info_t * request,
   uint16_t abs_slave_station);

/**
 * Get the corresponding RY area for a slave station, from a request message
 *
 * @param request                 Request
 * @param abs_slave_station       Absolute slave station number in the request.
 *                                Allowed values 1..slave_total_occupied as
 *                                given in the request.
 * @return Pointer to corresponding RY area, or NULL on error
 */
const cl_ry_t * cl_iefb_request_get_ry (
   const cls_cciefb_cyclic_request_info_t * request,
   uint16_t abs_slave_station);

/**
 * Prepare an outgoing cyclic response frame, in a buffer.
 *
 * @param buffer              Output buffer
 * @param buf_size            Buffer size
 * @param occupied_stations   Number of occupied stations by this slave
 * @param vendor_code         Vendor code
 * @param model_code          Model code
 * @param equipment_ver       Equipment version
 * @param frame_info          Frame information to be updated. Will also keep
 *                            a reference to the buffer.
 *
 * @req REQ_CL_PROTOCOL_01
 * @req REQ_CL_PROTOCOL_02
 * @req REQ_CL_PROTOCOL_40
 * @req REQ_CL_PROTOCOL_41
 * @req REQ_CL_PROTOCOL_42
 * @req REQ_CL_PROTOCOL_43
 * @req REQ_CL_PROTOCOL_44
 * @req REQ_CL_PROTOCOL_45
 * @req REQ_CL_PROTOCOL_46
 * @req REQ_CL_PROTOCOL_47
 * @req REQ_CL_PROTOCOL_50
 * @req REQ_CL_PROTOCOL_53
 * @req REQ_CL_PROTOCOL_56
 * @req REQ_CLS_CONFORMANCE_01
 * @req REQ_CLS_VERSION_02
 *
 */
void cl_iefb_initialise_cyclic_response_frame (
   uint8_t * buffer,
   uint32_t buf_size,
   uint16_t occupied_stations,
   uint16_t vendor_code,
   uint32_t model_code,
   uint16_t equipment_ver,
   cls_cciefb_cyclic_response_info_t * frame_info);

/**
 * Update headers of an outgoing cyclic response frame in a buffer
 *
 * @param frame_info              Frame to be updated
 * @param slave_id                Slave ID (IP address)
 * @param end_code                End code
 * @param group_no                Group number
 * @param frame_sequence_no       Frame sequence number
 * @param slave_local_unit_info   Slave application status (slave local unit
 *                                info)
 * @param slave_err_code          Slave error code
 * @param local_management_info   Local management info
 *
 * @req REQ_CL_PROTOCOL_52
 */
void cl_iefb_update_cyclic_response_frame (
   cls_cciefb_cyclic_response_info_t * frame_info,
   cl_ipaddr_t slave_id,
   cl_slmp_error_codes_t end_code,
   uint16_t group_no,
   uint16_t frame_sequence_no,
   cl_slave_appl_operation_status_t slave_local_unit_info,
   uint16_t slave_err_code,
   uint32_t local_management_info);

/**
 * Parse (and partially validate) a cyclic request frame
 *
 * @param buffer           Buffer to be parsed
 * @param recv_len         UDP payload length
 * @param remote_ip        Remote IP. Will be used at response and for
 *                         verification.
 * @param remote_port      Remote UDP port. Will be used at response.
 * @param local_ip         Local IP address. Will be used at response.
 * @param request          Resulting parsed request
 * @return 0 on success, -1 on failure
 */
int cl_iefb_parse_cyclic_request (
   uint8_t * buffer,
   size_t recv_len,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port,
   cl_ipaddr_t local_ip,
   cls_cciefb_cyclic_request_info_t * request);

/**
 * Get a pointer to an RX memory area.
 *
 * @param frame_info       Response frame info
 * @param area_number      Area number. First area is 0. No validation is done.
 * @return Pointer to RX memory area
 */
cl_rx_t * cl_iefb_get_rx_area (
   cls_cciefb_cyclic_response_info_t * frame_info,
   uint16_t area_number);

/**
 * Get a pointer to an RWr memory area.
 *
 * @param frame_info       Response frame info
 * @param area_number      Area number. First area is 1. No validation is done.
 * @return Pointer to RWr memory area
 */
cl_rwr_t * cl_iefb_get_rwr_area (
   cls_cciefb_cyclic_response_info_t * frame_info,
   uint16_t area_number);

/**
 * Parse CCIEFB response header
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param header           Resulting header
 *
 * @return 0 on success, -1 on failure
 */
int cl_iefb_parse_response_header (
   uint8_t * buffer,
   size_t recv_len,
   cl_cciefb_resp_header_t ** header);

/**
 * Validate CCIEFB response header
 *
 * @param header           Header to be validated
 * @param recv_len         UDP payload length (for comparison to header value)
 *
 * @return 0 on valid header, -1 on invalid
 */
int cl_iefb_validate_response_header (
   const cl_cciefb_resp_header_t * header,
   size_t recv_len);

/**
 * Parse CCIEFB cyclic response message headers
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param full_headers     Resulting header
 *
 * @return 0 on success, -1 on failure
 */
int cl_iefb_parse_resp_full_cyclic_headers (
   uint8_t * buffer,
   size_t recv_len,
   cl_cciefb_cyclic_resp_full_headers_t ** full_headers);

/**
 * Validate CCIEFB cyclic response header
 *
 * @param cyclic_header           Header to be validated
 *
 * @return 0 on valid header, -1 on invalid
 */
int cl_iefb_validate_cyclic_resp_header (
   const cl_cciefb_cyclic_resp_header_t * cyclic_header);

/**
 * Validate slave station notification
 *
 * @param slave_station_notification   Struct to be validated
 * @return 0 on valid, -1 on invalid
 */
int cl_iefb_validate_slave_station_notification (
   const cl_cciefb_slave_station_notification_t * slave_station_notification);

/**
 * Validate CCIEFB cyclic response data header
 *
 * @param cyclic_data_header      Header to be validated
 * @param remote_ip               Remote IP, for comparison to header value
 * @return 0 on valid header, -1 on invalid
 */
int cl_iefb_validate_resp_cyclic_data_header (
   const cl_cciefb_cyclic_resp_data_header_t * cyclic_data_header,
   cl_ipaddr_t remote_ip);

/**
 * Validate size etc for a cyclic data response frame.
 *
 * @param protocol_ver            CCIEFB protocol version. Typically 1 or 2
 * @param recv_len                UDP payload length
 * @param dl                      Length given in request header.
 * @return 0 for valid values, -1 on error
 */
int cl_iefb_validate_resp_cyclic_frame_size (
   uint16_t protocol_ver,
   size_t recv_len,
   uint16_t dl);

/**
 * Validate CCIEFB cyclic response headers
 *
 * @param full_headers            Headers to be validated
 * @param recv_len                UDP payload length, for comparison to content
 * @param remote_ip               Remote IP, for comparison to header value
 * @return 0 on valid, -1 on invalid
 */
int cl_iefb_validate_resp_full_cyclic_headers (
   const cl_cciefb_cyclic_resp_full_headers_t * full_headers,
   size_t recv_len,
   cl_ipaddr_t remote_ip);

/**
 * Parse CCIEFB response cyclic data
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param number_of_occupied_stations  Total number of occupied slave
 *                                     stations in response
 * @param first_rwr        Resulting pointer to first RWr in response
 * @param first_rx         Resulting pointer to first RX in response
 * @return 0 on success, -1 on failure
 */
int cl_iefb_parse_response_cyclic_data (
   uint8_t * buffer,
   size_t recv_len,
   uint16_t number_of_occupied_stations,
   cl_rwr_t ** first_rwr,
   cl_rx_t ** first_rx);

/**
 * Parse (and partially validate) a cyclic response frame
 *
 * @param buffer           Buffer to be parsed
 * @param recv_len         UDP payload length
 * @param remote_ip        Remote IP. Also used for validation.
 * @param remote_port      Remote UDP port.
 * @param now              Current timestamp, in microseconds
 * @param response         Resulting parsed response
 * @return 0 on success, -1 on failure
 *
 */
int cl_iefb_parse_cyclic_response (
   uint8_t * buffer,
   size_t recv_len,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port,
   uint32_t now,
   clm_cciefb_cyclic_response_info_t * response);

#ifdef __cplusplus
}
#endif

#endif /* CL_CCIEFB_H */
