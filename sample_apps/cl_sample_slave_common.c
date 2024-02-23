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

#include "cl_sample_slave_common.h"
#include "cls_api.h"

#include "osal.h"

#include <inttypes.h>
#include <stdbool.h>

/* RX0 = button. Sent to PLC where it is exposed as X100 */
#define APP_RX_NUMBER 0

/* RY16 = LED. Controlled from PLC, where it is Y120 (why?) */
#define APP_RY_NUMBER 16

/* RWr0 = button press counter. Sent to PLC, where it is exposed as W0 */
#define APP_RWR_NUMBER 0

/* RWw0 = some other integer. Sent from the PLC, where it is named W100 */
#define APP_RWW_NUMBER 0

#define APP_TICK_INTERVAL_US       1000
#define APP_VENDOR_CODE            0x1067
#define APP_MODEL_CODE             0x87654321
#define APP_EQUIPMENT_VER          0x0002
#define APP_BUTTON_READ_TICKS      100
#define APP_EVENT_PERIODIC         BIT (0)
#define APP_LOCAL_MANGAGEMENT_INFO 0x64656667

/**
 * Set LED state.
 *
 * Compares new state with previous state, to minimize system calls.
 *
 * Uses the hardware specific app_set_led() function.
 *
 * @param appdata       Appdata
 * @param led_state     New LED state
 */
static void app_handle_data_led_state (app_slave_data_t * appdata, bool led_state)
{
   if (led_state != appdata->previous_led_state)
   {
      app_set_led (appdata->user_arg, APP_DATA_LED_ID, led_state);
   }
   appdata->previous_led_state = led_state;
}

/**
 * Set outputs to default value
 *
 * For the sample application this means that LED 1 is turned off.
 *
 * @param cls           The slave stack instance
 * @param appdata       Appdata
 * @return 0 on success, -1 on error
 */
static int app_set_default_outputs (cls_t * cls, app_slave_data_t * appdata)
{
   printf ("SLAVEAPP: Setting outputs to default value\n");
   app_handle_data_led_state (appdata, false);

   return 0;
}

/*************************** Callbacks ************************************/

void slaveapp_state_ind (cls_t * cls, void * arg, cls_slave_state_t state)
{
   uint64_t timestamp = 0;

   const char * description;

   switch (state)
   {
   case CLS_SLAVE_STATE_SLAVE_DOWN:
      description = "SLAVE_DOWN";
      break;
   case CLS_SLAVE_STATE_MASTER_NONE:
      description = "MASTER_NONE";
      break;
   case CLS_SLAVE_STATE_MASTER_CONTROL:
      description = "MASTER_CONTROL";
      break;
   case CLS_SLAVE_STATE_SLAVE_DISABLED:
      description = "SLAVE_DISABLED";
      break;
   case CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE:
      description = "WAIT_DISABLING_SLAVE";
      break;
   default:
   case CLS_SLAVE_STATE_LAST:
      description = "ILLEGAL STATE";
      break;
   }
   printf (
      "SLAVEAPP: The slave stack indicates state change. New state: %s\n",
      description);

   if (cls_get_master_timestamp (cls, &timestamp) == 0)
   {
      printf (
         "SLAVEAPP:   Master timestamp is %" PRIu64
         " (UNIX timestamp with milliseconds)\n",
         timestamp);
   }
   else
   {
      printf ("SLAVEAPP:   No master timestamp available\n");
   }
}

void slaveapp_error_ind (
   cls_t * cls,
   void * arg,
   cls_error_message_t error_message,
   cl_ipaddr_t ip_addr,
   uint16_t argument_2)
{
   printf ("SLAVEAPP: Error message: ");

   switch (error_message)
   {
   case CLS_ERROR_MASTER_STATION_DUPLICATION:
      printf ("Detected other master with IP 0x%08" PRIX32 " \n", ip_addr);
      printf ("You must remove the other master.\n");
      break;
   case CLS_ERROR_SLAVE_STATION_DUPLICATION:
      printf ("Slave duplication detected.\n");
      printf ("You must remove the offending slave.\n");
      break;
   case CLS_ERROR_WRONG_NUMBER_OCCUPIED:
      printf (
         "Master requests wrong number of occupied slave stations: %u\n",
         argument_2);
      printf ("You must adjust the configuration in the master.\n");
      break;
   default:
      printf ("Unknown error\n");
      break;
   }
}

