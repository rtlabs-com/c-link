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
 * @brief Implements master mechanisms to send SLMP commands
 *
 *        See common/cl_slmp.h for SLMP protocol implementation.
 */

#ifdef UNIT_TEST
#define clal_udp_open                  mock_clal_udp_open
#define clal_udp_sendto                mock_clal_udp_sendto
#define clal_udp_close                 mock_clal_udp_close
#define clal_udp_recvfrom              mock_clal_udp_recvfrom
#define clal_udp_recvfrom_with_ifindex mock_clal_udp_recvfrom_with_ifindex
#endif

#include "master/clm_slmp.h"

#include "common/cl_literals.h"
#include "common/cl_slmp.h"
#include "common/cl_slmp_udp.h"
#include "common/cl_types.h"
#include "common/cl_util.h"
#include "common/clal.h"

#include "osal_log.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static void clm_slmp_set_ip_notify_cfm (
   clm_t * clm,
   clm_master_setip_status_t status);

/********************** Database of node search responses *******************/

/**
 * Add node search response to local database
 *
 * @param node_search_db                  Database to be updated
 * @param resp                            Incoming search response
 * @return 0 on success, -1 on failure
 */
int clm_slmp_node_search_add_db (
   clm_node_search_db_t * node_search_db,
   const cl_slmp_node_search_resp_t * resp)
{
   clm_node_search_response_entry_t * node_search_result;

   node_search_db->count++;

   if (node_search_db->stored == CLM_MAX_NODE_SEARCH_DEVICES)
   {
      return -1;
   }

   node_search_result = &node_search_db->entries[node_search_db->stored];

   cl_util_copy_mac_reverse (
      &node_search_result->slave_mac_addr,
      &resp->slave_mac_addr);

   node_search_result->slave_id      = CC_FROM_LE32 (resp->slave_ip_addr);
   node_search_result->slave_netmask = CC_FROM_LE32 (resp->slave_netmask);
   node_search_result->vendor_code   = CC_FROM_LE16 (resp->vendor_code);
   node_search_result->model_code    = CC_FROM_LE32 (resp->model_code);
   node_search_result->equipment_ver = CC_FROM_LE16 (resp->equipment_ver);

   node_search_db->stored++;

   return 0;
}

/**
 * Clear local database of node search responses
 *
 * @param node_search_db                  Database to be cleared
 */
void clm_slmp_node_search_clear_db (clm_node_search_db_t * node_search_db)
{
   node_search_db->count  = 0;
   node_search_db->stored = 0;
}

const clm_node_search_db_t * clm_slmp_get_node_search_result (clm_t * clm)
{
   return (const clm_node_search_db_t *)&clm->node_search_db;
}

/********************** Parse and write SLMP frames ************************/

/**
 * Send a SLMP request
 *
 * Opens a socket, sends the request and closes the socket.
 *
 * The request message should be available in clm->slmp_sendbuf
 *
 * @param clm                 c-link master stack instance handle
 * @param request_len         Number of bytes to send
 * @return 0 on success, or -1 on error
 */
static int clm_slmp_send_request (clm_t * clm, size_t request_len)
{
   return cl_slmp_udp_send (
      clm->config.use_single_slmp_socket ? &clm->slmp_receive_socket
                                         : &clm->slmp_send_socket,
      !clm->config.use_single_slmp_socket,
      clm->slmp_sendbuf,
      sizeof (clm->slmp_sendbuf),
      clm->config.master_id,
      clm->slmp_broadcast_ip,
      CL_SLMP_PORT,
      request_len);
}

/**
 * Handle error response to the set IP command
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param clm              c-link stack instance handle
 * @param now              Current timestamp, in microseconds
 * @param inputbuffer      Buffer to be parsed
 * @param recv_len         UDP payload length
 * @param remote_ip        Remote source IP address
 * @param remote_port      Remote source UDP port
 * @return 0 on success, -1 on failure
 */
