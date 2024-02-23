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
 * @brief Implementing the slave state machine etc required for handling the
 *        cyclic data communication with the master.
 *
 *        See common/cl_iefb.h for building and parsing CCIEFB frames */

#ifdef UNIT_TEST
#define clal_udp_open                  mock_clal_udp_open
#define clal_udp_recvfrom_with_ifindex mock_clal_udp_recvfrom_with_ifindex
#define clal_udp_sendto                mock_clal_udp_sendto
#define clal_udp_close                 mock_clal_udp_close
#endif

#include "slave/cls_iefb.h"

#include "common/cl_iefb.h"
#include "common/cl_literals.h"
#include "common/cl_timer.h"
#include "common/cl_types.h"
#include "common/cl_util.h"

#include "osal_log.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define CLS_CCIEFB_WAIT_TIME_DISABLE_SLAVE        2500000 /* microseconds */
#define CLS_CCIEFB_LOGWARNING_RETRIGGER_PERIOD    1000000 /* microseconds*/
#define CLS_CCIEFB_ERRORCALLBACK_RETRIGGER_PERIOD 1000000 /* microseconds*/

#if LOG_WARNING_ENABLED(CL_CCIEFB_LOG)
static void cls_loglimiter_log_warning (
   cls_cciefb_logwarning_message_t message,
   uint32_t arg_num_a,
   uint16_t arg_num_b,
   uint16_t arg_num_c)
{
   /* Use the LOG_ macros instead of printf(), in order to also
      print the timestamp (available on some platforms) */
   switch (message)
   {
   case CLS_LOGLIMITER_MASTER_DUPLICATION:
      LOG_WARNING (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): Master duplication detected. New master IP 0x%08" PRIX32
         ". Send error frame.\n",
         __LINE__,
         arg_num_a);
      break;
   case CLS_LOGLIMITER_WRONG_NUMBER_OCCUPIED:
      LOG_WARNING (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): Wrong number of occupied stations from the PLC. "
         "Expected %" PRIu16 " but requested is %" PRIu16 ".\n",
         __LINE__,
         arg_num_b,
         arg_num_c);
      break;
   default:
      LOG_WARNING (CL_CCIEFB_LOG, "CCIEFB(%d): Unknown warning.\n", __LINE__);
      break;
   }
}
#endif

/**
 * Log a warning message only once.
 *
 * If a message has recently been logged, any new similar message is silently
 * dropped. A different message type will be logged regardless of the timer.
 *
 * CLS_LOGLIMITER_MASTER_DUPLICATION:
 *  - arg_num_a = IP address of new master
 *
 * CLS_LOGLIMITER_WRONG_NUMBER_OCCUPIED:
 *  - arg_num_b = expected number of occupied stations
 *  - arg_num_c = actual requested number of occupied stations
 *
 * @param limiter       Loglimiter instance
 * @param message       Enum describing the message.
 * @param arg_num_a     First numerical argument to the message, if any.
 * @param arg_num_b     Second numerical argument to the message, if any.
 * @param arg_num_c     Third numerical argument to the message, if any.
 * @param now           Current timestamp, in microseconds.
 */
void cls_iefb_log_warning_once (
   cl_limiter_t * limiter,
   cls_cciefb_logwarning_message_t message,
   uint32_t arg_num_a,
   uint16_t arg_num_b,
   uint16_t arg_num_c,
   uint32_t now)
{
#if LOG_WARNING_ENABLED(CL_CCIEFB_LOG)

   if (!cl_limiter_should_run_now (limiter, (int)message, now))
   {
      return;
   }

   cls_loglimiter_log_warning (message, arg_num_a, arg_num_b, arg_num_c);
#endif
}

/**
 * Copy incoming cyclic data from CCIEFB request
 *
 * @param cls                    c-link slave stack instance handle
 * @param request                Incoming CCIEFB request
 * @param start_number           Our slave station start number
 * @param transmission_bit       Transmission bit. Store incoming data if true,
 *                               otherwise clear the stored data.
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CLS_CYCLIC_01
 * @req REQ_CLS_CYCLIC_02
 * @req REQ_CLS_CYCLIC_03
 * @req REQ_CLS_STATUSBIT_02
 * @req REQ_CLS_STATUSBIT_03
 *
 */
int cls_iefb_copy_cyclic_data_from_request (
   cls_t * cls,
   const cls_cciefb_cyclic_request_info_t * request,
   uint16_t start_number,
   bool transmission_bit)
{
   const cl_ry_t * our_first_request_ry   = NULL;
   const cl_rww_t * our_first_request_rww = NULL;
   uint16_t num_occupied_stations         = cls->config.num_occupied_stations;
   uint16_t end_number     = start_number + num_occupied_stations - 1;
   uint16_t total_occupied = CC_FROM_LE16 (
      request->full_headers->cyclic_data_header.slave_total_occupied_station_count);

   /* Check validity of total occupied stations */
   if (
      total_occupied == 0 ||
      total_occupied > CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP ||
      total_occupied < num_occupied_stations)
   {
      return -1;
   }

   /* Check validity of start number */
   if (start_number == 0 || start_number > CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP)
   {
      return -1;
   }

   /* Check validity of end number */
   if (end_number > CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP || end_number > total_occupied)
   {
      return -1;
   }

   /* Pointers to our data in incoming request */
   our_first_request_ry  = cl_iefb_request_get_ry (request, start_number);
   our_first_request_rww = cl_iefb_request_get_rww (request, start_number);
   if ((our_first_request_ry == NULL) || (our_first_request_rww == NULL))
   {
      return -1;
   }

   if (transmission_bit)
   {
      clal_memcpy (
         &cls->cyclic_data_area.ry,
         sizeof (cls->cyclic_data_area.ry),
         our_first_request_ry,
         num_occupied_stations * sizeof (cl_ry_t));
      clal_memcpy (
         &cls->cyclic_data_area.rww,
         sizeof (cls->cyclic_data_area.rww),
         our_first_request_rww,
         num_occupied_stations * sizeof (cl_rww_t));
   }
   else
   {
      clal_clear_memory (
         &cls->cyclic_data_area.ry,
         num_occupied_stations * sizeof (cl_ry_t));
      clal_clear_memory (
         &cls->cyclic_data_area.rww,
         num_occupied_stations * sizeof (cl_rww_t));
   }

   return 0;
}

/**
 * Send a CCIEFB cyclic data response frame
 *
 * @param cls                    c-link slave stack instance handle
 * @param frame_sequence_no      Frame sequence number
 * @param group_no               Group number
 * @param include_data           True if cyclic data should be included, false
 *                               if the data should be replaced by zeros.
 * @param end_code               End code
 * @param remote_ip              Destination IP address
 * @param remote_port            Destination UDP port
 * @param slave_ip_addr          Slave IP address
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CL_PROTOCOL_51
 * @req REQ_CLS_CONFORMANCE_01
 * @req REQ_CLS_ERROR_04
 * @req REQ_CLS_GROUPS_03
 * @req REQ_CLS_SEQUENCE_01
 * @req REQ_CLS_STATUSBIT_01
 * @req REQ_CLS_STATUSBIT_05
 *
 */
static int cls_iefb_send_cyclic_response_frame (
   cls_t * cls,
   uint16_t frame_sequence_no,
   uint16_t group_no,
   bool include_data,
   cl_slmp_error_codes_t end_code,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port,
   cl_ipaddr_t slave_ip_addr)
{
   cls_cciefb_cyclic_response_info_t * output_frame;
   ssize_t sent_size = 0;

   CC_ASSERT (cl_is_slave_endcode_valid (end_code));

   if (include_data)
   {
      output_frame = &cls->cciefb_resp_frame_normal;
   }
   else
   {
      output_frame = &cls->cciefb_resp_frame_error;
   }

   cl_iefb_update_cyclic_response_frame (
      output_frame,
      slave_ip_addr,
      end_code,
      group_no,
      frame_sequence_no,
      cls->slave_application_status,
      cls->slave_err_code,
      cls->local_management_info);

   sent_size = clal_udp_sendto (
      cls->cciefb_socket,
      remote_ip,
      remote_port,
      output_frame->buffer,
      output_frame->udp_payload_len);

   if (sent_size < 0 || (size_t)sent_size != output_frame->udp_payload_len)
   {
      return -1;
   }

   return 0;
}

