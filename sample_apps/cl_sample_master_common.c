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

#include "cl_sample_master_common.h"

#include "clm_api.h"

#include "osal.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#define APP_TICK_INTERVAL_US          1000
#define APP_EVENT_PERIODIC            BIT (0)
#define APP_DATA_UPDATE_TICKS         10
#define APP_BUTTON_READ_TICKS         100
#define APP_OSCILLATOR_UPDATE_COUNTS  10
#define APP_INDEX_GROUP1              0 /* Group number 1 */
#define APP_INDEX_GROUP2              1 /* Group number 2 */
#define APP_SLAVE_NEW_NETMASK         0xFFFFFF00
#define APP_PROTOCOL_VERSION          2
#define APP_NUMBER_STATISTICS_SAMPLES 1000
#define APP_DEFAULT_TIMEOUTCOUNT      3
#define APP_DEFAULT_TIMEOUT           500  /* milliseconds */
#define APP_ARBITRATION_TIME          2500 /* milliseconds */
#define APP_CALLBACK_TIME_NODE_SEARCH 2000 /* milliseconds */
#define APP_CALLBACK_TIME_SET_IP      500  /* milliseconds */

/* First slave, located in group 1.
 * RX0 = button. Sent from slave.
 * RY16 = LED. Controlled by master.
 * RWr0 = button press counter, sent from slave.
 * RWr0 from slave is multiplied, and sent back as RWw0 */

#define APP_DEVICE_INDEX_SLAVE1 0
#define APP_RX_NUMBER_SLAVE1    0
#define APP_RY_NUMBER_SLAVE1    16
#define APP_RWR_NUMBER_SLAVE1   0
#define APP_RWW_NUMBER_SLAVE1   0
#define APP_MULTIPLIER_SLAVE1   2

/* Second slave, also located in group 1.
 * The RWr value is multiplied and sent to slave as RWw.
 * The RX value is inverted and sent to slave as RY. */

#define APP_DEVICE_INDEX_SLAVE2 1
#define APP_RX_NUMBER_SLAVE2    0
#define APP_RY_NUMBER_SLAVE2    17
#define APP_RWR_NUMBER_SLAVE2   0
#define APP_RWW_NUMBER_SLAVE2   0
#define APP_MULTIPLIER_SLAVE2   3

/* Third slave, can be located in group 1 or 2,
 * and will have different device index accordingly.
 * The RWr value is multiplied and sent to slave as RWw.
 * The RX value is inverted and sent to slave as RY. */

#define APP_DEVICE_INDEX_SLAVE3_GROUP1 2
#define APP_DEVICE_INDEX_SLAVE3_GROUP2 0
#define APP_RX_NUMBER_SLAVE3           0
#define APP_RY_NUMBER_SLAVE3           16
#define APP_RWR_NUMBER_SLAVE3          0
#define APP_RWW_NUMBER_SLAVE3          0
#define APP_MULTIPLIER_SLAVE3          4

/*************************** Callbacks **************************************/

/**
 * Callback triggered for each timer tick.
 *
 * @param timer            Timer instance
 * @param arg              Application data. Should be app_master_data_t
 */
static void app_timer_tick (os_timer_t * timer, void * arg)
{
   app_master_data_t * appdata = arg;

   os_event_set (appdata->main_events, APP_EVENT_PERIODIC);
}

void masterapp_state_ind (clm_t * clm, void * arg, clm_master_state_t state)
{
   const char * description;

   switch (state)
   {
   case CLM_MASTER_STATE_DOWN:
      description = "DOWN";
      break;
   case CLM_MASTER_STATE_STANDBY:
      description = "STANDBY";
      break;
   case CLM_MASTER_STATE_ARBITRATION:
      description = "ARBITRATION";
      break;
   case CLM_MASTER_STATE_RUNNING:
      description = "RUNNING";
      break;
   default:
      description = "INVALID STATE";
      break;
   }
   printf (
      "MASTERAPP: The master stack indicates state change. New state: %s\n",
      description);

   if (state == CLM_MASTER_STATE_RUNNING)
   {
      printf ("\nMASTERAPP: Master running\n\n");
   }
}

