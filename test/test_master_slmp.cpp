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
#include "common/cl_iefb.h"
#include "common/cl_util.h"
#include "master/clm_master.h"
#include "master/clm_slmp.h"

#include "mocks.h"
#include "utils_for_testing.h"

#include <gtest/gtest.h>

/* TODO Suggestions for more integration tests:
 *    -  Use two masters simultaneously on different simulated
 *       network interfaces.
 *       Do node search first on one master and then on the
 *       other master (not simultaneously)
 *       Verify that the masters find slaves.
 */

/* For test fixtures suitable for master integration testing, see
 * utils_for_testing.h */

TEST_F (MasterUnitTest, SlmpDatabase)
{
   /* Compile the tests with sufficiently large memory area */
   ASSERT_GE (CLM_MAX_NODE_SEARCH_DEVICES, 3);

   clm_node_search_db_t db;
   cl_slmp_node_search_resp_t response;
   int i                             = 0;
   const uint16_t vendor_code        = 0x2122;
   const uint32_t model_code         = 0x31323334;
   const uint16_t equipment_ver      = 0x4142;
   const cl_ipaddr_t slave_ip_addr   = 0x51525354;
   const cl_ipaddr_t slave_netmask   = 0x00FFFFFF;
   const cl_macaddr_t slave_mac_addr = {0x61, 0x62, 0x63, 0x64, 0x65, 0x66};

   clal_clear_memory (&response, sizeof (response));
   response.vendor_code   = CC_TO_LE16 (vendor_code);
   response.model_code    = CC_TO_LE32 (model_code);
   response.equipment_ver = CC_TO_LE16 (equipment_ver);
   response.slave_ip_addr = CC_TO_LE32 (slave_ip_addr);
   response.slave_netmask = CC_TO_LE32 (slave_netmask);
   cl_util_copy_mac_reverse (
      (cl_macaddr_t *)response.slave_mac_addr,
      &slave_mac_addr);

   /* Initialise database */
   clm_slmp_node_search_clear_db (&db);
   EXPECT_EQ (db.count, 0);
   EXPECT_EQ (db.stored, 0);

   /* Add to database */
   EXPECT_EQ (clm_slmp_node_search_add_db (&db, &response), 0);
   EXPECT_EQ (db.count, 1);
   EXPECT_EQ (db.stored, 1);

   response.vendor_code       = CC_TO_LE16 (vendor_code + 1);
   response.model_code        = CC_TO_LE32 (model_code + 2);
   response.equipment_ver     = CC_TO_LE16 (equipment_ver + 3);
   response.slave_ip_addr     = CC_TO_LE32 (slave_ip_addr + 4);
   response.slave_netmask     = CC_TO_LE32 (slave_netmask + 5);
   response.slave_mac_addr[0] = 0x6A;
   EXPECT_EQ (clm_slmp_node_search_add_db (&db, &response), 0);
   EXPECT_EQ (db.count, 2);
   EXPECT_EQ (db.stored, 2);

   EXPECT_EQ (db.entries[0].vendor_code, vendor_code);
   EXPECT_EQ (db.entries[0].model_code, model_code);
   EXPECT_EQ (db.entries[0].equipment_ver, equipment_ver);
   EXPECT_EQ (db.entries[0].slave_id, slave_ip_addr);
   EXPECT_EQ (db.entries[0].slave_netmask, slave_netmask);
   EXPECT_TRUE (MacAddressMatch (&db.entries[0].slave_mac_addr, &slave_mac_addr));
   EXPECT_EQ (db.entries[1].vendor_code, vendor_code + 1);
   EXPECT_EQ (db.entries[1].model_code, model_code + 2);
   EXPECT_EQ (db.entries[1].equipment_ver, equipment_ver + 3);
   EXPECT_EQ (db.entries[1].slave_id, slave_ip_addr + 4);
   EXPECT_EQ (db.entries[1].slave_netmask, slave_netmask + 5);
   EXPECT_EQ (db.entries[1].slave_mac_addr[5], 0x6A);

   /* Fill database */
   for (i = 0; i < CLM_MAX_NODE_SEARCH_DEVICES - 2; i++)
   {
      response.vendor_code = CC_TO_LE16 (vendor_code + 2 + i);
      EXPECT_EQ (clm_slmp_node_search_add_db (&db, &response), 0);
   }
   EXPECT_EQ (db.count, CLM_MAX_NODE_SEARCH_DEVICES);
   EXPECT_EQ (db.stored, CLM_MAX_NODE_SEARCH_DEVICES);

   /* Add to full database */
   response.vendor_code = CC_TO_LE16 (0);
   EXPECT_EQ (clm_slmp_node_search_add_db (&db, &response), -1);
   EXPECT_EQ (db.count, CLM_MAX_NODE_SEARCH_DEVICES + 1);
   EXPECT_EQ (db.stored, CLM_MAX_NODE_SEARCH_DEVICES);
   EXPECT_EQ (clm_slmp_node_search_add_db (&db, &response), -1);
   EXPECT_EQ (db.count, CLM_MAX_NODE_SEARCH_DEVICES + 2);
   EXPECT_EQ (db.stored, CLM_MAX_NODE_SEARCH_DEVICES);

   /* Validate contents */
   EXPECT_EQ (db.entries[0].vendor_code, vendor_code);
   EXPECT_EQ (db.entries[1].vendor_code, vendor_code + 1);
   EXPECT_EQ (
      db.entries[CLM_MAX_NODE_SEARCH_DEVICES - 1].vendor_code,
      vendor_code + CLM_MAX_NODE_SEARCH_DEVICES - 1);

   /* Clear database */
   clm_slmp_node_search_clear_db (&db);
   EXPECT_EQ (db.count, 0);
   EXPECT_EQ (db.stored, 0);
}

