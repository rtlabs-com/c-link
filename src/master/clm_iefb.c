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
 * @brief Implementing the master state machines etc required for handling the
 *        cyclic data communication with the slaves.
 *
 *        See common/cl_iefb.h for building and parsing CCIEFB frames */

#ifdef UNIT_TEST
#define clal_udp_open              mock_clal_udp_open
#define clal_udp_recvfrom          mock_clal_udp_recvfrom
#define clal_udp_sendto            mock_clal_udp_sendto
#define clal_udp_close             mock_clal_udp_close
#define clal_get_unix_timestamp_ms mock_clal_get_unix_timestamp_ms
#endif

#include "master/clm_iefb.h"

#include "common/cl_eth.h"
#include "common/cl_file.h"
#include "common/cl_iefb.h"
#include "common/cl_literals.h"
#include "common/cl_timer.h"
#include "common/cl_types.h"
#include "common/cl_util.h"

#include "osal_log.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define LOG_DISABLED(type, ...)

#define CLM_CCIEFB_ERRORCALLBACK_RETRIGGER_PERIOD 1000000 /* microseconds */

/**
 * Calculate next frame sequence number.
 *
 * @param frame_sequence_no   Frame sequence number to be updated
 *
 * @req REQ_CLM_SEQUENCE_02
 */
void clm_iefb_update_frame_sequence_no (uint16_t * frame_sequence_no)
{
   /* The frame sequence number starts at 0 and increases to UINT16_MAX.
      Then it starts over from 1 (not from 0) */

   if (*frame_sequence_no == UINT16_MAX)
   {
      *frame_sequence_no = 1;
      return;
   }

   *frame_sequence_no += 1;
}

/**
 * Calculate master local unit info (whether master application is running)
 *
 * @param protocol_ver     Protocol version. No validation is done.
 * @param running          True if master application is running, false if
 *                         stopped.
 * @param stopped_by_user  True if stopped by user, false if stopped by error.
 * @return The master local unit info
 *
 * @req REQ_CL_PROTOCOL_13
 * @req REQ_CL_PROTOCOL_14
 * @req REQ_CL_PROTOCOL_15
 * @req REQ_CL_PROTOCOL_16
 * @req REQ_CL_PROTOCOL_56
 *
 */
uint16_t clm_iefb_calc_master_local_unit_info (
   uint16_t protocol_ver,
   bool running,
   bool stopped_by_user)
{
   if (running)
   {
      return CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING;
   }

   if (protocol_ver < 2 || stopped_by_user == false)
   {
      return CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED;
   }

   return CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED_BY_USER;
}

uint16_t clm_iefb_calc_occupied_per_group (const clm_group_setting_t * group_setting)
{
   uint16_t slave_device_index       = 0;
   uint16_t total_occupied_per_group = 0;
   const clm_slave_device_setting_t * slave_device_setting;

   for (slave_device_index = 0;
        slave_device_index < group_setting->num_slave_devices;
        slave_device_index++)
   {
      slave_device_setting = &group_setting->slave_devices[slave_device_index];
      total_occupied_per_group += slave_device_setting->num_occupied_stations;
   }

   return total_occupied_per_group;
}

/**
 * Calculate slave station number for a device
 *
 * @param group_setting          Group settings
 * @param slave_device_index     Index of the device in the group. Starts at 0.
 * @param slave_station_no       Resulting slave station number. Starts at 1.
 * @return 0 on success, -1 on error
 */
int clm_iefb_calc_slave_station_no (
   const clm_group_setting_t * group_setting,
   uint16_t slave_device_index,
   uint16_t * slave_station_no)
{
   uint16_t current_slave_device_index = 0;
   uint16_t station_no                 = 1;
   const clm_slave_device_setting_t * slave_device_setting;

   if (slave_device_index >= group_setting->num_slave_devices)
   {
      return -1;
   }

   /* Count the occupied stations for devices with lower index */
   for (current_slave_device_index = 0;
        current_slave_device_index < slave_device_index;
        current_slave_device_index++)
   {
      slave_device_setting =
         &group_setting->slave_devices[current_slave_device_index];

      station_no += slave_device_setting->num_occupied_stations;
   }

   *slave_station_no = station_no;

   return 0;
}

/**
 * Find device index from an IP address
 *
 * @param group_setting          Group settings
 * @param ip_addr                IP address
 * @param slave_device_index     Resulting slave device index
 * @return 0 on success, -1 if the IP address not is found
 */
int clm_iefb_calc_slave_device_index (
   const clm_group_setting_t * group_setting,
   cl_ipaddr_t ip_addr,
   uint16_t * slave_device_index)
{
   uint16_t index = 0;
   const clm_slave_device_setting_t * slave_device_setting;

   for (index = 0; index < group_setting->num_slave_devices; index++)
   {
      slave_device_setting = &group_setting->slave_devices[index];

      if (slave_device_setting->slave_id == ip_addr)
      {
         *slave_device_index = index;

         return 0;
      }
   }

   return -1;
}

/**
 * Check if we received responses from all (enabled) slave devices in the group
 *
 * When arriving here, the devices are typically in one of these states:
 * - CLM_DEVICE_STATE_WAIT_TD
 * - CLM_DEVICE_STATE_CYCLIC_SUSPEND
 * - CLM_DEVICE_STATE_CYCLIC_SENT
 * - CLM_DEVICE_STATE_CYCLIC_SENDING
 *
 * After the first request frame (where frame_sequence_no == 0) we should wait
 * if there is any slave in state CLM_DEVICE_STATE_WAIT_TD (which means not yet
 * responded). However, if all slaves are disabled we should wait.
 *
 * For consecutive frames we should wait if any of the connected devices (those
 * with transmission bit on) has not yet responded in this link scan (if they
 * still are in state CLM_DEVICE_STATE_CYCLIC_SENDING). However we should also
 * wait if there is no connected device at all.
 *
 * We do not wait for recently re-enabled slaves. They might respond late, but
 * will be re-enabled in the next link scan (even if the sequence number is
 * wrong).
 *
 * Note that the slave_device_data->enabled field affects which transitions
 * should occur, but does not affect the checks in this function.
 *
 * @param group_setting    Group setting
 * @param group_data       Group data
 * @return true if we have received data from all enabled slave devices,
 *         or false if not (then we should wait for more responses)
 *
 * @req REQ_CLM_TIMING_07
 * @req REQ_CLM_TIMING_08
 * @req REQ_CLM_TIMING_09
 * @req REQ_CLM_TIMING_10
 *
 */
bool clm_iefb_group_have_received_from_all_devices (
   const clm_group_setting_t * group_setting,
   const clm_group_data_t * group_data)
{
   uint16_t slave_device_index = 0;
   bool has_received           = false;
   const clm_slave_device_data_t * slave_device_data;
   bool first_frame = group_data->frame_sequence_no == 0;

   if (first_frame)
   {
      for (slave_device_index = 0;
           slave_device_index < group_setting->num_slave_devices;
           slave_device_index++)
      {
         slave_device_data = &group_data->slave_devices[slave_device_index];

         if (slave_device_data->device_state == CLM_DEVICE_STATE_WAIT_TD)
         {
            return false;
         }

         if (slave_device_data->device_state == CLM_DEVICE_STATE_CYCLIC_SENT)
         {
            has_received = true;
         }
      }

      return has_received ? true : false;
   }

   /* Consecutive frames */
   for (slave_device_index = 0;
        slave_device_index < group_setting->num_slave_devices;
        slave_device_index++)
   {
      slave_device_data = &group_data->slave_devices[slave_device_index];

      if (slave_device_data->transmission_bit)
      {
         has_received = true;
      }

      if (slave_device_data->transmission_bit && slave_device_data->device_state == CLM_DEVICE_STATE_CYCLIC_SENDING)
      {
         return false;
      }
   }

   return has_received ? true : false;
}

/**
 * Send a CCIEFB data request frame for one group.
 *
 * Update outgoing frame in buffer.
 *
 * @param socket                 Socket handle
 * @param destination_ip_addr    Destination IP address
 * @param group_data             Runtime data for one group (has frame
 *                               buffer)
 * @param now                    Timestamp in microseconds
 * @param unix_timestamp_ms      Unix timestamp in ms, or 0 if not available
 * @param master_local_unit_info Is master application running or not. See
 *                               CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_xxx
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CL_UDP_01
 * @req REQ_CLM_COMMUNIC_01
 * @req REQ_CLM_CONFORMANCE_01
 *
 */
static int clm_iefb_send_cyclic_request_frame (
   int socket,
   cl_ipaddr_t destination_ip_addr,
   clm_group_data_t * group_data,
   uint32_t now,
   uint64_t unix_timestamp_ms,
   uint16_t master_local_unit_info)
{
   ssize_t sent_size = 0;

   cl_iefb_update_request_frame_headers (
      &group_data->req_frame,
      group_data->frame_sequence_no,
      unix_timestamp_ms,
      master_local_unit_info,
      group_data->cyclic_transmission_state);

   sent_size = clal_udp_sendto (
      socket,
      destination_ip_addr,
      CL_CCIEFB_PORT,
      group_data->req_frame.buffer,
      group_data->req_frame.udp_payload_len);

   if (sent_size < 0 || (size_t)sent_size != group_data->req_frame.udp_payload_len)
   {
      return -1;
   }

   return 0;
}

/**
 * Store the slave IDs in an outgoing request frame buffer
 *
 * @param first_slave_id   Pointer to buffer with little-endian IP addresses.
 * @param group_setting    Configuration values for this group
 *
 * @req REQ_CL_PROTOCOL_34
 * @req REQ_CL_PROTOCOL_52
 *
 */
void clm_iefb_initialise_request_slave_ids (
   uint32_t * first_slave_id,
   const clm_group_setting_t * group_setting)
{
   /* The IP address for each slave is put in the frame buffer.
      If a slave device occupies more than one slave station, its remaining
      positions are filled with 255.255.255.255 */

   uint16_t slave_device_index = 0;
   uint16_t occupied_index     = 0;
   uint16_t pos                = 0;
   const clm_slave_device_setting_t * slave_device_setting;
   cl_ipaddr_t * id_value_pointer;

   for (slave_device_index = 0;
        slave_device_index < group_setting->num_slave_devices;
        slave_device_index++)
   {
      slave_device_setting = &group_setting->slave_devices[slave_device_index];

      for (occupied_index = 0;
           occupied_index < slave_device_setting->num_occupied_stations;
           occupied_index++)
      {
         id_value_pointer = first_slave_id + pos;

         if (occupied_index == 0)
         {
            *id_value_pointer = CC_TO_LE32 (slave_device_setting->slave_id);
         }
         else
         {
            *id_value_pointer = CC_TO_LE32 (CL_CCIEFB_MULTISTATION_INDICATOR);
         }

         pos++;
      }
   }
}

/**
 * Update the slave ID in an outgoing request frame buffer, for one slave
 * device
 *
 * A disabled slave will have its IP address replaced with 0.0.0.0
 *
 * @param first_slave_id         Pointer to buffer with little-endian IP
 *                               addresses.
 * @param slave_station_no       Slave station number. First station is 1.
 * @param enable                 Insert the IP address (instead of 0.0.0.0)
 * @param slave_id               Slave IP address to be written
 *
 * @req REQ_CL_PROTOCOL_37
 *
 */
void clm_iefb_update_request_slave_id (
   uint32_t * first_slave_id,
   uint16_t slave_station_no,
   bool enable,
   cl_ipaddr_t slave_id)
{
   uint16_t pos = 0;
   cl_ipaddr_t * id_value_pointer;

   CC_ASSERT (slave_station_no >= 1);
   pos = slave_station_no - 1;

   id_value_pointer = first_slave_id + pos;

   if (enable)
   {
      *id_value_pointer = CC_TO_LE32 (slave_id);
   }
   else
   {
      *id_value_pointer = CC_TO_LE32 (CL_IPADDR_INVALID);
   }
}

