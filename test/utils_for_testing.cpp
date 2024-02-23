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

#include "utils_for_testing.h"

#include "mocks.h"

#include "common/cl_util.h"

/************************* Callbacks in slave *******************************/

void my_slave_state_ind (cls_t * cls, void * arg, cls_slave_state_t state)
{
   auto * mock = static_cast<cl_mock_data_t *> (arg);

   mock->slave_cb_state.calls++;
   mock->slave_cb_state.state = state;
}

void my_slave_error_ind (
   cls_t * cls,
   void * arg,
   cls_error_message_t error_message,
   cl_ipaddr_t ip_addr,
   uint16_t argument_2)
{
   auto * mock = static_cast<cl_mock_data_t *> (arg);

   mock->slave_cb_error.calls++;
   mock->slave_cb_error.error_message = error_message;
   mock->slave_cb_error.ip_addr       = ip_addr;
   mock->slave_cb_error.argument_2    = argument_2;
}

void my_slave_connect_ind (
   cls_t * cls,
   void * arg,
   cl_ipaddr_t master_ip_addr,
   uint16_t group_no,
   uint16_t slave_station_no)
{
   auto * mock = static_cast<cl_mock_data_t *> (arg);

   mock->slave_cb_connect.calls++;
   mock->slave_cb_connect.master_ip_addr   = master_ip_addr;
   mock->slave_cb_connect.group_no         = group_no;
   mock->slave_cb_connect.slave_station_no = slave_station_no;
}

void my_slave_disconnect_ind (cls_t * cls, void * arg)
{
   auto * mock = static_cast<cl_mock_data_t *> (arg);

   mock->slave_cb_disconnect.calls++;
}

void my_slave_master_running_ind (
   cls_t * cls,
   void * arg,
   bool connected_to_master,
   bool connected_and_running,
   bool stopped_by_user,
   uint16_t protocol_ver,
   uint16_t master_application_status)
{
   auto * mock = static_cast<cl_mock_data_t *> (arg);

   mock->slave_cb_master_running.calls++;
   mock->slave_cb_master_running.connected_to_master   = connected_to_master;
   mock->slave_cb_master_running.connected_and_running = connected_and_running;
   mock->slave_cb_master_running.stopped_by_user       = stopped_by_user;
   mock->slave_cb_master_running.protocol_ver          = protocol_ver;
   mock->slave_cb_master_running.master_application_status =
      master_application_status;
}

void my_slave_node_search_ind (
   cls_t * cls,
   void * arg,
   cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr)
{
   auto * mock = static_cast<cl_mock_data_t *> (arg);

   mock->slave_cb_nodesearch.calls++;
   mock->slave_cb_nodesearch.master_ip_addr = master_ip_addr;
   cl_util_copy_mac (&mock->slave_cb_nodesearch.master_mac_addr, master_mac_addr);
}

void my_slave_set_ip_ind (
   cls_t * cls,
   void * arg,
   cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr,
   cl_ipaddr_t new_ip_addr,
   cl_ipaddr_t new_netmask,
   bool ip_setting_allowed,
   bool did_set_ip)
{
   auto * mock = static_cast<cl_mock_data_t *> (arg);

   mock->slave_cb_set_ip.calls++;
   mock->slave_cb_set_ip.master_ip_addr     = master_ip_addr;
   mock->slave_cb_set_ip.new_ip_addr        = new_ip_addr;
   mock->slave_cb_set_ip.new_netmask        = new_netmask;
   mock->slave_cb_set_ip.ip_setting_allowed = ip_setting_allowed;
   mock->slave_cb_set_ip.did_set_ip         = did_set_ip;
   cl_util_copy_mac (&mock->slave_cb_set_ip.master_mac_addr, master_mac_addr);
}

/************************* Callbacks in master ******************************/

void my_master_state_ind (clm_t * clm, void * arg, clm_master_state_t state)
{
   auto * counters = static_cast<cl_mock_master_callback_counters_t *> (arg);

   counters->master_cb_state.calls++;
   counters->master_cb_state.state = state;
}

void my_master_connect_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   cl_ipaddr_t slave_id)
{
   auto * counters = static_cast<cl_mock_master_callback_counters_t *> (arg);

   counters->master_cb_connect.calls++;
   counters->master_cb_connect.group_index        = group_index;
   counters->master_cb_connect.slave_device_index = slave_device_index;
   counters->master_cb_connect.slave_id           = slave_id;
}

void my_master_disconnect_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   cl_ipaddr_t slave_id)
{
   auto * counters = static_cast<cl_mock_master_callback_counters_t *> (arg);

   counters->master_cb_disconnect.calls++;
   counters->master_cb_disconnect.group_index        = group_index;
   counters->master_cb_disconnect.slave_device_index = slave_device_index;
   counters->master_cb_disconnect.slave_id           = slave_id;
}

void my_master_linkscan_complete_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   bool success)
{
   auto * counters = static_cast<cl_mock_master_callback_counters_t *> (arg);

   counters->master_cb_linkscan.calls++;
   counters->master_cb_linkscan.group_index = group_index;
   counters->master_cb_linkscan.success     = success;
}

void my_master_alarm_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t end_code,
   uint16_t slave_err_code,
   uint32_t local_management_info)
{
   auto * counters = static_cast<cl_mock_master_callback_counters_t *> (arg);

   counters->master_cb_alarm.calls++;
   counters->master_cb_alarm.group_index           = group_index;
   counters->master_cb_alarm.slave_device_index    = slave_device_index;
   counters->master_cb_alarm.end_code              = end_code;
   counters->master_cb_alarm.slave_err_code        = slave_err_code;
   counters->master_cb_alarm.local_management_info = local_management_info;
}

void my_master_error_ind (
   clm_t * clm,
   void * arg,
   clm_error_message_t error_message,
   cl_ipaddr_t ip_addr,
   uint16_t argument_2)
{
   auto * counters = static_cast<cl_mock_master_callback_counters_t *> (arg);

   counters->master_cb_error.calls++;
   counters->master_cb_error.error_message = error_message;
   counters->master_cb_error.ip_addr       = ip_addr;
   counters->master_cb_error.argument_2    = argument_2;
}

void my_master_changed_slave_info_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t end_code,
   uint16_t slave_err_code,
   uint32_t local_management_info)
{
   auto * counters = static_cast<cl_mock_master_callback_counters_t *> (arg);

   counters->master_cb_changed_slave_info.calls++;
   counters->master_cb_changed_slave_info.group_index = group_index;
   counters->master_cb_changed_slave_info.slave_device_index = slave_device_index;
   counters->master_cb_changed_slave_info.end_code       = end_code;
   counters->master_cb_changed_slave_info.slave_err_code = slave_err_code;
   counters->master_cb_changed_slave_info.local_management_info =
      local_management_info;
}

void my_master_node_search_result_cfm (
   clm_t * clm,
   void * arg,
   clm_node_search_db_t * db)
{
   auto * counters = static_cast<cl_mock_master_callback_counters_t *> (arg);

   counters->master_cb_node_search.calls++;
   clal_memcpy (
      &counters->master_cb_node_search.db,
      sizeof (counters->master_cb_node_search.db),
      db,
      sizeof (clm_node_search_db_t));
}

void my_master_set_ip_cfm (clm_t * clm, void * arg, clm_master_setip_status_t status)
{
   auto * counters = static_cast<cl_mock_master_callback_counters_t *> (arg);

   counters->master_cb_set_ip.calls++;
   counters->master_cb_set_ip.status = status;
}
