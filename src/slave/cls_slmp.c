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
 * @brief Implements slave mechanisms to handle incoming SLMP commands
 *
 *        See common/cl_slmp.h for SLMP protocol implementation.
 */

#ifdef UNIT_TEST
#define clal_udp_open                  mock_clal_udp_open
#define clal_udp_recvfrom_with_ifindex mock_clal_udp_recvfrom_with_ifindex
#define clal_udp_sendto                mock_clal_udp_sendto
#define clal_udp_close                 mock_clal_udp_close
#define clal_get_mac_address           mock_clal_get_mac_address
#define clal_get_netmask               mock_clal_get_netmask
#endif

#include "slave/cls_slmp.h"

#include "common/cl_eth.h"
#include "common/cl_slmp.h"
#include "common/cl_slmp_udp.h"
#include "common/cl_types.h"
#include "common/cl_util.h"
#include "common/clal.h"
#include "slave/cls_iefb.h"

#include "osal_log.h"

#include <stdio.h>
#include <string.h>

/**
 * Schedule a node search response to be transmitted
 *
 * @param cls                 c-link slave stack instance handle
 * @param now                 Current timestamp, in microseconds
 * @param master_mac_address  MAC address for the master that sent the request
 * @param addr_info           Address info about incoming request
 * @param request_serial      Serial number of the request
 * @return 0 on success, -1 on failure
 */
static int cls_slmp_schedule_node_search_response (
   cls_t * cls,
   uint32_t now,
   cl_macaddr_t * master_mac_address,
   const cls_addr_info_t * addr_info,
   uint16_t request_serial)
{
   uint32_t delay =
      cl_slmp_calculate_node_search_delay (&addr_info->local_mac_address);

   cls->node_search.master_ip_addr = addr_info->remote_ip;
   cls->node_search.master_port    = addr_info->remote_port;
   cls->node_search.request_serial = request_serial;
   cl_util_copy_mac (&cls->node_search.master_mac_address, master_mac_address);
   cls->node_search.slave_ip_address = addr_info->local_ip;
   cls->node_search.slave_netmask    = addr_info->local_netmask;
   cl_util_copy_mac (
      &cls->node_search.slave_mac_address,
      &addr_info->local_mac_address);

   cl_timer_start (&cls->node_search.response_timer, delay, now);

   return 0;
}

/**
 * Send a SLMP response
 *
 * Opens a socket, sends the response and closes the socket.
 *
 * The response message should be available in cls->slmp_sendbuf
 *
 * @param cls                 c-link slave stack instance handle
 * @param local_ip            Local IP address for opening the socket
 * @param remote_ip           Remote (destination) IP address
 * @param remote_port         Remote UDP port
 * @param response_len        Number of bytes to send
 * @return 0 on success, or -1 on error
 */
static int cls_slmp_send_response (
   cls_t * cls,
   cl_ipaddr_t local_ip,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port,
   size_t response_len)
{
   return cl_slmp_udp_send (
      &cls->slmp_send_socket,
      true,
      cls->slmp_sendbuf,
      sizeof (cls->slmp_sendbuf),
      local_ip,
      remote_ip,
      remote_port,
      response_len);
}

/**
 * Handle incoming Node Search frame
 *
 * The incoming frame is an UDP broadcast
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param cls              c-link slave stack instance handle
 * @param now              Current timestamp, in microseconds
 * @param inputbuffer      Buffer to be parsed
 * @param recv_len         UDP payload length
 * @param addr_info        Address info about incoming request
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CLS_CONFORMANCE_12
 */
static int cls_slmp_handle_node_search_request (
   cls_t * cls,
   uint32_t now,
   const uint8_t * inputbuffer,
   size_t recv_len,
   const cls_addr_info_t * addr_info)
{
   cl_slmp_node_search_request_t * node_search_request;
   cl_macaddr_t master_mac_addr = {0};
   cl_ipaddr_t master_ip_addr   = 0;
   uint16_t request_serial      = 0;

#if LOG_INFO_ENABLED(CL_SLMP_LOG)
   char master_ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
   char slave_ip_string[CL_INET_ADDRSTR_SIZE]  = {0}; /** Terminated string */
   char slave_netmask_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated str */
#endif

   if (cl_slmp_parse_node_search_request (inputbuffer, recv_len, &node_search_request) != 0)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming node search request has wrong size.\n",
         __LINE__);
      return -1;
   }

   if (cl_slmp_validate_node_search_request (node_search_request) != 0)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming node search request is invalid.\n",
         __LINE__);
      return -1;
   }

   master_ip_addr = CC_FROM_LE32 (node_search_request->master_ip_addr);
   request_serial = CC_FROM_LE16 (node_search_request->header.serial);

   /* MAC address in the payload part of the request has reversed
      byte order */
   cl_util_copy_mac_reverse (
      &master_mac_addr,
      &node_search_request->master_mac_addr);

