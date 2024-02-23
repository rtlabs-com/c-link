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
 * @brief Implement the functions defined in the public slave API.
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
#include "slave/cls_iefb.h"
#include "slave/cls_slave.h"
#include "slave/cls_slmp.h"

#include "osal_log.h"

#include <stdio.h>
#include <stdlib.h>

CC_STATIC_ASSERT (sizeof (cl_rx_t) == 8);
CC_STATIC_ASSERT (sizeof (cl_ry_t) == 8);
CC_STATIC_ASSERT (sizeof (cl_rww_t) == 64);
CC_STATIC_ASSERT (sizeof (cl_rwr_t) == 64);

int cls_init_only (cls_t * cls, const cls_cfg_t * cfg)
{
   if (cls == NULL)
   {
      return -1;
   }

   return cls_slave_init (cls, cfg, os_get_current_time_us());
}

cls_t * cls_init (const cls_cfg_t * cfg)
{
   cls_t * cls;

   cls = calloc (1, sizeof (*cls));
   if (cls == NULL)
   {
      LOG_ERROR (CL_CCIEFB_LOG, "CLS_API(%d): Failed to allocate.\n", __LINE__);
      return NULL;
   }

   if (cls_init_only (cls, cfg) == 0)
   {
      return cls;
   }

   free (cls);
   return NULL;
}

int cls_exit (cls_t * cls)
{
   if (cls == NULL)
   {
      return -1;
   }

   return cls_slave_exit (cls);
}

void cls_handle_periodic (cls_t * cls)
{
   uint32_t now = os_get_current_time_us();

   CC_ASSERT (cls != NULL);

   /* We might have received SLMP command to change IP, so run SLMP first */
   cls_slmp_periodic (cls, now);
   cls_iefb_periodic (cls, now);
}

void cls_stop_cyclic_data (cls_t * cls, bool is_error)
{
   uint32_t now = os_get_current_time_us();

   CC_ASSERT (cls != NULL);

   cls_iefb_disable_slave (cls, now, is_error);
}

void cls_restart_cyclic_data (cls_t * cls)
{
   uint32_t now = os_get_current_time_us();

   CC_ASSERT (cls != NULL);

   cls_iefb_reenable_slave (cls, now);
}

int cls_get_master_timestamp (cls_t * cls, uint64_t * master_timestamp)
{
   if (cls == NULL)
   {
      return -1;
   }

   return cls_iefb_get_master_timestamp (cls, master_timestamp);
}

void cls_set_slave_application_status (
   cls_t * cls,
   cl_slave_appl_operation_status_t slave_application_status)
{
   CC_ASSERT (cls != NULL);

   cls_iefb_set_slave_application_status (cls, slave_application_status);
}

cl_slave_appl_operation_status_t cls_get_slave_application_status (cls_t * cls)
{
   CC_ASSERT (cls != NULL);

   return cls_iefb_get_slave_application_status (cls);
}

void cls_set_local_management_info (cls_t * cls, uint32_t local_management_info)
{
   CC_ASSERT (cls != NULL);

   cls_iefb_set_local_management_info (cls, local_management_info);
}

uint32_t cls_get_local_management_info (cls_t * cls)
{
   CC_ASSERT (cls != NULL);

   return cls_iefb_get_local_management_info (cls);
}

void cls_set_slave_error_code (cls_t * cls, uint16_t slave_err_code)
{
   CC_ASSERT (cls != NULL);

   cls_iefb_set_slave_error_code (cls, slave_err_code);
}

uint16_t cls_get_slave_error_code (cls_t * cls)
{
   CC_ASSERT (cls != NULL);

   return cls_iefb_get_slave_error_code (cls);
}

const cls_master_connection_t * cls_get_master_connection_details (cls_t * cls)
{
   if (cls == NULL)
   {
      return NULL;
   }

   return cls_iefb_get_master_connection_details (cls);
}

cl_rx_t * cls_get_first_rx_area (cls_t * cls)
{
   if (cls == NULL)
   {
      return NULL;
   }

   return cls_iefb_get_first_rx_area (cls);
}

const cl_ry_t * cls_get_first_ry_area (cls_t * cls)
{
   if (cls == NULL)
   {
      return NULL;
   }

   return cls_iefb_get_first_ry_area (cls);
}

cl_rwr_t * cls_get_first_rwr_area (cls_t * cls)
{
   if (cls == NULL)
   {
      return NULL;
   }

   return cls_iefb_get_first_rwr_area (cls);
}

const cl_rww_t * cls_get_first_rww_area (cls_t * cls)
{
   if (cls == NULL)
   {
      return NULL;
   }

   return cls_iefb_get_first_rww_area (cls);
}

void cls_set_rx_bit (cls_t * cls, uint16_t number, bool value)
{
   CC_ASSERT (cls != NULL);

   cls_iefb_set_rx_bit (cls, number, value);
}

bool cls_get_rx_bit (cls_t * cls, uint16_t number)
{
   CC_ASSERT (cls != NULL);

   return cls_iefb_get_rx_bit (cls, number);
}

bool cls_get_ry_bit (cls_t * cls, uint16_t number)
{
   CC_ASSERT (cls != NULL);

   return cls_iefb_get_ry_bit (cls, number);
}

void cls_set_rwr_value (cls_t * cls, uint16_t number, uint16_t value)
{
   CC_ASSERT (cls != NULL);

   cls_iefb_set_rwr_value (cls, number, value);
}

uint16_t cls_get_rwr_value (cls_t * cls, uint16_t number)
{
   CC_ASSERT (cls != NULL);

   return cls_iefb_get_rwr_value (cls, number);
}

uint16_t cls_get_rww_value (cls_t * cls, uint16_t number)
{
   CC_ASSERT (cls != NULL);

   return cls_iefb_get_rww_value (cls, number);
}