/**
 * Update the cyclic data (RY and RWw) in an outgoing request frame, for one
 * device (all of its occupied stations).
 *
 * No validation is done on input parameters.
 *
 * Copying is done from \a group_cyclic_data to \a req_frame
 *
 * @param req_frame              Frame to be updated
 * @param group_cyclic_data      Memory area with cyclic data
 * @param slave_station_no       Slave station number. First station is 1.
 * @param num_occupied_stations  Number of stations this device occupies
 * @param valid                  True if data should be written, or false if
 *                               zeros should be written to the frame.
 */
static void clm_iefb_update_request_frame_cyclic_data_one_device (
   clm_cciefb_cyclic_request_info_t * req_frame,
   const clm_group_memory_area_t * group_cyclic_data,
   uint16_t slave_station_no,
   uint16_t num_occupied_stations,
   bool valid)
{
   const uint16_t start_index = slave_station_no - 1;
   const size_t num_rww_bytes = num_occupied_stations * sizeof (cl_rww_t);
   const size_t num_ry_bytes  = num_occupied_stations * sizeof (cl_ry_t);

   if (valid)
   {
      clal_memcpy (
         &req_frame->first_rww[start_index],
         num_rww_bytes,
         &group_cyclic_data->rww[start_index],
         num_rww_bytes);
      clal_memcpy (
         &req_frame->first_ry[start_index],
         num_ry_bytes,
         &group_cyclic_data->ry[start_index],
         num_ry_bytes);
   }
   else
   {
      clal_clear_memory (&req_frame->first_rww[start_index], num_rww_bytes);
      clal_clear_memory (&req_frame->first_ry[start_index], num_ry_bytes);
   }
}

/**
 * Store runtime info from incoming response frame headers.
 *
 * Converts endianness.
 *
 * @param cyclic_response        Incoming response frame
 * @param transmission_timestamp Timestamp when corresponding request was sent
 * @param framevalues            Info to be updated
 *
 * @req REQ_CL_PROTOCOL_51
 */
static void clm_iefb_store_incoming_slave_runtime_info (
   const clm_cciefb_cyclic_response_info_t * cyclic_response,
   uint32_t transmission_timestamp,
   clm_device_framevalues_t * framevalues)
{
   cl_cciefb_cyclic_resp_full_headers_t * headers = cyclic_response->full_headers;

   framevalues->has_been_received = true;
   framevalues->response_time =
      cyclic_response->reception_timestamp - transmission_timestamp;
   framevalues->end_code = CC_FROM_LE16 (headers->cyclic_header.end_code);
   framevalues->num_occupied_stations = cyclic_response->number_of_occupied;
   framevalues->protocol_ver = CC_FROM_LE16 (headers->cyclic_header.protocol_ver);
   framevalues->vendor_code =
      CC_FROM_LE16 (headers->slave_station_notification.vendor_code);
   framevalues->model_code =
      CC_FROM_LE32 (headers->slave_station_notification.model_code);
   framevalues->equipment_ver =
      CC_FROM_LE16 (headers->slave_station_notification.equipment_ver);
   framevalues->slave_local_unit_info =
      CC_FROM_LE16 (headers->slave_station_notification.slave_local_unit_info);
   framevalues->local_management_info =
      CC_FROM_LE32 (headers->slave_station_notification.local_management_info);
   framevalues->slave_err_code =
      CC_FROM_LE16 (headers->slave_station_notification.slave_err_code);
   framevalues->slave_id = CC_FROM_LE32 (headers->cyclic_data_header.slave_id);
   framevalues->group_no = headers->cyclic_data_header.group_no; /* uint8_t */
   framevalues->frame_sequence_no =
      CC_FROM_LE16 (headers->cyclic_data_header.frame_sequence_no);
}

/**
 * Update the statistics regarding the response time for a slave device
 *
 * The statistics will no longer be modified when we reach max_number_of_samples
 * or if the sum of the previous values is too large.
 *
 * @param statistics             Statistics details to be updated
 * @param max_number_of_samples  Max number of samples to consider
 * @param response_time          Response_time, in microseconds
 */
void clm_iefb_statistics_update_response_time (
   clm_slave_device_statistics_t * statistics,
   uint16_t max_number_of_samples,
   uint32_t response_time)
{
   if (
      statistics->measured_time.number_of_samples < max_number_of_samples &&
      statistics->measured_time.sum < UINT32_MAX / 2)
   {
      statistics->measured_time.number_of_samples++;

      statistics->measured_time.max =
         MAX (response_time, statistics->measured_time.max);

      statistics->measured_time.min =
         MIN (response_time, statistics->measured_time.min);

      statistics->measured_time.sum += response_time;

      statistics->measured_time.average =
         statistics->measured_time.sum /
         statistics->measured_time.number_of_samples;
   }
}

/**
 * Clear the statistics for a slave device
 *
 * @param statistics       Statistics details
 */
void clm_iefb_statistics_clear (clm_slave_device_statistics_t * statistics)
{
   clal_clear_memory (statistics, sizeof (*statistics));
   statistics->measured_time.min = UINT32_MAX;
}

/**
 * Clear the storage of the latest received frame headers, for a slave device
 *
 * @param latest_frame     Details from header of latest received frame
 */
static void clm_iefb_latest_received_clear (clm_device_framevalues_t * latest_frame)
{
   clal_clear_memory (latest_frame, sizeof (*latest_frame));
}

/**
 * Store cyclic data (RX and RWw) from incoming response frame.
 *
 * Copying is done from \a cyclic_response to \a group_cyclic_data
 *
 * @param group_cyclic_data      Stored cyclic data to be updated
 * @param cyclic_response        Incoming response frame
 * @param slave_station_no       Slave station number. Starts at 1.
 * @param num_occupied_stations  Number of occupied stations for slave device
 * @param valid                  True if incoming data should be stored,
 *                               or false if zeros should be stored
 *
 * @req REQ_CLM_STATUSBIT_04
 * @req REQ_CLM_STATUSBIT_06
 * @req REQ_CLM_STATUSBIT_09
 */
static void clm_iefb_store_incoming_cyclic_data (
   clm_group_memory_area_t * group_cyclic_data,
   const clm_cciefb_cyclic_response_info_t * cyclic_response,
   uint16_t slave_station_no,
   uint16_t num_occupied_stations,
   bool valid)
{
   const uint16_t first_index = slave_station_no - 1;
   const size_t num_rwr_bytes = num_occupied_stations * sizeof (cl_rwr_t);
   const size_t num_rx_bytes  = num_occupied_stations * sizeof (cl_rx_t);

   if (valid)
   {
      clal_memcpy (
         &group_cyclic_data->rwr[first_index],
         num_rwr_bytes,
         cyclic_response->first_rwr,
         num_rwr_bytes);
      clal_memcpy (
         &group_cyclic_data->rx[first_index],
         num_rx_bytes,
         cyclic_response->first_rx,
         num_rx_bytes);
   }
   else
   {
      clal_clear_memory (&group_cyclic_data->rwr[first_index], num_rwr_bytes);
      clal_clear_memory (&group_cyclic_data->rx[first_index], num_rx_bytes);
   }
}

/**
 * Set the master state, for informing the application.
 *
 * The master state is a simplification of the group state, to make it
 * easy for the application to know whether we are in arbitration state
 * etc.
 *
 * Will call an application callback, if implemented.
 *
 * @param clm              c-link master stack instance handle
 * @param new_state        New master state
 */
static void clm_iefb_set_master_state (clm_t * clm, clm_master_state_t new_state)
{
   if (new_state != clm->master_state)
   {
#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): %s user state-change callback with "
         "state %s\n",
         __LINE__,
         (clm->config.state_cb != NULL) ? "Calling" : "No",
         cl_literals_get_master_state (new_state));
#endif
      if (clm->config.state_cb != NULL)
      {
         clm->config.state_cb (clm, clm->config.cb_arg, new_state);
      }

      clm->master_state = new_state;
   }
}

/**
 * Trigger the link scan application callback, if implemented.
 *
 * @param clm              c-link master stack instance handle
 * @param group_data       Group data
 * @param success          True if all slave devices have responded, or
 *                         false otherwise.
 */
static void clm_iefb_trigger_linkscan_callback (
   clm_t * clm,
   clm_group_data_t * group_data,
   bool success)
{
   if (clm->config.linkscan_cb != NULL)
   {
      clm->config.linkscan_cb (
         clm,
         clm->config.cb_arg,
         group_data->group_index,
         success);
   }
}

/**
 * Trigger the error application callback, if implemented.
 *
 * See \a clm_error_ind_t() for details on argument value combinations.
 *
 * If several similar messages are triggered repeatedly (interval max
 * CLM_CCIEFB_ERRORCALLBACK_RETRIGGER_PERIOD microseconds), then
 * the callback is triggered once only.
 *
 * @param clm              c-link master stack instance handle
 * @param now              Current timestamp, in microseconds
 * @param error_message    Error message enum
 * @param ip_addr          IP address, if available
 * @param argument_2       Numeric argument, if available
 */
static void clm_iefb_trigger_error_callback (
   clm_t * clm,
   uint32_t now,
   clm_error_message_t error_message,
   cl_ipaddr_t ip_addr,
   uint16_t argument_2)
{
   if (!cl_limiter_should_run_now (&clm->errorlimiter, (int)error_message, now))
   {
      return;
   }

#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
   cl_util_ip_to_string (ip_addr, ip_string);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): %s user callback for error: %s  IP %s "
      "Argument 2 (if any): %u\n",
      __LINE__,
      (clm->config.error_cb != NULL) ? "Calling" : "No",
      cl_literals_get_master_error_message (error_message),
      ip_string,
      argument_2);
#endif
   if (clm->config.error_cb != NULL)
   {
      clm->config
         .error_cb (clm, clm->config.cb_arg, error_message, ip_addr, argument_2);
   }
}

/**
 * Trigger the connect callback, if implemented.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_data             Group data
 * @param slave_device_data      Runtime data for a slave device
 */
static void clm_iefb_trigger_connect_callback (
   clm_t * clm,
   clm_group_data_t * group_data,
   clm_slave_device_data_t * slave_device_data)
{
#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
#endif

   clm_group_setting_t * group_setting =
      &clm->config.hier.groups[group_data->group_index];
   clm_slave_device_setting_t * slave_device_setting =
      &group_setting->slave_devices[slave_device_data->device_index];

#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   cl_util_ip_to_string (slave_device_setting->slave_id, ip_string);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): %s user callback for connect. Group index %u, "
      "slave device index %u. Slave IP address %s\n",
      __LINE__,
      (clm->config.connect_cb != NULL) ? "Calling" : "No",
      group_data->group_index,
      slave_device_data->device_index,
      ip_string);
#endif

   if (clm->config.connect_cb != NULL)
   {
      clm->config.connect_cb (
         clm,
         clm->config.cb_arg,
         group_data->group_index,
         slave_device_data->device_index,
         slave_device_setting->slave_id);
   }
}

/**
 * Trigger the disconnect callback, if implemented.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_data             Group data
 * @param slave_device_data      Runtime data for a slave device
 */
static void clm_iefb_trigger_disconnect_callback (
   clm_t * clm,
   clm_group_data_t * group_data,
   clm_slave_device_data_t * slave_device_data)
{
#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
#endif

   clm_group_setting_t * group_setting =
      &clm->config.hier.groups[group_data->group_index];
   clm_slave_device_setting_t * slave_device_setting =
      &group_setting->slave_devices[slave_device_data->device_index];

#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   cl_util_ip_to_string (slave_device_setting->slave_id, ip_string);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): %s user callback for disconnect. Group index %u, "
      "slave device index %u. Slave IP address %s\n",
      __LINE__,
      (clm->config.disconnect_cb != NULL) ? "Calling" : "No",
      group_data->group_index,
      slave_device_data->device_index,
      ip_string);
