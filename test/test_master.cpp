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

#include "master/clm_master.h"

#include "cl_options.h"

#include "mocks.h"
#include "utils_for_testing.h"

#include <gtest/gtest.h>

// Test fixture
class MasterInitExitUnitTest : public UnitTest
{
 protected:
   clm_t clm = {};

   void SetUp() override
   {
      /* Reset mock call counters */
      mock_clear_master();
   }
};

/**
 * Validate master configuration
 *
 * @req REQ_CL_PROTOCOL_19
 * @req REQ_CL_PROTOCOL_26
 * @req REQ_CL_PROTOCOL_36
 * @req REQ_CLM_TIMING_02
 * @req REQ_CLM_CONFIGURATION_01
 *
 */
TEST_F (MasterInitExitUnitTest, ValidateMasterConfiguration)
{
   /* Compile the tests with sufficiently large memory area */
   ASSERT_GE (CLM_MAX_GROUPS, 5);
   ASSERT_GE (CLM_MAX_OCCUPIED_STATIONS_PER_GROUP, 13);

   clm_cfg_t config                                             = {};
   config.protocol_ver                                          = 2;
   config.hier.number_of_groups                                 = 5;
   config.master_id                                             = 0x01020304;
   config.hier.groups[0].parallel_off_timeout_count             = 3;
   config.hier.groups[0].timeout_value                          = 500;
   config.hier.groups[0].num_slave_devices                      = 2;
   config.hier.groups[0].slave_devices[0].slave_id              = 0x02030405;
   config.hier.groups[0].slave_devices[0].num_occupied_stations = 3;
   config.hier.groups[0].slave_devices[1].slave_id              = 0x02030406;
   config.hier.groups[0].slave_devices[1].num_occupied_stations = 3;
   config.hier.groups[1].num_slave_devices                      = 1;
   config.hier.groups[1].parallel_off_timeout_count             = 3;
   config.hier.groups[1].timeout_value                          = 500;
   config.hier.groups[1].slave_devices[0].slave_id              = 0x02030407;
   config.hier.groups[1].slave_devices[0].num_occupied_stations = 1;
   config.hier.groups[2].num_slave_devices                      = 1;
   config.hier.groups[2].parallel_off_timeout_count             = 3;
   config.hier.groups[2].timeout_value                          = 500;
   config.hier.groups[2].slave_devices[0].slave_id              = 0x02030408;
   config.hier.groups[2].slave_devices[0].num_occupied_stations = 1;
   config.hier.groups[3].num_slave_devices                      = 1;
   config.hier.groups[3].parallel_off_timeout_count             = 3;
   config.hier.groups[3].timeout_value                          = 500;
   config.hier.groups[3].slave_devices[0].slave_id              = 0x02030409;
   config.hier.groups[3].slave_devices[0].num_occupied_stations = 1;
   config.hier.groups[4].num_slave_devices                      = 1;
   config.hier.groups[4].parallel_off_timeout_count             = 3;
   config.hier.groups[4].timeout_value                          = 500;
   config.hier.groups[4].slave_devices[0].slave_id              = 0x0203040A;
   config.hier.groups[4].slave_devices[0].num_occupied_stations = 1;
   const clm_cfg_t default_config                               = config;

   EXPECT_EQ (clm_validate_config (nullptr), -1);
   EXPECT_EQ (clm_validate_config (&config), 0);

   /* Master IP address */
   config.master_id = CL_IPADDR_INVALID;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config.master_id = 0xFFFFFFFF;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config.master_id = 0xFFFFFFFE;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config = default_config;
   EXPECT_EQ (clm_validate_config (&config), 0);

   /* Timeout */
   config.hier.groups[0].timeout_value = 0;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config.hier.groups[0].timeout_value = 1;
   EXPECT_EQ (clm_validate_config (&config), 0);
   config = default_config;
   EXPECT_EQ (clm_validate_config (&config), 0);

   /* Timeout counts */
   config.hier.groups[0].parallel_off_timeout_count = 0;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config.hier.groups[0].parallel_off_timeout_count = 1;
   EXPECT_EQ (clm_validate_config (&config), 0);
   config = default_config;
   EXPECT_EQ (clm_validate_config (&config), 0);

   /* Constant link time */
   /* TODO test also 2000 ms */
   config.hier.groups[2].use_constant_link_scan_time = true;
   config.hier.groups[2].timeout_value               = 1999;
   EXPECT_EQ (clm_validate_config (&config), 0);
   config.hier.groups[2].timeout_value = 2001;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config = default_config;
   EXPECT_EQ (clm_validate_config (&config), 0);

   /* Protocol version */
   config.protocol_ver = 0;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config.protocol_ver = 3;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config.protocol_ver = 1;
   EXPECT_EQ (clm_validate_config (&config), 0);
   config = default_config;
   EXPECT_EQ (clm_validate_config (&config), 0);

   /* Number of groups */
   /* TODO test also 64 groups */
   config.hier.number_of_groups = 0;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config.hier.number_of_groups = 65;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config = default_config;
   EXPECT_EQ (clm_validate_config (&config), 0);

   /* Number of slave devices */
   /* TODO test also 16 devices */
   config.hier.groups[0].num_slave_devices = 0;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config.hier.groups[0].num_slave_devices = 17;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config = default_config;
   EXPECT_EQ (clm_validate_config (&config), 0);

   /* TODO Test also 64 slaves REQ_CL_CAPACITY_03 */

   /* Number of occupied slave stations */
   config.hier.groups[0].slave_devices[0].num_occupied_stations = 0;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config.hier.groups[0].slave_devices[0].num_occupied_stations = 17;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config = default_config;
   EXPECT_EQ (clm_validate_config (&config), 0);

   /* Invalid IP address */
   config.hier.groups[0].slave_devices[0].slave_id = CL_IPADDR_INVALID;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config.hier.groups[0].slave_devices[0].slave_id = 0xFFFFFFFE;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config.hier.groups[0].slave_devices[0].slave_id =
      CL_CCIEFB_MULTISTATION_INDICATOR;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config = default_config;
   EXPECT_EQ (clm_validate_config (&config), 0);

   /* Slave has same IP as master */
   config.hier.groups[0].slave_devices[0].slave_id = default_config.master_id;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config = default_config;
   EXPECT_EQ (clm_validate_config (&config), 0);

   /* Number of occupied slave stations per group */
   config.hier.groups[0].slave_devices[1].num_occupied_stations =
      CLM_MAX_OCCUPIED_STATIONS_PER_GROUP + 1 -
      config.hier.groups[0].slave_devices[0].num_occupied_stations;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config = default_config;
   EXPECT_EQ (clm_validate_config (&config), 0);

   /* Number of occupied slave stations for all groups in total */
   config.hier.groups[0].slave_devices[1].num_occupied_stations =
      CLM_MAX_OCCUPIED_STATIONS_PER_GROUP -
      config.hier.groups[0].slave_devices[0].num_occupied_stations;
   config.hier.groups[1].slave_devices[0].num_occupied_stations =
      CLM_MAX_OCCUPIED_STATIONS_PER_GROUP;
   config.hier.groups[2].slave_devices[0].num_occupied_stations =
      CLM_MAX_OCCUPIED_STATIONS_PER_GROUP;
   config.hier.groups[3].slave_devices[0].num_occupied_stations =
      CLM_MAX_OCCUPIED_STATIONS_PER_GROUP;
   config.hier.groups[4].slave_devices[0].num_occupied_stations =
      CLM_MAX_OCCUPIED_STATIONS_PER_GROUP;
   EXPECT_EQ (clm_validate_config (&config), -1);
   config = default_config;
   EXPECT_EQ (clm_validate_config (&config), 0);
}

