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

#ifndef CL_SAMPLE_MASTER_COMMON_H
#define CL_SAMPLE_MASTER_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cl_sample_common.h"
#include "clm_api.h"

#include "osal.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define APP_MASTER_BUTTON_ID 2

typedef enum app_master_demostate
{
   APP_MASTER_DEMOSTATE_NORMAL = 0,
   APP_MASTER_DEMOSTATE_NODE_SEARCH,
   APP_MASTER_DEMOSTATE_SET_IP,
   APP_MASTER_DEMOSTATE_MASTER_APPL_STOPPED,
   APP_MASTER_DEMOSTATE_MASTER_APPL_STOPPED_BY_USER,
   APP_MASTER_DEMOSTATE_MASTER_APPL_RUNNING,
   APP_MASTER_DEMOSTATE_DISABLE_SLAVE,
   APP_MASTER_DEMOSTATE_ENABLE_SLAVE,
   APP_MASTER_DEMOSTATE_SHOW_STATISTICS,
} app_master_demostate_t;

typedef struct app_master_data
{
   void * user_arg;
   clm_t * clm;
   os_timer_t * main_timer;
   os_event_t * main_events;
   cl_ipaddr_t new_slave_ip_addr;
   app_master_demostate_t demo_state;

   /* Cyclic data for slave 1 */

   bool button_state_from_slave1;
   bool led_state_to_slave1;
   uint16_t button_counter_from_slave1;
   uint16_t integer_to_slave1;

   /* Cyclic data for slave 2 */

   bool has_second_device;
   bool bit_from_slave2;
   bool bit_to_slave2;
   uint16_t integer_from_slave2;
   uint16_t integer_to_slave2;

   /* Cyclic data for slave 3, which can belong to
      group 1 or group 2 depending on config. */

   bool has_third_device;
   bool has_second_group;
   uint16_t group_index_slave3;
   uint16_t device_index_slave3;
   bool bit_from_slave3;
   bool bit_to_slave3;
   uint16_t integer_from_slave3;
   uint16_t integer_to_slave3;

} app_master_data_t;

/**
 * Sample master init function
 *
 * Initialise and start c-link master sample
 *
 * @param user_arg         User argument, passed on to app_get_button() etc
 *                         for use in different operating system ports.
 * @param cfg              c-link master configuration, partly populated
 * @param appdata_master   Datastructure for master app, will be cleared.
 * @param priority         Master thread priority
 * @param stacksize        Master thread stack size
 * @return 0 on success, -1 on failure
 */
int app_master_main (
   void * user_arg,
   clm_cfg_t * cfg,
   app_master_data_t * appdata_master,
   uint32_t priority,
   size_t stacksize);

/**************** Callbacks  See include/clm_api.h *********/

void masterapp_state_ind (clm_t * clm, void * arg, clm_master_state_t state);

void masterapp_connect_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   cl_ipaddr_t slave_id);

void masterapp_disconnect_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   cl_ipaddr_t slave_id);

void masterapp_alarm_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t end_code,
   uint16_t slave_err_code,
   uint32_t local_management_info);

void masterapp_slave_info_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t end_code,
   uint16_t slave_err_code,
   uint32_t local_management_info);

void masterapp_node_search_result_cfm (
   clm_t * clm,
   void * arg,
   clm_node_search_db_t * db);

void masterapp_set_ip_cfm (clm_t * clm, void * arg, clm_master_setip_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* CL_SAMPLE_MASTER_COMMON_H */