void slaveapp_connect_ind (
   cls_t * cls,
   void * arg,
   cl_ipaddr_t master_ip_addr,
   uint16_t group_no,
   uint16_t slave_station_no)
{
   const cls_master_connection_t * master_connection = NULL;

   printf (
      "SLAVEAPP: The master connects. Master IP 0x%08" PRIX32
      "  We are slave station %u in group %u\n",
      master_ip_addr,
      slave_station_no,
      group_no);

   master_connection = cls_get_master_connection_details (cls);
   printf (
      "SLAVEAPP:   Parameter no %u. Timeout %u x %u ms\n",
      master_connection->parameter_no,
      master_connection->parallel_off_timeout_count,
      master_connection->timeout_value);
}

void slaveapp_disconnect_ind (cls_t * cls, void * arg)
{
   printf ("SLAVEAPP: The master disconnects\n");
}

void slaveapp_node_search_ind (
   cls_t * cls,
   void * arg,
   cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr)
{
   printf (
      "SLAVEAPP: The master does a node search. Master IP address 0x%08" PRIX32
      "\n",
      master_ip_addr);
}

void slaveapp_set_ip_ind (
   cls_t * cls,
   void * arg,
   cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr,
   cl_ipaddr_t new_ip_addr,
   cl_ipaddr_t new_netmask,
   bool ip_setting_allowed,
   bool did_set_ip)
{
   printf (
      "SLAVEAPP: The master does set our IP address: %s, %s\n",
      ip_setting_allowed ? "Allowed" : "Not allowed",
      did_set_ip ? "Success" : "Failure");
}

static void app_master_state_ind (
   cls_t * cls,
   void * arg,
   bool connected_to_master,
   bool connected_and_running,
   bool stopped_by_user,
   uint16_t protocol_ver,
   uint16_t master_application_status)
{
   /* Uses app_slave_data_t, so it is of less use for other applications */
   app_slave_data_t * appdata = arg;

   printf (
      "SLAVEAPP: The stack indicates that master application state has "
      "changed. Connected: %s  Running: %s  Stopped by user: %s\n",
      connected_to_master ? "Yes" : "No",
      connected_and_running ? "Yes" : "No",
      stopped_by_user ? "Yes" : "No");
   appdata->master_running = connected_and_running;

   if (appdata->master_running == false)
   {
      app_set_default_outputs (cls, appdata);
   }
}

/**
 * Callback triggered for each timer tick.
 *
 * @param timer            Timer instance
 * @param arg              Application data. Should be app_slave_data_t
 */
static void app_timer_tick (os_timer_t * timer, void * arg)
{
   app_slave_data_t * appdata = arg;

   os_event_set (appdata->main_events, APP_EVENT_PERIODIC);
}

/*************************************************************************/

/**
 * Print some configuration settings
 *
 * @param config        Configuration to print
 */
static void app_print_settings (cls_cfg_t * config)
{
   printf ("  c-link version %s\n", cl_version());
   printf ("  Application tick interval: %u microseconds\n", APP_TICK_INTERVAL_US);
   printf ("  Vendor code: 0x%04x\n", config->vendor_code);
   printf ("  Model code: 0x%08" PRIX32 "\n", config->model_code);
   printf ("  Equipment version: 0x%04x\n", config->equipment_ver);
   printf ("  Occupied stations: %u\n", config->num_occupied_stations);
   printf (
      "  Max occupied stations (compile time setting): %u\n",
      CLS_MAX_OCCUPIED_STATIONS);
   printf (
      "  Is remote IP address setting allowed: %s\n",
      config->ip_setting_allowed ? "Yes" : "No");
   if (config->iefb_ip_addr == CL_IPADDR_ANY)
   {
      printf ("  Listening on all IP addresses\n");
   }
   else
   {
      printf ("  Listening on IP address 0x%08" PRIX32 "\n", config->iefb_ip_addr);
   }
   printf ("  Data button ID: %u\n", APP_DATA_BUTTON_ID);
   printf ("  Alarm button ID: %u\n", APP_ALARM_BUTTON_ID);
   printf ("  Data LED ID: %u\n", APP_DATA_LED_ID);
   printf ("  Data button input to PLC: RX%u\n", APP_RX_NUMBER);
   printf ("  Data LED output from PLC: RY%u\n", APP_RY_NUMBER);
   printf (
      "  Button press counter (integer) input to PLC: RWr%u\n",
      APP_RWR_NUMBER);
   printf ("  Some integer output from PLC: RWw%u\n", APP_RWW_NUMBER);
}

