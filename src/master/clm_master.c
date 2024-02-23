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
 * @brief Initialisation and validation at master startup.
 *
 * Also handles master shutdown.
 */

#ifdef UNIT_TEST
#define clal_init mock_clal_init
#define clal_exit mock_clal_exit
#endif

#define CLM_DEFAULT_PARAMETER_NO 0

#include "master/clm_master.h"

#include "cl_options.h"
#include "common/cl_eth.h"
#include "common/cl_file.h"
#include "common/cl_literals.h"
#include "common/cl_types.h"
#include "common/cl_util.h"
#include "master/clm_iefb.h"
#include "master/clm_slmp.h"

#include "osal_log.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#if CLM_MAX_GROUPS < CL_CCIEFB_MIN_GROUP_NO
#error "CLM_MAX_GROUPS is too small"
#endif

#if CLM_MAX_GROUPS > CL_CCIEFB_MAX_GROUP_NO
#error "CLM_MAX_GROUPS is too large"
#endif

#if CLM_MAX_OCCUPIED_STATIONS_PER_GROUP <                                      \
   CL_CCIEFB_MIN_OCCUPIED_STATIONS_PER_GROUP
#error "CLM_MAX_OCCUPIED_STATIONS_PER_GROUP is too small"
#endif

#if CLM_MAX_OCCUPIED_STATIONS_PER_GROUP >                                      \
   CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP
#error "CLM_MAX_OCCUPIED_STATIONS_PER_GROUP is too large"
#endif

#define CLM_MASTER_BITS_PER_LINE  32U
#define CLM_MASTER_WORDS_PER_LINE 8U

/**
 * Validate the master configuration
 *
 * @param cfg              c-link master stack configuration
 * @return 0 for valid configuration, -1 for invalid.
 *
 * @req REQ_CL_CAPACITY_03
 * @req REQ_CL_PROTOCOL_19
 * @req REQ_CL_PROTOCOL_31
 * @req REQ_CL_PROTOCOL_36
 * @req REQ_CLM_CAPACITY_05
 * @req REQ_CLM_CONFIGURATION_01
 * @req REQ_CLM_TIMING_01
 * @req REQ_CLM_TIMING_02
 * @req REQ_CLM_TIMING_05
 *
 */
