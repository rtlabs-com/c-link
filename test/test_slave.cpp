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

#include "slave/cls_slave.h"

#include "cl_options.h"

#include "mocks.h"
#include "utils_for_testing.h"

#include <gtest/gtest.h>

// Test fixture
class InitExitUnitTest : public UnitTest
{
 protected:
   cls_t cls = {};
};

/**
 * Validate slave configuration
 *
 * @req REQ_CL_PROTOCOL_36
 *
 */
TEST_F (InitExitUnitTest, ValidateSlaveConfiguration)
{
   cls_cfg_t config = {
      .vendor_code                 = 0x1234,
      .model_code                  = 0x87654321,
      .equipment_ver               = 0x5678,
      .num_occupied_stations       = 1,
      .ip_setting_allowed          = true,
      .cb_arg                      = nullptr,
      .state_cb                    = nullptr,
      .error_cb                    = nullptr,
      .connect_cb                  = nullptr,
      .disconnect_cb               = nullptr,
      .master_running_cb           = nullptr,
      .node_search_cb              = nullptr,
      .set_ip_cb                   = nullptr,
      .iefb_ip_addr                = CL_IPADDR_ANY,
      .use_slmp_directed_broadcast = false};

   EXPECT_EQ (cls_validate_config (&config), 0);

   EXPECT_EQ (cls_validate_config (nullptr), -1);

   /* Number of occupied stations */
   config.num_occupied_stations = CLS_MAX_OCCUPIED_STATIONS;
   EXPECT_EQ (cls_validate_config (&config), 0);
   config.num_occupied_stations = CLS_MAX_OCCUPIED_STATIONS + 1;
   EXPECT_EQ (cls_validate_config (&config), -1);
   config.num_occupied_stations = 65;
   EXPECT_EQ (cls_validate_config (&config), -1);
   config.num_occupied_stations = 0;
   EXPECT_EQ (cls_validate_config (&config), -1);
   config.num_occupied_stations = 1;
   EXPECT_EQ (cls_validate_config (&config), 0);

   /* IP address*/
   config.iefb_ip_addr = 0xFFFFFFFF;
   EXPECT_EQ (cls_validate_config (&config), -1);
   config.iefb_ip_addr = 0xFFFFFFFE;
   EXPECT_EQ (cls_validate_config (&config), -1);
   config.iefb_ip_addr = CL_IPADDR_ANY;
   EXPECT_EQ (cls_validate_config (&config), 0);
   config.iefb_ip_addr = 0x00000001;
   EXPECT_EQ (cls_validate_config (&config), 0);
   config.iefb_ip_addr = 0x01020304;
   EXPECT_EQ (cls_validate_config (&config), 0);
}

TEST_F (InitExitUnitTest, SlaveConfigShow)
{
   const cls_cfg_t config = {
      .vendor_code                 = 0x1234,
      .model_code                  = 0x87654321,
      .equipment_ver               = 0x5678,
      .num_occupied_stations       = 1,
      .ip_setting_allowed          = true,
      .cb_arg                      = nullptr,
      .state_cb                    = nullptr,
      .error_cb                    = nullptr,
      .connect_cb                  = nullptr,
      .disconnect_cb               = nullptr,
      .master_running_cb           = nullptr,
      .node_search_cb              = nullptr,
      .set_ip_cb                   = nullptr,
      .iefb_ip_addr                = CL_IPADDR_ANY,
      .use_slmp_directed_broadcast = false};

   cls_slave_config_show (&config);

   /* Should not crash */
   cls_slave_config_show (nullptr);
}