TEST_F (MasterInitExitUnitTest, MasterConfigShow)
{
   clm_cfg_t config                                             = {};
   config.protocol_ver                                          = 2;
   config.master_id                                             = 0x01020304;
   config.hier.number_of_groups                                 = 1;
   config.hier.groups[0].parallel_off_timeout_count             = 3;
   config.hier.groups[0].timeout_value                          = 500;
   config.hier.groups[0].use_constant_link_scan_time            = false;
   config.hier.groups[0].num_slave_devices                      = 1;
   config.hier.groups[0].slave_devices[0].slave_id              = 0x01020305;
   config.hier.groups[0].slave_devices[0].num_occupied_stations = 3;

   clm_master_config_show (&config);

   config.hier.groups[0].use_constant_link_scan_time = true;
   clm_master_config_show (&config);

   /* Should not crash */
   clm_master_config_show (nullptr);
}

TEST_F (MasterInitExitUnitTest, MasterInternalsShow)
{
   /* Compile the tests with sufficiently large memory area */
   ASSERT_GE (CLM_MAX_GROUPS, 1);
   ASSERT_GE (CLM_MAX_OCCUPIED_STATIONS_PER_GROUP, 5);

   clm_cfg_t config                                             = {};
   config.protocol_ver                                          = 2;
   config.master_id                                             = 0x01020304;
   config.hier.number_of_groups                                 = 1;
   config.hier.groups[0].parallel_off_timeout_count             = 3;
   config.hier.groups[0].timeout_value                          = 500;
   config.hier.groups[0].num_slave_devices                      = 2;
   config.hier.groups[0].slave_devices[0].slave_id              = 0x01020305;
   config.hier.groups[0].slave_devices[0].num_occupied_stations = 3;
   config.hier.groups[0].slave_devices[1].slave_id              = 0x01020306;
   config.hier.groups[0].slave_devices[1].num_occupied_stations = 2;

   ASSERT_EQ (clm_master_init (&clm, &config, 0), 0);

   /* Should not crash */
   clm_master_internals_show (&clm);
   clm.groups[0].slave_devices[0].enabled                = false;
   clm.groups[0].slave_devices[0].force_transmission_bit = true;
   clm.groups[0].slave_devices[0].transmission_bit       = true;
   clm_master_internals_show (&clm);
   clm_master_internals_show (nullptr);

   /* Should not crash */
   clm_master_cyclic_data_show (
      &clm,
      0,
      config.hier.groups[0].slave_devices[0].num_occupied_stations,
      0,
      0);
   clm_master_cyclic_data_show (
      nullptr,
      0,
      config.hier.groups[0].slave_devices[0].num_occupied_stations,
      0,
      0);
}