/**
 * Master sends Node Search command. Slave responds.
 *
 * @req REQ_CL_UDP_02
 * @req REQ_CL_SLMP_02
 *
 */
TEST_F (MasterIntegrationTestNotConnected, SlmpNodeSearch)
{
   const clm_node_search_db_t * db;
   cl_mock_node_search_request_result_t result;
   clm_master_status_details_t master_details;
   clal_clear_memory (&result, sizeof (result));
   clal_clear_memory (&master_details, sizeof (master_details));

   ASSERT_TRUE (mock_data.udp_ports[0].in_use);
   ASSERT_TRUE (mock_data.udp_ports[0].is_open);
   ASSERT_TRUE (mock_data.udp_ports[1].in_use);
   ASSERT_TRUE (mock_data.udp_ports[1].is_open);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);

   ASSERT_EQ (&mock_data.udp_ports[0], mock_cciefb_port);
   ASSERT_EQ (&mock_data.udp_ports[1], mock_slmp_port);
   ASSERT_EQ (&mock_data.udp_ports[2], mock_slmp_send_port);
   EXPECT_EQ (mock_cciefb_port->port_number, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_cciefb_port->ip_addr, my_ip);
   EXPECT_EQ (mock_cciefb_port->ifindex, my_ifindex);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_port->ip_addr, CL_IPADDR_ANY);
   EXPECT_EQ (mock_slmp_port->ifindex, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, (size_t)0);

   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), 0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   ASSERT_TRUE (mock_data.udp_ports[0].in_use);
   ASSERT_TRUE (mock_data.udp_ports[0].is_open);
   ASSERT_TRUE (mock_data.udp_ports[1].in_use);
   ASSERT_TRUE (mock_data.udp_ports[1].is_open);
   ASSERT_TRUE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);
   ASSERT_EQ (mock_slmp_send_port->port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_send_port->ip_addr, my_ip);
   EXPECT_EQ (mock_slmp_send_port->ifindex, my_ifindex);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, CL_SLMP_PORT);
   EXPECT_EQ (
      mock_slmp_send_port->remote_destination_ip,
      CL_IPADDR_LOCAL_BROADCAST);

   db = clm_slmp_get_node_search_result (&clm);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);

   ASSERT_EQ (
      mock_analyze_fake_node_search_request (mock_slmp_send_port, &result),
      0);
   EXPECT_EQ (result.serial, 0);
   EXPECT_EQ (
      result.length,
      SIZE_REQUEST_NODE_SEARCH - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_NODE_SEARCH);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_NODE_SEARCH);
   EXPECT_TRUE (
      MacAddressMatch (&result.master_mac_addr, &mock_interface->mac_address));
   EXPECT_EQ (result.master_ip_addr, mock_interface->ip_address);

   EXPECT_EQ (clm_iefb_get_master_status (&clm, &master_details), 0);
   EXPECT_EQ (master_details.master_state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (master_details.node_search_serial, clm.node_search_serial);
   EXPECT_EQ (master_details.set_ip_request_serial, CLM_SLMP_SERIAL_NONE);
   EXPECT_NE (clm.node_search_serial, CLM_SLMP_SERIAL_NONE);

   /* Slave responds */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_node_search,
      SIZE_RESPONSE_NODE_SEARCH);
   now += tick_size;
   clm.node_search_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_RESPONSE_NODE_SEARCH);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (db->count, 1);
   EXPECT_EQ (db->stored, 1);

   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 6);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_RESPONSE_NODE_SEARCH);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (db->count, 1);
   EXPECT_EQ (db->stored, 1);

   /* Timer for node search callback is expired */
   now += callback_time_nodesearch_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_RESPONSE_NODE_SEARCH);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.count, 1);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.stored, 1);
   EXPECT_EQ (
      cb_counters->master_cb_node_search.db.entries[0].slave_netmask,
      remote_netmask);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.entries[0].slave_id, remote_ip);
   EXPECT_EQ (
      cb_counters->master_cb_node_search.db.entries[0].vendor_code,
      slave_vendor_code);
   EXPECT_EQ (
      cb_counters->master_cb_node_search.db.entries[0].model_code,
      slave_model_code);
   EXPECT_EQ (
      cb_counters->master_cb_node_search.db.entries[0].equipment_ver,
      slave_equipment_ver);
   EXPECT_EQ (db->count, 1);
   EXPECT_EQ (db->stored, 1);
   EXPECT_EQ (db->entries[0].slave_id, remote_ip);
   EXPECT_EQ (db->entries[0].vendor_code, slave_vendor_code);
   EXPECT_EQ (db->entries[0].model_code, slave_model_code);
   EXPECT_EQ (db->entries[0].equipment_ver, slave_equipment_ver);
   EXPECT_TRUE (
      MacAddressMatch (&db->entries[0].slave_mac_addr, &remote_mac_addr));
}

