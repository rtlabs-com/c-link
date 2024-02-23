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

#include "cl_options.h"

#include "mocks.h"
#include "utils_for_testing.h"

#include <gtest/gtest.h>

// Test fixture
class MasterApiUnitTest : public UnitTest
{
 protected:
   cl_mock_udp_port_t * mock_cciefb_port       = &mock_data.udp_ports[0];
   const uint32_t tick_size                    = 1000;     /* microseconds */
   const uint32_t longer_than_arbitration      = 10000000; /* 10 seconds */
   clm_master_status_details_t master_details  = {};
   cl_mock_cyclic_request_result_t cycl_result = {};
   cl_ipaddr_t resulting_ip                    = CL_IPADDR_INVALID;
   const cl_ipaddr_t slave_id                  = 0x01020306;
   const uint16_t parameter_no                 = 501;

   void SetUp() override
   {
      /* Reset mock call counters */
      mock_clear_master();
   }
};

// Tests

TEST_F (MasterApiUnitTest, GetClmVersion)
{
   EXPECT_GE (strlen (cl_version()), 3U);
}

TEST_F (MasterIntegrationTestNoResponseYet, ApiSlmpCommands)
{
   const clm_node_search_db_t * node_search_result = nullptr;

   /* Node search, no time for slaves to respond */
   EXPECT_EQ (clm_perform_node_search (&clm), 0);
   node_search_result = clm_get_node_search_result (&clm);
   ASSERT_TRUE (node_search_result != nullptr);
   EXPECT_EQ (node_search_result->count, 0);
   EXPECT_EQ (node_search_result->stored, 0);

   /* Set slave IP address, no time for slave to respond */
   EXPECT_EQ (
      clm_set_slave_ipaddr (&clm, &remote_mac_addr, new_ip, new_netmask),
      0);
}

TEST_F (MasterIntegrationTestNoResponseYet, ApiSlmpInvalidIpAddress)
{
   EXPECT_EQ (
      clm_set_slave_ipaddr (&clm, &remote_mac_addr, CL_IPADDR_INVALID, new_netmask),
      -1);
}

TEST_F (MasterIntegrationTestNoResponseYet, ApiSlmpInvalidNetmask)
{
   EXPECT_EQ (
      clm_set_slave_ipaddr (&clm, &remote_mac_addr, new_ip, 0xFF00FF00),
      -1);
}