/**
 * Send a CCIEFB error response frame
 *
 * Uses master IP address, group number and frame sequence number from the
 * incoming request frame.
 *
 * @param cls                    c-link slave stack instance handle
 * @param request                Incoming CCIEFB request
 * @param end_code               End code
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CLS_ERROR_03
 * @req REQ_CLS_ERROR_02
 *
 */
static int cls_iefb_send_cyclic_response_error_frame (
   cls_t * cls,
   const cls_cciefb_cyclic_request_info_t * request,
   cl_slmp_error_codes_t end_code)
{
   return cls_iefb_send_cyclic_response_frame (
      cls,
      CC_FROM_LE16 (request->full_headers->cyclic_data_header.frame_sequence_no),
      request->full_headers->cyclic_data_header.group_no, /* uint8_t */
      false,
      end_code,
      request->remote_ip,
      request->remote_port,
      request->slave_ip_addr);
}

/**
 * Check if the given master ID is correct (equal to our master ID)
 *
 * @param cls              c-link slave stack instance handle
 * @param master_id        Master ID to look up
 * @return true if the given master ID is our master ID
 */
bool cls_iefb_is_master_id_correct (cls_t * cls, cl_ipaddr_t master_id)
{
   if (cls->master.master_id == CL_IPADDR_INVALID)
   {
      return false;
   }

   return master_id == cls->master.master_id;
}

/**
 * Determine whether we should trigger the master state change callback
 *
 * @param executed_before                    True if this has executed before
 * @param connected_to_master                True if there is cyclic data.
 * @param previous_connected_to_master       Previous value
 * @param protocol_ver                       Protocol version from the PLC.
 * @param previous_protocol_ver              Previous value
 * @param master_application_status          MasterLocalUnitInfo from the PLC.
 * @param previous_master_application_status Previous value
 * @return true if the callback should be triggered
 */
bool cls_iefb_should_trigger_master_state_callback (
   bool executed_before,
   bool connected_to_master,
   bool previous_connected_to_master,
   uint16_t protocol_ver,
   uint16_t previous_protocol_ver,
   uint16_t master_application_status,
   uint16_t previous_master_application_status)
{
   if (executed_before == false)
   {
      return true;
   }

   if (connected_to_master != previous_connected_to_master)
   {
      return true;
   }

   if (protocol_ver != previous_protocol_ver)
   {
      return true;
   }

   if (master_application_status != previous_master_application_status)
   {
      return true;
   }

   return false;
}

/**
 * Prepare values for the master application state change callback
 *
 * @param connected_to_master                True if there is cyclic data to
 *                                           and from the PLC.
 * @param protocol_ver                       Protocol version from the PLC.
 * @param master_application_status          MasterLocalUnitInfo from the PLC.
 * @param filtered_connected_and_running     Resulting value showing whether the
 *                                           master application is running on
 *                                           the PLC. Set to false if the PLC is
 *                                           disconnected.
 * @param filtered_stopped_by_user           Resluting value whether the master
 *                                           application is stopped by user.
 *                                           Only supported for version 2 and
 *                                           higher. Set to false if the PLC
 *                                           is disconnected.
 * @param filtered_protocol_ver              Resulting protocol version. Set
 *                                           to 0 if the PLC is disconnected.
 * @param filtered_master_application_status Resulting MasterLocalUnitInfo.
 *                                           Set to 0 if the PLC is
 *                                           disconnected.
 */
void cls_iefb_filter_master_state_callback_values (
   bool connected_to_master,
   uint16_t protocol_ver,
   uint16_t master_application_status,
   bool * filtered_connected_and_running,
   bool * filtered_stopped_by_user,
   uint16_t * filtered_protocol_ver,
   uint16_t * filtered_master_application_status)
{
   bool running = false;

   if (connected_to_master == false)
   {
      *filtered_connected_and_running     = false;
      *filtered_protocol_ver              = 0;
      *filtered_master_application_status = 0;
      *filtered_stopped_by_user           = false;
      return;
   }

   running = master_application_status & BIT (0);
   if (protocol_ver > 1 && running == false)
   {
      *filtered_stopped_by_user = master_application_status & BIT (1);
   }
   else
   {
      *filtered_stopped_by_user = false;
   }
   *filtered_connected_and_running     = running;
   *filtered_protocol_ver              = protocol_ver;
   *filtered_master_application_status = master_application_status;
}

/**
 * Trigger the master application state change callback, if necessary
 *
 * @param cls                       c-link slave stack instance handle
 * @param connected_to_master       True if there is cyclic data to and from the
 *                                  PLC.
 * @param protocol_ver              Protocol version from the PLC.
 * @param master_application_status MasterLocalUnitInfo from the PLC.
 *
 * @req REQ_CLS_CONFORMANCE_06
 *
 */
static void cls_iefb_handle_master_state_callback (
   cls_t * cls,
   bool connected_to_master,
   uint16_t protocol_ver,
   uint16_t master_application_status)
{
   bool filtered_connected_and_running         = false;
   bool filtered_stopped_by_user               = false;
   uint16_t filtered_protocol_ver              = 0;
   uint16_t filtered_master_application_status = 0;

   if (cls_iefb_should_trigger_master_state_callback (
          cls->master_state_callback_trigger_data.executed_before,
          connected_to_master,
          cls->master_state_callback_trigger_data.previous_connected_to_master,
          protocol_ver,
          cls->master_state_callback_trigger_data.previous_protocol_ver,
          master_application_status,
          cls->master_state_callback_trigger_data.previous_master_application_status))
   {
      cls_iefb_filter_master_state_callback_values (
         connected_to_master,
         protocol_ver,
         master_application_status,
         &filtered_connected_and_running,
         &filtered_stopped_by_user,
         &filtered_protocol_ver,
         &filtered_master_application_status);

      /* Trigger master state change callback to application */
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): %s user callback for changing master application "
         "state. Connected: %s  Running: %s  Stopped by user: %s\n",
         __LINE__,
         (cls->config.master_running_cb != NULL) ? "Calling" : "No",
         connected_to_master ? "Yes" : "No",
         filtered_connected_and_running ? "Yes" : "No",
         filtered_stopped_by_user ? "Yes" : "No");
      if (cls->config.master_running_cb != NULL)
      {
         cls->config.master_running_cb (
            cls,
            cls->config.cb_arg,
            connected_to_master,
            filtered_connected_and_running,
            filtered_stopped_by_user,
            filtered_protocol_ver,
            filtered_master_application_status);
      }
   }

   cls->master_state_callback_trigger_data.executed_before = true;
   cls->master_state_callback_trigger_data.previous_connected_to_master =
      connected_to_master;
   cls->master_state_callback_trigger_data.previous_protocol_ver = protocol_ver;
   cls->master_state_callback_trigger_data.previous_master_application_status =
      master_application_status;
}

/**
 * Trigger the error application callback, if implemented.
 *
 * See \a cls_error_ind_t() for details on argument value combinations.
 *
 * If several similar messages are triggered repeatedly (interval max
 * CLS_CCIEFB_ERRORCALLBACK_RETRIGGER_PERIOD microseconds), then
 * the callback is triggered once only.
 *
 * @param cls              c-link slave stack instance handle
 * @param now              Current timestamp, in microseconds
 * @param error_message    Error message enum
 * @param ip_addr          IP address, if available
 * @param argument_2       Numeric argument, if available
 */
