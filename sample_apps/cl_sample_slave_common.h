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

#ifndef CL_SAMPLE_SLAVE_COMMON_H
#define CL_SAMPLE_SLAVE_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cl_sample_common.h"
#include "cls_api.h"

#include "osal.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define APP_DATA_LED_ID     1
#define APP_DATA_BUTTON_ID  1
#define APP_ALARM_BUTTON_ID 2

typedef enum app_slave_demostate
{
   APP_SLAVE_DEMOSTATE_NORMAL = 0,
   APP_SLAVE_DEMOSTATE_ERRORCODE_A,
   APP_SLAVE_DEMOSTATE_ERRORCODE_B,
   APP_SLAVE_DEMOSTATE_NO_ERRORCODE,
   APP_SLAVE_DEMOSTATE_DISABLE_SLAVE,
   APP_SLAVE_DEMOSTATE_ENABLE_SLAVE,
} app_slave_demostate_t;

typedef struct app_slave_data
{
   void * user_arg;
   cls_t * cls;
   os_timer_t * main_timer;
   os_event_t * main_events;
   bool master_running;
   app_slave_demostate_t demo_state;
   bool previous_led_state;
} app_slave_data_t;

/**
 * Sample slave init function
 *
 * Initialise and start c-link slave sample
 *
 * @param user_arg      User argument, passed on to app_get_button() etc
 *                      for use in different operating system ports.
 * @param cfg           c-link configuration, partly populated
 * @param appdata_slave Datastructure for slave app, will be cleared.
 * @param priority      Slave thread priority
 * @param stacksize     Slave thread stack size
 * @return 0 on success, -1 on failure
 */
int app_slave_main (
   void * user_arg,
   cls_cfg_t * cfg,
   app_slave_data_t * appdata_slave,
   uint32_t priority,
   size_t stacksize);

/**************** Callbacks  See include/cls_api.h *********/

void slaveapp_state_ind (cls_t * cls, void * arg, cls_slave_state_t state);

void slaveapp_error_ind (
   cls_t * cls,
   void * arg,
   cls_error_message_t error_message,
   cl_ipaddr_t ip_addr,
   uint16_t argument_2);

void slaveapp_connect_ind (
   cls_t * cls,
   void * arg,
   cl_ipaddr_t master_ip_addr,
   uint16_t group_no,
   uint16_t slave_station_no);

void slaveapp_disconnect_ind (cls_t * cls, void * arg);

void slaveapp_node_search_ind (
   cls_t * cls,
   void * arg,
   cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr);

void slaveapp_set_ip_ind (
   cls_t * cls,
   void * arg,
   cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr,
   cl_ipaddr_t new_ip_addr,
   cl_ipaddr_t new_netmask,
   bool ip_setting_allowed,
   bool did_set_ip);

#ifdef __cplusplus
}
#endif

#endif /* CL_SAMPLE_SLAVE_COMMON_H */