TEST_F (MasterApiUnitTest, ClmInit)
{
   clm_group_status_details_t group_details;
   const clm_slave_device_data_t * slave_connction_details      = nullptr;
   const uint16_t num_occupied                                  = 3;
   uint16_t resulting_occupied                                  = UINT16_MAX;
   clm_t * clm                                                  = nullptr;
   clm_cfg_t config                                             = {};
   config.protocol_ver                                          = 2;
   config.arbitration_time                                      = 2500;
   config.master_id                                             = 0x01020304;
   config.hier.number_of_groups                                 = 1;
   config.hier.groups[0].timeout_value                          = 500;
   config.hier.groups[0].parallel_off_timeout_count             = 3;
   config.hier.groups[0].num_slave_devices                      = 1;
   config.hier.groups[0].slave_devices[0].slave_id              = slave_id;
   config.hier.groups[0].slave_devices[0].num_occupied_stations = num_occupied;
   uint16_t rynumber                                            = 0;
   uint16_t rxnumber                                            = 0;
   uint16_t rwr_number                                          = 0;
   uint16_t rww_number                                          = 0;
   const uint16_t gi = 0; /* Group index */
   const uint16_t di = 0; /* Slave device index */

   clal_clear_memory (&master_details, sizeof (master_details));
   clal_clear_memory (&group_details, sizeof (group_details));

   ASSERT_EQ (
      clal_copy_string (
         config.file_directory,
         "my_directory",
         sizeof (config.file_directory)),
      0);

   /* Compile the tests with sufficiently large memory area */
   ASSERT_GE (CLM_MAX_OCCUPIED_STATIONS_PER_GROUP, 3);

   /* Start the stack */
   clm = clm_init (&config);
   ASSERT_TRUE (clm != nullptr);
   clm_handle_periodic (clm);

   mock_data.timestamp_us += tick_size;
   clm_handle_periodic (clm);
   EXPECT_EQ (clm_get_master_status (clm, &master_details), 0);
   EXPECT_EQ (master_details.master_state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (master_details.parameter_no, parameter_no);

   mock_data.timestamp_us += longer_than_arbitration;
   clm_handle_periodic (clm);
   EXPECT_EQ (clm_get_master_status (clm, &master_details), 0);
   EXPECT_EQ (master_details.master_state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (master_details.parameter_no, parameter_no);

   /* Verify transmission to slave */
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &cycl_result), 0);
   EXPECT_EQ (cycl_result.frame_sequence_no, 0);
   EXPECT_EQ (cycl_result.parameter_no, parameter_no);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, slave_id);

   clm_set_master_application_status (clm, true, false);
   ASSERT_EQ (
      clm_get_master_application_status (clm),
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);

   /* Disable and enable cyclic communication to a slave */
   // TODO (rtljobe): Check resulting frame in integration tests
   EXPECT_EQ (clm_set_slave_communication_status (clm, gi, di, false), 0);
   EXPECT_EQ (clm_set_slave_communication_status (clm, gi, di, true), 0);
   EXPECT_EQ (clm_set_slave_communication_status (clm, gi, 1000, true), -1);
   EXPECT_EQ (clm_set_slave_communication_status (clm, 1000, di, true), -1);

   /* Force cyclic transmission bit */
   // TODO (rtljobe): Check resulting frame in integration tests
   EXPECT_EQ (clm_force_cyclic_transmission_bit (clm, gi, di, true), 0);
   EXPECT_EQ (clm_force_cyclic_transmission_bit (clm, gi, di, false), 0);
   EXPECT_EQ (clm_force_cyclic_transmission_bit (clm, gi, 1000, false), -1);
   EXPECT_EQ (clm_force_cyclic_transmission_bit (clm, 1000, di, false), -1);

   /* Debug information */
   slave_connction_details = clm_get_device_connection_details (clm, gi, di);
   ASSERT_TRUE (slave_connction_details != nullptr);
   EXPECT_EQ (slave_connction_details->enabled, true);
   EXPECT_TRUE (clm_get_device_connection_details (clm, gi, 1000) == nullptr);
   EXPECT_TRUE (clm_get_device_connection_details (clm, 1000, di) == nullptr);

   EXPECT_EQ (clm_get_master_status (clm, &master_details), 0);
   EXPECT_EQ (master_details.master_state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (master_details.parameter_no, parameter_no);
   EXPECT_EQ (master_details.node_search_serial, CLM_SLMP_SERIAL_NONE);
   EXPECT_EQ (master_details.set_ip_request_serial, CLM_SLMP_SERIAL_NONE);
   EXPECT_EQ (clm_get_master_status (nullptr, &master_details), -1);

   EXPECT_EQ (clm_get_group_status (clm, gi, &group_details), 0);
   EXPECT_EQ (
      group_details.cyclic_transmission_state,
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF);
   EXPECT_EQ (group_details.frame_sequence_no, 0);
   EXPECT_EQ (group_details.group_index, gi);
   EXPECT_EQ (group_details.group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (group_details.timestamp_link_scan_start, mock_data.timestamp_us);
   EXPECT_EQ (group_details.total_occupied, num_occupied); /* One device only */
   EXPECT_EQ (clm_get_group_status (nullptr, gi, &group_details), -1);
   EXPECT_EQ (clm_get_group_status (clm, 1000, &group_details), -1);

   /* Clean slave communication statistics */
   clm_clear_statistics (clm);

   /* Set and read back RY bits */
   rynumber = 0;
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), false);
   clm_set_ry_bit (clm, gi, di, rynumber, true);
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), true);
   clm_set_ry_bit (clm, gi, di, rynumber, false);
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), false);

   rynumber = 63;
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), false);
   clm_set_ry_bit (clm, gi, di, rynumber, true);
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), true);
   clm_set_ry_bit (clm, gi, di, rynumber, false);
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), false);

   rynumber = 64;
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), false);
   clm_set_ry_bit (clm, gi, di, rynumber, true);
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), true);
   clm_set_ry_bit (clm, gi, di, rynumber, false);
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), false);

   rynumber = 127;
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), false);
   clm_set_ry_bit (clm, gi, di, rynumber, true);
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), true);
   clm_set_ry_bit (clm, gi, di, rynumber, false);
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), false);

   rynumber = 128;
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), false);
   clm_set_ry_bit (clm, gi, di, rynumber, true);
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), true);
   clm_set_ry_bit (clm, gi, di, rynumber, false);
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), false);

   rynumber = 191;
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), false);
   clm_set_ry_bit (clm, gi, di, rynumber, true);
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), true);
   clm_set_ry_bit (clm, gi, di, rynumber, false);
   EXPECT_EQ (clm_get_ry_bit (clm, gi, di, rynumber), false);

   /* Set and read back RWw word */
   rww_number = 0;
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0);
   clm_set_rww_value (clm, gi, di, rww_number, 0x1234);
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0x1234);
   clm_set_rww_value (clm, gi, di, rww_number, 0);
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0);

   rww_number = 31;
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0);
   clm_set_rww_value (clm, gi, di, rww_number, 0x1234);
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0x1234);
   clm_set_rww_value (clm, gi, di, rww_number, 0);
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0);

   rww_number = 32;
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0);
   clm_set_rww_value (clm, gi, di, rww_number, 0x1234);
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0x1234);
   clm_set_rww_value (clm, gi, di, rww_number, 0);
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0);

   rww_number = 63;
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0);
   clm_set_rww_value (clm, gi, di, rww_number, 0x1234);
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0x1234);
   clm_set_rww_value (clm, gi, di, rww_number, 0);
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0);

   rww_number = 64;
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0);
   clm_set_rww_value (clm, gi, di, rww_number, 0x1234);
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0x1234);
   clm_set_rww_value (clm, gi, di, rww_number, 0);
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0);

   rww_number = 95;
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0);
   clm_set_rww_value (clm, gi, di, rww_number, 0x1234);
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0x1234);
   clm_set_rww_value (clm, gi, di, rww_number, 0);
   EXPECT_EQ (clm_get_rww_value (clm, gi, di, rww_number), 0);

   /* Read RX bits */
   rxnumber = 0;
   EXPECT_EQ (clm_get_rx_bit (clm, gi, di, rxnumber), false);

   /* Read RWr registers */
   rwr_number = 0;
   EXPECT_EQ (clm_get_rwr_value (clm, gi, di, rwr_number), 0x0000);

   /* Find memory areas for a group */
   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)clm_get_first_rx_area (clm, gi, &resulting_occupied),
      (void *)&clm->groups[gi].memory_area.rx);
   EXPECT_EQ (resulting_occupied, num_occupied);

   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)clm_get_first_ry_area (clm, gi, &resulting_occupied),
      (void *)&clm->groups[gi].memory_area.ry);
   EXPECT_EQ (resulting_occupied, num_occupied);

   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)clm_get_first_rwr_area (clm, gi, &resulting_occupied),
      (void *)&clm->groups[gi].memory_area.rwr);
   EXPECT_EQ (resulting_occupied, num_occupied);

   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)clm_get_first_rww_area (clm, gi, &resulting_occupied),
      (void *)&clm->groups[gi].memory_area.rww);
   EXPECT_EQ (resulting_occupied, num_occupied);

   /* Find memory areas for a slave device */
   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)clm_get_first_device_rx_area (clm, gi, di, &resulting_occupied),
      (void *)&clm->groups[gi].memory_area.rx);
   EXPECT_EQ (resulting_occupied, num_occupied);

   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)clm_get_first_device_ry_area (clm, gi, di, &resulting_occupied),
      (void *)&clm->groups[gi].memory_area.ry);
   EXPECT_EQ (resulting_occupied, num_occupied);

   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)clm_get_first_device_rwr_area (clm, gi, di, &resulting_occupied),
      (void *)&clm->groups[gi].memory_area.rwr);
   EXPECT_EQ (resulting_occupied, num_occupied);

   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)clm_get_first_device_rww_area (clm, gi, di, &resulting_occupied),
      (void *)&clm->groups[gi].memory_area.rww);
   EXPECT_EQ (resulting_occupied, num_occupied);

   /* Close down the stack */
   EXPECT_EQ (clm_exit (clm), 0);
   free (clm);
}

