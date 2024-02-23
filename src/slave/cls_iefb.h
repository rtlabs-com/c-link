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

#ifndef CLS_CCIEFB_H
#define CLS_CCIEFB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cls_api.h"
#include "common/cl_types.h"

/**
 * Initialise CCIEFB
 *
 * @param cls              c-link slave stack instance handle
 * @param now              timestamp in microseconds
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CL_UDP_01
 */
int cls_iefb_init (cls_t * cls, uint32_t now);

/**
 * Exit CCIEFB
 *
 * @param cls              c-link slave stack instance handle
 */
void cls_iefb_exit (cls_t * cls);

/**
 * Execute CCIEFB internals.
 *
 * Use this function every tick.
 *
 * @param cls              c-link slave stack instance handle
 * @param now              timestamp in microseconds
 *
 * @req REQ_CLS_TIMING_01
 * @req REQ_CLS_CONFORMANCE_08
 *
 */
void cls_iefb_periodic (cls_t * cls, uint32_t now);

/**
 * Tell the PLC to stop the cyclic communication.
 *
 * Use the \a is_error flag to indicate that we would like to stop due to an
 * error in software/hardware.
 *
 * @param cls              c-link slave stack instance handle
 * @param now              timestamp in microseconds
 * @param is_error         true if the stop is due to an error
 *
 * @req REQ_CLS_ERROR_06
 * @req REQ_CLS_ERROR_09
 *
 */
void cls_iefb_disable_slave (cls_t * cls, uint32_t now, bool is_error);

/**
 * Restart the cyclic communication.
 *
 * @param cls              c-link slave stack instance handle
 * @param now              timestamp in microseconds
 */
void cls_iefb_reenable_slave (cls_t * cls, uint32_t now);

/**
 * Get a pointer to the first RX memory area.
 *
 * Note that the slave has \a num_occupied_stations of these memory areas in an
 * array.
 *
 * The data is stored in the outgoing frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * @param cls              c-link slave stack instance handle
 * @return                 Pointer to first memory area
 */
cl_rx_t * cls_iefb_get_first_rx_area (cls_t * cls);

/**
 * Get a pointer to the first RY memory area.
 *
 * Note that the slave has \a num_occupied_stations of these memory areas in an
 * array.
 *
 * The data is copied directly from incoming frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * @param cls              c-link slave stack instance handle
 * @return                 Pointer to first memory area
 */
const cl_ry_t * cls_iefb_get_first_ry_area (cls_t * cls);

/**
 * Get a pointer to the first RWr memory area.
 *
 * Note that the slave has \a num_occupied_stations of these memory areas in an
 * array.
 *
 * The data is stored in the outgoing frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * @param cls              c-link slave stack instance handle
 * @return                 Pointer to first memory area
 */
cl_rwr_t * cls_iefb_get_first_rwr_area (cls_t * cls);

/**
 * Get a pointer to the first RWw memory area.
 *
 * Note that the slave has \a num_occupied_stations of these memory areas in an
 * array.
 *
 * The data is copied directly from incoming frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * @param cls              c-link slave stack instance handle
 * @return                 Pointer to first memory area
 */
const cl_rww_t * cls_iefb_get_first_rww_area (cls_t * cls);

/**
 * Set an individual bit for sending to the PLC.
 *
 * Note that the slave has \a 64*num_occupied_stations of these bits.
 * Will assert for invalid number.
 *
 * @param cls              c-link slave stack instance handle
 * @param number           Bit number. Use 0 for RX0.
 * @param value            Value to send to the PLC
 */
void cls_iefb_set_rx_bit (cls_t * cls, uint16_t number, bool value);

/**
 * Read back an individual bit (for sending to the PLC).
 *
 * Note that the slave has \a 64*num_occupied_stations of these bits.
 * Will assert for invalid number.
 *
 * @param cls              c-link slave stack instance handle
 * @param number           Bit number. Use 0 for RX0.
 * @return the bit value
 */
bool cls_iefb_get_rx_bit (cls_t * cls, uint16_t number);

/**
 * Read an individual bit from to the PLC.
 *
 * Note that the slave has \a 64*num_occupied_stations of these bits.
 * Will assert for invalid number.
 *
 * @param cls              c-link slave stack instance handle
 * @param number           Bit number. Use 0 for RY0.
 * @return the bit value
 */
bool cls_iefb_get_ry_bit (cls_t * cls, uint16_t number);

/**
 * Set a 16-bit register value for sending to the PLC.
 *
 * Note that the slave has \a 32*num_occupied_stations of these registers.
 * Will assert for invalid number.
 *
 * @param cls              c-link slave stack instance handle
 * @param number           Register number. Use 0 for RWr0.
 * @param value            Value to send to the PLC
 */
void cls_iefb_set_rwr_value (cls_t * cls, uint16_t number, uint16_t value);

/**
 * Read back a 16-bit register value (for sending to the PLC).
 *
 * Note that the slave has \a 32*num_occupied_stations of these registers.
 * Will assert for invalid values.
 *
 * @param cls              c-link slave stack instance handle
 * @param number           Register number. Use 0 for RWr0.
 * @return the register value
 */