int clm_validate_config (const clm_cfg_t * cfg)
{
   uint16_t group_index                   = 0;
   uint16_t slave_device_index            = 0;
   uint16_t num_slave_devices             = 0;
   uint16_t total_occupied_per_group      = 0;
   uint16_t total_occupied_for_all_groups = 0;
   const clm_group_setting_t * group_setting;
   const clm_slave_device_setting_t * slave_device_setting;

   if (cfg == NULL)
   {
      return -1;
   }

   /* Protocol version */
   if (
      (cfg->protocol_ver < CL_CCIEFB_MIN_SUPPORTED_PROTOCOL_VER) ||
      (cfg->protocol_ver > CL_CCIEFB_MAX_SUPPORTED_PROTOCOL_VER))
   {
      LOG_ERROR (
         CL_CCIEFB_LOG,
         "CLM_MASTER(%d): Wrong protocol version in the configuration. "
         "Given %u but min is %u and max is %u.\n",
         __LINE__,
         cfg->protocol_ver,
         CL_CCIEFB_MIN_SUPPORTED_PROTOCOL_VER,
         CL_CCIEFB_MAX_SUPPORTED_PROTOCOL_VER);
      return -1;
   }

   /* Is master IP address in valid range? See REQ_CL_PROTOCOL_19 */
   if (!cl_utils_is_ipaddr_range_valid (cfg->master_id))
   {
      LOG_ERROR (
         CL_CCIEFB_LOG,
         "CLM_MASTER(%d): The master IP address is invalid. Given 0x%08" PRIX32
         "\n",
         __LINE__,
         cfg->master_id);
      return -1;
   }

   /* Number of groups */
   if (
      (cfg->hier.number_of_groups < CL_CCIEFB_MIN_GROUP_NO) ||
      (cfg->hier.number_of_groups > CLM_MAX_GROUPS))
   {
      LOG_ERROR (
         CL_CCIEFB_LOG,
         "CLM_MASTER(%d): Wrong number of groups in the "
         "configuration. Given %u but min is %u and max is %u. Possibly "
         "increase CLM_MAX_GROUPS.\n",
         __LINE__,
         cfg->hier.number_of_groups,
         CL_CCIEFB_MIN_GROUP_NO,
         CLM_MAX_GROUPS);
      return -1;
   }

   /* Check groups and slave IDs */
   for (group_index = 0; group_index < cfg->hier.number_of_groups; group_index++)
   {
      total_occupied_per_group = 0;
      group_setting            = &cfg->hier.groups[group_index];
      num_slave_devices        = group_setting->num_slave_devices;

      /* Group timeout value */
      if (group_setting->timeout_value < CL_CCIEFB_MIN_TIMEOUT)
      {
         LOG_ERROR (
            CL_CCIEFB_LOG,
            "CLM_MASTER(%d): Wrong timeout for group %u (index "
            "%u) in the configuration. Given %u but min is %d.\n",
            __LINE__,
            group_index + 1U,
            group_index,
            group_setting->timeout_value,
            CL_CCIEFB_MIN_TIMEOUT);
         return -1;
      }

      if (
         group_setting->use_constant_link_scan_time &&
         group_setting->timeout_value > CL_CCIEFB_MAX_TIMEOUT_CONSTANT_LINKSCAN)
      {
         LOG_ERROR (
            CL_CCIEFB_LOG,
            "CLM_MASTER(%d): Too large constant link scan time in group %u "
            "(index %u) in the configuration. Given %u but max is %d.\n",
            __LINE__,
            group_index + 1U,
            group_index,
            group_setting->timeout_value,
            CL_CCIEFB_MAX_TIMEOUT_CONSTANT_LINKSCAN);
         return -1;
      }

      /* Group timeout count */
      if (group_setting->parallel_off_timeout_count < CL_CCIEFB_MIN_TIMEOUT_COUNT)
      {
         LOG_ERROR (
            CL_CCIEFB_LOG,
            "CLM_MASTER(%d): Wrong timeout count setting for group %u (index "
            "%u) in the configuration. Given %u but min is %d.\n",
            __LINE__,
            group_index + 1U,
            group_index,
            group_setting->parallel_off_timeout_count,
            CL_CCIEFB_MIN_TIMEOUT_COUNT);
         return -1;
      }

      if (num_slave_devices == 0 || num_slave_devices > CLM_MAX_OCCUPIED_STATIONS_PER_GROUP)
      {
         LOG_ERROR (
            CL_CCIEFB_LOG,
            "CLM_MASTER(%d): Wrong number of slave devices for group %u (index "
            "%u) in the configuration. Given %u but min is 1 and max is %d. "
            "Possibly increase CLM_MAX_OCCUPIED_STATIONS_PER_GROUP.\n",
            __LINE__,
            group_index + 1U,
            group_index,
            num_slave_devices,
            CLM_MAX_OCCUPIED_STATIONS_PER_GROUP);
         return -1;
      }

      for (slave_device_index = 0; slave_device_index < num_slave_devices;
           slave_device_index++)
      {
         slave_device_setting = &group_setting->slave_devices[slave_device_index];

         /* Is the IP address in valid range? See REQ_CL_PROTOCOL_36 */
         if (!cl_utils_is_ipaddr_range_valid (slave_device_setting->slave_id))
         {
            LOG_ERROR (
               CL_CCIEFB_LOG,
               "CLM_MASTER(%d): Invalid IP address for "
               "group index %u slave index %u in the configuration. Given "
               "0x%08" PRIX32 "\n",
               __LINE__,
               group_index,
               slave_device_index,
               slave_device_setting->slave_id);
            return -1;
         }

         if (slave_device_setting->slave_id == cfg->master_id)
         {
            LOG_ERROR (
               CL_CCIEFB_LOG,
               "CLM_MASTER(%d): The slave IP address in the config is the same "
               "as the master IP address. Group index %u slave index %u. Given "
               "0x%08" PRIX32 "\n",
               __LINE__,
               group_index,
               slave_device_index,
               cfg->master_id);
            return -1;
         }

         if (
            slave_device_setting->num_occupied_stations == 0 ||
            slave_device_setting->num_occupied_stations >
               CLM_MAX_OCCUPIED_STATIONS_PER_GROUP)
         {
            LOG_ERROR (
               CL_CCIEFB_LOG,
               "CLM_MASTER(%d): Wrong number of occupied slave stations for "
               "group index %u, slave index %u in the configuration. Given %u "
               "but min is 1 and max is %u. "
               "Possibly increase CLM_MAX_OCCUPIED_STATIONS_PER_GROUP.\n",
               __LINE__,
               group_index,
               slave_device_index,
               slave_device_setting->num_occupied_stations,
               CLM_MAX_OCCUPIED_STATIONS_PER_GROUP);
            return -1;
         }

         total_occupied_per_group += slave_device_setting->num_occupied_stations;
         total_occupied_for_all_groups +=
            slave_device_setting->num_occupied_stations;
      }

      if (total_occupied_per_group > CLM_MAX_OCCUPIED_STATIONS_PER_GROUP)
      {
         LOG_ERROR (
            CL_CCIEFB_LOG,
            "CLM_MASTER(%d): Too many occupied slave stations in group %u "
            "(group index %u). Given %u but max is %d. "
            "Possibly increase CLM_MAX_OCCUPIED_STATIONS_PER_GROUP.\n",
            __LINE__,
            group_index + 1U,
            group_index,
            total_occupied_per_group,
            CLM_MAX_OCCUPIED_STATIONS_PER_GROUP);
         return -1;
      }
   }

   if (total_occupied_for_all_groups > CL_CCIEFB_MAX_OCCUPIED_STATIONS_FOR_ALL_GROUPS)
   {
      LOG_ERROR (
         CL_CCIEFB_LOG,
         "CLM_MASTER(%d): Too many occupied slave stations in total. Given %u "
         "but max is %u.\n",
         __LINE__,
         total_occupied_for_all_groups,
         CL_CCIEFB_MAX_OCCUPIED_STATIONS_FOR_ALL_GROUPS);
      return -1;
   }

   return 0;
}