#endif

   if (clm->config.disconnect_cb != NULL)
   {
      clm->config.disconnect_cb (
         clm,
         clm->config.cb_arg,
         group_data->group_index,
         slave_device_data->device_index,
         slave_device_setting->slave_id);
   }
}

/**
 * Trigger the 'updated slaveinfo' callback, if implemented.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index
 * @param slave_device_index     Slave device index
 * @param end_code               End code from slave
 * @param slave_err_code         Slave error code
 * @param local_management_info  Local management info
 */
static void clm_iefb_trigger_slaveinfo_callback (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t end_code,
   uint16_t slave_err_code,
   uint32_t local_management_info)
{
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): %s user callback for incoming change in slave"
      " info (end_code = 0x%X, slave_err_code = 0x%X, local_management_info = "
      "0x%X)\n",
      __LINE__,
      (clm->config.changed_slave_info_cb != NULL) ? "Calling" : "No",
      end_code,
      slave_err_code,
      (unsigned int)local_management_info);
   if (clm->config.changed_slave_info_cb != NULL)
   {
      clm->config.changed_slave_info_cb (
         clm,
         clm->config.cb_arg,
         group_index,
         slave_device_index,
         end_code,
         slave_err_code,
         local_management_info);
   }
}

/**
 * Trigger the alarm frame callback, if implemented.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index
 * @param slave_device_index     Slave device index
 * @param end_code               End code from slave
 * @param slave_err_code         Slave error code
 * @param local_management_info  Local management info
 */
static void clm_iefb_trigger_alarmframe_callback (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t end_code,
   uint16_t slave_err_code,
   uint32_t local_management_info)
{
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): %s user callback for alarm frame. Group index %u, "
      "slave device index %u. (end_code = 0x%X, slave_err_code = "
      "0x%X, local_management_info = 0x%X)\n",
      __LINE__,
      (clm->config.alarm_cb != NULL) ? "Calling" : "No",
      group_index,
      slave_device_index,
      end_code,
      slave_err_code,
      (unsigned int)local_management_info);
   if (clm->config.alarm_cb != NULL)
   {
      clm->config.alarm_cb (
         clm,
         clm->config.cb_arg,
         group_index,
         slave_device_index,
         end_code,
         slave_err_code,
         local_management_info);
   }
}

/**
 * Prepare group data, slave device data and outgoing frame buffer for a
 * group.
 *
 * This is done once the configuration settings are known.
 *
 * @param clm              c-link master stack instance handle
 * @param group_data       Group data to be updated
 *
 * @req REQ_CL_PARAMETERID_01
 * @req REQ_CL_PROTOCOL_30
 * @req REQ_CL_PROTOCOL_35
 * @req REQ_CLM_SEQUENCE_01
 * @req REQ_CLM_VERSION_02
 *
 */
void clm_iefb_reflect_group_parameters (clm_t * clm, clm_group_data_t * group_data)
{
   int result                  = 0;
   uint16_t slave_device_index = 0;
   const clm_group_setting_t * group_setting =
      &clm->config.hier.groups[group_data->group_index];
   clm_slave_device_data_t * slave_device_data;

   /* Update group data
      Note: group_data->group_index and ->group_state are already set */
   group_data->total_occupied = clm_iefb_calc_occupied_per_group (group_setting);
   group_data->frame_sequence_no = 0; /* Restart from zero */
   group_data->cyclic_transmission_state =
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF;

   /* Update slave device data (for all devices in group) */
   for (slave_device_index = 0;
        slave_device_index < group_setting->num_slave_devices;
        slave_device_index++)
   {
      slave_device_data = &group_data->slave_devices[slave_device_index];

      slave_device_data->timeout_count = 0;
      result                           = clm_iefb_calc_slave_station_no (
         group_setting,
         slave_device_index,
         &slave_device_data->slave_station_no);
      CC_ASSERT (result == 0);
   }

   /* Prepare the request frame for this group */
   cl_iefb_initialise_request_frame (
      group_data->sendbuf,
      sizeof (group_data->sendbuf),
      clm->config.protocol_ver,
      group_setting->timeout_value,
      group_setting->parallel_off_timeout_count,
      clm->config.master_id,
      group_data->group_index + 1,
      group_data->total_occupied,
      clm->parameter_no,
      &group_data->req_frame);
   clm_iefb_initialise_request_slave_ids (
      group_data->req_frame.first_slave_id,
      group_setting);
}

/*********************** Device state machine *****************************/

/**
 * Initialise a slave device representation.
 *
 * This is a callback for the device state machine.
 *
 * Known as InitMaster() [for device representation] in the specification
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param slave_device_data      Runtime data for a slave device
 * @param event                  Triggering event
 * @param new_state              State we are about to enter
 * @return Next event to trigger
 */
static clm_device_event_t clm_iefb_device_init (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_slave_device_data_t * slave_device_data,
   clm_device_event_t event,
   clm_device_state_t new_state)
{
   /* Note: slave_device_data->device_index and device_state are already set */

   const clm_group_setting_t * group_setting =
      &clm->config.hier.groups[group_data->group_index];
   const clm_slave_device_setting_t * slave_device_setting =
      &group_setting->slave_devices[slave_device_data->device_index];

   slave_device_data->enabled = !slave_device_setting->reserved_slave_device;
   slave_device_data->transmission_bit = false;

   clm_iefb_statistics_clear (&slave_device_data->statistics);
   clm_iefb_latest_received_clear (&slave_device_data->latest_frame);

   slave_device_data->timeout_count = 0;

   /* This will be updated when we get a new config */
   slave_device_data->slave_station_no = 0;

   return CLM_DEVICE_EVENT_NONE;
}

/**
 * Set the outgoing data, slave IP address and transmission bit in outgoing
 * request frame for one device.
 *
 * This is a callback for the device state machine.
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param slave_device_data      Runtime data for a slave device
 * @param event                  Triggering event
 * @param new_state              State we are about to enter
 * @return Next event to trigger
 *
 * @req REQ_CLM_STATUSBIT_01
 * @req REQ_CLM_STATUSBIT_02
 * @req REQ_CLM_STATUSBIT_05
 * @req REQ_CLM_STATUSBIT_06
 * @req REQ_CLM_STATUSBIT_07
 *
 */
static clm_device_event_t clm_iefb_set_data_ip (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_slave_device_data_t * slave_device_data,
   clm_device_event_t event,
   clm_device_state_t new_state)
{
   bool combined_transmission_bit = false;
   clm_group_setting_t * group_setting =
      &clm->config.hier.groups[group_data->group_index];
   clm_slave_device_setting_t * slave_device_setting =
      &group_setting->slave_devices[slave_device_data->device_index];

   if (new_state == CLM_DEVICE_STATE_CYCLIC_SENDING)
   {
      if (!slave_device_data->transmission_bit)
      {
         slave_device_data->statistics.number_of_connects++;
         clm_iefb_trigger_connect_callback (clm, group_data, slave_device_data);
      }
      slave_device_data->transmission_bit = true;
   }
   else
   {
      if (slave_device_data->transmission_bit)
      {
         slave_device_data->statistics.number_of_disconnects++;
         clm_iefb_trigger_disconnect_callback (clm, group_data, slave_device_data);
      }
      slave_device_data->transmission_bit = false;
   }

   combined_transmission_bit = slave_device_data->transmission_bit ||
                               slave_device_data->force_transmission_bit;

   /* Set bit in cyclic_transmission_state */
   cl_iefb_set_cyclic_transmission_state (
      &group_data->cyclic_transmission_state,
      slave_device_data->slave_station_no,
      combined_transmission_bit);

   /* Set slave IP address or 0.0.0.0 in outgoing frame */
   clm_iefb_update_request_slave_id (
      group_data->req_frame.first_slave_id,
      slave_device_data->slave_station_no,
      new_state != CLM_DEVICE_STATE_CYCLIC_SUSPEND,
      slave_device_setting->slave_id);

   /* Set cyclic data */
   clm_iefb_update_request_frame_cyclic_data_one_device (
      &group_data->req_frame,
      &group_data->memory_area,
      slave_device_data->slave_station_no,
      slave_device_setting->num_occupied_stations,
      combined_transmission_bit);

   if (slave_device_data->enabled)
   {
      slave_device_data->statistics.number_of_sent_frames++;
   }

   return CLM_DEVICE_EVENT_NONE;
}

/**
 * Evaluate the number of timeouts for a slave device, and trigger more
 * events depending on the result.
 *
 * This is a callback for the device state machine.
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param slave_device_data      Runtime data for a slave device
 * @param event                  Triggering event
 * @param new_state              State we are about to enter
 * @return Next event to trigger
 *
 * @req REQ_CLM_ERROR_07
 * @req REQ_CLM_CONFORMANCE_06
 *
 */
static clm_device_event_t clm_iefb_evaluate_timeoutcounter (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_slave_device_data_t * slave_device_data,
   clm_device_event_t event,
   clm_device_state_t new_state)
{
   clm_group_setting_t * group_setting =
      &clm->config.hier.groups[group_data->group_index];

   slave_device_data->timeout_count++;
   if (slave_device_data->timeout_count >= group_setting->parallel_off_timeout_count)
   {
      slave_device_data->statistics.number_of_timeouts++;
      return CLM_DEVICE_EVENT_TIMEOUTCOUNTER_FULL;
   }

   return CLM_DEVICE_EVENT_TIMEOUTCOUNTER_NOT_FULL;
}

/**
 * Reset the timeout count for a slave device.
 *
 * This is a callback for the device state machine.
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param slave_device_data      Runtime data for a slave device
 * @param event                  Triggering event
 * @param new_state              State we are about to enter
 * @return Next event to trigger
 */
static clm_device_event_t clm_iefb_reset_timeout_count (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_slave_device_data_t * slave_device_data,
   clm_device_event_t event,
   clm_device_state_t new_state)
{
   slave_device_data->timeout_count = 0;

   return CLM_DEVICE_EVENT_NONE;
}