static void cls_iefb_trigger_error_callback (
   cls_t * cls,
   uint32_t now,
   cls_error_message_t error_message,
   cl_ipaddr_t ip_addr,
   uint16_t argument_2)
{
   if (!cl_limiter_should_run_now (&cls->errorlimiter, (int)error_message, now))
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
      (cls->config.error_cb != NULL) ? "Calling" : "No",
      cl_literals_get_slave_error_message (error_message),
      ip_string,
      argument_2);
#endif
   if (cls->config.error_cb != NULL)
   {
      cls->config
         .error_cb (cls, cls->config.cb_arg, error_message, ip_addr, argument_2);
   }
}

/******************* Access to cyclic data memory areas ********************/

cl_rx_t * cls_iefb_get_first_rx_area (cls_t * cls)
{
   return cls->cciefb_resp_frame_normal.first_rx;
}

const cl_ry_t * cls_iefb_get_first_ry_area (cls_t * cls)
{
   return (cl_ry_t *)&cls->cyclic_data_area.ry;
}

cl_rwr_t * cls_iefb_get_first_rwr_area (cls_t * cls)
{
   return cls->cciefb_resp_frame_normal.first_rwr;
}

const cl_rww_t * cls_iefb_get_first_rww_area (cls_t * cls)
{
   return (cl_rww_t *)&cls->cyclic_data_area.rww;
}

void cls_iefb_set_rx_bit (cls_t * cls, uint16_t number, bool value)
{
   uint8_t mask;
   uint16_t byte_in_area;
   cl_rx_t * rx_area;
   uint16_t areanumber =
      cl_iefb_bit_calculate_areanumber (number, &byte_in_area, &mask);

   /* Not possible to return error code to user */
   CC_ASSERT (areanumber < cls->config.num_occupied_stations);

   rx_area = cl_iefb_get_rx_area (&cls->cciefb_resp_frame_normal, areanumber);

   if (value)
   {
      rx_area->bytes[byte_in_area] |= mask;
   }
   else
   {
      rx_area->bytes[byte_in_area] &= ~mask;
   }
}

bool cls_iefb_get_rx_bit (cls_t * cls, uint16_t number)
{
   uint8_t mask;
   uint16_t byte_in_area;
   cl_rx_t * rx_area;
   uint16_t areanumber =
      cl_iefb_bit_calculate_areanumber (number, &byte_in_area, &mask);

   /* Not possible to return error code to user */
   CC_ASSERT (areanumber < cls->config.num_occupied_stations);

   rx_area = cl_iefb_get_rx_area (&cls->cciefb_resp_frame_normal, areanumber);

   return (rx_area->bytes[byte_in_area] & mask) > 0;
}

bool cls_iefb_get_ry_bit (cls_t * cls, uint16_t number)
{
   uint8_t mask;
   uint16_t byte_in_area;
   uint16_t areanumber =
      cl_iefb_bit_calculate_areanumber (number, &byte_in_area, &mask);

   /* Not possible to return error code to user */
   CC_ASSERT (areanumber < cls->config.num_occupied_stations);

   return (cls->cyclic_data_area.ry[areanumber].bytes[byte_in_area] & mask) > 0;
}

void cls_iefb_set_rwr_value (cls_t * cls, uint16_t number, uint16_t value)
{
   uint16_t register_in_area;
   cl_rwr_t * rwr_area;
   uint16_t areanumber =
      cl_iefb_register_calculate_areanumber (number, &register_in_area);

   /* Not possible to return error code to user */
   CC_ASSERT (areanumber < cls->config.num_occupied_stations);

   rwr_area = cl_iefb_get_rwr_area (&cls->cciefb_resp_frame_normal, areanumber);

   rwr_area->words[register_in_area] = CC_TO_LE16 (value);
}

uint16_t cls_iefb_get_rwr_value (cls_t * cls, uint16_t number)
{
   uint16_t register_in_area;
   cl_rwr_t * rwr_area;
   uint16_t areanumber =
      cl_iefb_register_calculate_areanumber (number, &register_in_area);

   /* Not possible to return error code to user */
   CC_ASSERT (areanumber < cls->config.num_occupied_stations);

   rwr_area = cl_iefb_get_rwr_area (&cls->cciefb_resp_frame_normal, areanumber);

   return CC_FROM_LE16 (rwr_area->words[register_in_area]);
}

uint16_t cls_iefb_get_rww_value (cls_t * cls, uint16_t number)
{
   uint16_t register_in_area;
   uint16_t areanumber =
      cl_iefb_register_calculate_areanumber (number, &register_in_area);

   /* Not possible to return error code to user */
   CC_ASSERT (areanumber < cls->config.num_occupied_stations);

   return CC_FROM_LE16 (
      cls->cyclic_data_area.rww[areanumber].words[register_in_area]);
}

/***************************************************************************/

/**
 * Clear information on connected master. Stop the receive timeout timer.
 *
 * Might trigger the cls_master_state_ind_t callback.
 *
 * @param cls                    c-link slave stack instance handle
 *
 * @req REQ_CLS_CONTRMASTER_02
 *
 */
static void cls_iefb_clear_master_info (cls_t * cls)
{
   LOG_DEBUG (CL_CCIEFB_LOG, "CCIEFB(%d): Clearing master information.\n", __LINE__);

   clal_clear_memory (&cls->master, sizeof (cls->master));
   cls->master.master_id = CL_IPADDR_INVALID;
   cl_timer_stop (&cls->receive_timer);
   cl_timer_stop (&cls->timer_for_disabling_slave);

   cls_iefb_handle_master_state_callback (
      cls,
      false, /* Not connected to master */
      0,     /* No protocol version */
      0 /* No master application status available */);
}

/**
 * Search incoming request for slave parameters.
 *
 * Depending on current state, it will trigger event for "new master
 * detected" and others.
 *
 * Stores \a cls->slave_station_no
 *
 * Returns -1 if our current slave_id is invalid (0.0.0.0)
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param cyclic_request         Incoming CCIEFB request
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CLS_GROUPS_01
 * @req REQ_CLS_CONFORMANCE_09
 * @req REQ_CLS_CONFORMANCE_10
 * @req REQ_CLS_STATUSBIT_04
 *
 */
int cls_iefb_search_slave_parameters (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * cyclic_request)
{
   /* Algorithm according to CCIEFB overview section 5.3.1 a3 */

#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
#endif

   bool found_my_slave_id                    = false;
   bool my_master_transmission_state_enabled = false;
   uint16_t my_slave_station_no              = 0;
   uint16_t implied_occupation_count         = 0;
   const cl_cciefb_cyclic_req_data_header_t * cyclic_data_header =
      &cyclic_request->full_headers->cyclic_data_header;
   uint16_t total_occupied =
      CC_FROM_LE16 (cyclic_data_header->slave_total_occupied_station_count);
   uint16_t transmission_states =
      CC_FROM_LE16 (cyclic_data_header->cyclic_transmission_state);
   cl_ipaddr_t master_id = CC_FROM_LE32 (cyclic_data_header->master_id);

   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): Searching slave parameters in incoming request. Total "
      "occupied stations: %u\n",
      __LINE__,
      total_occupied);

   if (
      cls->state == CLS_SLAVE_STATE_SLAVE_DOWN ||
      cls->state == CLS_SLAVE_STATE_SLAVE_DISABLED ||
      cls->state == CLS_SLAVE_STATE_LAST)
   {
      /* Discard all incoming frames */
      return 0;
   }

   /* Check that message is for me, and has correct number of stations */
   if (
      cl_iefb_analyze_slave_ids (
         cyclic_request->slave_ip_addr,
         total_occupied,
         cyclic_request->first_slave_id,
         &found_my_slave_id,
         &my_slave_station_no,
         &implied_occupation_count) != 0)
   {
      /* cls->slave_id is invalid */
      return -1;
   }
   if (found_my_slave_id == false)
   {
      /* Frame for other group */
#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
      cl_util_ip_to_string (cyclic_request->slave_ip_addr, ip_string);
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): The frame was for another group (It does not contain my "
         "IP address %s)\n",
         __LINE__,
         ip_string);