/**
 * Demonstrate use of different parts of the public API.
 *
 * This includes setting slave error codes and disabling the cyclic data.
 *
 * @param appdata          Application data.
 */
static void app_update_state (app_slave_data_t * appdata)
{
   cls_t * cls               = appdata->cls;
   uint16_t slave_error_code = 0;

   switch (appdata->demo_state)
   {
   case APP_SLAVE_DEMOSTATE_NORMAL:
      appdata->demo_state = APP_SLAVE_DEMOSTATE_ERRORCODE_A;
      break;
   case APP_SLAVE_DEMOSTATE_ERRORCODE_A:
      appdata->demo_state = APP_SLAVE_DEMOSTATE_ERRORCODE_B;
      break;
   case APP_SLAVE_DEMOSTATE_ERRORCODE_B:
      appdata->demo_state = APP_SLAVE_DEMOSTATE_NO_ERRORCODE;
      break;
   case APP_SLAVE_DEMOSTATE_NO_ERRORCODE:
      appdata->demo_state = APP_SLAVE_DEMOSTATE_DISABLE_SLAVE;
      break;
   case APP_SLAVE_DEMOSTATE_DISABLE_SLAVE:
      appdata->demo_state = APP_SLAVE_DEMOSTATE_ENABLE_SLAVE;
      break;
   case APP_SLAVE_DEMOSTATE_ENABLE_SLAVE:
   default:
      appdata->demo_state = APP_SLAVE_DEMOSTATE_NORMAL;
      break;
   }

   slave_error_code = 0;
   switch (appdata->demo_state)
   {
   case APP_SLAVE_DEMOSTATE_ERRORCODE_B:
      slave_error_code += 0x001;
      // fall through
   case APP_SLAVE_DEMOSTATE_ERRORCODE_A:
      slave_error_code += 0x0300;
      // fall through
   case APP_SLAVE_DEMOSTATE_NO_ERRORCODE:
      printf (
         "SLAVEAPP: *** Setting slave error code to 0x%04X ***\n",
         slave_error_code);
      cls_set_slave_error_code (cls, slave_error_code);
      break;
   case APP_SLAVE_DEMOSTATE_DISABLE_SLAVE:
      printf (
         "SLAVEAPP: *** Disabling cyclic communication (No error, according to "
         "sample app). ***\n");
      cls_stop_cyclic_data (cls, false);
      break;
   case APP_SLAVE_DEMOSTATE_ENABLE_SLAVE:
      printf ("SLAVEAPP: *** Enabling cyclic communication ***\n");
      cls_restart_cyclic_data (cls);
      break;
   default:
   case APP_SLAVE_DEMOSTATE_NORMAL:
      break;
   }
}

/**
 * Main loop for slave sample application
 *
 * This function is used by \a os_thread_create()
 *
 * @param arg              Application data. Should be app_slave_data_t
 */