/**
 * Validate that the configuration is free from slave IP address duplicates.
 *
 * Except for IP address duplicates, the configuration must have been
 * validated before (no out of bound checks are done in this function).
 *
 * @param cfg                 c-link master stack configuration
 * @return 0 for valid configuration, -1 for invalid.
 */
int clm_validate_config_duplicates (const clm_cfg_t * cfg)
{
   /* The implementation is trivial and lean on memory, but slow for large
      configurations. Note that the complexity is O(n**2)

      Change implementation if this ends up being a problem. */

   uint16_t gi  = 0; /* Group index */
   uint16_t sdi = 0; /* Slave device index */
   const clm_group_setting_t * group_setting;
   const clm_slave_device_setting_t * slave_device_setting;
   uint16_t inner_gi  = 0;
   uint16_t inner_sdi = 0;
   const clm_group_setting_t * inner_group_setting;
   const clm_slave_device_setting_t * inner_slave_device_setting;

   for (gi = 0; gi < cfg->hier.number_of_groups; gi++)
   {
      group_setting = &cfg->hier.groups[gi];

      for (sdi = 0; sdi < group_setting->num_slave_devices; sdi++)
      {
         slave_device_setting = &group_setting->slave_devices[sdi];

         for (inner_gi = 0; inner_gi < cfg->hier.number_of_groups; inner_gi++)
         {
            inner_group_setting = &cfg->hier.groups[inner_gi];

            for (inner_sdi = 0;
                 inner_sdi < inner_group_setting->num_slave_devices;
                 inner_sdi++)
            {
               inner_slave_device_setting =
                  &inner_group_setting->slave_devices[inner_sdi];

               if (inner_gi == gi && inner_sdi == sdi)
               {
                  continue;
               }

               if (slave_device_setting->slave_id == inner_slave_device_setting->slave_id)
               {
                  LOG_ERROR (
                     CL_CCIEFB_LOG,
                     "CLM_MASTER(%d): Slave IP address duplicates found in the "
                     "config. The value for (group index, slave device index) "
                     "(%u, %u) is same as for (%u, %u).\n",
                     __LINE__,
                     gi,
                     sdi,
                     inner_gi,
                     inner_sdi);
                  return -1;
               }
            }
         }
      }
   }

   return 0;
}