#endif
      return 0;
   }

   /* Wrong number of occupied slave stations */
   if (implied_occupation_count != cls->config.num_occupied_stations)
   {
      cls_iefb_log_warning_once (
         &cls->loglimiter,
         CLS_LOGLIMITER_WRONG_NUMBER_OCCUPIED,
         CL_IPADDR_INVALID,
         cls->config.num_occupied_stations,
         implied_occupation_count,
         now);
      cls_iefb_trigger_error_callback (
         cls,
         now,
         CLS_ERROR_WRONG_NUMBER_OCCUPIED,
         CL_IPADDR_INVALID,
         implied_occupation_count);
      cls_iefb_fsm_event (
         cls,
         now,
         cyclic_request,
         CLS_SLAVE_EVENT_CYCLIC_WRONG_STATIONCOUNT);

      return 0;
   }

   my_master_transmission_state_enabled = cl_iefb_extract_my_transmission_state (
      transmission_states,
      my_slave_station_no);

   if (cls->state == CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE)
   {
      if (my_master_transmission_state_enabled)
      {
         cls_iefb_fsm_event (
            cls,
            now,
            cyclic_request,
            CLS_SLAVE_EVENT_CYCLIC_INCOMING_WHEN_DISABLED);
      }
      /* Do not care about frames trying to connect */

      return 0;
   }

   if (cls->state == CLS_SLAVE_STATE_MASTER_NONE)
   {
      if (my_master_transmission_state_enabled)
      {
         /* Slave duplication, or master has not yet timed out. Drop frame. */
         LOG_DEBUG (
            CL_CCIEFB_LOG,
            "CCIEFB(%d): Duplicate SLAVE ID. (This slave might just be "
            "restarted, wait for master timeout). Drop frame.\n",
            __LINE__);
         cls_iefb_trigger_error_callback (
            cls,
            now,
            CLS_ERROR_SLAVE_STATION_DUPLICATION,
            CL_IPADDR_INVALID,
            0);

         return 0;
      }

      cls->master.slave_station_no = my_slave_station_no;
      cls_iefb_fsm_event (
         cls,
         now,
         cyclic_request,
         CLS_SLAVE_EVENT_CYCLIC_NEW_MASTER);

      return 0;
   }

   /* State CLS_SLAVE_STATE_MASTER_CONTROL */
   if (cls_iefb_is_master_id_correct (cls, master_id) == false)
   {
      /* Typically checked already before calling this function */
      return 0;
   }

   if (my_master_transmission_state_enabled)
   {
      /* Slave duplication? Is this correct? */
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): Master should have disabled transmission state when "
         "changing parameters. Drop frame.\n",
         __LINE__);

      return 0;
   }

   /* Update communication parameter values.
      Note that the master ID is actually the same as before */
   cls->master.slave_station_no = my_slave_station_no;
   cls_iefb_fsm_event (cls, now, cyclic_request, CLS_SLAVE_EVENT_CYCLIC_NEW_MASTER);

   return 0;
}

/*********************** State machine ******************************/

/**
 * Initialise the slave.
 *
 * This is a callback for the state machine.
 *
 * Might trigger the cls_master_state_ind_t callback.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 * @return Next event to trigger
 */
cls_slave_event_t cls_iefb_slave_init (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   /* Note that request typically is NULL in this function */

   LOG_INFO (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): Initialising CCIEFB slave stack\n",
      __LINE__);

   /* Initialize timers */
   cl_timer_stop (&cls->receive_timer);
   cl_timer_stop (&cls->timer_for_disabling_slave);

   clal_clear_memory (&cls->cyclic_data_area, sizeof (cls->cyclic_data_area));
   clal_clear_memory (
      &cls->master_state_callback_trigger_data,
      sizeof (cls->master_state_callback_trigger_data));

   cls_iefb_clear_master_info (cls);

   cls->slave_application_status = CL_SLAVE_APPL_OPERATION_STATUS_OPERATING;
   cls->local_management_info =
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO;
   cls->slave_err_code =
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE;
   cls->endcode_slave_disabled = CL_SLMP_ENDCODE_CCIEFB_SLAVE_REQUESTS_DISCONNECT;

   /* Prepare frame used for normal responses.
      This will hold RX and RWw data, as updated by user application */

   cl_iefb_initialise_cyclic_response_frame (
      cls->cciefb_sendbuf_normal,
      sizeof (cls->cciefb_sendbuf_normal),
      cls->config.num_occupied_stations,
      cls->config.vendor_code,
      cls->config.model_code,
      cls->config.equipment_ver,
      &cls->cciefb_resp_frame_normal);

   /* Prepare frame used for error responses.
      All RX and RWw data is zero */

   cl_iefb_initialise_cyclic_response_frame (
      cls->cciefb_sendbuf_error,
      sizeof (cls->cciefb_sendbuf_error),
      cls->config.num_occupied_stations,
      cls->config.vendor_code,
      cls->config.model_code,
      cls->config.equipment_ver,
      &cls->cciefb_resp_frame_error);

   return CLS_SLAVE_EVENT_NONE;
}

/**
 * Handle wrong master, by sending an error frame (to the illegitimate master).
 *
 * This is a callback for the state machine.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 * @return Next event to trigger
 *
 * @req REQ_CLS_ERROR_07
 * @req REQ_CLS_ERROR_08
 * @req REQ_CLS_CONFORMANCE_07
 *
 */
cls_slave_event_t cls_iefb_handle_wrong_master (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   cl_ipaddr_t new_master_id = CL_IPADDR_INVALID;
   if (request == NULL)
   {
      return CLS_SLAVE_EVENT_NONE;
   }

   new_master_id =
      CC_FROM_LE32 (request->full_headers->cyclic_data_header.master_id);

   cls_iefb_log_warning_once (
      &cls->loglimiter,
      CLS_LOGLIMITER_MASTER_DUPLICATION,
      new_master_id,
      0,
      0,
      now);
   cls_iefb_trigger_error_callback (
      cls,
      now,
      CLS_ERROR_MASTER_STATION_DUPLICATION,
      new_master_id,
      0);

   (void)cls_iefb_send_cyclic_response_error_frame (
      cls,
      request,
      CL_SLMP_ENDCODE_CCIEFB_MASTER_DUPLICATION);

   return CLS_SLAVE_EVENT_NONE;
}

/**
 * Handle wrong station count in the request, by sending an error frame.
 *
 * This is a callback for the state machine.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 * @return Next event to trigger
 *
 * @req REQ_CLS_ERROR_01
 * @req REQ_CLS_ERROR_10
 *
 */
cls_slave_event_t cls_iefb_handle_wrong_stationcount (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   if (request == NULL)
   {
      return CLS_SLAVE_EVENT_NONE;
   }

   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): Wrong number of occupied stations in the request. "
      "Send error frame.\n",
      __LINE__);
   (void)cls_iefb_send_cyclic_response_error_frame (
      cls,
      request,
      CL_SLMP_ENDCODE_CCIEFB_WRONG_NUMBER_OCCUPIED_STATIONS);

   return CLS_SLAVE_EVENT_NONE;
}

/**
 * Handle incoming request when we are disabled, by sending an error frame.
 *
 * This is a callback for the state machine.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 * @return Next event to trigger
 *
 * @req REQ_CLS_ERROR_05
 *
 */
cls_slave_event_t cls_iefb_handle_incoming_when_disabled (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   /* We have already checked that cyclic_transmission bit is on for us */

   if (request == NULL)
   {
      return CLS_SLAVE_EVENT_NONE;
   }

   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): Incoming request when we are disabled. "
      "Send error frame.\n",
      __LINE__);
   (void)cls_iefb_send_cyclic_response_error_frame (
      cls,
      request,
      cls->endcode_slave_disabled);

   return CLS_SLAVE_EVENT_NONE;
}

