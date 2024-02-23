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
 * @brief Implement the functions defined in the public master API.
 *
 * Uses functions defined in other files, but does some basic validation
 * before calling them.
 */

#ifdef UNIT_TEST
#define calloc                 mock_calloc
#define os_get_current_time_us mock_os_get_current_time_us
#endif

#include "cl_options.h"
#include "common/cl_types.h"
#include "master/clm_iefb.h"
#include "master/clm_master.h"
#include "master/clm_slmp.h"

#include "osal_log.h"

#include <stdio.h>
#include <stdlib.h>

int clm_init_only (clm_t * clm, const clm_cfg_t * cfg)
{
   if (clm == NULL)
   {
      return -1;
   }

   return clm_master_init (clm, cfg, os_get_current_time_us());
}

clm_t * clm_init (const clm_cfg_t * cfg)
{
   clm_t * clm;

   clm = calloc (1, sizeof (*clm));
   if (clm == NULL)
   {
      LOG_ERROR (CL_CCIEFB_LOG, "CLM_API(%d): Failed to allocate.\n", __LINE__);
      return NULL;
   }

   if (clm_init_only (clm, cfg) == 0)
   {
      return clm;
   }

   free (clm);
   return NULL;
}

int clm_exit (clm_t * clm)
{
   if (clm == NULL)
   {
      return -1;
   }

   return clm_master_exit (clm);
}

void clm_handle_periodic (clm_t * clm)
{
   uint32_t now = os_get_current_time_us();

   CC_ASSERT (clm != NULL);

   clm_slmp_periodic (clm, now);
   clm_iefb_periodic (clm, now);
}

void clm_set_master_application_status (clm_t * clm, bool running, bool stopped_by_user)
{
   CC_ASSERT (clm != NULL);

   clm_iefb_set_master_application_status (clm, running, stopped_by_user);
}

uint16_t clm_get_master_application_status (clm_t * clm)
{
   CC_ASSERT (clm != NULL);

   return clm_iefb_get_master_application_status (clm);
}

int clm_set_slave_communication_status (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   bool enabled)
{
   CC_ASSERT (clm != NULL);

   return clm_iefb_set_slave_communication_status (
      clm,
      group_index,
      slave_device_index,
      enabled);
}

int clm_force_cyclic_transmission_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   bool force)
{
   CC_ASSERT (clm != NULL);

   return clm_iefb_force_cyclic_transmission_bit (
      clm,
      group_index,
      slave_device_index,
      force);
}

void clm_clear_statistics (clm_t * clm)
{
   CC_ASSERT (clm != NULL);

   clm_iefb_statistics_clear_all (clm);
}

int clm_get_master_status (const clm_t * clm, clm_master_status_details_t * details)
{
   if (clm == NULL)
   {
      return -1;
   }

   return clm_iefb_get_master_status (clm, details);
}

int clm_get_group_status (
   const clm_t * clm,
   uint16_t group_index,
   clm_group_status_details_t * details)
{
   if (clm == NULL)
   {
      return -1;
   }

   return clm_iefb_get_group_status (clm, group_index, details);
}

const clm_slave_device_data_t * clm_get_device_connection_details (
   const clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index)
{
   if (clm == NULL)
   {
      return NULL;
   }

   return clm_iefb_get_device_connection_details (
      clm,
      group_index,
      slave_device_index);
}

int clm_perform_node_search (clm_t * clm)
{
   uint32_t now = os_get_current_time_us();

   CC_ASSERT (clm != NULL);

   return clm_slmp_perform_node_search (clm, now);
}

const clm_node_search_db_t * clm_get_node_search_result (clm_t * clm)
{
   CC_ASSERT (clm != NULL);

   return clm_slmp_get_node_search_result (clm);
}

int clm_set_slave_ipaddr (
   clm_t * clm,
   const cl_macaddr_t * slave_mac_addr,
   cl_ipaddr_t slave_new_ip_addr,
   cl_ipaddr_t slave_new_netmask)
{
   uint32_t now = os_get_current_time_us();

   CC_ASSERT (clm != NULL);

   return clm_slmp_perform_set_ipaddr_request (
      clm,
      now,
      slave_mac_addr,
      slave_new_ip_addr,
      slave_new_netmask);
}

const cl_rx_t * clm_get_first_rx_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied)
{
   if (clm == NULL)
   {
      *total_occupied = 0;
      return NULL;
   }

   return clm_iefb_get_first_rx_area (clm, group_index, total_occupied);
}

cl_ry_t * clm_get_first_ry_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied)
{
   if (clm == NULL)
   {
      *total_occupied = 0;
      return NULL;
   }

   return clm_iefb_get_first_ry_area (clm, group_index, total_occupied);
}

const cl_rwr_t * clm_get_first_rwr_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied)
{
   if (clm == NULL)
   {
      *total_occupied = 0;
      return NULL;
   }

   return clm_iefb_get_first_rwr_area (clm, group_index, total_occupied);
}

cl_rww_t * clm_get_first_rww_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied)
{
   if (clm == NULL)
   {
      *total_occupied = 0;
      return NULL;
   }

   return clm_iefb_get_first_rww_area (clm, group_index, total_occupied);
}

const cl_rx_t * clm_get_first_device_rx_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations)
{
   if (clm == NULL)
   {
      *num_occupied_stations = 0;
      return NULL;
   }

   return clm_iefb_get_first_device_rx_area (
      clm,
      group_index,
      slave_device_index,
      num_occupied_stations);
}

cl_ry_t * clm_get_first_device_ry_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations)
{
   if (clm == NULL)
   {
      *num_occupied_stations = 0;
      return NULL;
   }

   return clm_iefb_get_first_device_ry_area (
      clm,
      group_index,
      slave_device_index,
      num_occupied_stations);
}

const cl_rwr_t * clm_get_first_device_rwr_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations)
{
   if (clm == NULL)
   {
      *num_occupied_stations = 0;
      return NULL;
   }

   return clm_iefb_get_first_device_rwr_area (
      clm,
      group_index,
      slave_device_index,
      num_occupied_stations);
}

cl_rww_t * clm_get_first_device_rww_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations)
{
   if (clm == NULL)
   {
      *num_occupied_stations = 0;
      return NULL;
   }

   return clm_iefb_get_first_device_rww_area (
      clm,
      group_index,
      slave_device_index,
      num_occupied_stations);
}

bool clm_get_rx_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number)
{
   CC_ASSERT (clm != NULL);

   return clm_iefb_get_rx_bit (clm, group_index, slave_device_index, number);
}

void clm_set_ry_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number,
   bool value)
{
   CC_ASSERT (clm != NULL);

   clm_iefb_set_ry_bit (clm, group_index, slave_device_index, number, value);
}

bool clm_get_ry_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number)
{
   CC_ASSERT (clm != NULL);

   return clm_iefb_get_ry_bit (clm, group_index, slave_device_index, number);
}

uint16_t clm_get_rwr_value (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number)
{
   CC_ASSERT (clm != NULL);

   return clm_iefb_get_rwr_value (clm, group_index, slave_device_index, number);
}

void clm_set_rww_value (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number,
   uint16_t value)
{
   CC_ASSERT (clm != NULL);

   clm_iefb_set_rww_value (clm, group_index, slave_device_index, number, value);
}

uint16_t clm_get_rww_value (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number)
{
   CC_ASSERT (clm != NULL);

   return clm_iefb_get_rww_value (clm, group_index, slave_device_index, number);
}