/** Device state machine transitions */
// clang-format off
const clm_device_fsm_transition_t device_transitions[] = {
   /* State MASTER_DOWN */
   {CLM_DEVICE_STATE_MASTER_DOWN, CLM_DEVICE_EVENT_GROUP_STARTUP, CLM_DEVICE_STATE_LISTEN, clm_iefb_device_init},

   /* State LISTEN */
   {CLM_DEVICE_STATE_LISTEN, CLM_DEVICE_EVENT_GROUP_STANDBY,           CLM_DEVICE_STATE_LISTEN,         NULL},
   {CLM_DEVICE_STATE_LISTEN, CLM_DEVICE_EVENT_SCAN_START_DEVICE_START, CLM_DEVICE_STATE_WAIT_TD,        clm_iefb_set_data_ip},
   {CLM_DEVICE_STATE_LISTEN, CLM_DEVICE_EVENT_SCAN_START_DEVICE_STOP,  CLM_DEVICE_STATE_CYCLIC_SUSPEND, clm_iefb_set_data_ip},
   {CLM_DEVICE_STATE_LISTEN, CLM_DEVICE_EVENT_SLAVE_DUPLICATION,       CLM_DEVICE_STATE_LISTEN,         NULL},

   /* State WAIT_TD */
   {CLM_DEVICE_STATE_WAIT_TD, CLM_DEVICE_EVENT_GROUP_STANDBY,       CLM_DEVICE_STATE_LISTEN,      NULL},
   {CLM_DEVICE_STATE_WAIT_TD, CLM_DEVICE_EVENT_RECEIVE_OK,          CLM_DEVICE_STATE_CYCLIC_SENT, NULL},
   {CLM_DEVICE_STATE_WAIT_TD, CLM_DEVICE_EVENT_RECEIVE_ERROR,       CLM_DEVICE_STATE_LISTEN,      NULL},
   {CLM_DEVICE_STATE_WAIT_TD, CLM_DEVICE_EVENT_GROUP_TIMEOUT,       CLM_DEVICE_STATE_LISTEN,      NULL},
   {CLM_DEVICE_STATE_WAIT_TD, CLM_DEVICE_EVENT_GROUP_ALL_RESPONDED, CLM_DEVICE_STATE_LISTEN,      NULL},
   {CLM_DEVICE_STATE_WAIT_TD, CLM_DEVICE_EVENT_SLAVE_DUPLICATION,   CLM_DEVICE_STATE_LISTEN,      NULL},

   /* State CYCLIC_SUSPEND */
   {CLM_DEVICE_STATE_CYCLIC_SUSPEND, CLM_DEVICE_EVENT_GROUP_STANDBY,       CLM_DEVICE_STATE_LISTEN, NULL},
   {CLM_DEVICE_STATE_CYCLIC_SUSPEND, CLM_DEVICE_EVENT_GROUP_TIMEOUT,       CLM_DEVICE_STATE_LISTEN, NULL},
   {CLM_DEVICE_STATE_CYCLIC_SUSPEND, CLM_DEVICE_EVENT_GROUP_ALL_RESPONDED, CLM_DEVICE_STATE_LISTEN, NULL},
   {CLM_DEVICE_STATE_CYCLIC_SUSPEND, CLM_DEVICE_EVENT_SLAVE_DUPLICATION,   CLM_DEVICE_STATE_LISTEN, NULL},

   /* State CYCLIC_SENT */
   {CLM_DEVICE_STATE_CYCLIC_SENT, CLM_DEVICE_EVENT_GROUP_STANDBY,           CLM_DEVICE_STATE_LISTEN,         NULL},
   {CLM_DEVICE_STATE_CYCLIC_SENT, CLM_DEVICE_EVENT_SCAN_START_DEVICE_START, CLM_DEVICE_STATE_CYCLIC_SENDING, clm_iefb_set_data_ip},
   {CLM_DEVICE_STATE_CYCLIC_SENT, CLM_DEVICE_EVENT_SCAN_START_DEVICE_STOP,  CLM_DEVICE_STATE_CYCLIC_SUSPEND, NULL},
   {CLM_DEVICE_STATE_CYCLIC_SENT, CLM_DEVICE_EVENT_SLAVE_DUPLICATION,       CLM_DEVICE_STATE_LISTEN,         NULL},

   /* State CYCLIC_SENDING */
   {CLM_DEVICE_STATE_CYCLIC_SENDING, CLM_DEVICE_EVENT_GROUP_STANDBY,           CLM_DEVICE_STATE_LISTEN,         NULL},
   {CLM_DEVICE_STATE_CYCLIC_SENDING, CLM_DEVICE_EVENT_RECEIVE_OK,              CLM_DEVICE_STATE_CYCLIC_SENT,    clm_iefb_reset_timeout_count},
   {CLM_DEVICE_STATE_CYCLIC_SENDING, CLM_DEVICE_EVENT_RECEIVE_ERROR,           CLM_DEVICE_STATE_LISTEN,         NULL},
   {CLM_DEVICE_STATE_CYCLIC_SENDING, CLM_DEVICE_EVENT_GROUP_TIMEOUT,           CLM_DEVICE_STATE_CYCLIC_SENDING, clm_iefb_evaluate_timeoutcounter},
   {CLM_DEVICE_STATE_CYCLIC_SENDING, CLM_DEVICE_EVENT_TIMEOUTCOUNTER_FULL,     CLM_DEVICE_STATE_LISTEN,         NULL},
   {CLM_DEVICE_STATE_CYCLIC_SENDING, CLM_DEVICE_EVENT_TIMEOUTCOUNTER_NOT_FULL, CLM_DEVICE_STATE_CYCLIC_SENT,    NULL},
   {CLM_DEVICE_STATE_CYCLIC_SENDING, CLM_DEVICE_EVENT_SLAVE_DUPLICATION,       CLM_DEVICE_STATE_LISTEN,         NULL}};
// clang-format on

// clang-format off
const clm_device_fsm_on_entry_exit_table_t device_state_actions[] = {
   {CLM_DEVICE_STATE_LISTEN, clm_iefb_reset_timeout_count, NULL}};
// clang-format on

/**
 * Send an event to a device state machine
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param slave_device_data      Slave device data (holds state machine)
 * @param event                  Triggering event
 */
void clm_iefb_device_fsm_event (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_slave_device_data_t * slave_device_data,
   clm_device_event_t event)
{
   if (event == CLM_DEVICE_EVENT_LAST)
   {
      return;
   }

   do
   {
      clm_device_state_t previous = slave_device_data->device_state;
      clm_device_fsm_t * element  = &(clm->device_fsm_matrix[previous][event]);
      clm_device_fsm_entry_exit_t * previous_state_actions =
         &(clm->device_fsm_on_entry_exit[previous]);
      clm_device_fsm_entry_exit_t * next_state_actions =
         &(clm->device_fsm_on_entry_exit[element->next]);

      /* Transition to next state */
      slave_device_data->device_state = element->next;

      if (slave_device_data->device_state != previous)
      {
         /* Show actions when we change state. Note that actions
            might occur also when we stay in the same state. */
         (void)previous_state_actions;

#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
         LOG_DISABLED (
            CL_CCIEFB_LOG,
            "CCIEFB(%d): Device event %s for group index %u device "
            "index %u. "
            "New state %s (was %s). Trigger on-exit action: %u  "
            "Transition action: %u  On-entry action: %u\n",
            __LINE__,
            cl_literals_get_device_event (event),
            group_data->group_index,
            slave_device_data->device_index,
            cl_literals_get_device_state (slave_device_data->device_state),
            cl_literals_get_device_state (previous),
            previous_state_actions->on_exit != NULL,
            element->action != NULL,
            next_state_actions->on_entry != NULL);
#endif
      }

      /* Perform action */
      if (element->action != NULL)
      {
         event = element->action (
            clm,
            now,
            group_data,
            slave_device_data,
            event,
            slave_device_data->device_state);
      }
      else
      {
         event = CLM_DEVICE_EVENT_NONE;
      }

      /* Perform state on_entry action */
      if ((slave_device_data->device_state != previous) && (next_state_actions->on_entry != NULL))
      {
         (void)next_state_actions->on_entry (
            clm,
            now,
            group_data,
            slave_device_data,
            event,
            slave_device_data->device_state);
      }
   } while (event != CLM_DEVICE_EVENT_NONE);
}

/**
 * Send an event to all device state machines in a group
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param event                  Triggering event
 */
static void clm_iefb_device_fsm_event_all_in_group (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_device_event_t event)
{
   uint16_t slave_device_index = 0;
   clm_slave_device_data_t * slave_device_data;
   const clm_group_setting_t * group_setting =
      &clm->config.hier.groups[group_data->group_index];

   for (slave_device_index = 0;
        slave_device_index < group_setting->num_slave_devices;
        slave_device_index++)
   {
      slave_device_data = &group_data->slave_devices[slave_device_index];
      clm_iefb_device_fsm_event (clm, now, group_data, slave_device_data, event);
   }
}

/**
 * Initialise the device state machine tables.
 *
 * @param clm              c-link master stack instance handle
 */
void clm_iefb_device_fsm_tables_init (clm_t * clm)
{
   unsigned int i;
   unsigned int j;

   /* Set FSM defaults */
   for (i = 0; i < CLM_DEVICE_STATE_LAST; i++)
   {
      for (j = 0; j < CLM_DEVICE_EVENT_LAST; j++)
      {
         /* Stay in state, no action */
         clm->device_fsm_matrix[i][j].next   = i;
         clm->device_fsm_matrix[i][j].action = NULL;
      }

      clm->device_fsm_on_entry_exit[i].on_entry = NULL;
      clm->device_fsm_on_entry_exit[i].on_exit  = NULL;
   }

   /* Set FSM transitions from table */
   for (i = 0; i < NELEMENTS (device_transitions); i++)
   {
      const clm_device_fsm_transition_t * t           = &device_transitions[i];
      clm->device_fsm_matrix[t->state][t->event].next = t->next;
      clm->device_fsm_matrix[t->state][t->event].action = t->action;
   }

   /* Set FSM on_entry and on_exit from table */
   for (i = 0; i < NELEMENTS (device_state_actions); i++)
   {
      const clm_device_fsm_on_entry_exit_table_t * a = &device_state_actions[i];
      clm->device_fsm_on_entry_exit[a->state].on_entry = a->on_entry;
      clm->device_fsm_on_entry_exit[a->state].on_exit  = a->on_exit;
   }
}

/*********************** Group state machine ****************************/

/**
 * Initialise the group.
 *
 * Also known as InitMaster() [for the group] in the specification.
 *
 * This is a callback for the group state machine.
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param event                  Triggering event
 * @param new_state              State we are about to enter
 * @return Next event to trigger
 *
 * @req REQ_CLM_STATUSBIT_05
 */
static clm_group_event_t clm_iefb_group_init (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_group_event_t event,
   clm_group_state_t new_state)
{
   uint16_t slave_device_index = 0;
   const clm_group_setting_t * group_setting =
      &clm->config.hier.groups[group_data->group_index];
   clm_slave_device_data_t * slave_device_data;

   /* Initialise timers */
   cl_timer_stop (&group_data->response_wait_timer);
   cl_timer_stop (&group_data->constant_linkscan_timer);

   /* These will be updated when we get a new config */
   group_data->total_occupied            = 0;
   group_data->frame_sequence_no         = 0;
   group_data->timestamp_link_scan_start = 0;

   group_data->cyclic_transmission_state =
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF;

   /* Initialise state for each slave device */
   for (slave_device_index = 0;
        slave_device_index < group_setting->num_slave_devices;
        slave_device_index++)
   {
      slave_device_data = &group_data->slave_devices[slave_device_index];

      slave_device_data->device_index = slave_device_index;
      slave_device_data->device_state = CLM_DEVICE_STATE_MASTER_DOWN;
   }

   /* Initialise rest of each slave device */
   clm_iefb_device_fsm_event_all_in_group (
      clm,
      now,
      group_data,
      CLM_DEVICE_EVENT_GROUP_STARTUP);

   return CLM_GROUP_EVENT_NONE;
}

/**
 * Handle that we have a new valid configuration, so update parameters.
 *
 * This is a callback for the group state machine.
 *
 * Known as ReflectParameter(), ArbitrationTimer startup and StartArbitration().
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param event                  Triggering event
 * @param new_state              State we are about to enter
 * @return Next event to trigger
 *
 * @req REQ_CLM_ARBITR_01
 */
static clm_group_event_t clm_iefb_group_new_config (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_group_event_t event,
   clm_group_state_t new_state)
{
   clm_iefb_reflect_group_parameters (clm, group_data);

   /* Safe to be executed from more than one group */
   cl_timer_start_if_not_running (
      &clm->arbitration_timer,
      clm->config.arbitration_time * CL_TIMER_MICROSECONDS_PER_MILLISECOND,
      now);

   return CLM_GROUP_EVENT_NONE;
}

/**
 * Handle that we lost the arbitration
 *
 * This is a callback for the group state machine.
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param event                  Triggering event
 * @param new_state              State we are about to enter
 * @return Next event to trigger
 */
static clm_group_event_t clm_iefb_on_arbitration_failed (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_group_event_t event,
   clm_group_state_t new_state)
{
   LOG_INFO (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): Arbitration failed, as we received request from "
      "other master.\n",
      __LINE__);

   clm_iefb_trigger_error_callback (
      clm,
      now,
      CLM_ERROR_ARBITRATION_FAILED,
      clm->latest_conflicting_master_ip,
      0);

   return CLM_GROUP_EVENT_NONE;
}