void masterapp_connect_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   cl_ipaddr_t slave_id)
{
   const clm_slave_device_data_t * device_details =
      clm_get_device_connection_details (clm, group_index, slave_device_index);

   printf (
      "MASTERAPP: A slave connects. Group index %u, slave device index %u. "
      "IP "
      "0x%08" PRIX32 "\n",
      group_index,
      slave_device_index,
      slave_id);
   printf (
      "  Device vendor: 0x%04X Device model: 0x%08" PRIX32
      " Version: 0x%04X Protocol version: %u\n",
      device_details->latest_frame.vendor_code,
      device_details->latest_frame.model_code,
      device_details->latest_frame.equipment_ver,
      device_details->latest_frame.protocol_ver);
   printf (
      "  Slave operation: %s  Slave err code: 0x%04X  Local management info: "
      "0x%08" PRIX32 "\n",
      device_details->latest_frame.slave_local_unit_info == 0 ? "Stopped"
                                                              : "Running",
      device_details->latest_frame.slave_err_code,
      device_details->latest_frame.local_management_info);
   printf (
      "  Response time (dependent also on when master listens): %" PRIu32
      " microseconds\n",
      device_details->latest_frame.response_time);
   printf (
      "  Connection counter for this slave device since restart: %" PRIu32 "\n",
      device_details->statistics.number_of_connects);
}

void masterapp_disconnect_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   cl_ipaddr_t slave_id)
{
   printf (
      "MASTERAPP: A slave disconnects. Group index %u, slave device index "
      "%u. IP 0x%08" PRIX32 "\n",
      group_index,
      slave_device_index,
      slave_id);
}

void masterapp_alarm_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t end_code,
   uint16_t slave_err_code,
   uint32_t local_management_info)
{
   printf (
      "MASTERAPP: Alarm indication from a slave. Group index %u, slave device "
      "index "
      "%u. End code 0x%04X  Slave error code 0x%04X  Local management info  "
      "0x%08" PRIX32 "\n",
      group_index,
      slave_device_index,
      end_code,
      slave_err_code,
      local_management_info);
   if (end_code == 0xCFFF)
   {
      printf ("  The slave indicates that it would like to disconnect.\n");
   }
}

void masterapp_error_ind (
   clm_t * clm,
   void * arg,
   clm_error_message_t error_message,
   cl_ipaddr_t ip_addr,
   uint16_t argument_2)
{
   printf ("MASTERAPP: Error message: ");
   switch (error_message)
   {
   case CLM_ERROR_ARBITRATION_FAILED:
      printf (
         "Arbitration failed: Detected other master with IP 0x%08" PRIX32 " \n",
         ip_addr);
      printf (
         "You must restart the application after adjusting your network.\n");
      break;
   case CLM_ERROR_SLAVE_DUPLICATION:
      printf ("Slave duplication detected. IP 0x%08" PRIX32 "\n", ip_addr);
      printf ("You must remove the offending slave.\n");
      break;
   case CLM_ERROR_SLAVE_REPORTS_WRONG_NUMBER_OCCUPIED:
      printf (
         "Slave reports that master sends wrong number of occupied slave "
         "stations. IP 0x%08" PRIX32 "\n",
         ip_addr);
      printf (
         "You must replace the slave with one that fits your configuration.\n");
      break;
   case CLM_ERROR_SLAVE_REPORTS_MASTER_DUPLICATION:
      printf (
         "Slave reports master duplication. Slave IP 0x%08" PRIX32 "\n",
         ip_addr);
      printf ("You must restart the application after adjusting your "
              "configuration.\n");
      break;
   default:
      printf ("Unknown error\n");
      break;
   }
}

void masterapp_slave_info_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t end_code,
   uint16_t slave_err_code,
   uint32_t local_management_info)
{
   printf (
      "MASTERAPP: Slave info changed. Group index %u, slave device index "
      "%u. End code 0x%04X  Slave error code 0x%04X  Local management info  "
      "0x%08" PRIX32 "\n",
      group_index,
      slave_device_index,
      end_code,
      slave_err_code,
      local_management_info);
}