#if LOG_INFO_ENABLED(CL_SLMP_LOG)
   cl_util_ip_to_string (addr_info->local_ip, slave_ip_string);
   cl_util_ip_to_string (addr_info->local_netmask, slave_netmask_string);
   cl_util_ip_to_string (master_ip_addr, master_ip_string);

   LOG_INFO (
      CL_SLMP_LOG,
      "SLMP(%d): Incoming node search from master with IP address %s"
      " and MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
      __LINE__,
      master_ip_string,
      master_mac_addr[0],
      master_mac_addr[1],
      master_mac_addr[2],
      master_mac_addr[3],
      master_mac_addr[4],
      master_mac_addr[5]);
   LOG_INFO (
      CL_SLMP_LOG,
      "SLMP(%d):    Interface index: %d  My current IP address: %s  netmask "
      "%s\n",
      __LINE__,
      addr_info->ifindex,
      slave_ip_string,
      slave_netmask_string);
   LOG_INFO (
      CL_SLMP_LOG,
      "SLMP(%d):    My MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
      __LINE__,
      addr_info->local_mac_address[0],
      addr_info->local_mac_address[1],
      addr_info->local_mac_address[2],
      addr_info->local_mac_address[3],
      addr_info->local_mac_address[4],
      addr_info->local_mac_address[5]);
   LOG_INFO (
      CL_SLMP_LOG,
      "SLMP(%d):    Request serial number: %u  Scheduling response...\n",
      __LINE__,
      request_serial);
#endif

   if (master_ip_addr != addr_info->remote_ip)
   {
#if LOG_INFO_ENABLED(CL_SLMP_LOG)
      cl_util_ip_to_string (addr_info->remote_ip, master_ip_string);
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): The master IP address in the node search request payload"
         " does not match the IP address in the IP header (%s).\n ",
         __LINE__,
         master_ip_string);
#endif
      return -1;
   }

   /* Trigger informational callback */
   LOG_DEBUG (
      CL_SLMP_LOG,
      "SLMP(%d): %s user callback for node search.\n",
      __LINE__,
      (cls->config.node_search_cb != NULL) ? "Calling" : "No");
   if (cls->config.node_search_cb != NULL)
   {
      cls->config.node_search_cb (
         cls,
         cls->config.cb_arg,
         &master_mac_addr,
         master_ip_addr);
   }

   return cls_slmp_schedule_node_search_response (
      cls,
      now,
      &master_mac_addr,
      addr_info,
      request_serial);
}

/**
 * Send a response to a node search request
 *
 * @param cls             c-link slave stack instance handle
 * @return 0 on success, -1 on failure
 */
int cls_slmp_send_node_search_response (cls_t * cls)
{
   cl_ipaddr_t broadcast_address = 0;
#if LOG_INFO_ENABLED(CL_SLMP_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
#endif

   if (cls->node_search.master_ip_addr == CL_IPADDR_INVALID)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Trying to send node search response, but the stored remote "
         " IP address is invalid.\n ",
         __LINE__);
      return -1;
   }

#if LOG_INFO_ENABLED(CL_SLMP_LOG)
   cl_util_ip_to_string (cls->node_search.master_ip_addr, ip_string);
   LOG_INFO (
      CL_SLMP_LOG,
      "SLMP(%d): Sending node search response to master with IP %s"
      " port %u and MAC %02X:%02X:%02X:%02X:%02X:%02X"
      "  Request serial number: %u\n",
      __LINE__,
      ip_string,
      cls->node_search.master_port,
      cls->node_search.master_mac_address[0],
      cls->node_search.master_mac_address[1],
      cls->node_search.master_mac_address[2],
      cls->node_search.master_mac_address[3],
      cls->node_search.master_mac_address[4],
      cls->node_search.master_mac_address[5],
      cls->node_search.request_serial);