/**
 * Handle that the arbitration is done
 *
 * This is a callback for the group state machine.
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param event                  Triggering event
 * @param new_state              State we are about to enter
 * @return Next event to trigger
 *
 * @req REQ_CLM_ARBITR_02
 */
static clm_group_event_t clm_iefb_on_arbitration_done (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_group_event_t event,
   clm_group_state_t new_state)
{
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): Arbitration done. Reported to state machine for group %u\n",
      __LINE__,
      group_data->group_index + 1U);

   return CLM_GROUP_EVENT_LINKSCAN_START;
}

/**
 * Handle that the link scan is starting
 *
 * This is a callback for the group state machine.
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param event                  Triggering event
 * @param new_state              State we are about to enter
 * @return Next event to trigger
 *
 * @req REQ_CLM_TIMING_06
 *
 */
static clm_group_event_t clm_iefb_on_linkscan_start (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_group_event_t event,
   clm_group_state_t new_state)
{
   uint16_t slave_device_index     = 0;
   clm_device_event_t device_event = CLM_DEVICE_EVENT_NONE;
   uint64_t unix_timestamp_ms      = clal_get_unix_timestamp_ms();
   clm_slave_device_data_t * slave_device_data;
   const clm_group_setting_t * group_setting =
      &clm->config.hier.groups[group_data->group_index];

   /* Tell slave device representations to update outgoing frame */
   for (slave_device_index = 0;
        slave_device_index < group_setting->num_slave_devices;
        slave_device_index++)
   {
      slave_device_data = &group_data->slave_devices[slave_device_index];
      device_event      = (slave_device_data->enabled)
                             ? CLM_DEVICE_EVENT_SCAN_START_DEVICE_START
                             : CLM_DEVICE_EVENT_SCAN_START_DEVICE_STOP;
      clm_iefb_device_fsm_event (
         clm,
         now,
         group_data,
         slave_device_data,
         device_event);
   }

   group_data->timestamp_link_scan_start = now;

   clm_iefb_send_cyclic_request_frame (
      clm->cciefb_socket,
      clm->iefb_broadcast_ip,
      group_data,
      now,
      unix_timestamp_ms,
      clm->master_local_unit_info);

   cl_timer_start (
      &group_data->response_wait_timer,
      group_setting->timeout_value * CL_TIMER_MICROSECONDS_PER_MILLISECOND,
      now);

   if (group_setting->use_constant_link_scan_time)
   {
      cl_timer_start (
         &group_data->constant_linkscan_timer,
         group_setting->timeout_value * CL_TIMER_MICROSECONDS_PER_MILLISECOND,
         now);
   }

   return CLM_GROUP_EVENT_NONE;
}

/**
 * Handle that the link scan times out
 *
 * This is a callback for the group state machine.
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param event                  Triggering event
 * @param new_state              State we are about to enter
 * @return Next event to trigger
 *
 * @req REQ_CLM_SEQUENCE_03
 */
static clm_group_event_t clm_iefb_on_linkscan_timeout (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_group_event_t event,
   clm_group_state_t new_state)
{
   clm_iefb_device_fsm_event_all_in_group (
      clm,
      now,
      group_data,
      CLM_DEVICE_EVENT_GROUP_TIMEOUT);

   /* Timing out, thus some devices have failed to respond */
   clm_iefb_trigger_linkscan_callback (clm, group_data, false);

   clm_iefb_update_frame_sequence_no (&group_data->frame_sequence_no);

   return CLM_GROUP_EVENT_LINKSCAN_START;
}

/**
 * Handle that the link scan is done
 *
 * This is a callback for the group state machine.
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param event                  Triggering event
 * @param new_state              State we are about to enter
 * @return Next event to trigger
 */
static clm_group_event_t clm_iefb_on_linkscan_complete (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_group_event_t event,
   clm_group_state_t new_state)
{
   const clm_group_setting_t * group_setting =
      &clm->config.hier.groups[group_data->group_index];

   cl_timer_stop (&group_data->response_wait_timer);

   clm_iefb_device_fsm_event_all_in_group (
      clm,
      now,
      group_data,
      CLM_DEVICE_EVENT_GROUP_ALL_RESPONDED);

   /* Link scan is done as all devices have responded */
   clm_iefb_trigger_linkscan_callback (clm, group_data, true);

   clm_iefb_update_frame_sequence_no (&group_data->frame_sequence_no);

   if (group_setting->use_constant_link_scan_time)
   {
      return CLM_GROUP_EVENT_NONE;
   }

   return CLM_GROUP_EVENT_LINKSCAN_START;
}

/**
 * On entry for state MASTER LISTEN
 *
 * This is a callback for the group state machine.
 *
 * Known as StopCyclic(), DiscardParameter() and Arbitration Timer stopped.
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param event                  Triggering event
 * @param new_state              State we are about to enter
 * @return Next event to trigger
 */
static clm_group_event_t clm_iefb_group_on_entry_listen (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_group_event_t event,
   clm_group_state_t new_state)
{
   /* Safe to be executed from more than one group */
   cl_timer_stop (&clm->arbitration_timer);

   clm_iefb_device_fsm_event_all_in_group (
      clm,
      now,
      group_data,
      CLM_DEVICE_EVENT_GROUP_STANDBY);
   clm_iefb_set_master_state (clm, CLM_MASTER_STATE_STANDBY);

   return CLM_GROUP_EVENT_NONE;
}

/**
 * On entry for state MASTER ARBITRATION
 *
 * This is a callback for the group state machine.
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param event                  Triggering event
 * @param new_state              State we are about to enter
 * @return Next event to trigger
 */
static clm_group_event_t clm_iefb_group_on_entry_arbitration (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_group_event_t event,
   clm_group_state_t new_state)
{
   clm_iefb_set_master_state (clm, CLM_MASTER_STATE_ARBITRATION);

   return CLM_GROUP_EVENT_NONE;
}

/**
 * On entry for state MASTER LINK SCAN COMPLETE
 *
 * This is a callback for the group state machine.
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data
 * @param event                  Triggering event
 * @param new_state              State we are about to enter
 * @return Next event to trigger
 */
static clm_group_event_t clm_iefb_group_on_entry_scan_complete (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_group_event_t event,
   clm_group_state_t new_state)
{
   clm_iefb_set_master_state (clm, CLM_MASTER_STATE_RUNNING);

   return CLM_GROUP_EVENT_NONE;
}

/** Group state machine transitions */
// clang-format off
const clm_group_fsm_transition_t group_transitions[] = {
   /* State MASTER_DOWN */
   {CLM_GROUP_STATE_MASTER_DOWN, CLM_GROUP_EVENT_STARTUP, CLM_GROUP_STATE_MASTER_LISTEN, clm_iefb_group_init},

   /* State MASTER_LISTEN */
   {CLM_GROUP_STATE_MASTER_LISTEN, CLM_GROUP_EVENT_NEW_CONFIG,       CLM_GROUP_STATE_MASTER_ARBITRATION, clm_iefb_group_new_config},
   {CLM_GROUP_STATE_MASTER_LISTEN, CLM_GROUP_EVENT_PARAMETER_CHANGE, CLM_GROUP_STATE_MASTER_LISTEN,      NULL},

   /* State MASTER_ARBITRATION */
   {CLM_GROUP_STATE_MASTER_ARBITRATION, CLM_GROUP_EVENT_PARAMETER_CHANGE, CLM_GROUP_STATE_MASTER_LISTEN,         NULL},
   {CLM_GROUP_STATE_MASTER_ARBITRATION, CLM_GROUP_EVENT_ARBITRATION_DONE, CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP, clm_iefb_on_arbitration_done},
   {CLM_GROUP_STATE_MASTER_ARBITRATION, CLM_GROUP_EVENT_REQ_FROM_OTHER,   CLM_GROUP_STATE_MASTER_LISTEN,         clm_iefb_on_arbitration_failed},

   /* State MASTER_LINK_SCAN_COMP */
   {CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP, CLM_GROUP_EVENT_PARAMETER_CHANGE, CLM_GROUP_STATE_MASTER_LISTEN,         NULL},
   {CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP, CLM_GROUP_EVENT_LINKSCAN_START,   CLM_GROUP_STATE_MASTER_LINK_SCAN,      clm_iefb_on_linkscan_start},
   {CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP, CLM_GROUP_EVENT_REQ_FROM_OTHER,   CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP, NULL},
   {CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP, CLM_GROUP_EVENT_MASTERDUPL_ALARM, CLM_GROUP_STATE_MASTER_LISTEN,         NULL},

   /* State MASTER_LINK_SCAN */
   {CLM_GROUP_STATE_MASTER_LINK_SCAN, CLM_GROUP_EVENT_PARAMETER_CHANGE,  CLM_GROUP_STATE_MASTER_LISTEN,         NULL},
   {CLM_GROUP_STATE_MASTER_LINK_SCAN, CLM_GROUP_EVENT_LINKSCAN_COMPLETE, CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP, clm_iefb_on_linkscan_complete},
   {CLM_GROUP_STATE_MASTER_LINK_SCAN, CLM_GROUP_EVENT_LINKSCAN_TIMEOUT,  CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP, clm_iefb_on_linkscan_timeout},
   {CLM_GROUP_STATE_MASTER_LINK_SCAN, CLM_GROUP_EVENT_REQ_FROM_OTHER,    CLM_GROUP_STATE_MASTER_LINK_SCAN,      NULL},
   {CLM_GROUP_STATE_MASTER_LINK_SCAN, CLM_GROUP_EVENT_MASTERDUPL_ALARM,  CLM_GROUP_STATE_MASTER_LISTEN,         NULL},
   };
// clang-format on

// clang-format off
const clm_group_fsm_on_entry_exit_table_t group_state_actions[] = {
   {CLM_GROUP_STATE_MASTER_LISTEN,         clm_iefb_group_on_entry_listen,        NULL},
   {CLM_GROUP_STATE_MASTER_ARBITRATION,    clm_iefb_group_on_entry_arbitration,   NULL},
   {CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP, clm_iefb_group_on_entry_scan_complete, NULL}};
// clang-format on

/**
 * Send an event to a group state machine
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param group_data             Group data (holds state machine)
 * @param event                  Triggering event
 */
void clm_iefb_group_fsm_event (
   clm_t * clm,
   uint32_t now,
   clm_group_data_t * group_data,
   clm_group_event_t event)
{
   if (event == CLM_GROUP_EVENT_LAST)
   {
      return;
   }

   do
   {
      clm_group_state_t previous = group_data->group_state;
      clm_group_fsm_t * element  = &(clm->group_fsm_matrix[previous][event]);
      clm_group_fsm_entry_exit_t * previous_state_actions =
         &(clm->group_fsm_on_entry_exit[previous]);
      clm_group_fsm_entry_exit_t * next_state_actions =
         &(clm->group_fsm_on_entry_exit[element->next]);

      /* Transition to next state */
      group_data->group_state = element->next;

      if (group_data->group_state != previous)
      {
         /* Show actions when we change state. Note that actions
            might occur also when we stay in the same state. */
         (void)previous_state_actions;
#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
         LOG_DISABLED (
            CL_CCIEFB_LOG,
            "CCIEFB(%d): Group event %s for group index %u. New state "
            "%s (was %s). Trigger on-exit action: %u  "
            "Transition action: %u  On-entry action: %u\n",
            __LINE__,
            cl_literals_get_group_event (event),
            group_data->group_index,
            cl_literals_get_group_state (group_data->group_state),
            cl_literals_get_group_state (previous),
            previous_state_actions->on_exit != NULL,
            element->action != NULL,
            next_state_actions->on_entry != NULL);
#endif
      }

      /* Perform action */
      if (element->action != NULL)
      {
         event =
            element->action (clm, now, group_data, event, group_data->group_state);
      }
      else
      {
         event = CLM_GROUP_EVENT_NONE;
      }

      /* Perform state on_entry action */
      if ((group_data->group_state != previous) && (next_state_actions->on_entry != NULL))
      {
         (void)next_state_actions
            ->on_entry (clm, now, group_data, event, group_data->group_state);
      }

   } while (event != CLM_GROUP_EVENT_NONE);
}