static void masterapp_linkscan_ind (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   bool success)
{
   /* Uses app_data_t, so it is of less use for other applications */

   app_master_data_t * appdata = arg;

   if (group_index == APP_INDEX_GROUP1)
   {
      /* Data from and to slave 1 */
      appdata->button_state_from_slave1 = clm_get_rx_bit (
         clm,
         APP_INDEX_GROUP1,
         APP_DEVICE_INDEX_SLAVE1,
         APP_RX_NUMBER_SLAVE1);
      appdata->button_counter_from_slave1 = clm_get_rwr_value (
         clm,
         APP_INDEX_GROUP1,
         APP_DEVICE_INDEX_SLAVE1,
         APP_RWR_NUMBER_SLAVE1);
      clm_set_rww_value (
         clm,
         APP_INDEX_GROUP1,
         APP_DEVICE_INDEX_SLAVE1,
         APP_RWW_NUMBER_SLAVE1,
         appdata->integer_to_slave1);
      clm_set_ry_bit (
         clm,
         APP_INDEX_GROUP1,
         APP_DEVICE_INDEX_SLAVE1,
         APP_RY_NUMBER_SLAVE1,
         appdata->led_state_to_slave1);

      /* Data from and to slave 2 */
      if (appdata->has_second_device)
      {
         appdata->bit_from_slave2 = clm_get_rx_bit (
            clm,
            APP_INDEX_GROUP1,
            APP_DEVICE_INDEX_SLAVE2,
            APP_RX_NUMBER_SLAVE2);
         appdata->integer_from_slave2 = clm_get_rwr_value (
            clm,
            APP_INDEX_GROUP1,
            APP_DEVICE_INDEX_SLAVE2,
            APP_RWR_NUMBER_SLAVE2);
         clm_set_rww_value (
            clm,
            APP_INDEX_GROUP1,
            APP_DEVICE_INDEX_SLAVE2,
            APP_RWW_NUMBER_SLAVE2,
            appdata->integer_to_slave2);
         clm_set_ry_bit (
            clm,
            APP_INDEX_GROUP1,
            APP_DEVICE_INDEX_SLAVE2,
            APP_RY_NUMBER_SLAVE2,
            appdata->bit_to_slave2);
      }
   }

   /* Data from and to slave 3, which might be in group 1 or group 2 */
   if ((group_index == APP_INDEX_GROUP1 && appdata->has_third_device) || group_index == APP_INDEX_GROUP2)
   {
      appdata->bit_from_slave3 = clm_get_rx_bit (
         clm,
         appdata->group_index_slave3,
         appdata->device_index_slave3,
         APP_RX_NUMBER_SLAVE3);
      appdata->integer_from_slave3 = clm_get_rwr_value (
         clm,
         appdata->group_index_slave3,
         appdata->device_index_slave3,
         APP_RWR_NUMBER_SLAVE3);
      clm_set_rww_value (
         clm,
         appdata->group_index_slave3,
         appdata->device_index_slave3,
         APP_RWW_NUMBER_SLAVE3,
         appdata->integer_to_slave3);
      clm_set_ry_bit (
         clm,
         appdata->group_index_slave3,
         appdata->device_index_slave3,
         APP_RY_NUMBER_SLAVE3,
         appdata->bit_to_slave3);
   }
}

void masterapp_node_search_result_cfm (
   clm_t * clm,
   void * arg,
   clm_node_search_db_t * db)
{
   clm_node_search_response_entry_t * entry;
   uint16_t i = 0;

   printf (
      "MASTERAPP: Did store %d slave devices during the node search (total %d "
      "found):\n",
      db->stored,
      db->count);
   printf ("  IP          Netmask     MAC                Vendor  Model\n");
   for (i = 0; i < db->stored; i++)
   {
      entry = &db->entries[i];
      printf (
         "  0x%08" PRIX32 "  0x%08" PRIX32
         "  %02X:%02X:%02X:%02X:%02X:%02X  0x%04X  0x%08" PRIX32 " \n",
         entry->slave_id,
         entry->slave_netmask,
         entry->slave_mac_addr[0],
         entry->slave_mac_addr[1],
         entry->slave_mac_addr[2],
         entry->slave_mac_addr[3],
         entry->slave_mac_addr[4],
         entry->slave_mac_addr[5],
         entry->vendor_code,
         entry->model_code);
   }
}

void masterapp_set_ip_cfm (clm_t * clm, void * arg, clm_master_setip_status_t status)
{
   const char * description = NULL;

   switch (status)
   {
   case CLM_MASTER_SET_IP_STATUS_SUCCESS:
      description = "SUCCESS";
      break;
   case CLM_MASTER_SET_IP_STATUS_ERROR:
      description = "ERROR";
      break;
   case CLM_MASTER_SET_IP_STATUS_TIMEOUT:
      description = "TIMEOUT";
      break;
   default:
      description = "UNKNOWN";
      break;
   }
   printf (
      "MASTERAPP: Set IP request completed with status: %d = %s\n",
      status,
      description);
}

/****************************************************************************/

/**
 * Print some application settings
 *
 * @param config        Configuration
 */