/**
 * Handle a new master, by updating data and starting the timeout timer.
 *
 * This is a callback for the state machine.
 *
 * Typically triggers a new event for the cyclic data.
 * Will trigger the cls_connect_ind_t callback.
 * Might trigger the cls_master_state_ind_t callback.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 * @return Next event to trigger
 *
 * @req REQ_CLS_CONTRMASTER_01
 */
cls_slave_event_t cls_iefb_handle_new_or_updated_master (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   cl_cciefb_cyclic_req_data_header_t * cyclic_data_header;
#if LOG_INFO_ENABLED(CL_CCIEFB_LOG)
   char master_ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
   char slave_ip_string[CL_INET_ADDRSTR_SIZE]  = {0}; /** Terminated string */
#endif

   if (request == NULL)
   {
      return CLS_SLAVE_EVENT_NONE;
   }

   cyclic_data_header        = &request->full_headers->cyclic_data_header;
   cls->master.master_id     = CC_FROM_LE32 (cyclic_data_header->master_id);
   cls->master.parameter_no  = CC_FROM_LE16 (cyclic_data_header->parameter_no);
   cls->master.group_no      = cyclic_data_header->group_no; /* uint8_t */
   cls->master.timeout_value = CC_FROM_LE16 (cyclic_data_header->timeout_value);
   cls->master.parallel_off_timeout_count =
      CC_FROM_LE16 (cyclic_data_header->parallel_off_timeout_count);
   cls->master.total_occupied_station_count =
      CC_FROM_LE16 (cyclic_data_header->slave_total_occupied_station_count);
   cls->master.clock_info = CC_FROM_LE64 (
      request->full_headers->master_station_notification.clock_info);
   cls->master.clock_info_valid = cls->master.clock_info != 0;
   cls->master.protocol_ver =
      CC_FROM_LE16 (request->full_headers->cyclic_header.protocol_ver);

#if LOG_INFO_ENABLED(CL_CCIEFB_LOG)
   cl_util_ip_to_string (cls->master.master_id, master_ip_string);
   cl_util_ip_to_string (request->slave_ip_addr, slave_ip_string);
   LOG_INFO (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): New master with IP: %s  My IP address: %s\n",
      __LINE__,
      master_ip_string,
      slave_ip_string);
   LOG_INFO (
      CL_CCIEFB_LOG,
      "CCIEFB(%d):    Master protocol version: %" PRIu16
      "  Group number: %" PRIu16 "  Total occupied stations in group: %" PRIu16
      "\n",
      __LINE__,
      cls->master.protocol_ver,
      cls->master.group_no,
      cls->master.total_occupied_station_count);
   LOG_INFO (
      CL_CCIEFB_LOG,
      "CCIEFB(%d):    Slave station number: %" PRIu16
      "  Parameter id number: %" PRIu16 "  Timeout: %" PRIu16 " ms x %" PRIu16
      "\n",
      __LINE__,
      cls->master.slave_station_no,
      cls->master.parameter_no,
      cls->master.timeout_value,
      cls->master.parallel_off_timeout_count);
   LOG_INFO (
      CL_CCIEFB_LOG,
      "CCIEFB(%d):    Master state (0=stop 1=run 2=stopped by user): %" PRIu16
      "  Master clock: %" PRIu64 " (UNIX timestamp in milliseconds)\n",
      __LINE__,
      CC_FROM_LE16 (request->full_headers->master_station_notification
                       .master_local_unit_info),
      cls->master.clock_info);
#endif

   cl_timer_start (
      &cls->receive_timer,
      (uint32_t)cl_calculate_total_timeout_us (
         cls->master.timeout_value,
         cls->master.parallel_off_timeout_count),
      now);

   /* Trigger informational callback to application */
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): %s user callback for connect\n",
      __LINE__,
      (cls->config.connect_cb != NULL) ? "Calling" : "No");
   if (cls->config.connect_cb != NULL)
   {
      cls->config.connect_cb (
         cls,
         cls->config.cb_arg,
         cls->master.master_id,
         cls->master.group_no,
         cls->master.slave_station_no);
   }

   cls_iefb_handle_master_state_callback (
      cls,
      true,
      CC_FROM_LE16 (request->full_headers->cyclic_header.protocol_ver),
      CC_FROM_LE16 (request->full_headers->master_station_notification
                       .master_local_unit_info));

   return CLS_SLAVE_EVENT_CYCLIC_CORRECT_MASTER;
}

/**
 * Handle incoming cyclic CCIEFB data, by sending a cyclic response frame.
 *
 * This is a callback for the state machine.
 *
 * Might trigger the cls_master_state_ind_t callback.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 * @return Next event to trigger
 *
 * @req REQ_CLS_UDP_01
 *
 */
cls_slave_event_t cls_iefb_handle_cyclic_event (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   uint16_t transmission_states;
   bool transmission_bit;

   if (request == NULL)
   {
      return CLS_SLAVE_EVENT_NONE;
   }

   transmission_states = CC_FROM_LE16 (
      request->full_headers->cyclic_data_header.cyclic_transmission_state);

   /* Use incoming cyclic data if transmission state is valid,
    * otherwise clear the stored data. */
   transmission_bit = cl_iefb_extract_my_transmission_state (
      transmission_states,
      cls->master.slave_station_no);

   if (
      cls_iefb_copy_cyclic_data_from_request (
         cls,
         request,
         cls->master.slave_station_no,
         transmission_bit) != 0)
   {
      return CLS_SLAVE_EVENT_NONE;
   }

   /* Use incoming master timestamp.
      It will be invalid if the value is 0 (no clock info in master) */
   cls->master.clock_info = CC_FROM_LE64 (
      request->full_headers->master_station_notification.clock_info);
   cls->master.clock_info_valid = cls->master.clock_info != 0;

   cls_iefb_handle_master_state_callback (
      cls,
      true,
      CC_FROM_LE16 (request->full_headers->cyclic_header.protocol_ver),
      CC_FROM_LE16 (request->full_headers->master_station_notification
                       .master_local_unit_info));

   (void)cls_iefb_send_cyclic_response_frame (
      cls,
      CC_FROM_LE16 (request->full_headers->cyclic_data_header.frame_sequence_no),
      request->full_headers->cyclic_data_header.group_no, /* uint8_t in frame */
      transmission_bit,
      CL_SLMP_ENDCODE_SUCCESS,
      request->remote_ip,
      request->remote_port,
      request->slave_ip_addr);

   cl_timer_restart (&cls->receive_timer, now);

   return CLS_SLAVE_EVENT_NONE;
}

/**
 * Handle that the master times out. Log only.
 *
 * This is a callback for the state machine.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 * @return Next event to trigger
 */
cls_slave_event_t cls_iefb_log_timeout_master (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   /* Note that request typically is NULL in this function */

   LOG_WARNING (CL_CCIEFB_LOG, "CCIEFB(%d): The master did time out.\n", __LINE__);

   return CLS_SLAVE_EVENT_NONE;
}

/**
 * Handle that the IP address has been updated by some external actor. Log only.
 *
 * This is a callback for the state machine.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 * @return Next event to trigger
 */
cls_slave_event_t cls_iefb_log_ip_addr_updated (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   /* Note that request typically is NULL in this function */

   LOG_INFO (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): The IP address has been changed. Restart PLC "
      "communication (if enabled).\n",
      __LINE__);

   return CLS_SLAVE_EVENT_NONE;
}

/**
 * Handle that the slave application stops the communication. Log only.
 *
 * This is a callback for the state machine.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 * @return Next event to trigger
 */
cls_slave_event_t cls_iefb_log_slave_disabled (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   /* Note that request typically is NULL in this function */

   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): State transition due to disabled slave.\n",
      __LINE__);

   return CLS_SLAVE_EVENT_NONE;
}

/**
 * Handle that the slave application enabled communication. Log only.
 *
 * This is a callback for the state machine.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 * @return Next event to trigger
 */