/**
 * Send an event to all group state machines
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param event                  Triggering event
 */
static void clm_iefb_group_fsm_event_all (
   clm_t * clm,
   uint32_t now,
   clm_group_event_t event)
{
   uint16_t group_index = 0;
   clm_group_data_t * group_data;

   for (group_index = 0; group_index < clm->config.hier.number_of_groups;
        group_index++)
   {
      group_data = &clm->groups[group_index];

      clm_iefb_group_fsm_event (clm, now, group_data, event);
   }
}

/**
 * Initialise the group state machine tables.
 *
 * @param clm              c-link master stack instance handle
 */
void clm_iefb_group_fsm_tables_init (clm_t * clm)
{
   unsigned int i;
   unsigned int j;

   /* Set FSM defaults */
   for (i = 0; i < CLM_GROUP_STATE_LAST; i++)
   {
      for (j = 0; j < CLM_GROUP_EVENT_LAST; j++)
      {
         /* Stay in state, no action */
         clm->group_fsm_matrix[i][j].next   = i;
         clm->group_fsm_matrix[i][j].action = NULL;
      }

      clm->group_fsm_on_entry_exit[i].on_entry = NULL;
      clm->group_fsm_on_entry_exit[i].on_exit  = NULL;
   }

   /* Set FSM transitions from table */
   for (i = 0; i < NELEMENTS (group_transitions); i++)
   {
      const clm_group_fsm_transition_t * t             = &group_transitions[i];
      clm->group_fsm_matrix[t->state][t->event].next   = t->next;
      clm->group_fsm_matrix[t->state][t->event].action = t->action;
   }

   /* Set FSM on_entry and on_exit from table */
   for (i = 0; i < NELEMENTS (group_state_actions); i++)
   {
      const clm_group_fsm_on_entry_exit_table_t * a   = &group_state_actions[i];
      clm->group_fsm_on_entry_exit[a->state].on_entry = a->on_entry;
      clm->group_fsm_on_entry_exit[a->state].on_exit  = a->on_exit;
   }
}

/**************************************************************************/

/**
 * Run timer events for all timers located in groups
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 */
void clm_iefb_monitor_all_group_timers (clm_t * clm, uint32_t now)
{
   uint16_t group_index = 0;
   clm_group_data_t * group_data;

   for (group_index = 0; group_index < clm->config.hier.number_of_groups;
        group_index++)
   {
      group_data = &clm->groups[group_index];

      if (cl_timer_is_expired (&group_data->response_wait_timer, now))
      {
         cl_timer_stop (&group_data->response_wait_timer);
         clm_iefb_group_fsm_event (
            clm,
            now,
            group_data,
            CLM_GROUP_EVENT_LINKSCAN_TIMEOUT);
      }

      if (cl_timer_is_expired (&group_data->constant_linkscan_timer, now))
      {
         cl_timer_stop (&group_data->constant_linkscan_timer);

         clm_iefb_group_fsm_event (
            clm,
            now,
            group_data,
            CLM_GROUP_EVENT_LINKSCAN_START);
      }
   }
}

/**
 * Handle that the configuration has been updated.
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 *
 * @req REQ_CLM_PARAMETERID_01
 * @req REQ_CLM_PARAMETERID_02
 */
static void clm_iefb_handle_new_config (clm_t * clm, uint32_t now)
{
   clm->parameter_no++;

   uint16_t temporary_buffer;

   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): Will save parameter_no %u to nvm\n",
      __LINE__,
      clm->parameter_no);

   /* Save parameter_no to file */
   cl_file_save_if_modified (
      clm->config.file_directory,
      CLM_FILENAME_PARAM_NO,
      &clm->parameter_no,
      &temporary_buffer,
      sizeof (clm->parameter_no));

   clm_iefb_group_fsm_event_all (clm, now, CLM_GROUP_EVENT_NEW_CONFIG);
}

/**
 * Handle incoming CCIEFB response frame
 *
 * Triggers a callback for incoming alarm frames.
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param buffer                 Buffer to be parsed
 * @param recv_len               UDP payload length
 * @param remote_ip              Remote source IP address
 * @param remote_port            Remote source UDP port
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CL_PROTOCOL_51
 * @req REQ_CL_PROTOCOL_52
 * @req REQ_CLM_COMMUNIC_02
 * @req REQ_CLM_COMMUNIC_04
 * @req REQ_CLM_CONFORMANCE_09
 * @req REQ_CLM_CONFORMANCE_13
 * @req REQ_CLM_CONFORMANCE_14
 * @req REQ_CLM_ERROR_01
 * @req REQ_CLM_ERROR_02
 * @req REQ_CLM_ERROR_04
 * @req REQ_CLM_ERROR_05
 * @req REQ_CLM_ERROR_06
 * @req REQ_CLM_GROUPS_02
 * @req REQ_CLM_STATUSBIT_04
 *
 */
static int clm_iefb_handle_response_frame (
   clm_t * clm,
   uint32_t now,
   uint8_t * buffer,
   size_t recv_len,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port)
{
   uint16_t group_index                              = 0;
   uint16_t slave_device_index                       = 0;
   uint16_t frame_sequence_no                        = 0;
   uint16_t end_code                                 = 0;
   uint16_t slave_err_code                           = 0;
   uint32_t local_management_info                    = 0;
   clm_device_event_t event                          = CLM_DEVICE_EVENT_NONE;
   clm_cciefb_cyclic_response_info_t cyclic_response = {0};
   const clm_group_setting_t * group_setting;
   clm_group_data_t * group_data;
   const clm_slave_device_setting_t * slave_device_setting;
   clm_slave_device_data_t * slave_device_data;
   clm_device_framevalues_t * latest;

   if (clm->config.master_id == CL_IPADDR_INVALID)
   {
      /* We have no IP address yet. Drop frame. */
      return -1;
   }

   if (
      cl_iefb_parse_cyclic_response (
         buffer,
         recv_len,
         remote_ip,
         remote_port,
         now,
         &cyclic_response) != 0)
   {
      return -1;
   }

   /* Validate that group number is in allowed range */
   CC_ASSERT (cyclic_response.full_headers != NULL);
   if (
      cyclic_response.full_headers->cyclic_data_header.group_no >
      clm->config.hier.number_of_groups)
   {
      /* Invalid group number. Drop frame. */
      return -1;
   }

   group_index = cyclic_response.full_headers->cyclic_data_header.group_no - 1;
   group_data  = &clm->groups[group_index];
   group_setting = &clm->config.hier.groups[group_index];

   /* Find slave_device_index from IP address, and validate group
    * REQ_CLM_GROUPS_02 */
   if (clm_iefb_calc_slave_device_index (group_setting, remote_ip, &slave_device_index) != 0)
   {
      /* The given IP address does not exist in the given group. Drop frame */
      return -1;
   }

   cyclic_response.device_index = slave_device_index;
   slave_device_data    = &group_data->slave_devices[slave_device_index];
   latest               = &slave_device_data->latest_frame;
   slave_device_setting = &group_setting->slave_devices[slave_device_index];

   slave_device_data->statistics.number_of_incoming_frames++;

   /* Validate that the response is from an enabled slave */
   if (slave_device_data->enabled == false)
   {
      /* We have disabled the slave, but it sends anyway. Drop frame.
         REQ_CLM_COMMUNIC_02 */
      slave_device_data->statistics.number_of_incoming_invalid_frames++;
      return -1;
   }

   frame_sequence_no = CC_FROM_LE16 (
      cyclic_response.full_headers->cyclic_data_header.frame_sequence_no);

   /* Master detects slave duplication, also for invalid sequence numbers.
      REQ_CLM_ERROR_05 */
   if (
      latest->has_been_received &&
      frame_sequence_no == latest->frame_sequence_no &&
      slave_device_data->device_state != CLM_DEVICE_STATE_LISTEN)
   {
      slave_device_data->statistics.number_of_incoming_invalid_frames++;
      clm_iefb_trigger_error_callback (
         clm,
         now,
         CLM_ERROR_SLAVE_DUPLICATION,
         remote_ip,
         0);
      clm_iefb_device_fsm_event (
         clm,
         now,
         group_data,
         slave_device_data,
         CLM_DEVICE_EVENT_SLAVE_DUPLICATION);
      return -1;
   }

   /* Validate frame sequence number.
      Allow invalid number for devices that are returning.
      REQ_CLM_COMMUNIC_04 */
   if (slave_device_data->transmission_bit && frame_sequence_no != group_data->frame_sequence_no)
   {
      /* Wrong frame sequence number. Drop frame. */
      slave_device_data->statistics.number_of_incoming_invalid_frames++;
      return -1;
   }

   end_code = CC_FROM_LE16 (cyclic_response.full_headers->cyclic_header.end_code);
   slave_err_code = CC_FROM_LE16 (
      cyclic_response.full_headers->slave_station_notification.slave_err_code);
   local_management_info =
      CC_FROM_LE32 (cyclic_response.full_headers->slave_station_notification
                       .local_management_info);

   if (end_code == CL_SLMP_ENDCODE_CCIEFB_WRONG_NUMBER_OCCUPIED_STATIONS)
   {
      slave_device_data->statistics.number_of_incoming_alarm_frames++;
      clm_iefb_trigger_error_callback (
         clm,
         now,
         CLM_ERROR_SLAVE_REPORTS_WRONG_NUMBER_OCCUPIED,
         remote_ip,
         0);
   }

   if (end_code == CL_SLMP_ENDCODE_CCIEFB_MASTER_DUPLICATION)
   {
      slave_device_data->statistics.number_of_incoming_alarm_frames++;
      clm_iefb_trigger_error_callback (
         clm,
         now,
         CLM_ERROR_SLAVE_REPORTS_MASTER_DUPLICATION,
         remote_ip,
         0);
   }

   /* Validate number of occupied stations, and that the response size is
       correct with regards to the number of occupied stations. */
   if (cyclic_response.number_of_occupied != slave_device_setting->num_occupied_stations)
   {
      /* Device sends response with wrong number of occupied stations, drop
       * frame. REQ_CLM_ERROR_02. Device will be disconnected due to timeout. */
      slave_device_data->statistics.number_of_incoming_invalid_frames++;
      return -1;
   }

   if (
      end_code == CL_SLMP_ENDCODE_CCIEFB_SLAVE_ERROR ||
      end_code == CL_SLMP_ENDCODE_CCIEFB_SLAVE_REQUESTS_DISCONNECT)
   {
      slave_device_data->statistics.number_of_incoming_alarm_frames++;

      /* Trigger 'alarm frame' callback to application, if it changed
         (even if there is no previously received frame) */
      if (end_code != latest->end_code)
      {
         clm_iefb_trigger_alarmframe_callback (
            clm,
            group_index,
            slave_device_index,
            end_code,
            slave_err_code,
            local_management_info);
      }
   }

   if (
      latest->has_been_received &&
      ((slave_err_code != latest->slave_err_code) ||
       (local_management_info != latest->local_management_info)))
   {
      /* Trigger slaveinfo callback to application, if it changed */
      clm_iefb_trigger_slaveinfo_callback (
         clm,
         group_index,
         slave_device_index,
         end_code,
         slave_err_code,
         local_management_info);
   }

   /* Store incoming frame details, statistics and cyclic data */
   clm_iefb_store_incoming_slave_runtime_info (
      &cyclic_response,
      group_data->timestamp_link_scan_start,
      &slave_device_data->latest_frame);

   clm_iefb_statistics_update_response_time (
      &slave_device_data->statistics,
      clm->config.max_statistics_samples,
      slave_device_data->latest_frame.response_time);

   clm_iefb_store_incoming_cyclic_data (
      &group_data->memory_area,
      &cyclic_response,
      slave_device_data->slave_station_no,
      slave_device_setting->num_occupied_stations,
      slave_device_data->transmission_bit);

   /* Trigger state machine event */
   if (end_code == CL_SLMP_ENDCODE_CCIEFB_MASTER_DUPLICATION)
   {
      clm_iefb_group_fsm_event_all (clm, now, CLM_GROUP_EVENT_MASTERDUPL_ALARM);
      return 0;
   }

   event = (end_code == CL_SLMP_ENDCODE_SUCCESS)
              ? CLM_DEVICE_EVENT_RECEIVE_OK
              : CLM_DEVICE_EVENT_RECEIVE_ERROR;
   clm_iefb_device_fsm_event (clm, now, group_data, slave_device_data, event);

   if (clm_iefb_group_have_received_from_all_devices (group_setting, group_data))
   {
      clm_iefb_group_fsm_event (
         clm,
         now,
         group_data,
         CLM_GROUP_EVENT_LINKSCAN_COMPLETE);
   }

   return 0;
}