/**
 * Master sends Node Search command via directed broadcast.
 *
 * @req REQ_CLM_UDP_02
 *
 */
TEST_F (MasterUnitTest, SlmpNodeSearchDirectedBroadcast)
{
   cl_mock_node_search_request_result_t result;
   clal_clear_memory (&result, sizeof (result));

   config.use_slmp_directed_broadcast = true;

   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);
   clm_slmp_periodic (&clm, now);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), 0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   ASSERT_TRUE (mock_data.udp_ports[0].in_use);
   ASSERT_TRUE (mock_data.udp_ports[0].is_open);
   ASSERT_TRUE (mock_data.udp_ports[1].in_use);
   ASSERT_TRUE (mock_data.udp_ports[1].is_open);
   ASSERT_TRUE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);
   ASSERT_EQ (mock_slmp_send_port->port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_send_port->ip_addr, my_ip);
   EXPECT_EQ (mock_slmp_send_port->ifindex, my_ifindex);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_ip, broadcast_ip);

   ASSERT_EQ (
      mock_analyze_fake_node_search_request (mock_slmp_send_port, &result),
      0);
   EXPECT_EQ (result.serial, 0);
   EXPECT_EQ (
      result.length,
      SIZE_REQUEST_NODE_SEARCH - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_NODE_SEARCH);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_NODE_SEARCH);
   EXPECT_TRUE (
      MacAddressMatch (&result.master_mac_addr, &mock_interface->mac_address));
   EXPECT_EQ (result.master_ip_addr, mock_interface->ip_address);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpNodeSearchDbIsFull)
{
   const clm_node_search_db_t * db;
   cl_mock_node_search_request_result_t result;
   clal_clear_memory (&result, sizeof (result));

   /* Force the database to be full */
   clm.node_search_db.stored = CLM_MAX_NODE_SEARCH_DEVICES;
   clm.node_search_db.count  = CLM_MAX_NODE_SEARCH_DEVICES;

   db = clm_slmp_get_node_search_result (&clm);
   EXPECT_EQ (db->count, CLM_MAX_NODE_SEARCH_DEVICES);
   EXPECT_EQ (db->stored, CLM_MAX_NODE_SEARCH_DEVICES);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Perform node search. Will clear database */
   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), 0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);

   /* Force the database to be full */
   clal_clear_memory (&clm.node_search_db, sizeof (clm.node_search_db));
   clm.node_search_db.stored = CLM_MAX_NODE_SEARCH_DEVICES;
   clm.node_search_db.count  = CLM_MAX_NODE_SEARCH_DEVICES;
   EXPECT_EQ (db->count, CLM_MAX_NODE_SEARCH_DEVICES);
   EXPECT_EQ (db->stored, CLM_MAX_NODE_SEARCH_DEVICES);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   ASSERT_EQ (
      mock_analyze_fake_node_search_request (mock_slmp_send_port, &result),
      0);
   EXPECT_EQ (result.serial, 0);
   EXPECT_EQ (
      result.length,
      SIZE_REQUEST_NODE_SEARCH - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_NODE_SEARCH);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_NODE_SEARCH);
   EXPECT_TRUE (
      MacAddressMatch (&result.master_mac_addr, &mock_interface->mac_address));
   EXPECT_EQ (result.master_ip_addr, mock_interface->ip_address);

   /* Slave responds */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_node_search,
      SIZE_RESPONSE_NODE_SEARCH);
   now += tick_size;
   clm.node_search_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   /* Timer for node search callback is expired */
   now += callback_time_nodesearch_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (db->count, CLM_MAX_NODE_SEARCH_DEVICES + 1);
   EXPECT_EQ (db->stored, CLM_MAX_NODE_SEARCH_DEVICES);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_node_search.db.count,
      CLM_MAX_NODE_SEARCH_DEVICES + 1);
   EXPECT_EQ (
      cb_counters->master_cb_node_search.db.stored,
      CLM_MAX_NODE_SEARCH_DEVICES);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpNodeSearchNoCallback)
{
   const clm_node_search_db_t * db;
   cl_mock_node_search_request_result_t result;
   clal_clear_memory (&result, sizeof (result));

   clm.config.node_search_cfm_cb = nullptr;

   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);

   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), 0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   db = clm_slmp_get_node_search_result (&clm);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);

   ASSERT_EQ (
      mock_analyze_fake_node_search_request (mock_slmp_send_port, &result),
      0);
   EXPECT_EQ (result.serial, 0);
   EXPECT_EQ (
      result.length,
      SIZE_REQUEST_NODE_SEARCH - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_NODE_SEARCH);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_NODE_SEARCH);
   EXPECT_TRUE (
      MacAddressMatch (&result.master_mac_addr, &mock_interface->mac_address));
   EXPECT_EQ (result.master_ip_addr, mock_interface->ip_address);

   /* Slave responds */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_node_search,
      SIZE_RESPONSE_NODE_SEARCH);
   now += tick_size;
   clm.node_search_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (db->count, 1);
   EXPECT_EQ (db->stored, 1);

   /* Timer for node search callback is expired */
   now += callback_time_nodesearch_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (db->count, 1);
   EXPECT_EQ (db->stored, 1);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpNodeSearchSingleSocket)
{
   const clm_node_search_db_t * db;
   cl_mock_node_search_request_result_t result;
   clal_clear_memory (&result, sizeof (result));

   clm.config.use_single_slmp_socket = true;

   ASSERT_TRUE (mock_data.udp_ports[0].in_use);
   ASSERT_TRUE (mock_data.udp_ports[0].is_open);
   ASSERT_TRUE (mock_data.udp_ports[1].in_use);
   ASSERT_TRUE (mock_data.udp_ports[1].is_open);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);

   ASSERT_EQ (&mock_data.udp_ports[0], mock_cciefb_port);
   ASSERT_EQ (&mock_data.udp_ports[1], mock_slmp_port);
   ASSERT_EQ (&mock_data.udp_ports[2], mock_slmp_send_port);
   EXPECT_EQ (mock_cciefb_port->port_number, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_cciefb_port->ip_addr, my_ip);
   EXPECT_EQ (mock_cciefb_port->ifindex, my_ifindex);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_port->ip_addr, CL_IPADDR_ANY);
   EXPECT_EQ (mock_slmp_port->ifindex, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, (size_t)0);

   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), 0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_TRUE (mock_data.udp_ports[0].in_use);
   EXPECT_TRUE (mock_data.udp_ports[0].is_open);
   EXPECT_TRUE (mock_data.udp_ports[1].in_use);
   EXPECT_TRUE (mock_data.udp_ports[1].is_open);
   EXPECT_FALSE (mock_data.udp_ports[2].in_use);
   EXPECT_FALSE (mock_data.udp_ports[3].in_use);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_port->total_sent_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_port->remote_destination_port, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_port->remote_destination_ip, CL_IPADDR_LOCAL_BROADCAST);

   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   db = clm_slmp_get_node_search_result (&clm);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);

   ASSERT_EQ (mock_analyze_fake_node_search_request (mock_slmp_port, &result), 0);
   EXPECT_EQ (result.serial, 0);
   EXPECT_EQ (
      result.length,
      SIZE_REQUEST_NODE_SEARCH - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_NODE_SEARCH);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_NODE_SEARCH);
   EXPECT_TRUE (
      MacAddressMatch (&result.master_mac_addr, &mock_interface->mac_address));
   EXPECT_EQ (result.master_ip_addr, mock_interface->ip_address);

   /* Slave responds */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_node_search,
      SIZE_RESPONSE_NODE_SEARCH);
   now += tick_size;
   clm.node_search_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (db->count, 1);
   EXPECT_EQ (db->stored, 1);

   /* Timer for node search callback is expired */
   now += callback_time_nodesearch_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.count, 1);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.stored, 1);
   EXPECT_EQ (
      cb_counters->master_cb_node_search.db.entries[0].slave_netmask,
      remote_netmask);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.entries[0].slave_id, remote_ip);
   EXPECT_EQ (
      cb_counters->master_cb_node_search.db.entries[0].vendor_code,
      slave_vendor_code);
   EXPECT_EQ (
      cb_counters->master_cb_node_search.db.entries[0].model_code,
      slave_model_code);
   EXPECT_EQ (
      cb_counters->master_cb_node_search.db.entries[0].equipment_ver,
      slave_equipment_ver);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (db->count, 1);
   EXPECT_EQ (db->stored, 1);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpNodeSearchTooShortHeader)
{
   const clm_node_search_db_t * db;

   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), 0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   /* Slave responds. Database not updated. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_node_search,
      5);
   now += tick_size;
   clm.node_search_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   db = clm_slmp_get_node_search_result (&clm);
   ASSERT_TRUE (db != nullptr);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);

   /* Timer for node search callback is expired */
   now += callback_time_nodesearch_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.count, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.stored, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpNodeSearchResponseWrongIfindex)
{
   const clm_node_search_db_t * db;

   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), 0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   /* Slave responds on wrong ifindex. Database not updated. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      123,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_node_search,
      SIZE_RESPONSE_NODE_SEARCH);
   now += tick_size;
   clm.node_search_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   db = clm_slmp_get_node_search_result (&clm);
   ASSERT_TRUE (db != nullptr);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);

   /* Timer for node search callback is expired */
   now += callback_time_nodesearch_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.count, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.stored, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpNodeSearchWrongSerial)
{
   const clm_node_search_db_t * db;

   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), 0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   /* Slave responds. Database not updated. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_node_search,
      SIZE_RESPONSE_NODE_SEARCH);
   now += tick_size;
   clm.node_search_serial = pending_slmp_serial + 1;
   clm_slmp_periodic (&clm, now);

   db = clm_slmp_get_node_search_result (&clm);
   ASSERT_TRUE (db != nullptr);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);

   /* Timer for node search callback is expired */
   now += callback_time_nodesearch_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.count, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.stored, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpNodeSearchTooShortMessage)
{
   const clm_node_search_db_t * db;
   uint8_t response_node_search_too_short_message[SIZE_RESPONSE_NODE_SEARCH] = {
      0};

   clal_memcpy (
      response_node_search_too_short_message,
      sizeof (response_node_search_too_short_message),
      response_node_search,
      SIZE_RESPONSE_NODE_SEARCH);
   response_node_search_too_short_message[11] -= 1; /* Wrong length */

   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), 0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   /* Slave responds. Database not updated. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_node_search_too_short_message,
      SIZE_RESPONSE_NODE_SEARCH - 1);

   now += tick_size;
   clm.node_search_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   db = clm_slmp_get_node_search_result (&clm);
   ASSERT_TRUE (db != nullptr);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);

   /* Timer for node search callback is expired */
   now += callback_time_nodesearch_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.count, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.stored, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpNodeSearchInvalid)
{
   const clm_node_search_db_t * db;
   uint8_t response_node_search_invalid[SIZE_RESPONSE_NODE_SEARCH] = {0};

   clal_memcpy (
      response_node_search_invalid,
      sizeof (response_node_search_invalid),
      response_node_search,
      SIZE_RESPONSE_NODE_SEARCH);
   response_node_search_invalid[32] = 3; /* Wrong IP size */

   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), 0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   /* Slave responds. Database not updated. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_node_search_invalid,
      SIZE_RESPONSE_NODE_SEARCH);
   now += tick_size;
   clm.node_search_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   db = clm_slmp_get_node_search_result (&clm);
   ASSERT_TRUE (db != nullptr);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);

   /* Timer for node search callback is expired */
   now += callback_time_nodesearch_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.count, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.stored, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpNodeSearchResponseIsRequest)
{
   const clm_node_search_db_t * db;

   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), 0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   /* Slave responds. Database not updated. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search,
      SIZE_RESPONSE_NODE_SEARCH);
   now += tick_size;
   clm.node_search_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   db = clm_slmp_get_node_search_result (&clm);
   ASSERT_TRUE (db != nullptr);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);

   /* Timer for node search callback is expired */
   now += callback_time_nodesearch_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpNodeSearchInvalidHeader)
{
   const clm_node_search_db_t * db;
   uint8_t response_node_search_invalid_header[SIZE_RESPONSE_NODE_SEARCH] = {0};

   clal_memcpy (
      response_node_search_invalid_header,
      sizeof (response_node_search_invalid_header),
      response_node_search,
      SIZE_RESPONSE_NODE_SEARCH);
   response_node_search_invalid_header[11] = 0x12; /* Wrong length */

   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), 0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   /* Slave responds. Database not updated. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_node_search_invalid_header,
      SIZE_RESPONSE_NODE_SEARCH);
   now += tick_size;
   clm.node_search_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   db = clm_slmp_get_node_search_result (&clm);
   ASSERT_TRUE (db != nullptr);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);

   /* Timer for node search callback is expired */
   now += callback_time_nodesearch_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.count, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.stored, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (db->count, 0);
   EXPECT_EQ (db->stored, 0);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpNodeSearchFailsToSend)
{
   mock_slmp_send_port->will_fail_send = true;
   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), -1);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpNodeSearchWrongSendSize)
{
   mock_slmp_send_port->use_modified_send_size_returnvalue = true;
   mock_slmp_send_port->modified_send_size_returnvalue     = 7;
   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), -1);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpMultipleNodeSearch)
{
   const clm_node_search_db_t * node_search_result = nullptr;

   /* Node search, no time for slaves to respond */
   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), 0);
   node_search_result = clm_slmp_get_node_search_result (&clm);
   ASSERT_TRUE (node_search_result != nullptr);
   EXPECT_EQ (node_search_result->count, 0);
   EXPECT_EQ (node_search_result->stored, 0);

   /* Do not sent a new node search when waiting for previous */
   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), -1);
   node_search_result = clm_slmp_get_node_search_result (&clm);
   ASSERT_TRUE (node_search_result != nullptr);
   EXPECT_EQ (node_search_result->count, 0);
   EXPECT_EQ (node_search_result->stored, 0);
}