static void app_print_master_settings (clm_cfg_t * config)
{
   printf ("  c-link version %s\n", cl_version());
   printf ("  Application tick interval: %u microseconds\n", APP_TICK_INTERVAL_US);
   printf ("  Arbitration time: %u milliseconds\n", config->arbitration_time);
   printf ("  Master IP address: 0x%08" PRIX32 "\n", config->master_id);
   printf ("  Number of groups: %u\n", config->hier.number_of_groups);
   printf ("  Slave1:\n");
   printf (
      "    IP address: 0x%08" PRIX32 "\n",
      config->hier.groups[APP_INDEX_GROUP1].slave_devices[0].slave_id);
   printf (
      "    Num occupied slave stations: %u\n",
      config->hier.groups[APP_INDEX_GROUP1].slave_devices[0].num_occupied_stations);
   printf ("    Slave data button input to me: RX%u (dec)\n", APP_RX_NUMBER_SLAVE1);
   printf ("    Slave data LED output from me: RY%u (dec)\n", APP_RY_NUMBER_SLAVE1);
   printf (
      "    Slave button press counter (integer) input to me: RWr%u (dec)\n",
      APP_RWR_NUMBER_SLAVE1);
   printf (
      "    Button press counter RWr%u is multiplied by %d and sent back as "
      "RWw%u (dec)\n",
      APP_RWR_NUMBER_SLAVE1,
      APP_MULTIPLIER_SLAVE1,
      APP_RWW_NUMBER_SLAVE1);

   if (config->hier.groups[APP_INDEX_GROUP1].num_slave_devices > 1)
   {
      printf ("  Slave2:\n");
      printf (
         "    IP address: 0x%08" PRIX32 "\n",
         config->hier.groups[APP_INDEX_GROUP1].slave_devices[1].slave_id);
      printf (
         "    Num occupied slave stations: %u\n",
         config->hier.groups[APP_INDEX_GROUP1].slave_devices[1].num_occupied_stations);
      printf (
         "    Register RWr%u from slave is multiplied by %d and sent back "
         "as RWw%u (dec)\n",
         APP_RWR_NUMBER_SLAVE2,
         APP_MULTIPLIER_SLAVE2,
         APP_RWW_NUMBER_SLAVE2);
      printf (
         "    Bit RX%u from slave is inverted and sent back "
         "as RY%u (dec)\n",
         APP_RX_NUMBER_SLAVE2,
         APP_RY_NUMBER_SLAVE2);
   }

   if (config->hier.groups[APP_INDEX_GROUP1].num_slave_devices > 2)
   {
      printf ("  Slave3:\n");
      printf (
         "    IP address: 0x%08" PRIX32 "\n",
         config->hier.groups[APP_INDEX_GROUP1].slave_devices[2].slave_id);
      printf (
         "    Num occupied slave stations: %u\n",
         config->hier.groups[APP_INDEX_GROUP1].slave_devices[2].num_occupied_stations);
      printf (
         "    Register RWr%u from slave is multiplied by %d and sent back "
         "as RWw%u (dec)\n",
         APP_RWR_NUMBER_SLAVE3,
         APP_MULTIPLIER_SLAVE3,
         APP_RWW_NUMBER_SLAVE3);
      printf (
         "    Bit RX%u from slave is inverted and sent back "
         "as RY%u (dec)\n",
         APP_RX_NUMBER_SLAVE3,
         APP_RY_NUMBER_SLAVE3);
   }

   if (
      config->hier.number_of_groups > 1 && CLM_MAX_GROUPS > 1 &&
      config->hier.groups[APP_INDEX_GROUP2].num_slave_devices > 0)
   {
      printf ("  First slave in second group:\n");
      printf (
         "    IP address: 0x%08" PRIX32 "\n",
         config->hier.groups[APP_INDEX_GROUP2].slave_devices[0].slave_id);
      printf (
         "    Num occupied slave stations: %u\n",
         config->hier.groups[APP_INDEX_GROUP2].slave_devices[0].num_occupied_stations);
      printf (
         "    Register RWr%u from slave is multiplied by %d and sent back "
         "as RWw%u (dec)\n",
         APP_RWR_NUMBER_SLAVE3,
         APP_MULTIPLIER_SLAVE3,
         APP_RWW_NUMBER_SLAVE3);
      printf (
         "    Bit RX%u from slave is inverted and sent back "
         "as RY%u (dec)\n",
         APP_RX_NUMBER_SLAVE3,
         APP_RY_NUMBER_SLAVE3);
   }
}

/**
 * Demonstrate use of different parts of the public API.
 *
 * This includes doing node scan and setting IP address of a slave.
 *
 * @param appdata          Application data.
 */