static int clm_slmp_handle_set_ip_error (
   clm_t * clm,
   uint32_t now,
   const uint8_t * inputbuffer,
   size_t recv_len,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port)
{
   cl_slmp_error_resp_t * error_resp;

   if (cl_slmp_parse_error_response (inputbuffer, recv_len, &error_resp) != 0)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming error response has wrong size.\n",
         __LINE__);
      return -1;
   }

   if (cl_slmp_validate_error_response (error_resp) != 0)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming error response is invalid.\n",
         __LINE__);
      return -1;
   }

   if (
      CC_FROM_LE16 (error_resp->command) != CL_SLMP_COMMAND_NODE_IPADDRESS_SET ||
      CC_FROM_LE16 (error_resp->sub_command) !=
         CL_SLMP_SUBCOMMAND_NODE_IPADDRESS_SET)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming error response has wrong command or subcommand\n",
         __LINE__);
      return -1;
   }

   LOG_INFO (
      CL_SLMP_LOG,
      "SLMP(%d): Incoming error response for set IP command.\n",
      __LINE__);

   clm_slmp_set_ip_notify_cfm (clm, CLM_MASTER_SET_IP_STATUS_ERROR);

   return 0;
}

/**
 * Handle node search response
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param clm              c-link stack instance handle
 * @param now              Current timestamp, in microseconds
 * @param inputbuffer      Buffer to be parsed
 * @param recv_len         UDP payload length
 * @param remote_ip        Remote source IP address
 * @param remote_port      Remote source UDP port
 * @return 0 on success, -1 on failure
 */
static int clm_slmp_handle_node_search_response (
   clm_t * clm,
   uint32_t now,
   const uint8_t * inputbuffer,
   size_t recv_len,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port)
{
   int db_result = 0;
   cl_slmp_node_search_resp_t * node_search_resp;
   cl_macaddr_t slave_mac_addr = {0};

#if LOG_INFO_ENABLED(CL_SLMP_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
#endif

   if (cl_slmp_parse_node_search_response (inputbuffer, recv_len, &node_search_resp) != 0)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming node search response has wrong size.\n",
         __LINE__);
      return -1;
   }

   if (cl_slmp_validate_node_search_response (node_search_resp) != 0)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming node search response is invalid.\n",
         __LINE__);
      return -1;
   }

   /* MAC address in the payload part of the request has reversed
      byte order */
   cl_util_copy_mac_reverse (&slave_mac_addr, &node_search_resp->slave_mac_addr);

   db_result =
      clm_slmp_node_search_add_db (&clm->node_search_db, node_search_resp);

#if LOG_INFO_ENABLED(CL_SLMP_LOG)
   cl_util_ip_to_string (remote_ip, ip_string);

   LOG_INFO (
      CL_SLMP_LOG,
      "SLMP(%d): Incoming node search response from slave with IP address %s"
      " and MAC address %02X:%02X:%02X:%02X:%02X:%02X. %s.\n",
      __LINE__,
      ip_string,
      slave_mac_addr[0],
      slave_mac_addr[1],
      slave_mac_addr[2],
      slave_mac_addr[3],
      slave_mac_addr[4],
      slave_mac_addr[5],
      (db_result == 0) ? "Was added to local database"
                       : "Local database is full");
#else
   (void)db_result;
#endif

   return 0;
}

/**
 * Handle set IP address response
 *
 * It is assumed that the header starts at buffer[0].
 *
 * The endcode should be validated to CL_SLMP_ENDCODE_SUCCESS before.
 *
 * @param clm              c-link stack instance handle
 * @param now              Current timestamp, in microseconds
 * @param inputbuffer      Buffer to be parsed
 * @param recv_len         UDP payload length
 * @param remote_ip        Remote source IP address
 * @param remote_port      Remote source UDP port
 * @return 0 on success, -1 on failure
 */
static int clm_slmp_handle_set_ipaddr_response (
   clm_t * clm,
   uint32_t now,
   const uint8_t * inputbuffer,
   size_t recv_len,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port)
{
   cl_slmp_set_ipaddr_resp_t * set_ipaddr_resp;

#if LOG_INFO_ENABLED(CL_SLMP_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
#endif

   if (cl_slmp_parse_set_ip_response (inputbuffer, recv_len, &set_ipaddr_resp) != 0)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming set IP address response has wrong size.\n",
         __LINE__);
      return -1;
   }

   if (cl_slmp_validate_set_ip_response (set_ipaddr_resp, &clm->mac_address) != 0)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming set IP address response is invalid (reports wrong "
         "master MAC address).\n",
         __LINE__);
      return -1;
   }