/**
 * Master sends Set IP command
 *
 * @req REQ_CL_UDP_02
 * @req REQ_CL_SLMP_03
 *
 */
TEST_F (MasterIntegrationTestNotConnected, SlmpSetIp)
{
   cl_mock_set_ip_request_result_t result;
   clm_master_status_details_t master_details;
   clal_clear_memory (&result, sizeof (result));
   clal_clear_memory (&master_details, sizeof (master_details));

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, (size_t)0);

   EXPECT_EQ (
      clm_slmp_perform_set_ipaddr_request (
         &clm,
         now,
         &remote_mac_addr,
         new_ip,
         new_netmask),
      0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, CL_SLMP_PORT);
   EXPECT_EQ (
      mock_slmp_send_port->remote_destination_ip,
      CL_IPADDR_LOCAL_BROADCAST);

   ASSERT_EQ (mock_analyze_fake_set_ip_request (mock_slmp_send_port, &result), 0);
   EXPECT_EQ (result.serial, 0);
   EXPECT_EQ (
      result.length,
      SIZE_REQUEST_SET_IP - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_NODE_IPADDRESS_SET);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_NODE_IPADDRESS_SET);
   EXPECT_TRUE (
      MacAddressMatch (&result.master_mac_addr, &mock_interface->mac_address));
   EXPECT_EQ (result.master_ip_addr, mock_interface->ip_address);
   EXPECT_TRUE (MacAddressMatch (&result.slave_mac_addr, &remote_mac_addr));
   EXPECT_EQ (result.slave_ip_addr, new_ip);
   EXPECT_EQ (result.slave_netmask, new_netmask);

   EXPECT_EQ (clm_iefb_get_master_status (&clm, &master_details), 0);
   EXPECT_EQ (master_details.master_state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (master_details.node_search_serial, CLM_SLMP_SERIAL_NONE);
   EXPECT_EQ (master_details.set_ip_request_serial, clm.set_ip_request_serial);
   EXPECT_NE (clm.set_ip_request_serial, CLM_SLMP_SERIAL_NONE);

   /* Slave responds */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_set_ip,
      SIZE_RESPONSE_SET_IP);
   now += tick_size;
   clm.set_ip_request_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_RESPONSE_SET_IP);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_set_ip.status,
      CLM_MASTER_SET_IP_STATUS_SUCCESS);
}