#endif

   cl_slmp_prepare_node_search_response_frame (
      cls->slmp_sendbuf,
      sizeof (cls->slmp_sendbuf),
      &cls->node_search.master_mac_address,
      cls->node_search.master_ip_addr,
      &cls->node_search.slave_mac_address,
      cls->node_search.slave_ip_address,
      cls->node_search.slave_netmask,
      CL_SLMP_NODE_SEARCH_RESP_SERVER_STATUS_NORMAL,
      cls->config.vendor_code,
      cls->config.model_code,
      cls->config.equipment_ver,
      cls->node_search.request_serial);

   broadcast_address = cls->config.use_slmp_directed_broadcast
                          ? cl_util_calc_broadcast_address (
                               cls->node_search.slave_netmask,
                               cls->node_search.slave_ip_address)
                          : CL_IPADDR_LOCAL_BROADCAST;

   if (
      cls_slmp_send_response (
         cls,
         cls->node_search.slave_ip_address,
         broadcast_address,
         cls->node_search.master_port,
         sizeof (cl_slmp_node_search_resp_t)) != 0)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Failed to send node search response.\n ",
         __LINE__);
      return -1;
   }

   return 0;
}

/**
 * Handle incoming Set IP Address frame
 *
 * The incoming frame is an UDP broadcast
 *
 * It is assumed that the header starts at buffer[0].
 *
 * @param cls                    c-link slave stack instance handle
 * @param now                    Current timestamp, in microseconds
 * @param inputbuffer            Buffer to be parsed
 * @param recv_len               UDP payload length
 * @param addr_info              Address info about incoming request
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CLS_CONFORMANCE_13
 * @req REQ_CLS_SLMP_IP_01
 *
 */
static int cls_slmp_handle_ipaddress_set_request (
   cls_t * cls,
   uint32_t now,
   const uint8_t * inputbuffer,
   size_t recv_len,
   const cls_addr_info_t * addr_info)
{
   cl_slmp_set_ipaddr_request_t * set_ip_request;
   cl_macaddr_t master_mac_addr      = {0};
   cl_macaddr_t destination_mac_addr = {0};
   cl_ipaddr_t master_ip_addr        = 0;
   cl_ipaddr_t new_ip_address        = 0;
   cl_ipaddr_t resulting_ip_address  = 0;
   cl_ipaddr_t new_netmask           = 0;
   cl_ipaddr_t broadcast_address     = 0;
   uint16_t request_serial           = 0;
   size_t response_len               = 0;
   int send_result                   = 0;
   bool successful                   = true;

#if LOG_INFO_ENABLED(CL_SLMP_LOG)
   char master_ip_string[CL_INET_ADDRSTR_SIZE]   = {0}; /** Terminated */
   char slave_ip_string[CL_INET_ADDRSTR_SIZE]    = {0}; /** Terminated */
   char new_ip_string[CL_INET_ADDRSTR_SIZE]      = {0}; /** Terminated */
   char new_netmask_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated */
#endif

   if (cl_slmp_parse_set_ip_request (inputbuffer, recv_len, &set_ip_request) != 0)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming request to change IP address has wrong size.\n",
         __LINE__);
      return -1;
   }

   if (cl_slmp_validate_set_ip_request (set_ip_request) != 0)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Invalid incoming request to change IP address.\n",
         __LINE__);
      return -1;
   }

   /* MAC address in the payload part of the request has reversed
      byte order */
   cl_util_copy_mac_reverse (&master_mac_addr, &set_ip_request->master_mac_addr);
   cl_util_copy_mac_reverse (
      &destination_mac_addr,
      &set_ip_request->slave_mac_addr);

   master_ip_addr = CC_FROM_LE32 (set_ip_request->master_ip_addr);
   new_ip_address = CC_FROM_LE32 (set_ip_request->slave_new_ip_addr);
   new_netmask    = CC_FROM_LE32 (set_ip_request->slave_new_netmask);
   request_serial = CC_FROM_LE16 (set_ip_request->header.serial);

   if (
      memcmp (
         &destination_mac_addr,
         &addr_info->local_mac_address,
         sizeof (destination_mac_addr)) != 0)
   {
      LOG_DEBUG (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming set IP address request for other slave.\n",
         __LINE__);
      return 0;
   }