/**
 * Print out current cyclic data, for debugging.
 *
 * There is no validity checking of the numerical arguments.
 *
 * @param clm                    Master instance
 * @param indent_size            Number of characters indentation
 * @param num_occupied_stations  Number of occupied stations for the slave
 * @param group_index            Group index
 * @param slave_device_index     Slave device index
 */
void clm_master_cyclic_data_show (
   clm_t * clm,
   int indent_size,
   uint16_t num_occupied_stations,
   uint16_t group_index,
   uint16_t slave_device_index)
{
#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   uint16_t i = 0;
   uint16_t j = 0;

   if (clm == NULL)
   {
      return;
   }

   for (i = 0; i < CL_WORDSIGNALS_PER_AREA * num_occupied_stations;
        i += CLM_MASTER_WORDS_PER_LINE)
   {
      printf ("%*sRWW%3u: ", indent_size, "", i);
      for (j = 0; j < CLM_MASTER_WORDS_PER_LINE; j++)
      {
         printf (
            "0x%04X ",
            clm_get_rww_value (clm, group_index, slave_device_index, i + j));
      }
      printf ("\n");
   }

   for (i = 0; i < CL_WORDSIGNALS_PER_AREA * num_occupied_stations;
        i += CLM_MASTER_WORDS_PER_LINE)
   {
      printf ("%*sRWR%3u: ", indent_size, "", i);
      for (j = 0; j < CLM_MASTER_WORDS_PER_LINE; j++)
      {
         printf (
            "0x%04X ",
            clm_get_rwr_value (clm, group_index, slave_device_index, i + j));
      }
      printf ("\n");
   }

   for (i = 0; i < (uint16_t)(CL_BITSIGNALS_PER_AREA * num_occupied_stations);
        i += CLM_MASTER_BITS_PER_LINE)
   {
      printf ("%*sRY%4u: ", indent_size, "", i);
      for (j = 0; j < CLM_MASTER_BITS_PER_LINE; j++)
      {
         if (j % 8 == 0)
         {
            printf (" ");
         }
         printf (
            "%d ",
            clm_get_ry_bit (clm, group_index, slave_device_index, i + j));
      }
      printf ("\n");
   }

   for (i = 0; i < (uint16_t)(CL_BITSIGNALS_PER_AREA * num_occupied_stations);
        i += CLM_MASTER_BITS_PER_LINE)
   {
      printf ("%*sRX%4u: ", indent_size, "", i);
      for (j = 0; j < CLM_MASTER_BITS_PER_LINE; j++)
      {
         if (j % 8 == 0)
         {
            printf (" ");
         }
         printf (
            "%d ",
            clm_get_rx_bit (clm, group_index, slave_device_index, i + j));
      }
      printf ("\n");
   }
#endif
}

/**
 * Print out current internal state values, for debugging.
 *
 * @param clm  Master instance
 */
