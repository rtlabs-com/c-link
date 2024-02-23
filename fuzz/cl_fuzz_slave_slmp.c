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

#include "cls_iefb.h"
#include "cls_slmp.h"

#include <stdio.h>

extern int cls_slmp_handle_input_frame (
   cls_t * cls,
   uint32_t now,
   const uint8_t * buffer,
   size_t recv_len,
   const cls_addr_info_t * addr_info);

static cls_cfg_t cfg = {
   /* Values that do not appear in SLMP request payload */
   .num_occupied_stations = 1,
   .vendor_code           = 0x1234,
   .model_code            = 0x87654321,
   .equipment_ver         = 0x0007,
   .ip_setting_allowed    = false,
   .state_cb              = NULL,
   .connect_cb            = NULL,
   .disconnect_cb         = NULL,
   .master_running_cb     = NULL,
   .node_search_cb        = NULL,
   .set_ip_cb             = NULL,
   .cb_arg                = NULL,
};

static cls_t cls;

static void cl_fuzz_init (void)
{
   cls.config = cfg;
   CC_ASSERT (cls_iefb_init (&cls, 0) == 0);
   CC_ASSERT (cls_slmp_init (&cls) == 0);
   cls.state = CLS_SLAVE_STATE_MASTER_CONTROL;

   /* This value does not have to match the one given in SLMP seed files */
   cls.master.master_id = 0xC0A800FA; /* 192.168.0.250 */

   /* Values that do not appear in SLMP request payload */
   cls.master.slave_station_no = 1;
   cls.master.group_no         = 1;
}

int LLVMFuzzerTestOneInput (const uint8_t * data, size_t size)
{
   static bool init_done = false;
   uint32_t ip;

   cls_addr_info_t addr_info = {
      .remote_ip   = CL_IPADDR_INVALID,
      .remote_port = CL_SLMP_PORT,

      /* Values that do not appear in SLMP request payload */
      .local_ip      = 0xC0A800C9,
      .local_netmask = 0xFFFFFF00,

      /* This value should match the one given in Set IP seed files */
      .local_mac_address = {0x28, 0xE9, 0x8E, 0x2F, 0xE4, 0xB7},
      .ifindex           = 1};

   if (!init_done)
   {
      cl_fuzz_init();
      init_done = true;
   }
   addr_info.remote_ip = cls.master.master_id;

   cls_slmp_handle_input_frame (&cls, 0, (uint8_t *)data, size, &addr_info);

   /* Wait 10 simulated seconds to send a node search response */
   if (cl_timer_is_expired (&cls.node_search.response_timer, 10000000))
   {
      cl_timer_stop (&cls.node_search.response_timer);
      (void)cls_slmp_send_node_search_response (&cls);
   }

   return 0;
}