TEST_F (MasterApiUnitTest, ClmInitFails)
{
   clm_t * clm                                                  = nullptr;
   clm_cfg_t config                                             = {};
   config.protocol_ver                                          = 2;
   config.master_id                                             = 0x01020304;
   config.hier.number_of_groups                                 = 1;
   config.hier.groups[0].timeout_value                          = 500;
   config.hier.groups[0].parallel_off_timeout_count             = 3;
   config.hier.groups[0].num_slave_devices                      = 1;
   config.hier.groups[0].slave_devices[0].slave_id              = slave_id;
   config.hier.groups[0].slave_devices[0].num_occupied_stations = 3;

   clm = clm_init (&config);
   ASSERT_TRUE (clm != nullptr);
   free (clm);

   /* Invalid configuration */
   mock_clear_master();
   config.master_id = CL_IPADDR_INVALID;
   clm              = clm_init (&config);
   ASSERT_TRUE (clm == nullptr);

   mock_clear_master();
   config.master_id = 0x01020304;
   clm              = clm_init (&config);
   ASSERT_TRUE (clm != nullptr);
   free (clm);

   /* calloc fails */
   mock_clear_master();
   mock_data.will_fail_calloc = true;
   clm                        = clm_init (&config);
   ASSERT_TRUE (clm == nullptr);

   mock_clear_master();
   clm = clm_init (&config);
   ASSERT_TRUE (clm != nullptr);
   free (clm);
}