cls_slave_event_t cls_iefb_log_slave_reenabled (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   /* Note that request typically is NULL in this function */

   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): State transition due to re-enabled slave.\n",
      __LINE__);

   return CLS_SLAVE_EVENT_NONE;
}

/**
 * On entry for state CLS_SLAVE_STATE_MASTER_NONE.
 *
 * This is a callback for the state machine.
 *
 * Triggers the cls_master_state_ind_t callback.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 * @return Next event to trigger
 */
cls_slave_event_t cls_iefb_on_entry_master_none (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   /* Note that request typically is NULL in this function */

   cls_iefb_clear_master_info (cls);

   return CLS_SLAVE_EVENT_NONE;
}

/**
 * On exit for state CLS_SLAVE_STATE_MASTER_CONTROL.
 *
 * This is a callback for the state machine.
 *
 * Triggers the cls_disconnect_ind_t callback.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 * @return Next event to trigger
 */
static cls_slave_event_t cls_iefb_on_exit_master_control (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   /* Note that request typically is NULL in this function */

   /* Trigger informational callback to application */
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): %s user callback for disconnect\n",
      __LINE__,
      (cls->config.disconnect_cb != NULL) ? "Calling" : "No");
   if (cls->config.disconnect_cb != NULL)
   {
      cls->config.disconnect_cb (cls, cls->config.cb_arg);
   }

   return CLS_SLAVE_EVENT_NONE;
}

/**
 * On entry for state CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE.
 *
 * This is a callback for the state machine.
 *
 * Triggers the cls_master_state_ind_t callback.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 * @return Next event to trigger
 */
cls_slave_event_t cls_iefb_on_entry_wait_disabling_slave (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   /* Note that request typically is NULL in this function */

   cls_iefb_clear_master_info (cls);

   cl_timer_start (
      &cls->timer_for_disabling_slave,
      CLS_CCIEFB_WAIT_TIME_DISABLE_SLAVE,
      now);

   return CLS_SLAVE_EVENT_NONE;
}

/**
 * On entry for state CLS_SLAVE_STATE_SLAVE_DISABLED.
 *
 * This is a callback for the state machine.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 * @return Next event to trigger
 */
cls_slave_event_t cls_iefb_on_entry_slave_disabled (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   /* Note that request typically is NULL in this function */

   LOG_INFO (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): The slave cyclic data is now fully disabled.\n",
      __LINE__);

   cl_timer_stop (&cls->timer_for_disabling_slave);

   return CLS_SLAVE_EVENT_NONE;
}

/** Slave state machine transitions */
// clang-format off
const cls_slave_fsm_transition_t transitions[] = {
   /* State SLAVE_DOWN */
   {CLS_SLAVE_STATE_SLAVE_DOWN, CLS_SLAVE_EVENT_STARTUP, CLS_SLAVE_STATE_MASTER_NONE, cls_iefb_slave_init},

   /* State MASTER_NONE
      First transition is required by test case 1.4(4) "Wrong number of occupied stations".
      Contradicts table "State Transition Table of Slave Station (MasterNone)"
      in the CCIEFB Protocol specification. */
   {CLS_SLAVE_STATE_MASTER_NONE, CLS_SLAVE_EVENT_CYCLIC_NEW_MASTER,         CLS_SLAVE_STATE_MASTER_CONTROL, cls_iefb_handle_new_or_updated_master},
   {CLS_SLAVE_STATE_MASTER_NONE, CLS_SLAVE_EVENT_CYCLIC_WRONG_STATIONCOUNT, CLS_SLAVE_STATE_MASTER_NONE,    cls_iefb_handle_wrong_stationcount},
   {CLS_SLAVE_STATE_MASTER_NONE, CLS_SLAVE_EVENT_DISABLE_SLAVE,             CLS_SLAVE_STATE_SLAVE_DISABLED, cls_iefb_log_slave_disabled},

   /* State MASTER_CONTROL */
   {CLS_SLAVE_STATE_MASTER_CONTROL, CLS_SLAVE_EVENT_CYCLIC_CORRECT_MASTER,     CLS_SLAVE_STATE_MASTER_CONTROL,       cls_iefb_handle_cyclic_event},
   {CLS_SLAVE_STATE_MASTER_CONTROL, CLS_SLAVE_EVENT_CYCLIC_WRONG_MASTER,       CLS_SLAVE_STATE_MASTER_CONTROL,       cls_iefb_handle_wrong_master},
   {CLS_SLAVE_STATE_MASTER_CONTROL, CLS_SLAVE_EVENT_CYCLIC_NEW_MASTER,         CLS_SLAVE_STATE_MASTER_CONTROL,       cls_iefb_handle_new_or_updated_master},
   {CLS_SLAVE_STATE_MASTER_CONTROL, CLS_SLAVE_EVENT_CYCLIC_WRONG_STATIONCOUNT, CLS_SLAVE_STATE_MASTER_NONE,          cls_iefb_handle_wrong_stationcount},
   {CLS_SLAVE_STATE_MASTER_CONTROL, CLS_SLAVE_EVENT_TIMEOUT_MASTER,            CLS_SLAVE_STATE_MASTER_NONE,          cls_iefb_log_timeout_master},
   {CLS_SLAVE_STATE_MASTER_CONTROL, CLS_SLAVE_EVENT_IP_UPDATED,                CLS_SLAVE_STATE_MASTER_NONE,          cls_iefb_log_ip_addr_updated},
   {CLS_SLAVE_STATE_MASTER_CONTROL, CLS_SLAVE_EVENT_DISABLE_SLAVE,             CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE, cls_iefb_log_slave_disabled},

   /* State WAIT_DISABLING_SLAVE */
   {CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE, CLS_SLAVE_EVENT_REENABLE_SLAVE,                CLS_SLAVE_STATE_MASTER_NONE,          cls_iefb_log_slave_reenabled},
   {CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE, CLS_SLAVE_EVENT_CYCLIC_INCOMING_WHEN_DISABLED, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE, cls_iefb_handle_incoming_when_disabled},
   {CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE, CLS_SLAVE_EVENT_DISABLE_SLAVE_WAIT_ENDED,      CLS_SLAVE_STATE_SLAVE_DISABLED,       NULL},
   {CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE, CLS_SLAVE_EVENT_IP_UPDATED,                    CLS_SLAVE_STATE_SLAVE_DISABLED,       cls_iefb_log_ip_addr_updated},

   /* State SLAVE_DISABLED */
    {CLS_SLAVE_STATE_SLAVE_DISABLED, CLS_SLAVE_EVENT_REENABLE_SLAVE, CLS_SLAVE_STATE_MASTER_NONE, cls_iefb_log_slave_reenabled}};
// clang-format on

// clang-format off
const cls_slave_fsm_on_entry_exit_table_t state_actions[] = {
   {CLS_SLAVE_STATE_MASTER_NONE,          cls_iefb_on_entry_master_none,          NULL},
   {CLS_SLAVE_STATE_MASTER_CONTROL,       NULL,                                   cls_iefb_on_exit_master_control},
   {CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE, cls_iefb_on_entry_wait_disabling_slave, NULL},
   {CLS_SLAVE_STATE_SLAVE_DISABLED,       cls_iefb_on_entry_slave_disabled,       NULL}};
// clang-format on

/**
 * Send an event to the state machine
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param request                Incoming CCIEFB request, or NULL
 * @param event                  Triggering event
 */
