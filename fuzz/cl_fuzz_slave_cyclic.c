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

#include <stdio.h>

extern int cls_iefb_handle_input_frame (
   cls_t * cls,
   uint32_t now,
   uint8_t * buffer,
   size_t recv_len,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port,
   cl_ipaddr_t slave_ip_addr);

static cls_cfg_t cfg = {
   /* These values should match the ones given in seed files */
   .num_occupied_stations = 1,

   /* Values that do not appear in CCIEFB cyclic data request payload */
   .vendor_code        = 0x1234,
   .model_code         = 0x87654321,
   .equipment_ver      = 0x0007,
   .ip_setting_allowed = false,
   .state_cb           = NULL,
   .connect_cb         = NULL,
   .disconnect_cb      = NULL,
   .master_running_cb  = NULL,
   .node_search_cb     = NULL,
   .set_ip_cb          = NULL,
   .cb_arg             = NULL,
};

static cls_t cls;

static void cl_fuzz_init (void)
{
   cls.config = cfg;
   CC_ASSERT (cls_iefb_init (&cls, 0) == 0);
}

int LLVMFuzzerTestOneInput (const uint8_t * data, size_t size)
{
   static bool init_done = false;
   cls_slave_state_t previous_state;

   /* This value should match the one given in CCIEFB seed files */
   const cl_ipaddr_t local_ip = 0xC0A800C9; /* 192.168.0.201 */

   if (!init_done)
   {
      cl_fuzz_init();
      init_done = true;
   }
   cls.state = CLS_SLAVE_STATE_MASTER_CONTROL;

   /* These values should match the ones given in CCIEFB seed files
      The UDP payload size for a request with one occupied slave is 143 bytes */
   cls.master.group_no         = 1;
   cls.master.slave_station_no = 1;
   cls.master.master_id        = 0xC0A800FA; /* 192.168.0.250 */

   previous_state = cls.state;
   cls_iefb_handle_input_frame (
      &cls,
      0,
      (uint8_t *)data,
      size,
      cls.master.master_id, /* Remote IP */
      CL_CCIEFB_PORT,
      local_ip);

   return 0;
}