uint16_t cls_iefb_get_rwr_value (cls_t * cls, uint16_t number);

/**
 * Read a 16-bit register value from the PLC.
 *
 * Note that the slave has \a 32*num_occupied_stations of these registers.
 * Will assert for invalid number.
 *
 * @param cls              c-link slave stack instance handle
 * @param number           Register number. Use 0 for RWw0.
 * @return the register value
 */
uint16_t cls_iefb_get_rww_value (cls_t * cls, uint16_t number);

/**
 * Get the master timestamp
 *
 * @param cls              c-link slave stack instance handle
 * @param master_timestamp Resulting timestamp in Unix time with milliseconds
 * @return 0 of the timestamp is valid, -1 if it is invalid.
 */
int cls_iefb_get_master_timestamp (cls_t * cls, uint64_t * master_timestamp);

/**
 * Set the slave application status, for sending to the PLC.
 *
 * This is information whether the slave application is running or is stopped.
 * When the stack is started, the slave application status is automatically set
 * to running.
 *
 * @param cls                     c-link slave stack instance handle
 * @param slave_application_status  Slave application status
 */
void cls_iefb_set_slave_application_status (
   cls_t * cls,
   cl_slave_appl_operation_status_t slave_application_status);

/**
 * Read back the slave application status
 *
 * @param cls                     c-link slave stack instance handle
 * @return the slave application status
 */
cl_slave_appl_operation_status_t cls_iefb_get_slave_application_status (
   cls_t * cls);

/**
 * Set the slave local management info
 *
 * @param cls                     c-link slave stack instance handle
 * @param local_management_info   Local management info
 *
 * @req REQ_CL_PROTOCOL_51
 */
void cls_iefb_set_local_management_info (
   cls_t * cls,
   uint32_t local_management_info);

/**
 * Read back the slave local management info
 *
 * @param cls                     c-link slave stack instance handle
 * @return the local management info
 */
uint32_t cls_iefb_get_local_management_info (cls_t * cls);

/**
 * Set the slave error code
 *
 * @param cls                     c-link slave stack instance handle
 * @param slave_err_code          Slave error code
 */
void cls_iefb_set_slave_error_code (cls_t * cls, uint16_t slave_err_code);

/**
 * Read back the slave error code
 *
 * @param cls                     c-link slave stack instance handle
 * @return the slave error code
 */
uint16_t cls_iefb_get_slave_error_code (cls_t * cls);

/**
 * Get a pointer to the master connection details (stored in slave).
 *
 * @param cls        c-link slave stack instance handle
 * @return           Pointer to master connection details
 */
const cls_master_connection_t * cls_iefb_get_master_connection_details (
   cls_t * cls);

/************ Internal functions made available for tests *******************/

void cls_iefb_log_warning_once (
   cl_limiter_t * limiter,
   cls_cciefb_logwarning_message_t message,
   uint32_t arg_num_a,
   uint16_t arg_num_b,
   uint16_t arg_num_c,
   uint32_t now);

int cls_iefb_copy_cyclic_data_from_request (
   cls_t * cls,
   const cls_cciefb_cyclic_request_info_t * request,
   uint16_t start_number,
   bool transmission_bit);

bool cls_iefb_is_master_id_correct (cls_t * cls, cl_ipaddr_t master_id);

void cls_iefb_filter_master_state_callback_values (
   bool connected_to_master,
   uint16_t protocol_ver,
   uint16_t master_application_status,
   bool * filtered_connected_and_running,
   bool * filtered_stopped_by_user,
   uint16_t * filtered_protocol_ver,
   uint16_t * filtered_master_application_status);

bool cls_iefb_should_trigger_master_state_callback (
   bool executed_before,
   bool connected_to_master,
   bool previous_connected_to_master,
   uint16_t protocol_ver,
   uint16_t previous_protocol_ver,
   uint16_t master_application_status,
   uint16_t previous_master_application_status);

int cls_iefb_search_slave_parameters (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * cyclic_request);

cls_slave_event_t cls_iefb_slave_init (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event);

cls_slave_event_t cls_iefb_handle_wrong_master (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event);

cls_slave_event_t cls_iefb_handle_wrong_stationcount (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event);

cls_slave_event_t cls_iefb_handle_incoming_when_disabled (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event);

cls_slave_event_t cls_iefb_handle_new_or_updated_master (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event);

cls_slave_event_t cls_iefb_handle_cyclic_event (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event);

cls_slave_event_t cls_iefb_log_timeout_master (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event);

cls_slave_event_t cls_iefb_log_ip_addr_updated (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event);

cls_slave_event_t cls_iefb_log_slave_disabled (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event);

cls_slave_event_t cls_iefb_log_slave_reenabled (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event);

cls_slave_event_t cls_iefb_on_entry_master_none (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event);

cls_slave_event_t cls_iefb_on_entry_wait_disabling_slave (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event);

cls_slave_event_t cls_iefb_on_entry_slave_disabled (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event);

void cls_iefb_fsm_event (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event);

void cls_fsm_init (cls_t * cls, uint32_t now);

#ifdef __cplusplus
}
#endif

#endif /* CLS_CCIEFB_H */
