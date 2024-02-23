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

#ifndef CL_ETH_H
#define CL_ETH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/cl_types.h"

#include <stdint.h>

/**
 * Read out current Ethernet interface settings for a given IP address
 *
 * Some of the resulting values might be updated even if the function fails.
 *
 * @param ip_addr          IP address
 * @param ifindex          Resulting interface index
 * @param netmask          Resulting netmask
 * @param mac_address      Resulting MAC address
 * @param ifname           Resulting network interface name. Buffer
 *                         size should be CLAL_IFNAME_SIZE.
 * @return 0 on success, -1 if no corresponding interface is found
 */
int cl_eth_get_network_settings (
   cl_ipaddr_t ip_addr,
   int * ifindex,
   cl_ipaddr_t * netmask,
   cl_macaddr_t * mac_address,
   char * ifname);

/**
 * Adjust Ethernet interface settings
 *
 * No validation of the IP address or the netmask is done.
 *
 * @param ifindex          Network interface number
 * @param ip               New IP address
 * @param netmask          New netmask
 * @return 0 on success, -1 on failure
 */
int cl_eth_set_network_settings (int ifindex, cl_ipaddr_t ip, cl_ipaddr_t netmask);

#ifdef __cplusplus
}
#endif

#endif /* CL_ETH_H */