#if LOG_INFO_ENABLED(CL_SLMP_LOG)
   cl_util_ip_to_string (remote_ip, ip_string);

   LOG_INFO (
      CL_SLMP_LOG,
      "SLMP(%d): Incoming set IP address response from slave with IP address %s"
      " Result OK.\n",
      __LINE__,
      ip_string);
#endif

   clm_slmp_set_ip_notify_cfm (clm, CLM_MASTER_SET_IP_STATUS_SUCCESS);

   return 0;
}

/**
 * Handle incoming SLMP response frame
 *
 * @param clm              c-link stack instance handle
 * @param now              Timestamp in microseconds
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param remote_ip        Remote IP address
 * @param remote_port      Remote UDP port number
 * @param ifindex          Interface index, for filtering frames
 * @return 0 on success, -1 on failure
 */
#ifndef FUZZ_TEST
static
#endif
   int
   clm_slmp_handle_input_frame (
      clm_t * clm,
      uint32_t now,
      const uint8_t * buffer,
      size_t recv_len,
      cl_ipaddr_t remote_ip,
      uint16_t remote_port,
      int ifindex)
{
   cl_slmp_resp_header_t * header = NULL;
   uint16_t serial                = 0;
   uint16_t endcode               = 0;

   if (cl_slmp_parse_response_header (buffer, recv_len, &header) != 0)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming SLMP frame has too short header.\n",
         __LINE__);
      return -1;
   }

   if (cl_slmp_is_header_response (header) == false)
   {
      /* Probably request broadcast echo from ourselves. Silently drop frame */
      return -1;
   }

   if (cl_slmp_validate_response_header (header, recv_len) != 0)
   {
      LOG_DEBUG (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming SLMP response header is not valid.\n",
         __LINE__);
      return -1;
   }

   /* Response to some other stack instance */
   if (ifindex != clm->ifindex)
   {
      return -1;
   }

   serial  = CC_FROM_LE16 (header->serial);
   endcode = CC_FROM_LE16 (header->endcode);

   /* Handle response if serial number matches current pending request */
   if (serial == clm->node_search_serial)
   {
      return clm_slmp_handle_node_search_response (
         clm,
         now,
         buffer,
         recv_len,
         remote_ip,
         remote_port);
   }
   if (serial == clm->set_ip_request_serial)
   {
      if (endcode == CL_SLMP_ENDCODE_SUCCESS)
      {
         return clm_slmp_handle_set_ipaddr_response (
            clm,
            now,
            buffer,
            recv_len,
            remote_ip,
            remote_port);
      }

      return clm_slmp_handle_set_ip_error (
         clm,
         now,
         buffer,
         recv_len,
         remote_ip,
         remote_port);
   }

   return -1;
}

int clm_slmp_perform_node_search (clm_t * clm, uint32_t now)
{
   /* Block multiple simultanenous search requests from user app */
   if (clm->node_search_serial != CLM_SLMP_SERIAL_NONE)
   {
      LOG_INFO (
         CL_SLMP_LOG,
         "SLMP(%d): Node search already in progress, with serial number: %d\n",
         __LINE__,
         clm->node_search_serial);
      return -1;
   }
   clm->node_search_serial = clm->slmp_request_serial;
   clm_slmp_node_search_clear_db (&clm->node_search_db);

   LOG_INFO (
      CL_SLMP_LOG,
      "SLMP(%d): Sending node search request broadcast, serial number: %d\n",
      __LINE__,
      clm->node_search_serial);

   cl_slmp_prepare_node_search_request_frame (
      clm->slmp_sendbuf,
      sizeof (clm->slmp_sendbuf),
      &clm->mac_address,
      clm->config.master_id,
      (uint16_t)clm->node_search_serial);

   clm->slmp_request_serial++;

   if (clm_slmp_send_request (clm, sizeof (cl_slmp_node_search_request_t)) != 0)
   {
      LOG_ERROR (
         CL_SLMP_LOG,
         "SLMP(%d): Failed to send node search request.\n",
         __LINE__);

      clm->node_search_serial = CLM_SLMP_SERIAL_NONE;
      return -1;
   }

   /* Start node search callback timer */
   cl_timer_start (
      &clm->node_search_timer,
      clm->config.callback_time_node_search *
         CL_TIMER_MICROSECONDS_PER_MILLISECOND,
      now);

   return 0;
}

