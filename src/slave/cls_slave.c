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
 * @brief Initialisation and validation at slave startup.
 *
 * Also handles slave shutdown.
 */

#ifdef UNIT_TEST
#define clal_init mock_clal_init
#define clal_exit mock_clal_exit
#endif

#include "slave/cls_slave.h"

#include "slave/cls_slmp.h"

#include "cl_options.h"
#include "common/cl_literals.h"
#include "common/cl_types.h"
#include "common/cl_util.h"
#include "slave/cls_iefb.h"

#include "osal_log.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#if CLS_MAX_OCCUPIED_STATIONS < CL_CCIEFB_MIN_OCCUPIED_STATIONS_PER_GROUP
#error "CLS_MAX_OCCUPIED_STATIONS is too small"
#endif

#if CLS_MAX_OCCUPIED_STATIONS > CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP
#error "CLS_MAX_OCCUPIED_STATIONS is too large"
#endif

#define CLS_SLAVE_BITS_PER_LINE  32U
#define CLS_SLAVE_WORDS_PER_LINE 8U

int cls_validate_config (const cls_cfg_t * cfg)
{
   if (cfg == NULL)
   {
      return -1;
   }

   /* IP address */
   if (cfg->iefb_ip_addr != CL_IPADDR_ANY && !cl_utils_is_ipaddr_range_valid (cfg->iefb_ip_addr))
   {
      LOG_ERROR (
         CL_CCIEFB_LOG,
         "CLS_SLAVE(%d): Invalid IP address in the configuration. Given "
         "0x%08" PRIX32 "\n",
         __LINE__,
         cfg->iefb_ip_addr);
      return -1;
   }

   /* Number of occupied stations */
   if (
      (cfg->num_occupied_stations < CL_CCIEFB_MIN_OCCUPIED_STATIONS_PER_GROUP) ||
      (cfg->num_occupied_stations > CLS_MAX_OCCUPIED_STATIONS))
   {
      LOG_ERROR (
         CL_CCIEFB_LOG,
         "CLS_SLAVE(%d): Wrong number of occupied stations in the "
         "configuration. Given %" PRIu16
         " but min is %u and max is %u. Increase CLS_MAX_OCCUPIED_STATIONS.\n",
         __LINE__,
         cfg->num_occupied_stations,
         CL_CCIEFB_MIN_OCCUPIED_STATIONS_PER_GROUP,
         CLS_MAX_OCCUPIED_STATIONS);
      return -1;
   }

   return 0;
}

/**
 * Print out configuration details, for debugging.
 *
 * @param cfg              Slave stack configuration
 */
void cls_slave_config_show (const cls_cfg_t * cfg)
{
   /* Use LOG_DEBUG() instead of printf(), for the output to
      be directed to the correct logfile */

#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */

   if (cls_validate_config (cfg) != 0)
   {
      LOG_DEBUG (
         CL_CCIEFB_LOG,
         "CLS_SLAVE(%d): The configuration is invalid\n",
         __LINE__);
      return;
   }

   LOG_DEBUG (CL_CCIEFB_LOG, "CLS_SLAVE(%d): Valid configuration:\n", __LINE__);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "  Number of occupied stations: %u\n",
      cfg->num_occupied_stations);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "  Master allowed to change my IP address: %s\n",
      cfg->ip_setting_allowed ? "Yes" : "No");

   cl_util_ip_to_string (cfg->iefb_ip_addr, ip_string);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "  Bind IP address for cyclic data socket: %s\n",
      ip_string);
   LOG_DEBUG (CL_CCIEFB_LOG, "  Vendor code: 0x%04" PRIx16 "\n", cfg->vendor_code);
   LOG_DEBUG (CL_CCIEFB_LOG, "  Model code: 0x%08" PRIx32 "\n", cfg->model_code);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "  Equipment version: 0x%04" PRIx16 "\n",
      cfg->equipment_ver);
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "  CLS_MAX_OCCUPIED_STATIONS: %u\n",
      CLS_MAX_OCCUPIED_STATIONS);
   /* Do not use %zu as it's not implemented on all platforms */
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "    Resulting sizeof(cls_cfg_t) %" PRIu32 "\n",
      (uint32_t)sizeof (cls_cfg_t));
   LOG_DEBUG (
      CL_CCIEFB_LOG,
      "    Resulting sizeof(cls_t) %" PRIu32 "\n",
      (uint32_t)sizeof (cls_t));
#endif
}

/**
 * Print out current cyclic data, for debugging.
 *
 * @param cls           Slave instance
 * @param indent_size   Number of characters indentation
 */