TEST_F (MasterApiUnitTest, ClmIsNull)
{
   const clm_cfg_t config         = {};
   uint16_t num_occupied_stations = UINT16_MAX;

   ASSERT_EQ (clm_init_only (nullptr, &config), -1);
   ASSERT_EQ (clm_exit (nullptr), -1);

   ASSERT_TRUE (clm_get_device_connection_details (nullptr, 0, 0) == nullptr);

   /* Memory areas for a group */
   num_occupied_stations = UINT16_MAX;
   ASSERT_TRUE (
      clm_get_first_rx_area (nullptr, 0, &num_occupied_stations) == nullptr);
   ASSERT_EQ (num_occupied_stations, 0);

   num_occupied_stations = UINT16_MAX;
   ASSERT_TRUE (
      clm_get_first_ry_area (nullptr, 0, &num_occupied_stations) == nullptr);
   ASSERT_EQ (num_occupied_stations, 0);

   num_occupied_stations = UINT16_MAX;
   ASSERT_TRUE (
      clm_get_first_rwr_area (nullptr, 0, &num_occupied_stations) == nullptr);
   ASSERT_EQ (num_occupied_stations, 0);

   num_occupied_stations = UINT16_MAX;
   ASSERT_TRUE (
      clm_get_first_rww_area (nullptr, 0, &num_occupied_stations) == nullptr);
   ASSERT_EQ (num_occupied_stations, 0);

   /* Memory areas for a slave device */
   num_occupied_stations = UINT16_MAX;
   ASSERT_TRUE (
      clm_get_first_device_rx_area (nullptr, 0, 0, &num_occupied_stations) ==
      nullptr);
   ASSERT_EQ (num_occupied_stations, 0);

   num_occupied_stations = UINT16_MAX;
   ASSERT_TRUE (
      clm_get_first_device_ry_area (nullptr, 0, 0, &num_occupied_stations) ==
      nullptr);
   ASSERT_EQ (num_occupied_stations, 0);

   num_occupied_stations = UINT16_MAX;
   ASSERT_TRUE (
      clm_get_first_device_rwr_area (nullptr, 0, 0, &num_occupied_stations) ==
      nullptr);
   ASSERT_EQ (num_occupied_stations, 0);

   num_occupied_stations = UINT16_MAX;
   ASSERT_TRUE (
      clm_get_first_device_rww_area (nullptr, 0, 0, &num_occupied_stations) ==
      nullptr);
   ASSERT_EQ (num_occupied_stations, 0);
}