/**
 * Master sends Set IP command via directed broadcast.
 *
 * @req REQ_CLM_UDP_02
 *
 */
TEST_F (MasterUnitTest, SlmpSetIpDirectedBroadcast)
{
   cl_mock_set_ip_request_result_t result;
   clal_clear_memory (&result, sizeof (result));

   config.use_slmp_directed_broadcast = true;

   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);
   clm_slmp_periodic (&clm, now);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (
      clm_slmp_perform_set_ipaddr_request (
         &clm,
         now,
         &remote_mac_addr,
         new_ip,
         new_netmask),
      0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_ip, broadcast_ip);

   ASSERT_EQ (mock_analyze_fake_set_ip_request (mock_slmp_send_port, &result), 0);
   EXPECT_EQ (result.serial, 0);
   EXPECT_EQ (
      result.length,
      SIZE_REQUEST_SET_IP - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_NODE_IPADDRESS_SET);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_NODE_IPADDRESS_SET);
   EXPECT_TRUE (
      MacAddressMatch (&result.master_mac_addr, &mock_interface->mac_address));
   EXPECT_EQ (result.master_ip_addr, mock_interface->ip_address);
   EXPECT_TRUE (MacAddressMatch (&result.slave_mac_addr, &remote_mac_addr));
   EXPECT_EQ (result.slave_ip_addr, new_ip);
   EXPECT_EQ (result.slave_netmask, new_netmask);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpSetIpNoCallbackRegistered)
{
   cl_mock_set_ip_request_result_t result;
   clal_clear_memory (&result, sizeof (result));

   /* Do not register a "Set IP" callback */
   clm.config.set_ip_cfm_cb = nullptr;

   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   EXPECT_EQ (
      clm_slmp_perform_set_ipaddr_request (
         &clm,
         now,
         &remote_mac_addr,
         new_ip,
         new_netmask),
      0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   ASSERT_EQ (mock_analyze_fake_set_ip_request (mock_slmp_send_port, &result), 0);
   EXPECT_EQ (result.serial, 0);
   EXPECT_EQ (
      result.length,
      SIZE_REQUEST_SET_IP - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_NODE_IPADDRESS_SET);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_NODE_IPADDRESS_SET);
   EXPECT_TRUE (
      MacAddressMatch (&result.master_mac_addr, &mock_interface->mac_address));
   EXPECT_EQ (result.master_ip_addr, mock_interface->ip_address);
   EXPECT_TRUE (MacAddressMatch (&result.slave_mac_addr, &remote_mac_addr));
   EXPECT_EQ (result.slave_ip_addr, new_ip);
   EXPECT_EQ (result.slave_netmask, new_netmask);

   /* Slave responds */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_set_ip,
      SIZE_RESPONSE_SET_IP);
   now += tick_size;
   clm.set_ip_request_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0); /* No callback */
}