void clm_master_internals_show (clm_t * clm)
{
#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   uint16_t group_index        = 0;
   uint16_t slave_device_index = 0;
   const clm_group_setting_t * group_setting;
   const clm_group_data_t * group_data;
   const clm_slave_device_setting_t * slave_device_setting;
   const clm_slave_device_data_t * slave_device_data;
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */

   if (clm == NULL)
   {
      return;
   }

   printf ("DEBUG: CLM_MASTER(%d): Master internal state:\n", __LINE__);
   cl_util_ip_to_string (clm->config.master_id, ip_string);
   printf ("  IP address in config: %s\n", ip_string);
   printf ("  Ethernet interface name: %s\n", clm->ifname);
   printf ("  Ethernet ifindex: %d\n", clm->ifindex);
   printf (
      "  MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
      clm->mac_address[0],
      clm->mac_address[1],
      clm->mac_address[2],
      clm->mac_address[3],
      clm->mac_address[4],
      clm->mac_address[5]);

   cl_util_ip_to_string (clm->master_netmask, ip_string);
   printf ("  Netmask: %s\n", ip_string);
   cl_util_ip_to_string (clm->slmp_broadcast_ip, ip_string);
   printf ("  SLMP broadcast address: %s\n", ip_string);
   cl_util_ip_to_string (clm->iefb_broadcast_ip, ip_string);
   printf ("  CCIEFB broadcast address: %s\n", ip_string);
   cl_util_ip_to_string (clm->latest_conflicting_master_ip, ip_string);
   printf ("  Latest conflicting master: %s\n", ip_string);
   printf (
      "  Master (simplified) state: %s (%d)\n",
      cl_literals_get_master_state (clm->master_state),
      clm->master_state);
   printf ("  Parameter number: %u\n", clm->parameter_no);
   printf ("  Master local unit info: 0x%04X\n", clm->master_local_unit_info);
   printf ("  SLMP request serial: %u\n", clm->slmp_request_serial);
   printf ("  Set IP serial: %d\n", clm->set_ip_request_serial);
   printf ("  Node search serial: %d\n", clm->node_search_serial);
   printf ("  Node search responses received: %u\n", clm->node_search_db.count);
   printf ("  Node search responses stored: %u\n", clm->node_search_db.stored);
   printf ("  Number of groups: %u\n", clm->config.hier.number_of_groups);
   for (group_index = 0; group_index < clm->config.hier.number_of_groups;
        group_index++)
   {
      group_data    = &clm->groups[group_index];
      group_setting = &clm->config.hier.groups[group_index];
      printf (
         "    Group %u (group index %u):\n",
         group_data->group_index + 1U,
         group_data->group_index);
      printf (
         "      Group state: %s (%d)\n",
         cl_literals_get_group_state (group_data->group_state),
         group_data->group_state);
      printf (
         "      Cyclic transmission state: 0x%04X\n",
         group_data->cyclic_transmission_state);
      printf ("      Frame sequence number: %u\n", group_data->frame_sequence_no);
      printf (
         "      Number of slave devices (in config): %u\n",
         group_setting->num_slave_devices);
      printf (
         "      Occupied slave stations in this group: %u\n",
         group_data->total_occupied);
      printf (
         "      Occupied slave stations in this group (in config): %u\n",
         clm_iefb_calc_occupied_per_group (group_setting));

      for (slave_device_index = 0;
           slave_device_index < group_setting->num_slave_devices;
           slave_device_index++)
      {
         slave_device_setting = &group_setting->slave_devices[slave_device_index];
         slave_device_data = &group_data->slave_devices[slave_device_index];
         cl_util_ip_to_string (slave_device_setting->slave_id, ip_string);
         printf (
            "        Slave device index: %u  Slave station number: %u\n",
            slave_device_index,
            slave_device_data->slave_station_no);
         printf ("          IP address: %s\n", ip_string);
         printf (
            "          Num occupied (in config): %u\n",
            slave_device_setting->num_occupied_stations);
         printf (
            "          Enabled: %s\n",
            slave_device_data->enabled ? "Yes" : "No");
         printf (
            "          Transmission bit: %s\n",
            slave_device_data->transmission_bit ? "On" : "Off");
         printf (
            "          Force transmission bit: %s\n",
            slave_device_data->force_transmission_bit ? "Yes" : "No");
         printf ("          Timeout count: %u\n", slave_device_data->timeout_count);
         printf (
            "          Slave device state: %s (%d)\n",
            cl_literals_get_device_state (slave_device_data->device_state),
            slave_device_data->device_state);
         printf (
            "          Number of connects: %" PRIu32 "\n",
            slave_device_data->statistics.number_of_connects);
         printf (
            "          Number of disconnects: %" PRIu32 "\n",
            slave_device_data->statistics.number_of_disconnects);
         printf (
            "          Number of timeouts: %" PRIu32 "\n",
            slave_device_data->statistics.number_of_timeouts);
         printf (
            "          Number of sent frames: %" PRIu32 "\n",
            slave_device_data->statistics.number_of_sent_frames);
         printf (
            "          Number of received frames: %" PRIu32 "\n",
            slave_device_data->statistics.number_of_incoming_frames);
         printf (
            "          Number of received alarm frames: %" PRIu32 "\n",
            slave_device_data->statistics.number_of_incoming_alarm_frames);
         printf (
            "          Number of received invalid frames: %" PRIu32 "\n",
            slave_device_data->statistics.number_of_incoming_invalid_frames);
         printf (
            "          Number of time statistics samples: %" PRIu32 "\n",
            slave_device_data->statistics.measured_time.number_of_samples);
         printf (
            "          Average response time: %" PRIu32 " microseconds\n",
            slave_device_data->statistics.measured_time.average);
         clm_master_cyclic_data_show (
            clm,
            10,
            slave_device_setting->num_occupied_stations,
            group_index,
            slave_device_index);
      }
      printf (
         "      Transmission buffer size: %u\n",
         (unsigned)group_data->req_frame.buf_size);
      printf (
         "      UDP transmission payload length: %u\n",
         (unsigned)group_data->req_frame.udp_payload_len);
      cl_util_buffer_show (
         group_data->sendbuf,
         (int)group_data->req_frame.udp_payload_len,
         8);
   }
#endif
}