TEST_F (InitExitUnitTest, SlaveInternalsShow)
{
   const cls_cfg_t config = {
      .vendor_code                 = 0x1234,
      .model_code                  = 0x87654321,
      .equipment_ver               = 0x5678,
      .num_occupied_stations       = 1,
      .ip_setting_allowed          = true,
      .cb_arg                      = nullptr,
      .state_cb                    = nullptr,
      .error_cb                    = nullptr,
      .connect_cb                  = nullptr,
      .disconnect_cb               = nullptr,
      .master_running_cb           = nullptr,
      .node_search_cb              = nullptr,
      .set_ip_cb                   = nullptr,
      .iefb_ip_addr                = CL_IPADDR_ANY,
      .use_slmp_directed_broadcast = false};

   ASSERT_EQ (cls_slave_init (&cls, &config, 0), 0);

   /* Should not crash */
   cls_slave_internals_show (&cls);
   cls.slave_application_status = CL_SLAVE_APPL_OPERATION_STATUS_STOPPED;
   cls_slave_internals_show (&cls);
   cls_slave_internals_show (nullptr);

   /* Should not crash */
   cls_slave_cyclic_data_show (&cls, 0);
   cls_slave_cyclic_data_show (nullptr, 0);
}

/**
 * Check initialisation of slave
 *
 * @req REQ_CLS_CONFIGURATION_01
 */
TEST_F (InitExitUnitTest, SlaveInitExit)
{
   cls_cfg_t config = {
      .vendor_code                 = 0x1234,
      .model_code                  = 0x87654321,
      .equipment_ver               = 0x5678,
      .num_occupied_stations       = 1,
      .ip_setting_allowed          = true,
      .cb_arg                      = nullptr,
      .state_cb                    = nullptr,
      .error_cb                    = nullptr,
      .connect_cb                  = nullptr,
      .disconnect_cb               = nullptr,
      .master_running_cb           = nullptr,
      .node_search_cb              = nullptr,
      .set_ip_cb                   = nullptr,
      .iefb_ip_addr                = CL_IPADDR_ANY,
      .use_slmp_directed_broadcast = true};
   const cls_cfg_t default_config = config;

   /* Valid configuration */
   EXPECT_EQ (cls_slave_init (&cls, &config, 0), 0);
   mock_clear();

   /* Invalid configuration */
   config.num_occupied_stations = 999;
   EXPECT_EQ (cls_slave_init (&cls, &config, 0), -1);
   config = default_config;
   mock_clear();

   /* Invalid arguments */
   EXPECT_EQ (cls_slave_init (nullptr, &config, 0), -1);
   EXPECT_EQ (cls_slave_init (&cls, nullptr, 0), -1);
   EXPECT_EQ (cls_slave_init (&cls, &config, 0), 0);
   mock_clear();

   /* CLAL fails to start */
   mock_data.clal_init_returnvalue = -1;
   EXPECT_EQ (cls_slave_init (&cls, &config, 0), -1);
   mock_data.clal_init_returnvalue = 0;
   EXPECT_EQ (cls_slave_init (&cls, &config, 0), 0);
   mock_clear();

   /* CLAL does not exit cleanly */
   mock_data.clal_exit_returnvalue = -1;
   EXPECT_EQ (cls_slave_exit (&cls), -1);
   mock_data.clal_exit_returnvalue = 0;
   EXPECT_EQ (cls_slave_exit (&cls), 0);

   /* Fail to open CCIEFB UDP port */
   mock_clear();
   EXPECT_EQ (cls_slave_init (&cls, &config, 0), 0);
   mock_clear();
   mock_data.udp_ports[0].will_fail_open = true;
   EXPECT_EQ (cls_slave_init (&cls, &config, 0), -1);
   mock_clear();
   EXPECT_EQ (cls_slave_init (&cls, &config, 0), 0);

   /* Fail to open SLMP UDP port */
   mock_clear();
   EXPECT_EQ (cls_slave_init (&cls, &config, 0), 0);
   mock_clear();
   mock_data.udp_ports[1].will_fail_open = true;
   EXPECT_EQ (cls_slave_init (&cls, &config, 0), -1);
   mock_clear();
   EXPECT_EQ (cls_slave_init (&cls, &config, 0), 0);

   /* Fail to read MAC address */
   mock_clear();
   mock_data.will_fail_read_mac_addr = true;
   EXPECT_EQ (cls_slave_init (&cls, &config, 0), 0);
   mock_clear();
}