TEST_F (MasterInitExitUnitTest, ValidateMasterConfigurationIpDuplicates)
{
   /* Compile the tests with sufficiently large memory area */
   ASSERT_GE (CLM_MAX_GROUPS, 5);
   ASSERT_GE (CLM_MAX_OCCUPIED_STATIONS_PER_GROUP, 13);

   clm_cfg_t config                                             = {};
   config.protocol_ver                                          = 2;
   config.master_id                                             = 0x01020304;
   config.hier.number_of_groups                                 = 5;
   config.hier.groups[0].parallel_off_timeout_count             = 3;
   config.hier.groups[0].timeout_value                          = 500;
   config.hier.groups[0].num_slave_devices                      = 2;
   config.hier.groups[0].slave_devices[0].slave_id              = 0x02030405;
   config.hier.groups[0].slave_devices[0].num_occupied_stations = 3;
   config.hier.groups[0].slave_devices[1].slave_id              = 0x02030406;
   config.hier.groups[0].slave_devices[1].num_occupied_stations = 3;
   config.hier.groups[1].num_slave_devices                      = 1;
   config.hier.groups[1].parallel_off_timeout_count             = 3;
   config.hier.groups[1].timeout_value                          = 500;
   config.hier.groups[1].slave_devices[0].slave_id              = 0x02030407;
   config.hier.groups[1].slave_devices[0].num_occupied_stations = 1;
   config.hier.groups[2].num_slave_devices                      = 1;
   config.hier.groups[2].parallel_off_timeout_count             = 3;
   config.hier.groups[2].timeout_value                          = 500;
   config.hier.groups[2].slave_devices[0].slave_id              = 0x02030408;
   config.hier.groups[2].slave_devices[0].num_occupied_stations = 1;
   config.hier.groups[3].num_slave_devices                      = 1;
   config.hier.groups[3].parallel_off_timeout_count             = 3;
   config.hier.groups[3].timeout_value                          = 500;
   config.hier.groups[3].slave_devices[0].slave_id              = 0x02030409;
   config.hier.groups[3].slave_devices[0].num_occupied_stations = 1;
   config.hier.groups[4].num_slave_devices                      = 1;
   config.hier.groups[4].parallel_off_timeout_count             = 3;
   config.hier.groups[4].timeout_value                          = 500;
   config.hier.groups[4].slave_devices[0].slave_id              = 0x0203040A;
   config.hier.groups[4].slave_devices[0].num_occupied_stations = 1;
   const clm_cfg_t default_config                               = config;

   ASSERT_EQ (clm_validate_config (&config), 0);

   /* Insert duplicate IP address in config */
   EXPECT_EQ (clm_validate_config_duplicates (&config), 0);
   config.hier.groups[3].slave_devices[0].slave_id = 0x02030407;
   EXPECT_EQ (clm_validate_config_duplicates (&config), -1);
   config = default_config;
   EXPECT_EQ (clm_validate_config_duplicates (&config), 0);
}