/**
 * Print out configuration details, for debugging.
 *
 * @param cfg              Master stack configuration
 */
void clm_master_config_show (const clm_cfg_t * cfg)
{
   /* Use LOG_DEBUG() instead of printf(), for the output to
      be directed to the correct logfile */

#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   uint16_t group_index        = 0;
   uint16_t slave_device_index = 0;
   const clm_group_setting_t * group_setting;
   const clm_slave_device_setting_t * slave_device_setting;
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */

   if (clm_validate_config (cfg) != 0)
   {
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "CLM_MASTER(%d): The configuration is invalid\n",
         __LINE__);
      return;
   }

   LOG_DEBUG (CL_CCIEFB_LOG, "CLM_MASTER(%d): Valid configuration:\n", __LINE__);
   cl_util_ip_to_string (cfg->master_id, ip_string);
   LOG_DEBUG (CL_CCIEFB_LOG, "  Master IP address: %s\n", ip_string);
   LOG_DEBUG (CL_CCIEFB_LOG, "  Arbitration time: %u ms\n", cfg->arbitration_time);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "  Delay time node search callback: %u ms\n",
      cfg->callback_time_node_search);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "  Response timeout for set IP command: %u ms\n",
      cfg->callback_time_set_ip);
   LOG_DEBUG (CL_CCIEFB_LOG, "  Protocol version: %u\n", cfg->protocol_ver);
   LOG_DEBUG (CL_CCIEFB_LOG, "  File directory: %s\n", cfg->file_directory);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "  Max statistics samples: %u\n",
      cfg->max_statistics_samples);
   LOG_DEBUG (CL_CCIEFB_LOG, "  Number of groups: %u\n", cfg->hier.number_of_groups);

   for (group_index = 0; group_index < cfg->hier.number_of_groups; group_index++)
   {
      group_setting = &cfg->hier.groups[group_index];
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "    Group %u (group index %u):\n",
         group_index + 1U,
         group_index);
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "      Timeout: %u ms\n",
         group_setting->timeout_value);
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "      Timeout count: %u\n",
         group_setting->parallel_off_timeout_count);
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "      Constant link scan time: %s\n",
         group_setting->use_constant_link_scan_time ? "Yes" : "No");
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "      Number of slave devices: %u\n",
         group_setting->num_slave_devices);
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "      Occupied slave stations in this group: %u\n",
         clm_iefb_calc_occupied_per_group (group_setting));

      for (slave_device_index = 0;
           slave_device_index < group_setting->num_slave_devices;
           slave_device_index++)
      {
         slave_device_setting = &group_setting->slave_devices[slave_device_index];
         cl_util_ip_to_string (slave_device_setting->slave_id, ip_string);
         LOG_DEBUG (
            CL_CCIEFB_LOG,
            "        Slave device index: %u  IP: %s  Num occupied: %u\n",
            slave_device_index,
            ip_string,
            slave_device_setting->num_occupied_stations);
      }
   }
   LOG_DEBUG (CL_CCIEFB_LOG, "  CLM_MAX_GROUPS: %u\n", CLM_MAX_GROUPS);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "  CLM_MAX_OCCUPIED_STATIONS_PER_GROUP: %u\n",
      CLM_MAX_OCCUPIED_STATIONS_PER_GROUP);
   /* Do not use %zu as it's not implemented on all platforms */
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "    Resulting sizeof(clm_cfg_t) %" PRIu32 " bytes\n",
      (uint32_t)sizeof (clm_cfg_t));
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "    Resulting sizeof(clm_t) %" PRIu32 " bytes\n",
      (uint32_t)sizeof (clm_t));
