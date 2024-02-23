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

#ifndef CLM_SLMP_H
#define CLM_SLMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/cl_types.h"

/**
 * Initialise master SLMP
 *
 * @param clm              c-link master stack instance handle
 *
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CL_UDP_02
 *
 */
int clm_slmp_init (clm_t * clm);

/**
 * Exit master SLMP
 *
 * @param clm              c-link master stack instance handle
 *
 */
void clm_slmp_exit (clm_t * clm);

/**
 * Execute master SLMP internals.
 *
 * Use this function every tick.
 *
 * @param clm              c-link master stack instance handle
 * @param now              timestamp in microseconds
 */
void clm_slmp_periodic (clm_t * clm, uint32_t now);

/**
 * Perform a node search
 *
 * @param clm               c-link stack instance handle
 * @param now               Current timestamp, in microseconds
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CL_UDP_02
 */
int clm_slmp_perform_node_search (clm_t * clm, uint32_t now);

/**
 * Get a pointer to the node search result database.
 *
 * @param clm              c-link master stack instance handle
 * @return                 Pointer to node search result database
 */
const clm_node_search_db_t * clm_slmp_get_node_search_result (clm_t * clm);

/**
 * Perform set IP address request
 *
 * @param clm                c-link stack instance handle
 * @param now                Current timestamp, in microseconds
 * @param slave_mac_addr     Slave MAC address
 * @param slave_new_ip_addr  New IP address
 * @param slave_new_netmask  New netmask *
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CL_UDP_02
 */
int clm_slmp_perform_set_ipaddr_request (
   clm_t * clm,
   uint32_t now,
   const cl_macaddr_t * slave_mac_addr,
   cl_ipaddr_t slave_new_ip_addr,
   cl_ipaddr_t slave_new_netmask);

/************ Internal functions made available for tests *******************/

int clm_slmp_node_search_add_db (
   clm_node_search_db_t * node_search_db,
   const cl_slmp_node_search_resp_t * resp);

void clm_slmp_node_search_clear_db (clm_node_search_db_t * node_search_db);

void clm_slmp_check_timeouts (clm_t * clm, uint32_t now);

#ifdef __cplusplus
}
#endif

#endif /* CLM_SLMP_H */