#if LOG_INFO_ENABLED(CL_SLMP_LOG)
   cl_util_ip_to_string (master_ip_addr, master_ip_string);
   cl_util_ip_to_string (addr_info->local_ip, slave_ip_string);
   cl_util_ip_to_string (new_ip_address, new_ip_string);
   cl_util_ip_to_string (new_netmask, new_netmask_string);

   LOG_INFO (
      CL_SLMP_LOG,
      "SLMP(%d): Incoming command to set IP from master with IP "
      "%s and MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
      __LINE__,
      master_ip_string,
      master_mac_addr[0],
      master_mac_addr[1],
      master_mac_addr[2],
      master_mac_addr[3],
      master_mac_addr[4],
      master_mac_addr[5]);
   LOG_INFO (
      CL_SLMP_LOG,
      "SLMP(%d):    Interface index: %d  My current IP address: %s\n",
      __LINE__,
      addr_info->ifindex,
      slave_ip_string);
   LOG_INFO (
      CL_SLMP_LOG,
      "SLMP(%d):    My new IP address: %s  Netmask: %s  Will send response.\n",
      __LINE__,
      new_ip_string,
      new_netmask_string);
#endif

   if (master_ip_addr != addr_info->remote_ip)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): The master IP address in the set IP request payload "
         "does not match the IP address in the IP header.\n",
         __LINE__);
      return -1;
   }

   if (cls->config.ip_setting_allowed)
   {
      /* Change network settings.
         Note that a reboot (if any) must be done after the response is sent */
      if (cl_eth_set_network_settings (addr_info->ifindex, new_ip_address, new_netmask) != 0)
      {
         LOG_WARNING (
            CL_SLMP_LOG,
            "SLMP(%d): Failed to set IP address. Check your permissions. "
            "Will send error frame.\n",
            __LINE__);
         successful = false;
      }
   }
   else
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): We got a command to change IP, but that is not "
         "allowed according to configuration. Will send error frame.\n",
         __LINE__);
      successful = false;
   }

   resulting_ip_address = successful ? new_ip_address : addr_info->local_ip;

   /* Build response frame */
   if (successful)
   {
      response_len = sizeof (cl_slmp_set_ipaddr_resp_t);
      cl_slmp_prepare_set_ip_response_frame (
         cls->slmp_sendbuf,
         sizeof (cls->slmp_sendbuf),
         &master_mac_addr,
         request_serial);
   }
   else
   {
      response_len = sizeof (cl_slmp_error_resp_t);
      cl_slmp_prepare_error_response_frame (
         cls->slmp_sendbuf,
         sizeof (cls->slmp_sendbuf),
         CL_SLMP_COMMAND_NODE_IPADDRESS_SET,
         CL_SLMP_SUBCOMMAND_NODE_IPADDRESS_SET,
         CL_SLMP_ENDCODE_COMMAND_REQUEST_MSG,
         request_serial);
   }

   LOG_DEBUG (
      CL_SLMP_LOG,
      "SLMP(%d): Sending set-IP response to master. It is %s frame.\n",
      __LINE__,
      (successful) ? "a normal" : "an error");

   broadcast_address =
      cls->config.use_slmp_directed_broadcast
         ? cl_util_calc_broadcast_address (new_netmask, new_ip_address)
         : CL_IPADDR_LOCAL_BROADCAST;

   /* Send response */
   send_result = cls_slmp_send_response (
      cls,
      resulting_ip_address,
      broadcast_address,
      addr_info->remote_port,
      response_len);

   /* Trigger informational callback to application */
   LOG_DEBUG (
      CL_SLMP_LOG,
      "SLMP(%d): %s user callback for set-IP command from master.\n",
      __LINE__,
      (cls->config.set_ip_cb != NULL) ? "Calling" : "No");
   if (cls->config.set_ip_cb != NULL)
   {
      cls->config.set_ip_cb (
         cls,
         cls->config.cb_arg,
         &master_mac_addr,
         master_ip_addr,
         new_ip_address,
         new_netmask,
         cls->config.ip_setting_allowed,
         cls->config.ip_setting_allowed && successful);
   }

   if (send_result != 0)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Failed to send set IP response.\n ",
         __LINE__);
      return -1;
   }

   return 0;
}

/**
 * Handle incoming SLMP request frame
 *
 * @param cls              c-link stack instance handle
 * @param now              Timestamp in microseconds
 * @param buffer           Input buffer
 * @param recv_len         UDP payload length
 * @param addr_info        Address info about incoming request
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CL_SLMP_02
 * @req REQ_CL_SLMP_03
 */
