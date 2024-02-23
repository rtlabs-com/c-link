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

#ifndef CLM_CCIEFB_H
#define CLM_CCIEFB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "clm_api.h"
#include "common/cl_types.h"

/**
 * Initialise master CCIEFB
 *
 * @param clm              c-link master stack instance handle
 * @param now              timestamp in microseconds
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CL_UDP_01
 * @req REQ_CL_PROTOCOL_20
 */
int clm_iefb_init (clm_t * clm, uint32_t now);

/**
 * Exit master CCIEFB
 *
 * @param clm              c-link master stack instance handle
 */
void clm_iefb_exit (clm_t * clm);

/**
 * Execute master CCIEFB internals.
 *
 * Use this function every tick.
 *
 * @param clm              c-link master stack instance handle
 * @param now              timestamp in microseconds
 */
void clm_iefb_periodic (clm_t * clm, uint32_t now);

/**
 * Set the master application status.
 *
 * @param clm              c-link master stack instance handle
 * @param running          True if running, false if stopped.
 * @param stopped_by_user  True if stopped by user, false if stopped by error.
 *                         Will only have an effect when `running` is false and
 *                         for protocol version 2 or later.
 */
void clm_iefb_set_master_application_status (
   clm_t * clm,
   bool running,
   bool stopped_by_user);

/**
 * Read back the master application status.
 *
 * Bit 0: 1=running, 0=stopped
 * Bit 1: 1=stopped by user, 0=stopped by error  (version 2 and later)
 *
 * @param clm              c-link master stack instance handle
 * @return Master local unit info
 */
uint16_t clm_iefb_get_master_application_status (clm_t * clm);

/**
 * Set the slave communication status.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param enabled                True if the cyclic data communication is
 *                               enabled, false otherwise
 * @return 0 on success, or -1 on illegal argument values.
 *
 * @req REQ_CLM_CONFORMANCE_04
 */
int clm_iefb_set_slave_communication_status (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   bool enabled);

/**
 * Force the cyclic transmission bit for a specific slave device.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param force                  True if the corresponding bit should be forced,
 *                               or false to stop forcing the bit value.
 * @return 0 on success, or -1 on illegal argument values.
 */
int clm_iefb_force_cyclic_transmission_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   bool force);

/**
 * Calculate the total number of occupied slave stations in a group.
 *
 * The group setting should already be validated.
 *
 * @param group_setting           Settings for a group
 * @return  The total number of occupied slave stations
 *
 * @req REQ_CL_PROTOCOL_31
 */
uint16_t clm_iefb_calc_occupied_per_group (
   const clm_group_setting_t * group_setting);

/**
 * Get a pointer to the first RX memory area for a group.
 *
 * @param clm              c-link slave stack instance handle
 * @param group_index      Group index (starts from 0).
 * @param total_occupied   Resulting number of total occupied stations for
 *                         this group. 0 on error.
 * @return                 Pointer to first memory area
 */
const cl_rx_t * clm_iefb_get_first_rx_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied);

/**
 * Get a pointer to the first RY memory area for a group.
 *
 * @param clm              c-link slave stack instance handle
 * @param group_index      Group index (starts from 0).
 * @param total_occupied   Resulting number of total occupied stations for
 *                         this group. 0 on error.
 * @return                 Pointer to first memory area
 */
cl_ry_t * clm_iefb_get_first_ry_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied);

/**
 * Get a pointer to the first RWr memory area for a group.
 *
 * @param clm              c-link slave stack instance handle
 * @param group_index      Group index (starts from 0).
 * @param total_occupied   Resulting number of total occupied stations for
 *                         this group. 0 on error.
 * @return                 Pointer to first memory area
 */
const cl_rwr_t * clm_iefb_get_first_rwr_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied);

/**
 * Get a pointer to the first RWw memory area for a group.
 *
 * @param clm              c-link slave stack instance handle
 * @param group_index      Group index (starts from 0).
 * @param total_occupied   Resulting number of total occupied stations for
 *                         this group. 0 on error.
 * @return                 Pointer to first memory area
 */
cl_rww_t * clm_iefb_get_first_rww_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied);

/**
 * Get a pointer to the first RX memory area for a device.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index           Device index in group (starts from 0).
 * @param num_occupied_stations  Resulting number of occupied stations for
 *                               this device. 0 on error.
 * @return                       Pointer to first memory area, or NULL on error
 */
const cl_rx_t * clm_iefb_get_first_device_rx_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations);

/**
 * Get a pointer to the first RY memory area for a device.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index           Device index in group (starts from 0).
 * @param num_occupied_stations  Resulting number of occupied stations for
 *                               this device. 0 on error.
 * @return                       Pointer to first memory area, or NULL on error
 */
cl_ry_t * clm_iefb_get_first_device_ry_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations);

/**
 * Get a pointer to the first RWr memory area for a device.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index           Device index in group (starts from 0).
 * @param num_occupied_stations  Resulting number of occupied stations for
 *                               this device. 0 on error.
 * @return                       Pointer to first memory area, or NULL on error
 */
const cl_rwr_t * clm_iefb_get_first_device_rwr_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations);