TEST_F (MasterIntegrationTestNotConnected, SlmpSetIpTimeout)
{
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   EXPECT_EQ (
      clm_slmp_perform_set_ipaddr_request (
         &clm,
         now,
         &remote_mac_addr,
         new_ip,
         new_netmask),
      0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);

   /* No slave response */
   now += tick_size;
   clm.set_ip_request_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Timer for set IP timeout is expired */
   now += callback_time_setip_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_set_ip.status,
      CLM_MASTER_SET_IP_STATUS_TIMEOUT);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpSetIpFailsToSend)
{
   mock_slmp_send_port->will_fail_send = true;
   EXPECT_EQ (
      clm_slmp_perform_set_ipaddr_request (
         &clm,
         now,
         &remote_mac_addr,
         new_ip,
         new_netmask),
      -1);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpSetIpWrongSendSize)
{
   mock_slmp_send_port->use_modified_send_size_returnvalue = true;
   mock_slmp_send_port->modified_send_size_returnvalue     = 7;
   EXPECT_EQ (
      clm_slmp_perform_set_ipaddr_request (
         &clm,
         now,
         &remote_mac_addr,
         new_ip,
         new_netmask),
      -1);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpSetIpTooShortMessage)
{
   uint8_t response_set_ip_too_short_message[SIZE_RESPONSE_SET_IP] = {0};

   clal_memcpy (
      response_set_ip_too_short_message,
      sizeof (response_set_ip_too_short_message),
      response_set_ip,
      SIZE_RESPONSE_SET_IP);
   response_set_ip_too_short_message[11] -= 1; /* Wrong length */

   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   EXPECT_EQ (
      clm_slmp_perform_set_ipaddr_request (
         &clm,
         now,
         &remote_mac_addr,
         new_ip,
         new_netmask),
      0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   /* Slave responds. Frame dropped */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_set_ip_too_short_message,
      SIZE_RESPONSE_SET_IP - 1);
   now += tick_size;
   clm.set_ip_request_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   /* Timer for set IP timeout is expired */
   now += callback_time_setip_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_set_ip.status,
      CLM_MASTER_SET_IP_STATUS_TIMEOUT);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpSetIpWrongMasterMac)
{
   uint8_t response_set_ip_wrong_mac[SIZE_RESPONSE_SET_IP] = {0};

   clal_memcpy (
      response_set_ip_wrong_mac,
      sizeof (response_set_ip_wrong_mac),
      response_set_ip,
      SIZE_RESPONSE_SET_IP);
   response_set_ip_wrong_mac[15] = 0x27;

   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   EXPECT_EQ (
      clm_slmp_perform_set_ipaddr_request (
         &clm,
         now,
         &remote_mac_addr,
         new_ip,
         new_netmask),
      0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   /* Slave responds. Frame dropped */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_set_ip_wrong_mac,
      SIZE_RESPONSE_SET_IP);
   now += tick_size;
   clm.set_ip_request_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   /* Timer for set IP timeout is expired */
   now += callback_time_setip_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_set_ip.status,
      CLM_MASTER_SET_IP_STATUS_TIMEOUT);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpSetIpSlaveError)
{
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   EXPECT_EQ (
      clm_slmp_perform_set_ipaddr_request (
         &clm,
         now,
         &remote_mac_addr,
         new_ip,
         new_netmask),
      0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);

   /* Slave responds with error frame */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_set_ip_error,
      SIZE_RESPONSE_ERROR);
   now += tick_size;
   clm.set_ip_request_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_RESPONSE_ERROR);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_set_ip.status, CLM_MASTER_SET_IP_STATUS_ERROR);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpSetIpSlaveErrorWrongSize)
{

   uint8_t response_set_ip_error_wrong_size[SIZE_RESPONSE_ERROR] = {0};

   clal_memcpy (
      response_set_ip_error_wrong_size,
      sizeof (response_set_ip_error_wrong_size),
      response_set_ip_error,
      SIZE_RESPONSE_ERROR);
   response_set_ip_error_wrong_size[11] -= 1; /* Wrong length */

   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   EXPECT_EQ (
      clm_slmp_perform_set_ipaddr_request (
         &clm,
         now,
         &remote_mac_addr,
         new_ip,
         new_netmask),
      0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);

   /* Slave responds with error frame. Drop frame */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_set_ip_error_wrong_size,
      SIZE_RESPONSE_ERROR - 1);
   now += tick_size;
   clm.set_ip_request_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_RESPONSE_ERROR - 1U);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Timer for set IP timeout is expired */
   now += callback_time_setip_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_set_ip.status,
      CLM_MASTER_SET_IP_STATUS_TIMEOUT);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpSetIpSlaveErrorInvalid)
{

   uint8_t response_set_ip_error_invalid[SIZE_RESPONSE_ERROR] = {0};

   clal_memcpy (
      response_set_ip_error_invalid,
      sizeof (response_set_ip_error_invalid),
      response_set_ip_error,
      SIZE_RESPONSE_ERROR);
   response_set_ip_error_invalid[15] = 0x12; /* Wrong Error network number */

   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   EXPECT_EQ (
      clm_slmp_perform_set_ipaddr_request (
         &clm,
         now,
         &remote_mac_addr,
         new_ip,
         new_netmask),
      0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);

   /* Slave responds with error frame. Drop frame */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_set_ip_error_invalid,
      SIZE_RESPONSE_ERROR);
   now += tick_size;
   clm.set_ip_request_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_RESPONSE_ERROR);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Timer for set IP timeout is expired */
   now += callback_time_setip_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_set_ip.status,
      CLM_MASTER_SET_IP_STATUS_TIMEOUT);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpSetIpSlaveErrorWrongCommand)
{

   uint8_t response_set_ip_error_wrong_command[SIZE_RESPONSE_ERROR] = {0};

   clal_memcpy (
      response_set_ip_error_wrong_command,
      sizeof (response_set_ip_error_wrong_command),
      response_set_ip_error,
      SIZE_RESPONSE_ERROR);
   response_set_ip_error_wrong_command[20] = 0x12; /* Wrong command */

   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   EXPECT_EQ (
      clm_slmp_perform_set_ipaddr_request (
         &clm,
         now,
         &remote_mac_addr,
         new_ip,
         new_netmask),
      0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);

   /* Slave responds with error frame. Drop frame */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_set_ip_error_wrong_command,
      SIZE_RESPONSE_ERROR);
   now += tick_size;
   clm.set_ip_request_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_RESPONSE_ERROR);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Timer for set IP timeout is expired */
   now += callback_time_setip_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_set_ip.status,
      CLM_MASTER_SET_IP_STATUS_TIMEOUT);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpSetIpSlaveErrorWrongSubcommand)
{

   uint8_t response_set_ip_error_wrong_subcommand[SIZE_RESPONSE_ERROR] = {0};

   clal_memcpy (
      response_set_ip_error_wrong_subcommand,
      sizeof (response_set_ip_error_wrong_subcommand),
      response_set_ip_error,
      SIZE_RESPONSE_ERROR);
   response_set_ip_error_wrong_subcommand[22] = 0x12; /* Wrong subcommand */

   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   EXPECT_EQ (
      clm_slmp_perform_set_ipaddr_request (
         &clm,
         now,
         &remote_mac_addr,
         new_ip,
         new_netmask),
      0);
   now += tick_size;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);

   /* Slave responds with error frame. Drop frame */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&response_set_ip_error_wrong_subcommand,
      SIZE_RESPONSE_ERROR);
   now += tick_size;
   clm.set_ip_request_serial = pending_slmp_serial;
   clm_slmp_periodic (&clm, now);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_RESPONSE_ERROR);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Timer for set IP timeout is expired */
   now += callback_time_setip_ms * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
   clm_slmp_periodic (&clm, now);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_set_ip.status,
      CLM_MASTER_SET_IP_STATUS_TIMEOUT);
}

TEST_F (MasterIntegrationTestNotConnected, SlmpMultipleSetIp)
{
   /* Set slave IP address, no time for slave to respond */
   EXPECT_EQ (
      clm_set_slave_ipaddr (&clm, &remote_mac_addr, new_ip, new_netmask),
      0);

   /* Do not sent a new set IP when waiting for previous */
   EXPECT_EQ (
      clm_set_slave_ipaddr (&clm, &remote_mac_addr, new_ip, new_netmask),
      -1);
}