TEST_F (MasterInitExitUnitTest, MasterInitExit)
{
   /* Compile the tests with sufficiently large memory area */
   ASSERT_GE (CLM_MAX_GROUPS, 1);
   ASSERT_GE (CLM_MAX_OCCUPIED_STATIONS_PER_GROUP, 5);

   clm_cfg_t config                                             = {};
   config.protocol_ver                                          = 2;
   config.master_id                                             = 0x01020304;
   config.hier.number_of_groups                                 = 1;
   config.hier.groups[0].parallel_off_timeout_count             = 3;
   config.hier.groups[0].timeout_value                          = 500;
   config.hier.groups[0].num_slave_devices                      = 2;
   config.hier.groups[0].slave_devices[0].slave_id              = 0x01020305;
   config.hier.groups[0].slave_devices[0].num_occupied_stations = 3;
   config.hier.groups[0].slave_devices[1].slave_id              = 0x01020306;
   config.hier.groups[0].slave_devices[1].num_occupied_stations = 2;
   const clm_cfg_t default_config                               = config;

   /* Valid configuration */
   EXPECT_EQ (clm_master_init (&clm, &config, 0), 0);
   mock_clear_master();

   /* Invalid arguments */
   EXPECT_EQ (clm_master_init (nullptr, &config, 0), -1);
   EXPECT_EQ (clm_master_init (&clm, nullptr, 0), -1);
   EXPECT_EQ (clm_master_init (&clm, &config, 0), 0);
   mock_clear_master();

   /* Slave IP address duplication */
   config.hier.groups[0].slave_devices[1].slave_id = 0x01020305;
   EXPECT_EQ (clm_master_init (&clm, &config, 0), -1);
   config = default_config;
   EXPECT_EQ (clm_master_init (&clm, &config, 0), 0);
   mock_clear_master();

   /* CLAL fails to start */
   mock_data.clal_init_returnvalue = -1;
   EXPECT_EQ (clm_master_init (&clm, &config, 0), -1);
   mock_data.clal_init_returnvalue = 0;
   EXPECT_EQ (clm_master_init (&clm, &config, 0), 0);
   mock_clear_master();

   /* CLAL does not exit cleanly */
   mock_data.clal_exit_returnvalue = -1;
   EXPECT_EQ (clm_master_exit (&clm), -1);
   mock_data.clal_exit_returnvalue = 0;
   EXPECT_EQ (clm_master_exit (&clm), 0);

   /* Fail to open CCIEFB UDP port */
   mock_clear_master();
   EXPECT_EQ (clm_master_init (&clm, &config, 0), 0);
   mock_data.udp_ports[0].will_fail_open = true;
   EXPECT_EQ (clm_master_init (&clm, &config, 0), -1);
   mock_clear_master();
   EXPECT_EQ (clm_master_init (&clm, &config, 0), 0);

   /* Fail to open SLMP UDP port */
   mock_clear_master();
   EXPECT_EQ (clm_master_init (&clm, &config, 0), 0);
   mock_clear_master();
   mock_data.udp_ports[1].will_fail_open = true;
   EXPECT_EQ (clm_master_init (&clm, &config, 0), -1);
   mock_clear_master();
   EXPECT_EQ (clm_master_init (&clm, &config, 0), 0);

   /* Fail to read MAC address */
   mock_clear_master();
   mock_data.will_fail_read_mac_addr = true;
   EXPECT_EQ (clm_master_init (&clm, &config, 0), -1);
   mock_clear_master();
}

TEST_F (MasterInitExitUnitTest, MasterInitExitArbitrationSocket)
{
   /* Compile the tests with sufficiently large memory area */
   ASSERT_GE (CLM_MAX_GROUPS, 1);
   ASSERT_GE (CLM_MAX_OCCUPIED_STATIONS_PER_GROUP, 5);

   cl_mock_udp_port_t * mock_cciefb_arbitration_port = &mock_data.udp_ports[1];
   const cl_ipaddr_t broadcast_ip                    = 0x0102FFFF;

   clm_cfg_t config                                             = {};
   config.protocol_ver                                          = 2;
   config.master_id                                             = 0x01020304;
   config.hier.number_of_groups                                 = 1;
   config.hier.groups[0].parallel_off_timeout_count             = 3;
   config.hier.groups[0].timeout_value                          = 500;
   config.hier.groups[0].num_slave_devices                      = 2;
   config.hier.groups[0].slave_devices[0].slave_id              = 0x01020305;
   config.hier.groups[0].slave_devices[0].num_occupied_stations = 3;
   config.hier.groups[0].slave_devices[1].slave_id              = 0x01020306;
   config.hier.groups[0].slave_devices[1].num_occupied_stations = 2;
   config.use_separate_arbitration_socket                       = true;

   EXPECT_EQ (clm_master_init (&clm, &config, 0), 0);

   cl_mock_show_mocked_interfaces();
   cl_mock_show_mocked_udp_ports();
   EXPECT_EQ (mock_cciefb_arbitration_port->is_open, true);
   EXPECT_EQ (mock_cciefb_arbitration_port->port_number, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_cciefb_arbitration_port->ip_addr, broadcast_ip);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_recv, 0);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_close, 0);

   /* Should close arbitration socket */
   EXPECT_EQ (clm_master_exit (&clm), 0);

   EXPECT_EQ (mock_cciefb_arbitration_port->is_open, false);
   EXPECT_EQ (mock_cciefb_arbitration_port->port_number, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_cciefb_arbitration_port->ip_addr, broadcast_ip);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_recv, 0);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_close, 1);
}