/**
 * Get a pointer to the first RWw memory area for a device.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index           Device index in group (starts from 0).
 * @param num_occupied_stations  Resulting number of occupied stations for
 *                               this device. 0 on error.
 * @return                       Pointer to first memory area, or NULL on error
 */
cl_rww_t * clm_iefb_get_first_device_rww_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations);

/**
 * Read an individual bit (RX) from a slave.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index           Device index in group (starts from 0).
 * @param number                 Bit number. Use 0 for RX0.
 * @return the bit value
 */
bool clm_iefb_get_rx_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number);

/**
 * Set an individual bit (RY) for sending to a slave.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index           Device index in group (starts from 0).
 * @param number                 Bit number. Use 0 for RY0.
 * @param value                  Value to send to the slave
 */
void clm_iefb_set_ry_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number,
   bool value);

/**
 * Read back an individual bit (RY, for sending to a slave).
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index           Device index in group (starts from 0).
 * @param number                 Bit number. Use 0 for RY0.
 * @return the bit value
 */
bool clm_iefb_get_ry_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number);

/**
 * Read a 16-bit register value (RWr) from a slave.
 *
 * The endianness is handled automatically.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index           Device index in group (starts from 0).
 * @param number                 Register number. Use 0 for RWr0.
 * @return the register value
 */
uint16_t clm_iefb_get_rwr_value (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number);

/**
 * Set a 16-bit register value (RWw) for sending to a slave.
 *
 * The endianness is handled automatically.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index           Device index in group (starts from 0).
 * @param number                 Register number. Use 0 for RWw0.
 * @param value                  Value to send to the PLC
 */
void clm_iefb_set_rww_value (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number,
   uint16_t value);

/**
 * Read back a 16-bit register value (RWw, for sending to a slave).
 *
 * The endianness is handled automatically.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index           Device index in group (starts from 0).
 * @param number                 Register number. Use 0 for RWw0.
 * @return the register value
 */
uint16_t clm_iefb_get_rww_value (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number);

/**
 * Clear statistics for communication with slave devices
 *
 * This clears all statistics.
 *
 * @param clm                    c-link master stack instance handle
 */
void clm_iefb_statistics_clear_all (clm_t * clm);

/**
 * Read out master internal details.
 *
 * This function is intended for debugging and for automated tests.
 *
 * Copies the information to the resulting struct.
 *
 * @param clm                    c-link master stack instance handle
 * @param details                Resulting details
 * @return 0 on success, or -1 on error.
 */
int clm_iefb_get_master_status (
   const clm_t * clm,
   clm_master_status_details_t * details);

/**
 * Read out details about the group of slave devices.
 *
 * This function is intended for debugging and for automated tests.
 *
 * Copies the information to the resulting struct.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param details                Resulting details
 * @return 0 on success, or -1 on error.
 */
int clm_iefb_get_group_status (
   const clm_t * clm,
   uint16_t group_index,
   clm_group_status_details_t * details);

/**
 * Get a pointer to connection details for a slave device (stored in master).
 *
 * This function is intended for debugging and for automated tests.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @return Pointer to slave device connection details, or NULL on error.
 *         No copying is done.
 *
 * @req REQ_CLM_DIAGNOSIS_02
 *
 */
const clm_slave_device_data_t * clm_iefb_get_device_connection_details (
   const clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index);

/************ Internal functions made available for tests *****************/

void clm_iefb_update_frame_sequence_no (uint16_t * frame_sequence_no);

void clm_iefb_initialise_request_slave_ids (
   uint32_t * first_slave_id,
   const clm_group_setting_t * group_setting);

uint16_t clm_iefb_calc_master_local_unit_info (
   uint16_t protocol_ver,
   bool running,
   bool stopped_by_user);

int clm_iefb_calc_slave_station_no (
   const clm_group_setting_t * group_setting,
   uint16_t slave_device_index,
   uint16_t * slave_station_no);

int clm_iefb_calc_slave_device_index (
   const clm_group_setting_t * group_setting,
   cl_ipaddr_t ip_addr,
   uint16_t * slave_device_index);

void clm_iefb_update_request_slave_id (
   uint32_t * first_slave_id,
   uint16_t slave_station_no,
   bool enable,
   cl_ipaddr_t slave_id);

bool clm_iefb_group_have_received_from_all_devices (
   const clm_group_setting_t * group_setting,
   const clm_group_data_t * group_data);

void clm_iefb_reflect_group_parameters (clm_t * clm, clm_group_data_t * group_data);

void clm_iefb_group_fsm_tables_init (clm_t * clm);

void clm_iefb_device_fsm_tables_init (clm_t * clm);

void clm_iefb_group_fsm_event (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_group_event_t event);

void clm_iefb_device_fsm_event (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_slave_device_data_t * slave_device_data,
   clm_device_event_t event);

void clm_iefb_statistics_update_response_time (
   clm_slave_device_statistics_t * statistics,
   uint16_t max_number_of_samples,
   uint32_t response_time);

void clm_iefb_statistics_clear (clm_slave_device_statistics_t * statistics);

void clm_iefb_monitor_all_group_timers (clm_t * clm, uint32_t now);

#ifdef __cplusplus
}
#endif

#endif /* CLM_CCIEFB_H */
