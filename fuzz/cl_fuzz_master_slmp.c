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

#include "clm_iefb.h"
#include "clm_master.h"
#include "clm_slmp.h"

#include <stdio.h>
#include <string.h>

extern int clm_slmp_handle_input_frame (
   clm_t * clm,
   uint32_t now,
   const uint8_t * buffer,
   size_t recv_len,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port,
   int ifindex);

static clm_cfg_t cfg;
static clm_t clm;

static void cl_fuzz_init (void)
{
   clal_clear_memory (&clm, sizeof (clm));
   clal_clear_memory (&cfg, sizeof (cfg));

   /* Values that do not appear in SLMP response payload */
   cfg.protocol_ver                                          = 2;
   cfg.hier.number_of_groups                                 = 1;
   cfg.hier.groups[0].parallel_off_timeout_count             = 3;
   cfg.hier.groups[0].timeout_value                          = 500;
   cfg.hier.groups[0].num_slave_devices                      = 1;
   cfg.hier.groups[0].slave_devices[0].num_occupied_stations = 1;

   /* This value appear in node search repsonse, but is not checked */
   /* 192.168.0.201 */
   cfg.hier.groups[0].slave_devices[0].slave_id = 0xC0A800C9;

   /* These values should match the ones given in SLMP seed files */
   cfg.master_id      = 0xC0A800FA; /* 192.168.0.250 */
   clm.mac_address[0] = 0x1C;
   clm.mac_address[1] = 0x39;
   clm.mac_address[2] = 0x47;
   clm.mac_address[3] = 0xCD;
   clm.mac_address[4] = 0xD4;
   clm.mac_address[5] = 0xEB;

   CC_ASSERT (clm_validate_config (&cfg) == 0);
   CC_ASSERT (clm_validate_config_duplicates (&cfg) == 0);

   clm.config = cfg;

   /* Values that do not appear in SLMP response payload */
   clm.slmp_broadcast_ip = 0xC0A800FF; /* 192.168.0.255 */
   clm.master_netmask    = 0xFFFFFF00; /* 255.255.255.0 */

   CC_ASSERT (clm_iefb_init (&clm, 0) == 0);
   CC_ASSERT (clm_slmp_init (&clm) == 0);

   /* Force new state */
   clm.parameter_no                        = 100;
   clm.master_state                        = CLM_MASTER_STATE_RUNNING;
   clm.ifindex                             = 1;
   clm.groups[0].group_index               = 0;
   clm.groups[0].group_state               = CLM_GROUP_STATE_MASTER_LINK_SCAN;
   clm.groups[0].total_occupied            = 1;
   clm.groups[0].frame_sequence_no         = 25;
   clm.groups[0].cyclic_transmission_state = 0x0001;
   clm.groups[0].slave_devices[0].device_index = 0;
   clm.groups[0].slave_devices[0].enabled      = true;
   clm.groups[0].slave_devices[0].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   clm.groups[0].slave_devices[0].slave_station_no = 1;
}

int LLVMFuzzerTestOneInput (const uint8_t * data, size_t size)
{
   static bool init_done = false;

   if (!init_done)
   {
      cl_fuzz_init();
      init_done = true;
   }

   /* Force new state */
   /* These values should match the ones given in SLMP seed files */
   clm.node_search_serial    = 1;
   clm.set_ip_request_serial = 2;

   /* Start timer for node search callback, use 1.5 simulated seconds */
   cl_timer_start (&clm.node_search_timer, 1500000, 0);

   clm_slmp_handle_input_frame (
      &clm,
      0,
      (uint8_t *)data,
      size,
      cfg.hier.groups[0].slave_devices[0].slave_id,
      CL_SLMP_PORT,
      1);

   /* Wait 10 simulated seconds and see if we should trigger a callback */
   clm_slmp_check_timeouts (&clm, 10000000);

   return 0;
}