static void app_update_state (app_master_data_t * appdata)
{
   clm_t * clm = appdata->clm;
   const clm_node_search_db_t * db;
   const clm_slave_device_data_t * slave_device_data;
   clm_master_status_details_t master_details;
   clm_group_status_details_t group_details;

   switch (appdata->demo_state)
   {
   case APP_MASTER_DEMOSTATE_NORMAL:
      appdata->demo_state = APP_MASTER_DEMOSTATE_NODE_SEARCH;
      break;
   case APP_MASTER_DEMOSTATE_NODE_SEARCH:
      appdata->demo_state = APP_MASTER_DEMOSTATE_SET_IP;
      break;
   case APP_MASTER_DEMOSTATE_SET_IP:
      appdata->demo_state = APP_MASTER_DEMOSTATE_MASTER_APPL_STOPPED;
      break;
   case APP_MASTER_DEMOSTATE_MASTER_APPL_STOPPED:
      appdata->demo_state = APP_MASTER_DEMOSTATE_MASTER_APPL_STOPPED_BY_USER;
      break;
   case APP_MASTER_DEMOSTATE_MASTER_APPL_STOPPED_BY_USER:
      appdata->demo_state = APP_MASTER_DEMOSTATE_MASTER_APPL_RUNNING;
      break;
   case APP_MASTER_DEMOSTATE_MASTER_APPL_RUNNING:
      appdata->demo_state = APP_MASTER_DEMOSTATE_DISABLE_SLAVE;
      break;
   case APP_MASTER_DEMOSTATE_DISABLE_SLAVE:
      appdata->demo_state = APP_MASTER_DEMOSTATE_ENABLE_SLAVE;
      break;
   case APP_MASTER_DEMOSTATE_ENABLE_SLAVE:
      appdata->demo_state = APP_MASTER_DEMOSTATE_SHOW_STATISTICS;
      break;
   case APP_MASTER_DEMOSTATE_SHOW_STATISTICS:
   default:
      appdata->demo_state = APP_MASTER_DEMOSTATE_NORMAL;
      break;
   }

   switch (appdata->demo_state)
   {
   case APP_MASTER_DEMOSTATE_NODE_SEARCH:
      printf ("MASTERAPP:   *** Perform node search ***\n");
      clm_perform_node_search (clm);
      break;
   case APP_MASTER_DEMOSTATE_SET_IP:
      printf ("MASTERAPP:   *** Set IP address of the slave that happens to "
              "appear first in the node search result ***\n");
      {
         db = clm_get_node_search_result (clm);
         if (db->stored > 0)
         {
            clm_set_slave_ipaddr (
               clm,
               &db->entries[0].slave_mac_addr,
               appdata->new_slave_ip_addr,
               APP_SLAVE_NEW_NETMASK);
         }
         else
         {
            printf (
               "MASTERAPP:   No slave info available. Will not send Set IP\n");
         }
      }
      break;
   case APP_MASTER_DEMOSTATE_MASTER_APPL_STOPPED:
      printf (
         "MASTERAPP:   *** Setting 'master application state' field to stopped "
         "(but the sample app calculations are still running) ***\n");
      clm_set_master_application_status (clm, false, false);
      printf (
         "MASTERAPP:    New master appl status: %u\n",
         clm_get_master_application_status (clm));
      break;
   case APP_MASTER_DEMOSTATE_MASTER_APPL_STOPPED_BY_USER:
      printf ("MASTERAPP:   *** Setting 'master application state' to stopped "
              "by user ***\n");
      clm_set_master_application_status (clm, false, true);
      printf (
         "MASTERAPP:    New master appl status: %u\n",
         clm_get_master_application_status (clm));
      break;
   case APP_MASTER_DEMOSTATE_MASTER_APPL_RUNNING:
      printf ("MASTERAPP:   *** Setting 'master application state' to running "
              "***\n");
      clm_set_master_application_status (clm, true, false);
      printf (
         "MASTERAPP:    New master appl status: %u\n",
         clm_get_master_application_status (clm));
      break;
   case APP_MASTER_DEMOSTATE_DISABLE_SLAVE:
      if (appdata->has_second_group && appdata->has_second_device)
      {
         printf ("MASTERAPP:   *** Disabling first two slave devices in first "
                 "group ***\n");
         printf (
            "MASTERAPP:       Enabling first slave device in second group\n");
         (void)clm_set_slave_communication_status (
            clm,
            APP_INDEX_GROUP1,
            APP_DEVICE_INDEX_SLAVE1,
            false);
         (void)clm_set_slave_communication_status (
            clm,
            APP_INDEX_GROUP1,
            APP_DEVICE_INDEX_SLAVE2,
            false);
         (void)clm_set_slave_communication_status (
            clm,
            APP_INDEX_GROUP2,
            APP_DEVICE_INDEX_SLAVE3_GROUP2,
            true);
      }
      else
      {
         printf (
            "MASTERAPP:   *** Disabling slave device with index 0 (in first "
            "group) ***\n");
         (void)clm_set_slave_communication_status (
            clm,
            APP_INDEX_GROUP1,
            APP_DEVICE_INDEX_SLAVE1,
            false);
      }
      break;
   case APP_MASTER_DEMOSTATE_ENABLE_SLAVE:
      if (appdata->has_second_group && appdata->has_second_device)
      {
         printf ("MASTERAPP:   *** Enabling first two slave devices in first "
                 "group ***\n");
         printf (
            "MASTERAPP:       Disabling first slave device in second group\n");
         (void)clm_set_slave_communication_status (
            clm,
            APP_INDEX_GROUP1,
            APP_DEVICE_INDEX_SLAVE1,
            true);
         (void)clm_set_slave_communication_status (
            clm,
            APP_INDEX_GROUP1,
            APP_DEVICE_INDEX_SLAVE2,
            true);
         (void)clm_set_slave_communication_status (
            clm,
            APP_INDEX_GROUP2,
            APP_DEVICE_INDEX_SLAVE3_GROUP2,
            false);
      }
      else
      {
         printf ("MASTERAPP:   *** Enabling slave device with index 0 (in "
                 "first group) ***\n");
         (void)clm_set_slave_communication_status (
            clm,
            APP_INDEX_GROUP1,
            APP_DEVICE_INDEX_SLAVE1,
            true);
      }
      break;
   case APP_MASTER_DEMOSTATE_SHOW_STATISTICS:
      printf (
         "MASTERAPP:   ***  Show communication statistics for one slave ***\n");
      slave_device_data = clm_get_device_connection_details (
         clm,
         APP_INDEX_GROUP1,
         APP_DEVICE_INDEX_SLAVE1);
      printf (
         "MASTERAPP:   Group index: %u   Slave device index: %u\n",
         APP_INDEX_GROUP1,
         APP_DEVICE_INDEX_SLAVE1);
      printf ("MASTERAPP:   Slave device connection details\n");
      printf (
         "MASTERAPP:      Slave enabled: %u   State of slave representation: "
         "%u\n",
         slave_device_data->enabled,
         slave_device_data->device_state);
      printf (
         "MASTERAPP:      Slave IP: 0x%08" PRIX32
         "   Local management info: 0x%08" PRIX32 "\n",
         slave_device_data->latest_frame.slave_id,
         slave_device_data->latest_frame.local_management_info);
      printf (
         "MASTERAPP:      Frame sequence number: %u\n",
         slave_device_data->latest_frame.frame_sequence_no);
      printf (
         "MASTERAPP:      Connects: %" PRIu32 "    Disconnects: %" PRIu32 "\n",
         slave_device_data->statistics.number_of_connects,
         slave_device_data->statistics.number_of_disconnects);
      printf (
         "MASTERAPP:      Received frames: %" PRIu32
         "    Timing measurements: %" PRIu32 "\n",
         slave_device_data->statistics.number_of_incoming_frames,
         slave_device_data->statistics.measured_time.number_of_samples);
      printf (
         "MASTERAPP:      Response time (microseconds) Average: %" PRIu32
         "    Min: %" PRIu32 "    Max: %" PRIu32 "\n",
         slave_device_data->statistics.measured_time.average,
         slave_device_data->statistics.measured_time.min,
         slave_device_data->statistics.measured_time.max);
      if (clm_get_group_status (clm, APP_INDEX_GROUP1, &group_details) == 0)
      {
         printf ("MASTERAPP:   Group status details\n");
         printf (
            "MASTERAPP:      Group index: %u   Total occupied slave stations: "
            "%u\n",
            group_details.group_index,
            group_details.total_occupied);
         printf (
            "MASTERAPP:      Group state: %u   Frame sequence no: %u\n",
            group_details.group_state,
            group_details.frame_sequence_no);
         printf (
            "MASTERAPP:      Timestamp link scan start: %" PRIu32 "\n",
            group_details.timestamp_link_scan_start);
         printf (
            "MASTERAPP:      Cyclic transmission state: 0x%04X\n",
            group_details.cyclic_transmission_state);
      }
      if (clm_get_master_status (clm, &master_details) == 0)
      {
         printf ("MASTERAPP:   Master status details\n");
         printf (
            "MASTERAPP:      Master state: %u   Parameter no: %u\n",
            master_details.master_state,
            master_details.parameter_no);
         printf (
            "MASTERAPP:      Node search serial: %d   Set IP serial: %d\n",
            master_details.node_search_serial,
            master_details.set_ip_request_serial);
      }
      printf ("MASTERAPP:   Clear statistics\n");
      clm_clear_statistics (clm);
      break;
   default:
   case APP_MASTER_DEMOSTATE_NORMAL:
      break;
   }
}