/**
 * Handle incoming CCIEFB request frame from other master
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param clm                    c-link master stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param buffer                 Buffer to be parsed
 * @param recv_len               UDP payload length
 * @param remote_ip              Remote source IP address
 * @param remote_port            Remote source UDP port
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CLM_COMMUNIC_05
 * @req REQ_CLM_CONFORMANCE_10
 * @req REQ_CLM_CONFORMANCE_12
 *
 */
static int clm_iefb_handle_request_frame (
   clm_t * clm,
   uint32_t now,
   uint8_t * buffer,
   size_t recv_len,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port)
{
#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */

   cl_util_ip_to_string (remote_ip, ip_string);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): Received request from other master, IP address "
      "%s\n",
      __LINE__,
      ip_string);
#endif

   clm->latest_conflicting_master_ip = remote_ip;
   clm_iefb_group_fsm_event_all (clm, now, CLM_GROUP_EVENT_REQ_FROM_OTHER);

   return 0;
}

/**
 * Handle incoming CCIEFB frame
 *
 * @param clm              c-link master stack instance handle
 * @param now              Timestamp in microseconds
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param remote_ip        Remote IP address
 * @param remote_port      Remote UDP port number
 *
 * @return 0 on success, -1 on failure
 */
#ifndef FUZZ_TEST
static
#endif
   int
   clm_iefb_handle_input_frame (
      clm_t * clm,
      uint32_t now,
      uint8_t * buffer,
      size_t recv_len,
      cl_ipaddr_t remote_ip,
      uint16_t remote_port)
{
   cl_cciefb_resp_header_t * response_header = NULL;
   cl_cciefb_req_header_t * request_header   = NULL;
   uint16_t command                          = 0;
   uint16_t sub_command                      = 0;

   if (remote_ip == clm->config.master_id)
   {
      return 0; /* Frame from ourself. Drop it. */
   }

   if (cl_iefb_parse_response_header (buffer, recv_len, &response_header) != 0)
   {
      return -1; /* Too short message */
   }
   if (cl_iefb_validate_response_header (response_header, recv_len) != 0)
   {
      /* It might be a request from another master */
      if (cl_iefb_parse_request_header (buffer, recv_len, &request_header) != 0)
      {
         return -1;
      }

      if (cl_iefb_validate_request_header (request_header, recv_len) != 0)
      {
         return -1;
      }

      command     = CC_FROM_LE16 (request_header->command);
      sub_command = CC_FROM_LE16 (request_header->sub_command);
      if (command != CL_SLMP_COMMAND_CCIEFB_CYCLIC || sub_command != CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC)
      {
         return -1;
      }

      return clm_iefb_handle_request_frame (
         clm,
         now,
         buffer,
         recv_len,
         remote_ip,
         remote_port);
   }

   return clm_iefb_handle_response_frame (
      clm,
      now,
      buffer,
      recv_len,
      remote_ip,
      remote_port);
}

/**
 * Get position of, and size of the cyclic data for one device.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has
 *                               group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param position               Resulting slave station index (Which
 *                               is the slave_station_no - 1)
 * @return Number of occupied stations, or 0 failure
 */
static uint16_t clm_iefb_get_occupied_and_position (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * position)
{
   const clm_group_setting_t * group_setting;
   const clm_slave_device_setting_t * slave_device_setting;
   const clm_slave_device_data_t * slave_device_data;

   if (group_index >= clm->config.hier.number_of_groups)
   {
      return 0;
   }

   group_setting = &clm->config.hier.groups[group_index];
   if (slave_device_index >= group_setting->num_slave_devices)
   {
      return 0;
   }

   slave_device_setting = &group_setting->slave_devices[slave_device_index];
   slave_device_data =
      &clm->groups[group_index].slave_devices[slave_device_index];

   *position = slave_device_data->slave_station_no - 1;
   return slave_device_setting->num_occupied_stations;
}

/******************** Public functions *******************************/

const cl_rx_t * clm_iefb_get_first_rx_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied)
{
   if (group_index >= clm->config.hier.number_of_groups)
   {
      *total_occupied = 0;
      return NULL;
   }

   *total_occupied = clm->groups[group_index].total_occupied;
   return (cl_rx_t *)&clm->groups[group_index].memory_area.rx;
}

cl_ry_t * clm_iefb_get_first_ry_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied)
{
   if (group_index >= clm->config.hier.number_of_groups)
   {
      *total_occupied = 0;
      return NULL;
   }

   *total_occupied = clm->groups[group_index].total_occupied;
   return (cl_ry_t *)&clm->groups[group_index].memory_area.ry;
}

const cl_rwr_t * clm_iefb_get_first_rwr_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied)
{
   if (group_index >= clm->config.hier.number_of_groups)
   {
      *total_occupied = 0;
      return NULL;
   }

   *total_occupied = clm->groups[group_index].total_occupied;
   return (cl_rwr_t *)&clm->groups[group_index].memory_area.rwr;
}

cl_rww_t * clm_iefb_get_first_rww_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied)
{
   if (group_index >= clm->config.hier.number_of_groups)
   {
      *total_occupied = 0;
      return NULL;
   }

   *total_occupied = clm->groups[group_index].total_occupied;
   return (cl_rww_t *)&clm->groups[group_index].memory_area.rww;
}

const cl_rx_t * clm_iefb_get_first_device_rx_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations)
{
   const cl_rx_t * first_group_area;
   uint16_t pos                     = 0;
   uint16_t occupied                = 0;
   uint16_t total_occupied_in_group = 0;

   occupied = clm_iefb_get_occupied_and_position (
      clm,
      group_index,
      slave_device_index,
      &pos);
   if (occupied == 0)
   {
      *num_occupied_stations = 0;
      return NULL;
   }

   first_group_area =
      clm_iefb_get_first_rx_area (clm, group_index, &total_occupied_in_group);
   CC_ASSERT (first_group_area != NULL);

   *num_occupied_stations = occupied;
   return first_group_area + pos;
}

cl_ry_t * clm_iefb_get_first_device_ry_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations)
{
   cl_ry_t * first_group_area;
   uint16_t pos                     = 0;
   uint16_t occupied                = 0;
   uint16_t total_occupied_in_group = 0;

   occupied = clm_iefb_get_occupied_and_position (
      clm,
      group_index,
      slave_device_index,
      &pos);
   if (occupied == 0)
   {
      *num_occupied_stations = 0;
      return NULL;
   }

   first_group_area =
      clm_iefb_get_first_ry_area (clm, group_index, &total_occupied_in_group);
   CC_ASSERT (first_group_area != NULL);

   *num_occupied_stations = occupied;
   return first_group_area + pos;
}

const cl_rwr_t * clm_iefb_get_first_device_rwr_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations)
{
   const cl_rwr_t * first_group_area;
   uint16_t pos                     = 0;
   uint16_t occupied                = 0;
   uint16_t total_occupied_in_group = 0;

   occupied = clm_iefb_get_occupied_and_position (
      clm,
      group_index,
      slave_device_index,
      &pos);
   if (occupied == 0)
   {
      *num_occupied_stations = 0;
      return NULL;
   }

   first_group_area =
      clm_iefb_get_first_rwr_area (clm, group_index, &total_occupied_in_group);
   CC_ASSERT (first_group_area != NULL);

   *num_occupied_stations = occupied;
   return first_group_area + pos;
}

cl_rww_t * clm_iefb_get_first_device_rww_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations)
{
   cl_rww_t * first_group_area;
   uint16_t pos                     = 0;
   uint16_t occupied                = 0;
   uint16_t total_occupied_in_group = 0;

   occupied = clm_iefb_get_occupied_and_position (
      clm,
      group_index,
      slave_device_index,
      &pos);
   if (occupied == 0)
   {
      *num_occupied_stations = 0;
      return NULL;
   }

   first_group_area =
      clm_iefb_get_first_rww_area (clm, group_index, &total_occupied_in_group);
   CC_ASSERT (first_group_area != NULL);

   *num_occupied_stations = occupied;
   return first_group_area + pos;
}

bool clm_iefb_get_rx_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number)
{
   uint16_t num_occupied_stations = 0;
   uint8_t mask;
   uint16_t byte_in_area;
   uint16_t areanumber =
      cl_iefb_bit_calculate_areanumber (number, &byte_in_area, &mask);
   const cl_rx_t * first_area = clm_iefb_get_first_device_rx_area (
      clm,
      group_index,
      slave_device_index,
      &num_occupied_stations);

   /* Not possible to return error code to user */
   CC_ASSERT (num_occupied_stations != 0);
   CC_ASSERT (areanumber < num_occupied_stations);

   return ((first_area + areanumber)->bytes[byte_in_area] & mask) > 0;
}

void clm_iefb_set_ry_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number,
   bool value)
{
   uint16_t num_occupied_stations = 0;
   uint8_t mask;
   uint16_t byte_in_area;
   uint16_t areanumber =
      cl_iefb_bit_calculate_areanumber (number, &byte_in_area, &mask);
   cl_ry_t * first_area = clm_iefb_get_first_device_ry_area (
      clm,
      group_index,
      slave_device_index,
      &num_occupied_stations);

   /* Not possible to return error code to user */
   CC_ASSERT (num_occupied_stations != 0);
   CC_ASSERT (areanumber < num_occupied_stations);

   if (value)
   {
      (first_area + areanumber)->bytes[byte_in_area] |= mask;
   }
   else
   {
      (first_area + areanumber)->bytes[byte_in_area] &= ~mask;
   }
}

bool clm_iefb_get_ry_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number)
{
   uint16_t num_occupied_stations = 0;
   uint8_t mask;
   uint16_t byte_in_area;
   uint16_t areanumber =
      cl_iefb_bit_calculate_areanumber (number, &byte_in_area, &mask);
   const cl_ry_t * first_area = clm_iefb_get_first_device_ry_area (
      clm,
      group_index,
      slave_device_index,
      &num_occupied_stations);

   /* Not possible to return error code to user */
   CC_ASSERT (num_occupied_stations != 0);
   CC_ASSERT (areanumber < num_occupied_stations);

   return ((first_area + areanumber)->bytes[byte_in_area] & mask) > 0;
}