#endif
}

/**
 * Initialise master Ethernet interface settings
 *
 * @param clm              c-link master stack instance handle
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CLM_UDP_01
 */
int clm_master_eth_init (clm_t * clm)
{

   if (
      cl_eth_get_network_settings (
         clm->config.master_id,
         &clm->ifindex,
         &clm->master_netmask,
         &clm->mac_address,
         clm->ifname) != 0)
   {
      return -1;
   }

   clm->iefb_broadcast_ip =
      cl_util_calc_broadcast_address (clm->master_netmask, clm->config.master_id);
   clm->slmp_broadcast_ip = clm->config.use_slmp_directed_broadcast
                               ? clm->iefb_broadcast_ip
                               : CL_IPADDR_LOCAL_BROADCAST;

   return 0;
}

/**
 * Exit master Ethernet interface settings
 *
 * @param clm              c-link master stack instance handle
 */
void clm_master_eth_exit (clm_t * clm)
{
   clm->iefb_broadcast_ip = CL_IPADDR_INVALID;
   clm->master_netmask    = CL_IPADDR_INVALID;
   clal_clear_memory (clm->mac_address, sizeof (clm->mac_address));
   clm->ifname[0] = '\0';
   clm->ifindex   = 0;
}

int clm_master_init (clm_t * clm, const clm_cfg_t * cfg, uint32_t now)
{
   if (clm == NULL)
   {
      LOG_ERROR (CL_CCIEFB_LOG, "CLM_MASTER(%d): clm is NULL.\n", __LINE__);
      return -1;
   }

   LOG_DEBUG (CL_CCIEFB_LOG, "CLM_MASTER(%d): Initialising\n", __LINE__);

   clal_clear_memory (clm, sizeof (*clm));

   if (clal_init() != 0)
   {
      LOG_ERROR (
         CL_CCIEFB_LOG,
         "CLM_MASTER(%d): Failed to initialise CLAL.\n",
         __LINE__);
      return -1;
   }

   if (clm_validate_config (cfg) != 0)
   {
      return -1;
   }

#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   clm_master_config_show (cfg);
#endif

   /* Copy the config */
   clm->config = *cfg;

   /* Read from file (nvm) */
   if (
      cl_file_load (
         clm->config.file_directory,
         CLM_FILENAME_PARAM_NO,
         &clm->parameter_no,
         sizeof (clm->parameter_no)) == 0)
   {
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "CLM_MASTER(%d): Did read parameter_no %u from nvm\n",
         __LINE__,
         clm->parameter_no);
   }
   else
   {
      LOG_INFO (
         CL_CCIEFB_LOG,
         "CLM_MASTER(%d): Could not read parameter_no from nvm."
         " Use default value %u.\n",
         __LINE__,
         CLM_DEFAULT_PARAMETER_NO);
      clm->parameter_no = CLM_DEFAULT_PARAMETER_NO;
   }

   if (clm_master_eth_init (clm) != 0)
   {
      return -1;
   }

   if (clm_validate_config_duplicates (cfg) != 0)
   {
      return -1;
   }

   if (clm_iefb_init (clm, now) != 0)
   {
      return -1;
   }

   if (clm_slmp_init (clm) != 0)
   {
      return -1;
   }

   return 0;
}

int clm_master_exit (clm_t * clm)
{
   int ret = 0; /* Assume success */

   LOG_DEBUG (CL_CCIEFB_LOG, "CLM_MASTER(%d): Exiting\n", __LINE__);

   clm_slmp_exit (clm);
   clm_iefb_exit (clm);
   clm_master_eth_exit (clm);

   if (clal_exit() != 0)
   {
      ret = -1;
   }

   return ret;
}