void cls_slave_cyclic_data_show (cls_t * cls, int indent_size)
{
#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   uint16_t num_occupied;
   uint16_t i = 0;
   uint16_t j = 0;

   if (cls == NULL)
   {
      return;
   }

   num_occupied = cls->config.num_occupied_stations;

   for (i = 0; i < CL_WORDSIGNALS_PER_AREA * num_occupied;
        i += CLS_SLAVE_WORDS_PER_LINE)
   {
      printf ("%*sRWW%3u: ", indent_size, "", i);
      for (j = 0; j < CLS_SLAVE_WORDS_PER_LINE; j++)
      {
         printf ("0x%04X ", cls_get_rww_value (cls, i + j));
      }
      printf ("\n");
   }

   for (i = 0; i < CL_WORDSIGNALS_PER_AREA * num_occupied;
        i += CLS_SLAVE_WORDS_PER_LINE)
   {
      printf ("%*sRWR%3u: ", indent_size, "", i);
      for (j = 0; j < CLS_SLAVE_WORDS_PER_LINE; j++)
      {
         printf ("0x%04X ", cls_get_rwr_value (cls, i + j));
      }
      printf ("\n");
   }

   for (i = 0; i < (uint16_t)(CL_BITSIGNALS_PER_AREA * num_occupied);
        i += CLS_SLAVE_BITS_PER_LINE)
   {
      printf ("%*sRY%4u: ", indent_size, "", i);
      for (j = 0; j < CLS_SLAVE_BITS_PER_LINE; j++)
      {
         if (j % 8 == 0)
         {
            printf (" ");
         }
         printf ("%d ", cls_get_ry_bit (cls, i + j));
      }
      printf ("\n");
   }

   for (i = 0; i < (uint16_t)(CL_BITSIGNALS_PER_AREA * num_occupied);
        i += CLS_SLAVE_BITS_PER_LINE)
   {
      printf ("%*sRX%4u: ", indent_size, "", i);
      for (j = 0; j < CLS_SLAVE_BITS_PER_LINE; j++)
      {
         if (j % 8 == 0)
         {
            printf (" ");
         }
         printf ("%d ", cls_get_rx_bit (cls, i + j));
      }
      printf ("\n");
   }
#endif
}

/**
 * Print out current internal state values, for debugging.
 *
 * @param cls  Slave instance
 */
void cls_slave_internals_show (cls_t * cls)
{
#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */

   if (cls == NULL)
   {
      return;
   }

   printf ("DEBUG: CLS_SLAVE(%d): Slave internal state:\n", __LINE__);
   printf (
      "  Number of occupied slave stations (in config): %u\n",
      cls->config.num_occupied_stations);
   cl_util_ip_to_string (cls->config.iefb_ip_addr, ip_string);
   printf ("  IP address for CCIEFB socket (in config): %s\n", ip_string);
   printf (
      "  Slave application status: %s\n",
      (cls->slave_application_status == 0) ? "Stopped" : "Running");
   printf ("  Slave state: %s\n", cl_literals_get_slave_state (cls->state));
   cl_util_ip_to_string (cls->master.master_id, ip_string);
   printf ("  Connected to master IP address (if any): %s\n", ip_string);
   printf ("  Master protocol version: %u\n", cls->master.protocol_ver);
   printf ("  Slave station number: %u\n", cls->master.slave_station_no);
   printf ("  Group number: %u\n", cls->master.group_no);
   printf (
      "  Total occupied slave stations in group: %u\n",
      cls->master.total_occupied_station_count);
   printf ("  Parameter number: %u\n", cls->master.parameter_no);
   printf (
      "  Timeout value setting from master: %u ms\n",
      cls->master.timeout_value);
   printf (
      "  Timeout count setting from master: %u\n",
      cls->master.parallel_off_timeout_count);
   printf (
      "  Endcode to use when slave is disabled: 0x%04X\n",
      (uint16_t)cls->endcode_slave_disabled);
   printf ("  Local management info: %" PRIu32 "\n", cls->local_management_info);
   printf ("  Slave error code: %u\n", cls->slave_err_code);
   cls_slave_cyclic_data_show (cls, 2);
   printf (
      "  Normal transmission buffer size: %u\n",
      (unsigned)cls->cciefb_resp_frame_normal.buf_size);
   printf (
      "  Normal UDP transmission payload length: %u\n",
      (unsigned)cls->cciefb_resp_frame_normal.udp_payload_len);
   cl_util_buffer_show (
      cls->cciefb_sendbuf_normal,
      (int)cls->cciefb_resp_frame_normal.udp_payload_len,
      4);
   printf (
      "  Error transmission buffer size: %u\n",
      (unsigned)cls->cciefb_resp_frame_error.buf_size);
   printf (
      "  Error UDP transmission payload length: %u\n",
      (unsigned)cls->cciefb_resp_frame_error.udp_payload_len);
   cl_util_buffer_show (
      cls->cciefb_sendbuf_error,
      (int)cls->cciefb_resp_frame_error.udp_payload_len,
      4);
#endif
}

int cls_slave_init (cls_t * cls, const cls_cfg_t * cfg, uint32_t now)
{
   if (cls == NULL)
   {
      LOG_ERROR (CL_CCIEFB_LOG, "CLS_SLAVE(%d): cls is NULL.\n", __LINE__);
      return -1;
   }

   LOG_DEBUG (CL_CCIEFB_LOG, "CLS_SLAVE(%d): Initialising\n", __LINE__);

   clal_clear_memory (cls, sizeof (*cls));

   if (clal_init() != 0)
   {
      LOG_ERROR (
         CL_CCIEFB_LOG,
         "CLS_SLAVE(%d): Failed to initialise CLAL.\n",
         __LINE__);
      return -1;
   }

   if (cls_validate_config (cfg) != 0)
   {
      return -1;
   }

#if LOG_DEBUG_ENABLED(CL_CCIEFB_LOG)
   cls_slave_config_show (cfg);
#endif

   /* Copy the config */
   cls->config = *cfg;

   if (cls_iefb_init (cls, now) != 0)
   {
      return -1;
   }

   if (cls_slmp_init (cls) != 0)
   {
      return -1;
   }

   return 0;
}

int cls_slave_exit (cls_t * cls)
{
   int ret = 0; /* Assume success */

   cls_slmp_exit (cls);
   cls_iefb_exit (cls);

   if (clal_exit() != 0)
   {
      ret = -1;
   }

   return ret;
}
