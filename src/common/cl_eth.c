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
 * @brief Reads out our own MAC address and IP address etc
 *
 * Intended to be useful for both master and slave.
 * The functions in this file should not have knowledge on master or slave
 * states.
 *
 */

#ifdef UNIT_TEST
#define clal_get_mac_address        mock_clal_get_mac_address
#define clal_get_ip_address         mock_clal_get_ip_address
#define clal_get_netmask            mock_clal_get_netmask
#define clal_set_ip_address_netmask mock_clal_set_ip_address_netmask
#define clal_get_ifindex            mock_clal_get_ifindex
#define clal_get_ifname             mock_clal_get_ifname
#endif

#include "common/cl_eth.h"

#include "common/cl_types.h"
#include "common/cl_util.h"
#include "common/clal.h"

#include "osal_log.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define CL_IP_NETMASK_HOST_ONLY 0xFFFFFFFF

int cl_eth_get_network_settings (
   cl_ipaddr_t ip_addr,
   int * ifindex,
   cl_ipaddr_t * netmask,
   cl_macaddr_t * mac_address,
   char * ifname)
{
#if LOG_DEBUG_ENABLED(CL_ETH_LOG)
   char netmask_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
#endif
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
   cl_util_ip_to_string (ip_addr, ip_string);

   /* Find interface index */
   if (clal_get_ifindex (ip_addr, ifindex) != 0)
   {
      LOG_ERROR (
         CL_ETH_LOG,
         "ETH(%d): Could not find any Ethernet interface with IP address %s\n",
         __LINE__,
         ip_string);

      return -1;
   }

   LOG_DEBUG (
      CL_ETH_LOG,
      "ETH(%d): IP address %s is found on Ethernet interface index %d\n",
      __LINE__,
      ip_string,
      *ifindex);

   /* Find interface name */
   if (clal_get_ifname (*ifindex, ifname) == 0)
   {
      LOG_DEBUG (
         CL_ETH_LOG,
         "ETH(%d): The Ethernet interface name is \"%s\"\n",
         __LINE__,
         ifname);
   }
   else
   {
      LOG_DEBUG (
         CL_ETH_LOG,
         "ETH(%d): Reading the Ethernet interface name is not implemented\n",
         __LINE__);
      clal_copy_string (ifname, "UNKNOWN", CLAL_IFNAME_SIZE);
   }

   /* Read MAC address */
   if (clal_get_mac_address (*ifindex, mac_address) != 0)
   {
      LOG_ERROR (
         CL_ETH_LOG,
         "ETH(%d): Failed to read MAC address for Ethernet interface index "
         "%d.\n",
         __LINE__,
         *ifindex);
      return -1;
   }

#if LOG_INFO_ENABLED(CL_ETH_LOG)
   LOG_DEBUG (
      CL_ETH_LOG,
      "ETH(%d): My MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
      __LINE__,
      (*mac_address)[0],
      (*mac_address)[1],
      (*mac_address)[2],
      (*mac_address)[3],
      (*mac_address)[4],
      (*mac_address)[5]);
#endif

   /* Read netmask (Only available if link is up) */
   if (clal_get_netmask (*ifindex, netmask) != 0)
   {
      LOG_DEBUG (
         CL_ETH_LOG,
         "ETH(%d): Failed to read netmask for Ethernet interface index %d. Is "
         "the network cable connected?\n",
         __LINE__,
         *ifindex);
      return -1;
   }

#if LOG_DEBUG_ENABLED(CL_ETH_LOG)
   cl_util_ip_to_string (*netmask, netmask_string);
   LOG_DEBUG (
      CL_ETH_LOG,
      "ETH(%d): My IP address: %s Netmask: %s\n",
      __LINE__,
      ip_string,
      netmask_string);
#endif

   return 0;
}

int cl_eth_set_network_settings (int ifindex, cl_ipaddr_t ip, cl_ipaddr_t netmask)
{
#if LOG_DEBUG_ENABLED(CL_ETH_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE]      = {0}; /** Terminated string */
   char netmask_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
#endif

#if LOG_DEBUG_ENABLED(CL_ETH_LOG)
   cl_util_ip_to_string (ip, ip_string);
   cl_util_ip_to_string (netmask, netmask_string);
   LOG_DEBUG (
      CL_ETH_LOG,
      "ETH(%d): Setting IP address to %s and netmask to %s for Ethernet "
      "interface index %d\n",
      __LINE__,
      ip_string,
      netmask_string,
      ifindex);
#endif

   return clal_set_ip_address_netmask (ifindex, ip, netmask);
}