/**
 * Main loop for master sample application
 *
 * The slave has a button, which will control the LED on the slave (via
 * this master program).
 *
 * The master has a button, which triggers node search etc.
 *
 * This function is used by \a os_thread_create()
 *
 * @param arg              Application data. Should be app_master_data_t
 */
static void app_master_thread (void * arg)
{
   app_master_data_t * appdata      = arg;
   clm_t * clm                      = appdata->clm;
   uint32_t mask                    = APP_EVENT_PERIODIC;
   uint32_t flags                   = 0;
   bool input_value_master_button   = false;
   bool previous_master_button      = false;
   bool previous_slave_button_state = false;
   uint16_t button_read_counter     = 0;
   uint16_t data_update_counter     = 0;
   bool oscillator_state            = false;
   uint16_t oscillator_counter      = 0;
   bool flashing                    = true;

   /* Main loop */
   for (;;)
   {
      os_event_wait (appdata->main_events, mask, &flags, OS_WAIT_FOREVER);

      if (flags & APP_EVENT_PERIODIC)
      {
         os_event_clr (appdata->main_events, APP_EVENT_PERIODIC);
         clm_handle_periodic (clm);

         /* Handle master button (do node search etc) */
         button_read_counter++;
         if (button_read_counter > APP_BUTTON_READ_TICKS)
         {
            button_read_counter = 0;
            input_value_master_button =
               app_get_button (appdata->user_arg, APP_MASTER_BUTTON_ID);
         }
         if (input_value_master_button == true && input_value_master_button != previous_master_button)
         {
            app_update_state (appdata);
         }
         previous_master_button = input_value_master_button;

         /* Update data */
         data_update_counter++;
         if (data_update_counter > APP_DATA_UPDATE_TICKS)
         {
            data_update_counter = 0;

            /* Calculate new data */
            oscillator_counter++;
            if (oscillator_counter > APP_OSCILLATOR_UPDATE_COUNTS)
            {
               oscillator_counter = 0;
               oscillator_state   = !oscillator_state;
            }

            if (appdata->button_state_from_slave1)
            {
               if (!previous_slave_button_state)
               {
                  flashing = !flashing;
               }
               appdata->led_state_to_slave1 = true;
            }
            else if (flashing)
            {
               appdata->led_state_to_slave1 = oscillator_state;
            }
            else
            {
               appdata->led_state_to_slave1 = false;
            }
            previous_slave_button_state = appdata->button_state_from_slave1;

            appdata->integer_to_slave1 =
               APP_MULTIPLIER_SLAVE1 * appdata->button_counter_from_slave1;

            appdata->bit_to_slave2 = !appdata->bit_from_slave2;
            appdata->integer_to_slave2 =
               APP_MULTIPLIER_SLAVE2 * appdata->integer_from_slave2;

            appdata->bit_to_slave3 = !appdata->bit_from_slave3;
            appdata->integer_to_slave3 =
               APP_MULTIPLIER_SLAVE3 * appdata->integer_from_slave3;
         }
      }
   }
}
int app_master_main (
   void * user_arg,
   clm_cfg_t * cfg,
   app_master_data_t * appdata_master,
   uint32_t priority,
   size_t stacksize)
{
   clm_group_status_details_t group_details;
   appdata_master->user_arg                   = user_arg;
   appdata_master->demo_state                 = APP_MASTER_DEMOSTATE_NORMAL;
   appdata_master->button_state_from_slave1   = false;
   appdata_master->led_state_to_slave1        = false;
   appdata_master->button_counter_from_slave1 = 0;
   appdata_master->integer_to_slave1          = 0;
   appdata_master->main_events                = os_event_create();

   /* Set common defaults */
   cfg->cb_arg                    = appdata_master;
   cfg->state_cb                  = masterapp_state_ind;
   cfg->connect_cb                = masterapp_connect_ind;
   cfg->disconnect_cb             = masterapp_disconnect_ind;
   cfg->linkscan_cb               = masterapp_linkscan_ind;
   cfg->node_search_cfm_cb        = masterapp_node_search_result_cfm;
   cfg->set_ip_cfm_cb             = masterapp_set_ip_cfm;
   cfg->alarm_cb                  = masterapp_alarm_ind;
   cfg->error_cb                  = masterapp_error_ind;
   cfg->changed_slave_info_cb     = masterapp_slave_info_ind;
   cfg->arbitration_time          = APP_ARBITRATION_TIME;
   cfg->callback_time_node_search = APP_CALLBACK_TIME_NODE_SEARCH;
   cfg->callback_time_set_ip      = APP_CALLBACK_TIME_SET_IP;
   cfg->max_statistics_samples    = APP_NUMBER_STATISTICS_SAMPLES;
   cfg->protocol_ver              = APP_PROTOCOL_VERSION;
   /* Note that cfg->master_id,
           cfg->hier.groups[0].use_constant_link_scan_time
           cfg->hier.groups[0].slave_devices[0].num_occupied_stations and
           cfg->hier.groups[0].slave_devices[0].slave_id are already set */

   /* If not given in calling function */
   if (cfg->hier.number_of_groups == 0)
   {
      cfg->hier.number_of_groups = 1;
   }
   for (uint16_t i = 0; i < cfg->hier.number_of_groups; i++)
   {
      if (cfg->hier.groups[i].num_slave_devices == 0)
      {
         cfg->hier.groups[i].num_slave_devices = 1;
      }
      if (cfg->hier.groups[i].timeout_value == 0)
      {
         cfg->hier.groups[i].timeout_value = APP_DEFAULT_TIMEOUT;
      }
      if (cfg->hier.groups[i].parallel_off_timeout_count == 0)
      {
         cfg->hier.groups[i].parallel_off_timeout_count =
            APP_DEFAULT_TIMEOUTCOUNT;
      }

      /* Copy constant link time setting from first group,
         if not already enabled in the other groups */
      if (!cfg->hier.groups[i].use_constant_link_scan_time)
      {
         cfg->hier.groups[i].use_constant_link_scan_time =
            cfg->hier.groups[APP_INDEX_GROUP1].use_constant_link_scan_time;
      }
   }

   printf ("MASTERAPP: Starting master sample app\n");

   app_print_master_settings (cfg);

   /* Initialise stack */
   appdata_master->clm = clm_init (cfg);
   if (appdata_master->clm == NULL)
   {
      return -1;
   }

   appdata_master->has_second_device = clm_get_device_connection_details (
                                          appdata_master->clm,
                                          APP_INDEX_GROUP1,
                                          APP_DEVICE_INDEX_SLAVE2) != NULL;
   appdata_master->has_third_device = clm_get_device_connection_details (
                                         appdata_master->clm,
                                         APP_INDEX_GROUP1,
                                         APP_DEVICE_INDEX_SLAVE3_GROUP1) != NULL;
   appdata_master->has_second_group = clm_get_group_status (
                                         appdata_master->clm,
                                         APP_INDEX_GROUP2,
                                         &group_details) == 0;

   /* Slave 3 (if available) can be located in group 1 or 2,
      so the device index (within its group) must be adjusted */
   if (appdata_master->has_third_device)
   {
      appdata_master->group_index_slave3  = APP_INDEX_GROUP1;
      appdata_master->device_index_slave3 = APP_DEVICE_INDEX_SLAVE3_GROUP1;
   }
   else if (appdata_master->has_second_group)
   {
      appdata_master->group_index_slave3  = APP_INDEX_GROUP2;
      appdata_master->device_index_slave3 = APP_DEVICE_INDEX_SLAVE3_GROUP2;
   }

   appdata_master->new_slave_ip_addr = cfg->hier.groups[APP_INDEX_GROUP1]
                                          .slave_devices[APP_DEVICE_INDEX_SLAVE1]
                                          .slave_id;

   printf (
      "MASTERAPP: Second device: %s  Third device: %s  Second group: %s\n",
      appdata_master->has_second_device ? "Yes" : "No",
      appdata_master->has_third_device ? "Yes" : "No",
      appdata_master->has_second_group ? "Yes" : "No");

   printf ("MASTERAPP: Master stack initialised. Initialise timer and main "
           "thread.\n\n"
           "MASTERAPP: Waiting for arbitration (listening for other "
           "masters)\n\n");

   appdata_master->main_timer =
      os_timer_create (APP_TICK_INTERVAL_US, app_timer_tick, appdata_master, false);

   os_thread_create (
      "clm_master",
      priority,
      stacksize,
      app_master_thread,
      appdata_master);

   os_timer_start (appdata_master->main_timer);

   return 0;
}