/**
 * Verify restarting the master stack with a new configuration
 *
 */
TEST_F (MasterApiUnitTest, ClmInitExit)
{
   const cl_ipaddr_t new_slave_id                   = 0x01020308;
   const uint16_t new_parameter_no                  = parameter_no + 1;
   clm_t * clm                                      = nullptr;
   clm_cfg_t config                                 = {};
   config.protocol_ver                              = 2;
   config.arbitration_time                          = 2500;
   config.master_id                                 = 0x01020304;
   config.hier.number_of_groups                     = 1;
   config.hier.groups[0].timeout_value              = 500;
   config.hier.groups[0].parallel_off_timeout_count = 3;
   config.hier.groups[0].num_slave_devices          = 1;
   config.hier.groups[0].slave_devices[0].slave_id  = slave_id;
   config.hier.groups[0].slave_devices[0].num_occupied_stations = 3;

   ASSERT_EQ (
      clal_copy_string (
         config.file_directory,
         "my_directory",
         sizeof (config.file_directory)),
      0);

   /* Compile the tests with sufficiently large memory area */
   ASSERT_GE (CLM_MAX_OCCUPIED_STATIONS_PER_GROUP, 3);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);

   /* Start the stack */
   clm = clm_init (&config);
   ASSERT_TRUE (clm != nullptr);
   clm_handle_periodic (clm);

   mock_data.timestamp_us += tick_size;
   clm_handle_periodic (clm);
   EXPECT_EQ (clm_get_master_status (clm, &master_details), 0);
   EXPECT_EQ (master_details.master_state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (master_details.parameter_no, parameter_no);

   mock_data.timestamp_us += longer_than_arbitration;
   clm_handle_periodic (clm);
   EXPECT_EQ (clm_get_master_status (clm, &master_details), 0);
   EXPECT_EQ (master_details.master_state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (master_details.parameter_no, parameter_no);

   /* Verify transmission to slave */
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &cycl_result), 0);
   EXPECT_EQ (cycl_result.frame_sequence_no, 0);
   EXPECT_EQ (cycl_result.parameter_no, parameter_no);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, slave_id);

   /* Close down the stack */
   EXPECT_EQ (clm_exit (clm), 0);
   free (clm);
   mock_data.timestamp_us += tick_size;

   /* Restart the stack with other configuration */
   config.hier.groups[0].slave_devices[0].slave_id = new_slave_id;
   clm                                             = clm_init (&config);
   ASSERT_TRUE (clm != nullptr);
   clm_handle_periodic (clm);

   mock_data.timestamp_us += tick_size;
   clm_handle_periodic (clm);
   EXPECT_EQ (clm_get_master_status (clm, &master_details), 0);
   EXPECT_EQ (master_details.master_state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (master_details.parameter_no, new_parameter_no);

   mock_data.timestamp_us += longer_than_arbitration;
   clm_handle_periodic (clm);
   EXPECT_EQ (clm_get_master_status (clm, &master_details), 0);
   EXPECT_EQ (master_details.master_state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (master_details.parameter_no, new_parameter_no);

   /* Verify transmission to slave, updated values */
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 6);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &cycl_result), 0);
   EXPECT_EQ (cycl_result.frame_sequence_no, 0);
   EXPECT_EQ (cycl_result.parameter_no, new_parameter_no);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, new_slave_id);

   /* Close down the stack */
   EXPECT_EQ (clm_exit (clm), 0);
   free (clm);
}