#ifndef FUZZ_TEST
static
#endif
   int
   cls_slmp_handle_input_frame (
      cls_t * cls,
      uint32_t now,
      const uint8_t * buffer,
      size_t recv_len,
      const cls_addr_info_t * addr_info)
{
   cl_slmp_req_header_t * header = NULL;
   uint16_t command              = 0;
   uint16_t sub_command          = 0;

   if (cl_slmp_parse_request_header (buffer, recv_len, &header) != 0)
   {
      LOG_WARNING (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming SLMP request has too short header.\n",
         __LINE__);
      return -1;
   }

   if (!cl_slmp_is_header_request (header))
   {
      /* Probably response from other slave. Silently drop frame */
      return -1;
   }

   if (cl_slmp_validate_request_header (header, recv_len) != 0)
   {
      LOG_DEBUG (
         CL_SLMP_LOG,
         "SLMP(%d): Incoming SLMP request has invalid header.\n",
         __LINE__);
      return -1;
   }

   command     = CC_FROM_LE16 (header->command);
   sub_command = CC_FROM_LE16 (header->sub_command);
   switch (command)
   {
   case CL_SLMP_COMMAND_NODE_SEARCH:
      if (sub_command == CL_SLMP_SUBCOMMAND_NODE_SEARCH)
      {
         return cls_slmp_handle_node_search_request (
            cls,
            now,
            buffer,
            recv_len,
            addr_info);
      }
      break;
   case CL_SLMP_COMMAND_NODE_IPADDRESS_SET:
      if (sub_command == CL_SLMP_SUBCOMMAND_NODE_IPADDRESS_SET)
      {
         return cls_slmp_handle_ipaddress_set_request (
            cls,
            now,
            buffer,
            recv_len,
            addr_info);
      }
      break;
   default:
      break;
   }

   LOG_INFO (
      CL_SLMP_LOG,
      "SLMP(%d): Incoming SLMP frame has unknown command.\n",
      __LINE__);
   return -1;
}

void cls_slmp_periodic (cls_t * cls, uint32_t now)
{
   cls_addr_info_t addr_info = {0};
   ssize_t recv_len          = 0;

   if (cl_timer_is_expired (&cls->node_search.response_timer, now))
   {
      cl_timer_stop (&cls->node_search.response_timer);
      (void)cls_slmp_send_node_search_response (cls);
   }

   /* We need ifindex also for setting IP address */
   recv_len = clal_udp_recvfrom_with_ifindex (
      cls->slmp_receive_socket,
      &addr_info.remote_ip,
      &addr_info.remote_port,
      &addr_info.local_ip,
      &addr_info.ifindex,
      cls->slmp_receivebuf,
      sizeof (cls->slmp_receivebuf));

   if (recv_len > 0)
   {
      if (clal_get_mac_address (addr_info.ifindex, &addr_info.local_mac_address) != 0)
      {
         return;
      }

      if (clal_get_netmask (addr_info.ifindex, &addr_info.local_netmask) != 0)
      {
         return;
      }

      (void)cls_slmp_handle_input_frame (
         cls,
         now,
         cls->slmp_receivebuf,
         (size_t)recv_len,
         &addr_info);
   }
}

int cls_slmp_init (cls_t * cls)
{
   LOG_DEBUG (
      CL_SLMP_LOG,
      "SLMP(%d): Initialising slave SLMP. Listening on IPADDR_ANY (0.0.0.0) "
      "port %u\n",
      __LINE__,
      CL_SLMP_PORT);

   clal_clear_memory (&cls->node_search, sizeof (cls->node_search));
   cl_timer_stop (&cls->node_search.response_timer); /* Initialise timer */

#ifndef FUZZ_TEST
   cls->slmp_receive_socket = clal_udp_open (CL_IPADDR_ANY, CL_SLMP_PORT);
   if (cls->slmp_receive_socket == -1)
   {
      LOG_ERROR (
         CL_SLMP_LOG,
         "SLMP(%d): Failed to open slave SLMP socket.\n",
         __LINE__);
      return -1;
   }
#else
   cls->slmp_receive_socket = -1;
#endif

   /* The cls->slmp_send_socket is opened just before use */
   cls->slmp_send_socket = -1;

   return 0;
}

void cls_slmp_exit (cls_t * cls)
{
   LOG_DEBUG (CL_SLMP_LOG, "SLMP(%d): Exiting slave SLMP\n", __LINE__);

   clal_udp_close (cls->slmp_receive_socket);
   cls->slmp_receive_socket = -1;
}