static void app_thread (void * arg)
{
   app_slave_data_t * appdata       = arg;
   cls_t * cls                      = appdata->cls;
   uint32_t mask                    = APP_EVENT_PERIODIC;
   uint32_t flags                   = 0;
   uint16_t button_read_counter     = 0;
   bool output_value_led            = false; /* One bit (LED) sent from PLC */
   uint16_t button_press_counter    = 0;     /* One integer sent to the PLC */
   uint16_t counter_answer          = 0;     /* One integer sent from the PLC */
   bool input_value_data_button     = false; /* One bit (button) sent to PLC */
   bool input_value_alarm_button    = false;
   uint16_t previous_counter_answer = 0;
   bool previous_data_button        = false;
   bool previous_alarm_button       = false;

   appdata->demo_state = APP_SLAVE_DEMOSTATE_NORMAL;

   app_set_led (appdata->user_arg, APP_DATA_LED_ID, false);
   cls_set_local_management_info (cls, APP_LOCAL_MANGAGEMENT_INFO);
   printf (
      "SLAVEAPP: Local management info now set to 0x%08" PRIX32 "\n",
      cls_get_local_management_info (cls));
   printf (
      "SLAVEAPP: Slave application state is %u\n",
      cls_get_slave_application_status (cls));
   printf (
      "SLAVEAPP: Current slave error code 0x%04X\n",
      cls_get_slave_error_code (cls));
   printf ("\nSLAVEAPP: Waiting for PLC to connect\n\n");

   /* Main loop */
   for (;;)
   {
      os_event_wait (appdata->main_events, mask, &flags, OS_WAIT_FOREVER);

      if (flags & APP_EVENT_PERIODIC)
      {
         os_event_clr (appdata->main_events, APP_EVENT_PERIODIC);
         cls_handle_periodic (cls);

         /* Read buttons */
         button_read_counter++;
         if (button_read_counter > APP_BUTTON_READ_TICKS)
         {
            button_read_counter = 0;
            input_value_data_button =
               app_get_button (appdata->user_arg, APP_DATA_BUTTON_ID);
            input_value_alarm_button =
               app_get_button (appdata->user_arg, APP_ALARM_BUTTON_ID);
         }

         /* Calculate number of presses on the data button */
         if (input_value_data_button == true && input_value_data_button != previous_data_button)
         {
            button_press_counter++;
         }
         previous_data_button = input_value_data_button;

         /* Handle alarm button (set slave error code etc) */
         if (input_value_alarm_button == true && input_value_alarm_button != previous_alarm_button)
         {
            app_update_state (appdata);
         }
         previous_alarm_button = input_value_alarm_button;

         if (appdata->master_running)
         {
            /* Read and write cyclic data */
            output_value_led = cls_get_ry_bit (cls, APP_RY_NUMBER);
            counter_answer   = cls_get_rww_value (cls, APP_RWW_NUMBER);
            cls_set_rx_bit (cls, APP_RX_NUMBER, input_value_data_button);
            cls_set_rwr_value (cls, APP_RWR_NUMBER, button_press_counter);

            /* React to arbitrary manual RWw changes from conformance tool.
               Print "fourtytwo" when RWw0 changes to 42 (which is 0x002A) */
            if (counter_answer != previous_counter_answer && counter_answer == 42)
            {
               printf ("fourtytwo\n");
            }
            previous_counter_answer = counter_answer;

            /* Set LED */
            app_handle_data_led_state (appdata, output_value_led);
         }
      }
   }
}

int app_slave_main (
   void * user_arg,
   cls_cfg_t * cfg,
   app_slave_data_t * appdata_slave,
   uint32_t priority,
   size_t stacksize)
{
   appdata_slave->user_arg           = user_arg;
   appdata_slave->previous_led_state = false;
   appdata_slave->demo_state         = APP_SLAVE_DEMOSTATE_NORMAL;
   appdata_slave->master_running     = false;
   appdata_slave->main_events        = os_event_create();

   /* Set common defaults */
   cfg->vendor_code        = APP_VENDOR_CODE;
   cfg->model_code         = APP_MODEL_CODE;
   cfg->equipment_ver      = APP_EQUIPMENT_VER;
   cfg->ip_setting_allowed = true;
   cfg->cb_arg             = appdata_slave;
   cfg->state_cb           = slaveapp_state_ind;
   cfg->error_cb           = slaveapp_error_ind;
   cfg->connect_cb         = slaveapp_connect_ind;
   cfg->disconnect_cb      = slaveapp_disconnect_ind;
   cfg->master_running_cb  = app_master_state_ind;
   cfg->node_search_cb     = slaveapp_node_search_ind;
   cfg->set_ip_cb          = slaveapp_set_ip_ind;

   printf ("SLAVEAPP: Starting slave sample app\n");

   app_print_settings (cfg);

   /* Initialise stack */
   appdata_slave->cls = cls_init (cfg);
   if (appdata_slave->cls == NULL)
   {
      return -1;
   }

   printf ("SLAVEAPP: Stack initialised. Initialise timer and main thread.\n");

   /* Initialise thread runtime data */
   appdata_slave->main_timer =
      os_timer_create (APP_TICK_INTERVAL_US, app_timer_tick, appdata_slave, false);

   os_thread_create ("cl_slave", priority, stacksize, app_thread, appdata_slave);

   os_timer_start (appdata_slave->main_timer);

   return 0;
}