/**
 * Notify node search results to user app
 *
 * Set the node_search_serial value to NONE.
 *
 * @param clm               c-link stack instance handle
 */
static void clm_slmp_node_search_notify_results (clm_t * clm)
{
   /* Trigger informational callback */
   LOG_DEBUG (
      CL_SLMP_LOG,
      "SLMP(%d): %s user callback for node search complete.\n",
      __LINE__,
      (clm->config.node_search_cfm_cb != NULL) ? "Calling" : "No");
   if (clm->config.node_search_cfm_cb != NULL)
   {
      clm->config.node_search_cfm_cb (clm, clm->config.cb_arg, &clm->node_search_db);
   }

   clm->node_search_serial = CLM_SLMP_SERIAL_NONE;
}

/**
 * Notify set IP result to user app.
 *
 * Set the set_ip_request_serial value to NONE.
 *
 * Will stop the corresponding timer (if running).
 *
 * @param clm              c-link stack instance handle
 * @param status           Result of set IP operation
 */
static void clm_slmp_set_ip_notify_cfm (clm_t * clm, clm_master_setip_status_t status)
{
   cl_timer_stop (&clm->set_ip_request_timer);

   /* Trigger informational callback */
   LOG_DEBUG (
      CL_SLMP_LOG,
      "SLMP(%d): %s user callback for set IP address (%s = %d).\n",
      __LINE__,
      (clm->config.set_ip_cfm_cb != NULL) ? "Calling" : "No",
      cl_literals_get_master_set_ip_result (status),
      status);
   if (clm->config.set_ip_cfm_cb != NULL)
   {
      clm->config.set_ip_cfm_cb (clm, clm->config.cb_arg, status);
   }

   clm->set_ip_request_serial = CLM_SLMP_SERIAL_NONE;
}

int clm_slmp_perform_set_ipaddr_request (
   clm_t * clm,
   uint32_t now,
   const cl_macaddr_t * slave_mac_addr,
   cl_ipaddr_t slave_new_ip_addr,
   cl_ipaddr_t slave_new_netmask)
{
#if LOG_INFO_ENABLED(CL_SLMP_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE]      = {0}; /** Terminated string */
   char netmask_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
   cl_util_ip_to_string (slave_new_ip_addr, ip_string);
   cl_util_ip_to_string (slave_new_netmask, netmask_string);
#endif

   /** Block multiple simultaneous set IP requests from user app */
   if (clm->set_ip_request_serial != CLM_SLMP_SERIAL_NONE)
   {
      LOG_INFO (
         CL_SLMP_LOG,
         "SLMP(%d): Set IP request already in progress, with serial number: "
         "%d\n",
         __LINE__,
         clm->set_ip_request_serial);
      return -1;
   }

   if (slave_new_ip_addr == CL_IPADDR_INVALID)
   {
      LOG_INFO (CL_SLMP_LOG, "SLMP(%d): The new IP address is invalid\n", __LINE__);
      return -1;
   }

   if (cl_utils_is_netmask_valid (slave_new_netmask) == false)
   {
#if LOG_INFO_ENABLED(CL_SLMP_LOG)
      LOG_INFO (
         CL_SLMP_LOG,
         "SLMP(%d): The new netmask is invalid: %s\n",
         __LINE__,
         netmask_string);
#endif
      return -1;
   }

   clm->set_ip_request_serial = clm->slmp_request_serial;

#if LOG_INFO_ENABLED(CL_SLMP_LOG)
   LOG_INFO (
      CL_SLMP_LOG,
      "SLMP(%d): Sending set IP request to MAC address "
      "%02X:%02X:%02X:%02X:%02X:%02X. New IP %s netmask %s Request serial "
      "number: %d\n",
      __LINE__,
      (*slave_mac_addr)[0],
      (*slave_mac_addr)[1],
      (*slave_mac_addr)[2],
      (*slave_mac_addr)[3],
      (*slave_mac_addr)[4],
      (*slave_mac_addr)[5],
      ip_string,
      netmask_string,
      clm->set_ip_request_serial);
#endif

   cl_slmp_prepare_set_ip_request_frame (
      clm->slmp_sendbuf,
      sizeof (clm->slmp_sendbuf),
      &clm->mac_address,
      clm->config.master_id,
      (uint16_t)clm->set_ip_request_serial,
      slave_mac_addr,
      slave_new_ip_addr,
      slave_new_netmask);

   clm->slmp_request_serial++;

   if (clm_slmp_send_request (clm, sizeof (cl_slmp_set_ipaddr_request_t)) != 0)
   {
      LOG_ERROR (
         CL_SLMP_LOG,
         "SLMP(%d): Failed to send set IP address request.\n",
         __LINE__);

      clm->set_ip_request_serial = CLM_SLMP_SERIAL_NONE;
      return -1;
   }

   /* Start set IP callback timer in case remote slave does not reply due
       to network problems or other */
   cl_timer_start (
      &clm->set_ip_request_timer,
      clm->config.callback_time_set_ip * CL_TIMER_MICROSECONDS_PER_MILLISECOND,
      now);

   return 0;
}