uint16_t clm_iefb_get_rwr_value (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number)
{
   uint16_t num_occupied_stations = 0;
   uint16_t register_in_area;
   uint16_t areanumber =
      cl_iefb_register_calculate_areanumber (number, &register_in_area);
   const cl_rwr_t * first_area = clm_iefb_get_first_device_rwr_area (
      clm,
      group_index,
      slave_device_index,
      &num_occupied_stations);

   /* Not possible to return error code to user */
   CC_ASSERT (num_occupied_stations != 0);
   CC_ASSERT (areanumber < num_occupied_stations);

   return CC_FROM_LE16 ((first_area + areanumber)->words[register_in_area]);
}

void clm_iefb_set_rww_value (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number,
   uint16_t value)
{
   uint16_t num_occupied_stations = 0;
   uint16_t register_in_area;
   uint16_t areanumber =
      cl_iefb_register_calculate_areanumber (number, &register_in_area);
   cl_rww_t * first_area = clm_iefb_get_first_device_rww_area (
      clm,
      group_index,
      slave_device_index,
      &num_occupied_stations);

   /* Not possible to return error code to user */
   CC_ASSERT (num_occupied_stations != 0);
   CC_ASSERT (areanumber < num_occupied_stations);

   (first_area + areanumber)->words[register_in_area] = CC_TO_LE16 (value);
}

uint16_t clm_iefb_get_rww_value (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number)
{
   uint16_t num_occupied_stations = 0;
   uint16_t register_in_area;
   uint16_t areanumber =
      cl_iefb_register_calculate_areanumber (number, &register_in_area);
   const cl_rww_t * first_area = clm_iefb_get_first_device_rww_area (
      clm,
      group_index,
      slave_device_index,
      &num_occupied_stations);

   /* Not possible to return error code to user */
   CC_ASSERT (num_occupied_stations != 0);
   CC_ASSERT (areanumber < num_occupied_stations);

   return CC_FROM_LE16 ((first_area + areanumber)->words[register_in_area]);
}

void clm_iefb_set_master_application_status (
   clm_t * clm,
   bool running,
   bool stopped_by_user)
{
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): Setting master application status. Running: %s  Stopped "
      "by user: %s\n",
      __LINE__,
      running ? "Yes" : "No",
      stopped_by_user ? "Yes" : "No");

   clm->master_local_unit_info = clm_iefb_calc_master_local_unit_info (
      clm->config.protocol_ver,
      running,
      stopped_by_user);
}

uint16_t clm_iefb_get_master_application_status (clm_t * clm)
{
   return clm->master_local_unit_info;
}

int clm_iefb_set_slave_communication_status (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   bool enabled)
{
   const clm_group_setting_t * group_setting;
   clm_group_data_t * group_data;
   clm_slave_device_data_t * slave_device_data;

   if (group_index >= clm->config.hier.number_of_groups)
   {
      return -1;
   }

   group_setting = &clm->config.hier.groups[group_index];
   group_data    = &clm->groups[group_index];

   if (slave_device_index >= group_setting->num_slave_devices)
   {
      return -1;
   }

   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): Setting slave communication status for group index %u, "
      "slave device index %u to %s.\n",
      __LINE__,
      group_index,
      slave_device_index,
      enabled ? "enabled" : "disabled");

   slave_device_data          = &group_data->slave_devices[slave_device_index];
   slave_device_data->enabled = enabled;

   return 0;
}

int clm_iefb_force_cyclic_transmission_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   bool force)
{
   const clm_group_setting_t * group_setting;
   clm_group_data_t * group_data;
   clm_slave_device_data_t * slave_device_data;

   if (group_index >= clm->config.hier.number_of_groups)
   {
      return -1;
   }

   group_setting = &clm->config.hier.groups[group_index];
   group_data    = &clm->groups[group_index];

   if (slave_device_index >= group_setting->num_slave_devices)
   {
      return -1;
   }

   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): Setting cyclic transmission bit for group index %u, "
      "slave device index %u to %s.\n",
      __LINE__,
      group_index,
      slave_device_index,
      force ? "forced high" : "normal");

   slave_device_data = &group_data->slave_devices[slave_device_index];
   slave_device_data->force_transmission_bit = force;

   return 0;
}

int clm_iefb_get_master_status (
   const clm_t * clm,
   clm_master_status_details_t * details)
{
   details->master_state          = clm->master_state;
   details->node_search_serial    = clm->node_search_serial;
   details->parameter_no          = clm->parameter_no;
   details->set_ip_request_serial = clm->set_ip_request_serial;

   return 0;
}

int clm_iefb_get_group_status (
   const clm_t * clm,
   uint16_t group_index,
   clm_group_status_details_t * details)
{
   const clm_group_data_t * group_data;

   if (group_index >= clm->config.hier.number_of_groups)
   {
      return -1;
   }

   group_data = &clm->groups[group_index];

   details->cyclic_transmission_state = group_data->cyclic_transmission_state;
   details->frame_sequence_no         = group_data->frame_sequence_no;
   details->group_index               = group_data->group_index;
   details->group_state               = group_data->group_state;
   details->timestamp_link_scan_start = group_data->timestamp_link_scan_start;
   details->total_occupied            = group_data->total_occupied;

   return 0;
}

const clm_slave_device_data_t * clm_iefb_get_device_connection_details (
   const clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index)
{
   const clm_group_setting_t * group_setting;
   const clm_group_data_t * group_data;

   if (group_index >= clm->config.hier.number_of_groups)
   {
      return NULL;
   }

   group_data    = &clm->groups[group_index];
   group_setting = &clm->config.hier.groups[group_index];

   if (slave_device_index >= group_setting->num_slave_devices)
   {
      return NULL;
   }

   return &group_data->slave_devices[slave_device_index];
}

void clm_iefb_statistics_clear_all (clm_t * clm)
{
   uint16_t group_index        = 0;
   uint16_t slave_device_index = 0;
   const clm_group_setting_t * group_setting;
   clm_group_data_t * group_data;
   clm_slave_device_data_t * slave_device_data;

   LOG_DEBUG (CL_CCIEFB_LOG, "CCIEFB(%d): Clear all statistics.\n", __LINE__);

   for (group_index = 0; group_index < clm->config.hier.number_of_groups;
        group_index++)
   {
      group_data    = &clm->groups[group_index];
      group_setting = &clm->config.hier.groups[group_index];

      for (slave_device_index = 0;
           slave_device_index < group_setting->num_slave_devices;
           slave_device_index++)
      {
         slave_device_data = &group_data->slave_devices[slave_device_index];

         clm_iefb_statistics_clear (&slave_device_data->statistics);
         clm_iefb_latest_received_clear (&slave_device_data->latest_frame);
      }
   }
}

void clm_iefb_periodic (clm_t * clm, uint32_t now)
{
   ssize_t recv_len = 0;
   cl_ipaddr_t remote_ip;
   uint16_t remote_port;

   cl_limiter_periodic (&clm->errorlimiter, now);

   /* Monitor state machine timers */
   if (cl_timer_is_expired (&clm->arbitration_timer, now))
   {
      cl_timer_stop (&clm->arbitration_timer);
      clm_iefb_group_fsm_event_all (clm, now, CLM_GROUP_EVENT_ARBITRATION_DONE);
   }
   clm_iefb_monitor_all_group_timers (clm, now);

   /* Receive frames from other masters, on operating systems where those
      broadcasts will not be received via the normal socket.*/
   if (clm->config.use_separate_arbitration_socket)
   {
      recv_len = clal_udp_recvfrom (
         clm->cciefb_arbitration_socket,
         &remote_ip,
         &remote_port,
         clm->cciefb_receivebuf,
         sizeof (clm->cciefb_receivebuf));

      if (recv_len > 0)
      {
         (void)clm_iefb_handle_input_frame (
            clm,
            now,
            clm->cciefb_receivebuf,
            (size_t)recv_len,
            remote_ip,
            remote_port);
      }
   }

   /* Receive and handle incoming CCIEFB data frames */
   do
   {
      recv_len = clal_udp_recvfrom (
         clm->cciefb_socket,
         &remote_ip,
         &remote_port,
         clm->cciefb_receivebuf,
         sizeof (clm->cciefb_receivebuf));

      if (recv_len > 0)
      {
         (void)clm_iefb_handle_input_frame (
            clm,
            now,
            clm->cciefb_receivebuf,
            (size_t)recv_len,
            remote_ip,
            remote_port);
      }
   } while (recv_len > 0);
}

int clm_iefb_init (clm_t * clm, uint32_t now)
{
#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
#endif
   uint16_t group_index = 0;
   clm_group_data_t * group_data;

   cl_limiter_init (&clm->errorlimiter, CLM_CCIEFB_ERRORCALLBACK_RETRIGGER_PERIOD);

#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   cl_util_ip_to_string (clm->config.master_id, ip_string);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): Initialising master CCIEFB. Listening on IP address %s "
      "port %u\n",
      __LINE__,
      ip_string,
      CL_CCIEFB_PORT);
#endif

#ifndef FUZZ_TEST
   clm->cciefb_socket = clal_udp_open (clm->config.master_id, CL_CCIEFB_PORT);
   if (clm->cciefb_socket == -1)
   {
      LOG_ERROR (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): Failed to open master CCIEFB socket.\n",
         __LINE__);
      return -1;
   }
#else
   clm->cciefb_socket = -1;
#endif

   if (clm->config.use_separate_arbitration_socket)
   {
#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
      cl_util_ip_to_string (clm->iefb_broadcast_ip, ip_string);
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): Opening separate CCIEFB arbitration socket. Listening "
         "on IP address %s port %u\n",
         __LINE__,
         ip_string,
         CL_CCIEFB_PORT);
#endif

#ifndef FUZZ_TEST
      clm->cciefb_arbitration_socket =
         clal_udp_open (clm->iefb_broadcast_ip, CL_CCIEFB_PORT);
      if (clm->cciefb_arbitration_socket == -1)
      {
         LOG_ERROR (
            CL_CCIEFB_LOG,
            "CCIEFB(%d): Failed to open master CCIEFB arbitration socket.\n",
            __LINE__);
         return -1;
      }
#else
      clm->cciefb_arbitration_socket = -1;
#endif

      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): Socket number for CCIEFB: %d  Arbitration: %d\n",
         __LINE__,
         clm->cciefb_socket,
         clm->cciefb_arbitration_socket);
   }
   else
   {
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): Socket number for CCIEFB: %d (No arbitration socket)\n",
         __LINE__,
         clm->cciefb_socket);
   }

   clm->latest_conflicting_master_ip = CL_IPADDR_INVALID;
   clm->master_state                 = CLM_MASTER_STATE_DOWN;
   clm->master_local_unit_info       = CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING;
   cl_timer_stop (&clm->arbitration_timer); /* Initialise timer */
   clm_iefb_device_fsm_tables_init (clm);
   clm_iefb_group_fsm_tables_init (clm);

   /* Initialise groups */
   clal_clear_memory (&clm->groups, sizeof (clm->groups));
   for (group_index = 0; group_index < clm->config.hier.number_of_groups;
        group_index++)
   {
      group_data = &clm->groups[group_index];

      group_data->group_state = CLM_GROUP_STATE_MASTER_DOWN;
      group_data->group_index = group_index;
   }
   clm_iefb_group_fsm_event_all (clm, now, CLM_GROUP_EVENT_STARTUP);
   clm_iefb_handle_new_config (clm, now);

   return 0;
}

void clm_iefb_exit (clm_t * clm)
{
   LOG_DEBUG (CL_CCIEFB_LOG, "CCIEFB(%d): Exiting master CCIEFB\n", __LINE__);

   clal_udp_close (clm->cciefb_socket);
   clm->cciefb_socket = -1;

   if (clm->config.use_separate_arbitration_socket)
   {
      clal_udp_close (clm->cciefb_arbitration_socket);
      clm->cciefb_arbitration_socket = -1;
   }
}