void cls_iefb_fsm_event (
   cls_t * cls,
   uint32_t now,
   const cls_cciefb_cyclic_request_info_t * request,
   cls_slave_event_t event)
{
   if (event == CLS_SLAVE_EVENT_LAST)
   {
      return;
   }

   do
   {
      cls_slave_state_t previous = cls->state;
      cls_slave_fsm_t * element  = &(cls->fsm_matrix[previous][event]);
      cls_slave_fsm_entry_exit_t * previous_state_actions =
         &(cls->fsm_on_entry_exit[previous]);
      cls_slave_fsm_entry_exit_t * next_state_actions =
         &(cls->fsm_on_entry_exit[element->next]);

      /* Perform state on_exit action */
      if ((element->next != previous) && (previous_state_actions->on_exit != NULL))
      {
         (void)previous_state_actions->on_exit (cls, now, request, event);
      }

      /* Transition to next state */
      cls->state = element->next;

      if (cls->state != previous)
      {
         /* Show actions when we change state. Note that actions
            might occur also when we stay in the same state. */
#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
         LOG_DEBUG (
            CL_CCIEFB_LOG,
            "CCIEFB(%d): Event %s. New state %s (was %s). Trigger on-exit "
            "action: %s  Transition action: %s  On-entry action: %s\n",
            __LINE__,
            cl_literals_get_slave_event (event),
            cl_literals_get_slave_state (cls->state),
            cl_literals_get_slave_state (previous),
            previous_state_actions->on_exit != NULL ? "Yes" : "No",
            element->action != NULL ? "Yes" : "No",
            next_state_actions->on_entry != NULL ? "Yes" : "No");
#endif
      }

      /* Perform action */
      event = (element->action) ? element->action (cls, now, request, event)
                                : CLS_SLAVE_EVENT_NONE;

      /* Perform state on_entry action */
      if ((cls->state != previous) && (next_state_actions->on_entry != NULL))
      {
         (void)next_state_actions->on_entry (cls, now, request, event);
      }

      /* Call user callback if state has changed */
      if (cls->state != previous)
      {
#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
         LOG_DEBUG (
            CL_CCIEFB_LOG,
            "CCIEFB(%d): %s user state-change callback, new state %s\n",
            __LINE__,
            (cls->config.state_cb != NULL) ? "Calling" : "No",
            cl_literals_get_slave_state (cls->state));
#endif
         if (cls->config.state_cb != NULL)
         {
            cls->config.state_cb (cls, cls->config.cb_arg, cls->state);
         }
      }
   } while (event != CLS_SLAVE_EVENT_NONE);
}

/**
 * Initialise the state machine.
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 */
void cls_fsm_init (cls_t * cls, uint32_t now)
{
   // TODO (rtljobe): Possibly use int i instead and (int)NELEMENTS
   unsigned int i;
   unsigned int j;

   /* Set FSM defaults */
   for (i = 0; i < CLS_SLAVE_STATE_LAST; i++)
   {
      for (j = 0; j < CLS_SLAVE_EVENT_LAST; j++)
      {
         /* Stay in state, no action */
         cls->fsm_matrix[i][j].next   = i;
         cls->fsm_matrix[i][j].action = NULL;
      }

      cls->fsm_on_entry_exit[i].on_entry = NULL;
      cls->fsm_on_entry_exit[i].on_exit  = NULL;
   }

   /* Set FSM transitions from table */
   for (i = 0; i < NELEMENTS (transitions); i++)
   {
      const cls_slave_fsm_transition_t * t       = &transitions[i];
      cls->fsm_matrix[t->state][t->event].next   = t->next;
      cls->fsm_matrix[t->state][t->event].action = t->action;
   }

   /* Set FSM on_entry and on_exit from table */
   for (i = 0; i < NELEMENTS (state_actions); i++)
   {
      const cls_slave_fsm_on_entry_exit_table_t * a = &state_actions[i];
      cls->fsm_on_entry_exit[a->state].on_entry     = a->on_entry;
      cls->fsm_on_entry_exit[a->state].on_exit      = a->on_exit;
   }

   cls->state = CLS_SLAVE_STATE_SLAVE_DOWN;
   cls_iefb_fsm_event (cls, now, NULL, CLS_SLAVE_EVENT_STARTUP);
}

/******************* Handle incoming frames *********************************/

/**
 * Handle incoming CCIEFB cyclic request frame
 *
 * It is assumed that the header starts at buffer[0].
 *
 * Triggers different state machine events depending on current state.
 *
 * Returns -1 immediately if our current slave_id is invalid (0.0.0.0)
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param buffer                 Buffer to be parsed
 * @param recv_len               UDP payload length
 * @param remote_ip              Remote source IP address
 * @param remote_port            Remote source UDP port
 * @param slave_ip_addr          Slave (own) IP address
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CLS_CAPACITY_06
 * @req REQ_CLS_COMMUNIC_01
 * @req REQ_CLS_GROUPS_02
 * @req REQ_CLS_PARAMETERID_01
 * @req REQ_CLS_PARAMETERID_02
 * @req REQ_CLS_PARAMETERID_03
 * @req REQ_CLS_PARAMETERID_04
 * @req REQ_CLS_PARAMETERID_05
 *
 */
static int cls_iefb_handle_cyclic_input_frame (
   cls_t * cls,
   uint32_t now,
   uint8_t * buffer,
   size_t recv_len,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port,
   cl_ipaddr_t slave_ip_addr)
{
   /* Algorithm according to CCIEFB overview section 5.3.1 a3 */
   cls_cciefb_cyclic_request_info_t cyclic_request = {0};
   uint16_t group_no                               = 0;
   cl_ipaddr_t master_id                           = 0;
   uint16_t parameter_no                           = 0;
   uint16_t frame_sequence_no                      = 0;
   uint16_t total_occupied                         = 0;
   cl_ipaddr_t extracted_slave_id                  = CL_IPADDR_INVALID;
   cl_cciefb_cyclic_req_data_header_t * cyclic_data_header;

   if (slave_ip_addr == CL_IPADDR_INVALID)
   {
      /* We have no IP address yet. Drop frame. */
      return -1;
   }

   if (
      cl_iefb_parse_cyclic_request (
         buffer,
         recv_len,
         remote_ip,
         remote_port,
         slave_ip_addr,
         &cyclic_request) != 0)
   {
      return -1;
   }

   if (
      cls->state == CLS_SLAVE_STATE_SLAVE_DOWN ||
      cls->state == CLS_SLAVE_STATE_SLAVE_DISABLED ||
      cls->state == CLS_SLAVE_STATE_LAST)
   {
      /* Discard all incoming frames */
      return 0;
   }

   /* TODO Check that masterID is valid:
    * Not 0.0.0.0, not broadcast, not our IP
    * REQ_CLS_UDP_01 */

   if (cls->state != CLS_SLAVE_STATE_MASTER_CONTROL)
   {
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): Not yet connected to master, search parameters\n",
         __LINE__);
      return cls_iefb_search_slave_parameters (cls, now, &cyclic_request);
   }

   /* Now in CLS_SLAVE_STATE_MASTER_CONTROL */

   CC_ASSERT (cyclic_request.full_headers != NULL);
   cyclic_data_header = &cyclic_request.full_headers->cyclic_data_header;
   master_id          = CC_FROM_LE32 (cyclic_data_header->master_id);
   parameter_no       = CC_FROM_LE16 (cyclic_data_header->parameter_no);
   group_no           = cyclic_data_header->group_no; /* uint8_t in frame */
   frame_sequence_no  = CC_FROM_LE16 (cyclic_data_header->frame_sequence_no);
   total_occupied = CC_FROM_LE16 (cyclic_request.full_headers->cyclic_data_header
                                     .slave_total_occupied_station_count);

   if (cls_iefb_is_master_id_correct (cls, master_id) == false)
   {
      cls_iefb_fsm_event (
         cls,
         now,
         &cyclic_request,
         CLS_SLAVE_EVENT_CYCLIC_WRONG_MASTER);
      return 0;
   }

   if (parameter_no != cls->master.parameter_no)
   {
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): Updated parameter number, search parameters\n",
         __LINE__);
      return cls_iefb_search_slave_parameters (cls, now, &cyclic_request);
   }

   if (frame_sequence_no == 0)
   {
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): Frame sequence is zero, search parameters\n",
         __LINE__);
      return cls_iefb_search_slave_parameters (cls, now, &cyclic_request);
   }

   if (group_no != cls->master.group_no)
   {
      /* Frame for other group, discard. */
      return 0;
   }

   /* Verify that our SlaveId in the frame still is valid */
   if (
      cl_iefb_request_get_slave_id (
         cyclic_request.first_slave_id,
         cls->master.slave_station_no,
         total_occupied,
         &extracted_slave_id) != 0)
   {
      return -1;
   }

   if (extracted_slave_id == CL_IPADDR_INVALID)
   {
      /* PLC is disabling our transmission, by setting slave ID = 0.0.0.0
         Drop frame. */
      return 0;
   }

   if (extracted_slave_id != slave_ip_addr)
   {
      /* We have changed our IP address while running.
         Drop frame, and let connection time out. */
      return -1;
   }

   cls_iefb_fsm_event (
      cls,
      now,
      &cyclic_request,
      CLS_SLAVE_EVENT_CYCLIC_CORRECT_MASTER);

   return 0;
}