/********************* Initialize and periodic update ***********************/

/**
 * Handle timers for the SLMP communication.
 *
 * @param clm              c-link stack instance handle
 * @param now              Current timestamp, in microseconds
 */
void clm_slmp_check_timeouts (clm_t * clm, uint32_t now)
{
   if (cl_timer_is_expired (&clm->node_search_timer, now))
   {
      cl_timer_stop (&clm->node_search_timer);
      clm_slmp_node_search_notify_results (clm);

      /* Node search db intentionally not reset as it may be requested
       * through clm_slmp_get_node_search_result(). Reset upon new node
       * search request. */
   }

   if (cl_timer_is_expired (&clm->set_ip_request_timer, now))
   {
      cl_timer_stop (&clm->set_ip_request_timer);
      clm_slmp_set_ip_notify_cfm (clm, CLM_MASTER_SET_IP_STATUS_TIMEOUT);
   }
}

void clm_slmp_periodic (clm_t * clm, uint32_t now)
{
   cl_ipaddr_t remote_ip = 0;
   uint16_t remote_port  = 0;
   ssize_t recv_len      = 0;
   cl_ipaddr_t local_ip;
   int ifindex;

   clm_slmp_check_timeouts (clm, now);

   do
   {
      recv_len = clal_udp_recvfrom_with_ifindex (
         clm->slmp_receive_socket,
         &remote_ip,
         &remote_port,
         &local_ip,
         &ifindex,
         clm->slmp_receivebuf,
         sizeof (clm->slmp_receivebuf));

      if (recv_len > 0)
      {
         (void)clm_slmp_handle_input_frame (
            clm,
            now,
            clm->slmp_receivebuf,
            (size_t)recv_len,
            remote_ip,
            remote_port,
            ifindex);
      }
   } while (recv_len > 0);
}

int clm_slmp_init (clm_t * clm)
{
   LOG_DEBUG (
      CL_SLMP_LOG,
      "SLMP(%d): Initialising master SLMP. Listening on IPADDR_ANY (0.0.0.0) "
      "port %u\n",
      __LINE__,
      CL_SLMP_PORT);

   clm->slmp_request_serial   = 0;
   clm->node_search_serial    = CLM_SLMP_SERIAL_NONE;
   clm->set_ip_request_serial = CLM_SLMP_SERIAL_NONE;
   cl_timer_stop (&clm->node_search_timer);    /* Initialise timer */
   cl_timer_stop (&clm->set_ip_request_timer); /* Initialise timer */
   clm_slmp_node_search_clear_db (&clm->node_search_db);

#ifndef FUZZ_TEST
   clm->slmp_receive_socket = clal_udp_open (CL_IPADDR_ANY, CL_SLMP_PORT);
   if (clm->slmp_receive_socket == -1)
   {
      return -1;
   }
#else
   clm->slmp_receive_socket = -1;
#endif

   /* The clm->slmp_send_socket (if used) is opened just before use */
   clm->slmp_send_socket = -1;

   LOG_DEBUG (
      CL_SLMP_LOG,
      "SLMP(%d): Socket for listening to %sSLMP messages: %d\n",
      __LINE__,
      clm->config.use_single_slmp_socket ? "and sending " : "",
      clm->slmp_receive_socket);

   return 0;
}

void clm_slmp_exit (clm_t * clm)
{
   LOG_DEBUG (CL_SLMP_LOG, "SLMP(%d): Exiting master SLMP\n", __LINE__);

   clal_udp_close (clm->slmp_receive_socket);
   clm->slmp_receive_socket = -1;
}