/**
 * Handle incoming CCIEFB frame
 *
 * @param cls              c-link slave stack instance handle
 * @param now              Timestamp in microseconds
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param remote_ip        Remote IP address
 * @param remote_port      Remote UDP port number
 * @param slave_ip_addr    Slave (own) IP address
 * @return 0 on success, -1 on failure
 */
#ifndef FUZZ_TEST
static
#endif
   int
   cls_iefb_handle_input_frame (
      cls_t * cls,
      uint32_t now,
      uint8_t * buffer,
      size_t recv_len,
      cl_ipaddr_t remote_ip,
      uint16_t remote_port,
      cl_ipaddr_t slave_ip_addr)
{
   cl_cciefb_req_header_t * header = NULL;
   uint16_t command                = 0;
   uint16_t sub_command            = 0;

   if (cl_iefb_parse_request_header (buffer, recv_len, &header) != 0)
   {
      return -1;
   }

   if (cl_iefb_validate_request_header (header, recv_len) != 0)
   {
      return -1;
   }

   command     = CC_FROM_LE16 (header->command);
   sub_command = CC_FROM_LE16 (header->sub_command);
   if (command == CL_SLMP_COMMAND_CCIEFB_CYCLIC && sub_command == CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC)
   {
      return cls_iefb_handle_cyclic_input_frame (
         cls,
         now,
         buffer,
         recv_len,
         remote_ip,
         remote_port,
         slave_ip_addr);
   }

   return -1;
}

/******************** Public functions ***********************************/

void cls_iefb_disable_slave (cls_t * cls, uint32_t now, bool is_error)
{
   if (is_error)
   {
      LOG_INFO (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): Application stops the cyclic communication. It says an "
         "error has occurred.\n",
         __LINE__);
      cls->endcode_slave_disabled = CL_SLMP_ENDCODE_CCIEFB_SLAVE_ERROR;
   }
   else
   {
      LOG_INFO (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): Application stops the cyclic communication. No error.\n",
         __LINE__);
      cls->endcode_slave_disabled =
         CL_SLMP_ENDCODE_CCIEFB_SLAVE_REQUESTS_DISCONNECT;
   }

   cls_iefb_fsm_event (cls, now, NULL, CLS_SLAVE_EVENT_DISABLE_SLAVE);
}

void cls_iefb_reenable_slave (cls_t * cls, uint32_t now)
{
   LOG_INFO (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): Application re-enables the cyclic communication.\n",
      __LINE__);

   cls_iefb_fsm_event (cls, now, NULL, CLS_SLAVE_EVENT_REENABLE_SLAVE);
}

int cls_iefb_get_master_timestamp (cls_t * cls, uint64_t * master_timestamp)
{
   if (cls->master.clock_info_valid)
   {
      *master_timestamp = cls->master.clock_info;
      return 0;
   }
   return -1;
}

void cls_iefb_periodic (cls_t * cls, uint32_t now)
{
   cl_ipaddr_t remote_ip;
   uint16_t remote_port;
   cl_ipaddr_t slave_ip_addr;
   ssize_t recv_len;
   int ifindex;

   /* We need both the remote and local IP addresses, but not the ifindex */
   recv_len = clal_udp_recvfrom_with_ifindex (
      cls->cciefb_socket,
      &remote_ip,
      &remote_port,
      &slave_ip_addr,
      &ifindex,
      cls->cciefb_receivebuf,
      sizeof (cls->cciefb_receivebuf));

   if (recv_len > 0)
   {
      (void)cls_iefb_handle_input_frame (
         cls,
         now,
         cls->cciefb_receivebuf,
         (size_t)recv_len,
         remote_ip,
         remote_port,
         slave_ip_addr);
   }

   /* Timer for monitoring incoming cyclic data */
   if (cl_timer_is_expired (&cls->receive_timer, now))
   {
      cl_timer_stop (&cls->receive_timer);
      cls_iefb_fsm_event (cls, now, NULL, CLS_SLAVE_EVENT_TIMEOUT_MASTER);
   }

   /* Timer for disabling slave */
   if (cl_timer_is_expired (&cls->timer_for_disabling_slave, now))
   {
      cl_timer_stop (&cls->timer_for_disabling_slave);
      cls_iefb_fsm_event (cls, now, NULL, CLS_SLAVE_EVENT_DISABLE_SLAVE_WAIT_ENDED);
   }

   cl_limiter_periodic (&cls->errorlimiter, now);
   cl_limiter_periodic (&cls->loglimiter, now);
}

void cls_iefb_set_local_management_info (cls_t * cls, uint32_t local_management_info)
{
   cls->local_management_info = local_management_info;
}

uint32_t cls_iefb_get_local_management_info (cls_t * cls)
{
   return cls->local_management_info;
}

void cls_iefb_set_slave_application_status (
   cls_t * cls,
   cl_slave_appl_operation_status_t slave_application_status)
{
   cls->slave_application_status = slave_application_status;
}

cl_slave_appl_operation_status_t cls_iefb_get_slave_application_status (cls_t * cls)
{
   return cls->slave_application_status;
}

void cls_iefb_set_slave_error_code (cls_t * cls, uint16_t slave_err_code)
{
   cls->slave_err_code = slave_err_code;
}

uint16_t cls_iefb_get_slave_error_code (cls_t * cls)
{
   return cls->slave_err_code;
}

const cls_master_connection_t * cls_iefb_get_master_connection_details (cls_t * cls)
{
   return &cls->master;
}

int cls_iefb_init (cls_t * cls, uint32_t now)
{
   cl_limiter_init (&cls->loglimiter, CLS_CCIEFB_LOGWARNING_RETRIGGER_PERIOD);
   cl_limiter_init (&cls->errorlimiter, CLS_CCIEFB_ERRORCALLBACK_RETRIGGER_PERIOD);

#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */

   cl_util_ip_to_string (cls->config.iefb_ip_addr, ip_string);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "CCIEFB(%d): Initialising slave CCIEFB. Listening on IP address %s port "
      "%u\n",
      __LINE__,
      ip_string,
      CL_CCIEFB_PORT);
#endif

#ifndef FUZZ_TEST
   cls->cciefb_socket = clal_udp_open (cls->config.iefb_ip_addr, CL_CCIEFB_PORT);
   if (cls->cciefb_socket == -1)
   {
      LOG_ERROR (
         CL_CCIEFB_LOG,
         "CCIEFB(%d): Failed to open slave CCIEFB socket.\n",
         __LINE__);
      return -1;
   }
#else
   cls->cciefb_socket = -1;
#endif

   cls_fsm_init (cls, now);

   return 0;
}

void cls_iefb_exit (cls_t * cls)
{
   LOG_DEBUG (CL_CCIEFB_LOG, "CCIEFB(%d): Exiting slave CCIEFB\n", __LINE__);

   clal_udp_close (cls->cciefb_socket);
}
