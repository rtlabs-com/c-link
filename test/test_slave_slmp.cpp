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
#include "common/cl_slmp.h"
#include "common/cl_types.h"
#include "slave/cls_slave.h"
#include "slave/cls_slmp.h"

#include "mocks.h"
#include "utils_for_testing.h"

#include <gtest/gtest.h>

/* For test fixtures suitable for slave integration testing, see
 * utils_for_testing.h */

/******************* Integration tests for SLMP node search ****************/

/**
 * Slave receives Node Search command
 *
 * @req REQ_CL_SLMP_02
 * @req REQ_CL_UDP_02
 * @req REQ_CLS_CONFORMANCE_12
 *
 */
TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearch)
{
   cl_mock_node_search_response_result_t result;
   clal_clear_memory (&result, sizeof (result));

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

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
   EXPECT_EQ (mock_cciefb_port->ip_addr, CL_IPADDR_ANY);
   EXPECT_EQ (mock_cciefb_port->ifindex, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_port->ip_addr, CL_IPADDR_ANY);
   EXPECT_EQ (mock_slmp_port->ifindex, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);

   /* Master does node search. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search,
      SIZE_REQUEST_NODE_SEARCH);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   /* No response is yet sent */
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_slmp_port->output_data_size, 0U);
   EXPECT_EQ (mock_slmp_port->remote_destination_ip, 0U);
   EXPECT_EQ (mock_slmp_port->remote_destination_port, 0U);

   /* Study callbacks */
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.master_ip_addr, remote_ip);
   EXPECT_TRUE (MacAddressMatch (
      &mock_data.slave_cb_nodesearch.master_mac_addr,
      &remote_mac_addr));
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);

   /* Wait for timer to send response */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   ASSERT_TRUE (mock_data.udp_ports[0].in_use);
   ASSERT_TRUE (mock_data.udp_ports[0].is_open);
   ASSERT_TRUE (mock_data.udp_ports[1].in_use);
   ASSERT_TRUE (mock_data.udp_ports[1].is_open);
   ASSERT_TRUE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[2].is_open);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);

   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_RESPONSE_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->output_data_size, SIZE_RESPONSE_NODE_SEARCH);
   EXPECT_EQ (
      mock_slmp_send_port->remote_destination_ip,
      CL_IPADDR_LOCAL_BROADCAST);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, CL_SLMP_PORT);

   /* Study response frame */
   ASSERT_EQ (
      mock_analyze_fake_node_search_response (mock_slmp_send_port, &result),
      0);
   EXPECT_EQ (result.serial, slmp_serial);
   EXPECT_EQ (
      result.length,
      SIZE_RESPONSE_NODE_SEARCH - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_TRUE (MacAddressMatch (&result.master_mac_addr, &remote_mac_addr));
   EXPECT_EQ (result.master_ip_addr, remote_ip);
   EXPECT_TRUE (
      MacAddressMatch (&result.slave_mac_addr, &mock_interface->mac_address));
   EXPECT_EQ (result.slave_ip_addr, my_ip);
   EXPECT_EQ (result.slave_netmask, my_netmask);
   EXPECT_EQ (result.vendor_code, config.vendor_code);
   EXPECT_EQ (result.model_code, config.model_code);
   EXPECT_EQ (result.equipment_ver, config.equipment_ver);
   EXPECT_EQ (result.slave_status, CL_SLMP_NODE_SEARCH_RESP_SERVER_STATUS_NORMAL);
}

/**
 * Slave receives Node Search command. Sends directed broadcast response.
 *
 * @req REQ_CLS_UDP_03
 *
 */
TEST_F (SlaveUnitTest, SlmpNodeSearchDirectedBroadcast)
{
   cl_mock_node_search_response_result_t result;
   clal_clear_memory (&result, sizeof (result));

   ASSERT_GE (CLS_MAX_OCCUPIED_STATIONS, 3);

   config.use_slmp_directed_broadcast = true;

   ASSERT_EQ (cls_slave_init (&cls, &config, now), 0);
   cls_iefb_periodic (&cls, now);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   /* Master does node search. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search,
      SIZE_REQUEST_NODE_SEARCH);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);

   /* Wait for timer to send response */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_RESPONSE_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->output_data_size, SIZE_RESPONSE_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_ip, 0x0102FFFFU);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, CL_SLMP_PORT);

   /* Study response frame */
   ASSERT_EQ (
      mock_analyze_fake_node_search_response (mock_slmp_send_port, &result),
      0);
   EXPECT_EQ (result.serial, slmp_serial);
   EXPECT_EQ (
      result.length,
      SIZE_RESPONSE_NODE_SEARCH - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_TRUE (MacAddressMatch (&result.master_mac_addr, &remote_mac_addr));
   EXPECT_EQ (result.master_ip_addr, remote_ip);
   EXPECT_TRUE (
      MacAddressMatch (&result.slave_mac_addr, &mock_interface->mac_address));
   EXPECT_EQ (result.slave_ip_addr, my_ip);
   EXPECT_EQ (result.slave_netmask, my_netmask);
   EXPECT_EQ (result.vendor_code, config.vendor_code);
   EXPECT_EQ (result.model_code, config.model_code);
   EXPECT_EQ (result.equipment_ver, config.equipment_ver);
   EXPECT_EQ (result.slave_status, CL_SLMP_NODE_SEARCH_RESP_SERVER_STATUS_NORMAL);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearchHeaderTooShort)
{
   const size_t too_short = 4;

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   /* Master does node search, with too few byte (shorter than header).
    * Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search,
      too_short);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   /* Wait for timer to send response. Nothing will be sent. */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, too_short);
   EXPECT_EQ (mock_slmp_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearchTooShort)
{
   const size_t wrong_length                                       = 23;
   uint8_t request_node_search_too_short[SIZE_REQUEST_NODE_SEARCH] = {0};

   /* Valid header, but overall message is too short */
   clal_memcpy (
      request_node_search_too_short,
      sizeof (request_node_search_too_short),
      request_node_search,
      SIZE_REQUEST_NODE_SEARCH);
   request_node_search_too_short[11] = (wrong_length - 13U) & UINT8_MAX;

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   /* Master does node search, with too short frame. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search_too_short,
      wrong_length);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   /* Wait for timer to send response. Nothing will be sent. */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, wrong_length);
   EXPECT_EQ (mock_slmp_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearchWrongLengthInHeader)
{
   uint8_t request_node_search_wrong_length[SIZE_REQUEST_NODE_SEARCH] = {0};

   /* Header with wrong length field */
   clal_memcpy (
      request_node_search_wrong_length,
      sizeof (request_node_search_wrong_length),
      request_node_search,
      SIZE_REQUEST_NODE_SEARCH);
   request_node_search_wrong_length[11] = 4U;

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   /* Master does node search, with wrong length info. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search_wrong_length,
      SIZE_REQUEST_NODE_SEARCH);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   /* Wait for timer to send response. Nothing will be sent. */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearchWrongAddressSize)
{
   uint8_t request_node_search_wrong_address_size[SIZE_REQUEST_NODE_SEARCH] = {0};

   /* Invalid message */
   clal_memcpy (
      request_node_search_wrong_address_size,
      sizeof (request_node_search_wrong_address_size),
      request_node_search,
      SIZE_REQUEST_NODE_SEARCH);
   request_node_search_wrong_address_size[25] = 3;

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   /* Master does node search, invalid. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search_wrong_address_size,
      SIZE_REQUEST_NODE_SEARCH);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   /* Wait for timer to send response. Nothing will be sent. */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearchSendWrongSize)
{
   const size_t wrong_size = 7;

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   /* The UDP port just accepts to send fewer bytes */
   mock_slmp_send_port->use_modified_send_size_returnvalue = true;
   mock_slmp_send_port->modified_send_size_returnvalue     = wrong_size;

   /* Master does node search. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search,
      SIZE_REQUEST_NODE_SEARCH);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   /* Wait for timer to send response (too short) */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, wrong_size);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.master_ip_addr, remote_ip);
   EXPECT_TRUE (MacAddressMatch (
      &mock_data.slave_cb_nodesearch.master_mac_addr,
      &remote_mac_addr));
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearchFailsToSend)
{
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   mock_slmp_send_port->will_fail_send = true;

   /* Master does node search. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search,
      SIZE_REQUEST_NODE_SEARCH);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   /* Wait for timer to send response, but it fails */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.master_ip_addr, remote_ip);
   EXPECT_TRUE (MacAddressMatch (
      &mock_data.slave_cb_nodesearch.master_mac_addr,
      &remote_mac_addr));
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearchWrongSubcommand)
{
   uint8_t request_node_search_wrong_subcommand[SIZE_REQUEST_NODE_SEARCH] = {0};

   /* Wrong subcommand */
   clal_memcpy (
      request_node_search_wrong_subcommand,
      sizeof (request_node_search_wrong_subcommand),
      request_node_search,
      SIZE_REQUEST_NODE_SEARCH);
   request_node_search_wrong_subcommand[17] = 3;

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   /* Master does node search. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search_wrong_subcommand,
      SIZE_REQUEST_NODE_SEARCH);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   /* Wait for timer to send response. Nothing will be sent. */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

/**
 * Wrong Node Search command
 *
 * @req REQ_CL_SLMP_02
 */
TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearchWrongCommand)
{
   uint8_t request_node_search_wrong_command[SIZE_REQUEST_NODE_SEARCH] = {0};

   /* Wrong command */
   clal_memcpy (
      request_node_search_wrong_command,
      sizeof (request_node_search_wrong_command),
      request_node_search,
      SIZE_REQUEST_NODE_SEARCH);
   request_node_search_wrong_command[15] = 3;

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   now += tick_size;
   cls_slmp_periodic (&cls, now); /* No data yet */

   /* Master does node search. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search_wrong_command,
      SIZE_REQUEST_NODE_SEARCH);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   /* Wait for timer to send response. Nothing will be sent. */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearchIncomingInvalidIp)
{
   uint8_t request_node_search_invalid_ip[SIZE_REQUEST_NODE_SEARCH] = {0};

   /* Invalid IP address*/
   clal_memcpy (
      request_node_search_invalid_ip,
      sizeof (request_node_search_invalid_ip),
      request_node_search,
      SIZE_REQUEST_NODE_SEARCH);
   request_node_search_invalid_ip[26] = 0;
   request_node_search_invalid_ip[27] = 0;
   request_node_search_invalid_ip[28] = 0;
   request_node_search_invalid_ip[29] = 0;

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   now += tick_size;
   cls_slmp_periodic (&cls, now); /* No data yet */

   /* Master does node search. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search_invalid_ip,
      SIZE_REQUEST_NODE_SEARCH);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   /* Wait for timer to send response. Nothing will be sent. */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearchInvalidStoredIp)
{
   cl_mock_node_search_response_result_t result;
   clal_clear_memory (&result, sizeof (result));
   const cl_macaddr_t given_master_mac_addr = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46};
   const cl_ipaddr_t given_master_ip   = 0x01021314; /* Within our netmask */
   const uint16_t given_master_port    = 2345;
   const uint16_t given_request_serial = 6789;

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   now += tick_size;
   cls_slmp_periodic (&cls, now); /* No data yet */

   /* Try to send, but there is no valid data */
   EXPECT_EQ (cls_slmp_send_node_search_response (&cls), -1);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_slmp_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);

   /* Set valid data, and send response */
   cls.node_search.request_serial        = given_request_serial;
   cls.node_search.slave_ip_address      = mock_interface->ip_address;
   cls.node_search.slave_netmask         = mock_interface->netmask;
   cls.node_search.slave_mac_address[0]  = mock_interface->mac_address[0];
   cls.node_search.slave_mac_address[1]  = mock_interface->mac_address[1];
   cls.node_search.slave_mac_address[2]  = mock_interface->mac_address[2];
   cls.node_search.slave_mac_address[3]  = mock_interface->mac_address[3];
   cls.node_search.slave_mac_address[4]  = mock_interface->mac_address[4];
   cls.node_search.slave_mac_address[5]  = mock_interface->mac_address[5];
   cls.node_search.master_ip_addr        = given_master_ip;
   cls.node_search.master_port           = given_master_port;
   cls.node_search.master_mac_address[0] = given_master_mac_addr[0];
   cls.node_search.master_mac_address[1] = given_master_mac_addr[1];
   cls.node_search.master_mac_address[2] = given_master_mac_addr[2];
   cls.node_search.master_mac_address[3] = given_master_mac_addr[3];
   cls.node_search.master_mac_address[4] = given_master_mac_addr[4];
   cls.node_search.master_mac_address[5] = given_master_mac_addr[5];

   EXPECT_EQ (cls_slmp_send_node_search_response (&cls), 0);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_RESPONSE_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->output_data_size, SIZE_RESPONSE_NODE_SEARCH);
   EXPECT_EQ (
      mock_slmp_send_port->remote_destination_ip,
      CL_IPADDR_LOCAL_BROADCAST);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, given_master_port);

   /* Study sent response frame */
   ASSERT_EQ (
      mock_analyze_fake_node_search_response (mock_slmp_send_port, &result),
      0);
   EXPECT_EQ (result.serial, given_request_serial);
   EXPECT_EQ (
      result.length,
      SIZE_RESPONSE_NODE_SEARCH - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (result.master_mac_addr[0], given_master_mac_addr[0]);
   EXPECT_EQ (result.master_mac_addr[1], given_master_mac_addr[1]);
   EXPECT_EQ (result.master_mac_addr[2], given_master_mac_addr[2]);
   EXPECT_EQ (result.master_mac_addr[3], given_master_mac_addr[3]);
   EXPECT_EQ (result.master_mac_addr[4], given_master_mac_addr[4]);
   EXPECT_EQ (result.master_mac_addr[5], given_master_mac_addr[5]);
   EXPECT_EQ (result.master_ip_addr, given_master_ip);
   EXPECT_EQ (result.slave_mac_addr[0], mock_interface->mac_address[0]);
   EXPECT_EQ (result.slave_mac_addr[1], mock_interface->mac_address[1]);
   EXPECT_EQ (result.slave_mac_addr[2], mock_interface->mac_address[2]);
   EXPECT_EQ (result.slave_mac_addr[3], mock_interface->mac_address[3]);
   EXPECT_EQ (result.slave_mac_addr[4], mock_interface->mac_address[4]);
   EXPECT_EQ (result.slave_mac_addr[5], mock_interface->mac_address[5]);
   EXPECT_EQ (result.slave_ip_addr, my_ip);
   EXPECT_EQ (result.slave_netmask, my_netmask);
   EXPECT_EQ (result.equipment_ver, config.equipment_ver);
   EXPECT_EQ (result.model_code, config.model_code);
   EXPECT_EQ (result.vendor_code, config.vendor_code);
   EXPECT_EQ (result.slave_status, CL_SLMP_NODE_SEARCH_RESP_SERVER_STATUS_NORMAL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_RESPONSE_NODE_SEARCH);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearchFailsToRead)
{
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   mock_slmp_port->will_fail_recv = true;

   /* Master does node search. We will fail to read it. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search,
      SIZE_REQUEST_NODE_SEARCH);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   //* Wait for timer to send response. Nothing will be sent. */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_slmp_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearchInvalidHeader)
{
   uint8_t request_node_search_invalid_header[SIZE_REQUEST_NODE_SEARCH] = {0};

   /* Invalid header */
   clal_memcpy (
      request_node_search_invalid_header,
      sizeof (request_node_search_invalid_header),
      request_node_search,
      SIZE_REQUEST_NODE_SEARCH);
   request_node_search_invalid_header[4] = 3;

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   /* Master does node search, drop frame */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search_invalid_header,
      SIZE_REQUEST_NODE_SEARCH);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   /* Wait for timer to send response. Nothing will be sent. */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearchNoCallback)
{
   cl_mock_node_search_response_result_t result;
   clal_clear_memory (&result, sizeof (result));

   cls.config.num_occupied_stations = 2;
   cls.config.vendor_code           = 0x3456;
   cls.config.model_code            = 0x789ABCDE;
   cls.config.equipment_ver         = 0xF012;
   cls.config.state_cb              = my_slave_state_ind;
   cls.config.node_search_cb        = nullptr;
   cls.config.set_ip_cb             = my_slave_set_ip_ind;
   cls.config.cb_arg                = &mock_data;

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   /* Master does node search. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search,
      SIZE_REQUEST_NODE_SEARCH);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   /* Wait for timer to send response */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_RESPONSE_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->output_data_size, SIZE_RESPONSE_NODE_SEARCH);
   EXPECT_EQ (
      mock_slmp_send_port->remote_destination_ip,
      CL_IPADDR_LOCAL_BROADCAST);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, CL_SLMP_PORT);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);

   /* Study response frame */
   ASSERT_EQ (
      mock_analyze_fake_node_search_response (mock_slmp_send_port, &result),
      0);
   EXPECT_EQ (result.serial, slmp_serial);
   EXPECT_EQ (
      result.length,
      SIZE_RESPONSE_NODE_SEARCH - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_TRUE (MacAddressMatch (&result.master_mac_addr, &remote_mac_addr));
   EXPECT_EQ (result.master_ip_addr, remote_ip);
   EXPECT_TRUE (
      MacAddressMatch (&result.slave_mac_addr, &mock_interface->mac_address));
   EXPECT_EQ (result.slave_ip_addr, my_ip);
   EXPECT_EQ (result.slave_netmask, my_netmask);
   EXPECT_EQ (result.vendor_code, config.vendor_code);
   EXPECT_EQ (result.model_code, config.model_code);
   EXPECT_EQ (result.equipment_ver, config.equipment_ver);
   EXPECT_EQ (result.slave_status, CL_SLMP_NODE_SEARCH_RESP_SERVER_STATUS_NORMAL);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpNodeSearchMasterIpAddressUnequal)
{
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   /* Master does node search, but the master IP address in IP header
      does not match the master IP address in payload.
      Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      0x0102030D, /* IP 1.2.3.13 */
      CL_SLMP_PORT,
      (uint8_t *)&request_node_search,
      SIZE_REQUEST_NODE_SEARCH);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   /* No response is yet sent */
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_slmp_port->output_data_size, 0U);
   EXPECT_EQ (mock_slmp_port->remote_destination_ip, 0U);
   EXPECT_EQ (mock_slmp_port->remote_destination_port, 0U);

   /* Study callbacks */
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);

   /* Wait for timer to send response. Nothing will be sent. */
   now += longer_than_nodesearch_us;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_slmp_port->output_data_size, 0U);
}

/***************** Integration tests for SLMP set IP address ****************/

/**
 * Slave receives Set IP command
 *
 * @req REQ_CL_UDP_02
 * @req REQ_CL_SLMP_03
 * @req REQ_CLS_SLMP_IP_01
 * @req REQ_CLS_CONFORMANCE_13
 *
 */
TEST_F (SlaveIntegrationTestNotConnected, SlmpSetIpAddress)
{
   cl_mock_set_ip_response_result_t result;
   clal_clear_memory (&result, sizeof (result));

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);

   /* Master does set slave IP address */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_set_ip,
      SIZE_REQUEST_SET_IP);

   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now); /* Read back network settings */

   EXPECT_EQ (mock_interface->ip_address, new_ip);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 1);
   EXPECT_EQ (mock_interface->netmask, new_netmask);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_RESPONSE_SET_IP);
   EXPECT_EQ (
      mock_slmp_send_port->remote_destination_ip,
      CL_IPADDR_LOCAL_BROADCAST);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, CL_SLMP_PORT);

   /* Study callbacks */
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_set_ip.master_ip_addr, remote_ip);
   EXPECT_EQ (mock_data.slave_cb_set_ip.new_ip_addr, new_ip);
   EXPECT_EQ (mock_data.slave_cb_set_ip.new_netmask, new_netmask);
   EXPECT_EQ (mock_data.slave_cb_set_ip.ip_setting_allowed, true);
   EXPECT_EQ (mock_data.slave_cb_set_ip.did_set_ip, true);
   EXPECT_TRUE (MacAddressMatch (
      &mock_data.slave_cb_set_ip.master_mac_addr,
      &remote_mac_addr));

   /* Study response frame */
   ASSERT_EQ (mock_analyze_fake_set_ip_response (mock_slmp_send_port, &result), 0);
   EXPECT_EQ (result.serial, slmp_serial);
   EXPECT_EQ (
      result.length,
      SIZE_RESPONSE_SET_IP - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_TRUE (MacAddressMatch (&result.master_mac_addr, &remote_mac_addr));
   EXPECT_EQ (
      mock_slmp_send_port->remote_destination_ip,
      CL_IPADDR_LOCAL_BROADCAST);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, CL_SLMP_PORT);
}

/**
 * Slave receives Set IP command. Sends directed broadcast response.
 *
 * @req REQ_CLS_UDP_03
 *
 */
TEST_F (SlaveUnitTest, SlmpSetIpAddressDirectedBroadcast)
{
   cl_mock_set_ip_response_result_t result;
   clal_clear_memory (&result, sizeof (result));

   ASSERT_GE (CLS_MAX_OCCUPIED_STATIONS, 3);

   config.use_slmp_directed_broadcast = true;

   ASSERT_EQ (cls_slave_init (&cls, &config, now), 0);
   cls_iefb_periodic (&cls, now);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   /* Master does set slave IP address */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_set_ip,
      SIZE_REQUEST_SET_IP);

   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now); /* Read back network settings */

   EXPECT_EQ (mock_interface->ip_address, new_ip);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 1);
   EXPECT_EQ (mock_interface->netmask, new_netmask);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_RESPONSE_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_ip, 0x010203FFU);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, CL_SLMP_PORT);

   /* Study callbacks */
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_set_ip.master_ip_addr, remote_ip);
   EXPECT_EQ (mock_data.slave_cb_set_ip.new_ip_addr, new_ip);
   EXPECT_EQ (mock_data.slave_cb_set_ip.new_netmask, new_netmask);
   EXPECT_EQ (mock_data.slave_cb_set_ip.ip_setting_allowed, true);
   EXPECT_EQ (mock_data.slave_cb_set_ip.did_set_ip, true);
   EXPECT_TRUE (MacAddressMatch (
      &mock_data.slave_cb_set_ip.master_mac_addr,
      &remote_mac_addr));

   /* Study response frame */
   ASSERT_EQ (mock_analyze_fake_set_ip_response (mock_slmp_send_port, &result), 0);
   EXPECT_EQ (result.serial, slmp_serial);
   EXPECT_EQ (
      result.length,
      SIZE_RESPONSE_SET_IP - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_TRUE (MacAddressMatch (&result.master_mac_addr, &remote_mac_addr));
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpSetIpAddressFailsReadMacAddress)
{
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);

   mock_data.will_fail_read_mac_addr = true;

   /* Master does set slave IP address. Drop frame */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_set_ip,
      SIZE_REQUEST_SET_IP);

   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now); /* Read back network settings */

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_ip, 0U);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, 0);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpSetIpAddressFailsReadNetmask)
{
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);

   mock_data.will_fail_read_netmask = true;

   /* Master does set slave IP address. Drop frame */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_set_ip,
      SIZE_REQUEST_SET_IP);

   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now); /* Read back network settings */

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_ip, 0U);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, 0);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

/* TODO test wrong command REQ_CL_SLMP_03 */

TEST_F (SlaveIntegrationTestNotConnected, SlmpSetIpAddressWrongSubcommand)
{
   uint8_t request_set_ip_wrong_subcommand[SIZE_REQUEST_SET_IP] = {0};

   /* Wrong subcommand */
   clal_memcpy (
      request_set_ip_wrong_subcommand,
      sizeof (request_set_ip_wrong_subcommand),
      request_set_ip,
      SIZE_REQUEST_SET_IP);
   request_set_ip_wrong_subcommand[17] = 0x21;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);

   /* Master does set slave IP address. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_set_ip_wrong_subcommand,
      SIZE_REQUEST_SET_IP);

   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now); /* Read back network settings */

   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpSetIpAddressTooShortFrame)
{
   const size_t wrong_length                             = 23;
   uint8_t request_set_ip_too_short[SIZE_REQUEST_SET_IP] = {0};

   /* Valid header, but overall message is too short */
   clal_memcpy (
      request_set_ip_too_short,
      sizeof (request_set_ip_too_short),
      request_set_ip,
      SIZE_REQUEST_SET_IP);
   request_set_ip_too_short[11] = (wrong_length - 13U) & UINT8_MAX;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);

   /* Master does set slave IP address, with too short frame. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_set_ip_too_short,
      wrong_length);

   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now); /* Read back network settings */

   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, wrong_length);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpSetIpAddressWrongAddressSize)
{

   uint8_t request_set_ip_addr_wrong_address_size[SIZE_REQUEST_SET_IP] = {0};

   /* Invalid address size */
   clal_memcpy (
      request_set_ip_addr_wrong_address_size,
      sizeof (request_set_ip_addr_wrong_address_size),
      request_set_ip,
      SIZE_REQUEST_SET_IP);
   request_set_ip_addr_wrong_address_size[36] = 3;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);

   /* Master does set slave IP address. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_set_ip_addr_wrong_address_size,
      SIZE_REQUEST_SET_IP);

   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now); /* Read back network settings */

   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpSetIpAddressWrongMacAddress)
{

   uint8_t request_set_ip_addr_wrong_mac_address[SIZE_REQUEST_SET_IP] = {0};

   /* Invalid address size */
   clal_memcpy (
      request_set_ip_addr_wrong_mac_address,
      sizeof (request_set_ip_addr_wrong_mac_address),
      request_set_ip,
      SIZE_REQUEST_SET_IP);
   request_set_ip_addr_wrong_mac_address[30] = 0x57;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);

   /* Master tries to set slave IP address, but wrong MAC addr. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_set_ip_addr_wrong_mac_address,
      SIZE_REQUEST_SET_IP);

   now += tick_size;
   cls_slmp_periodic (&cls, now);

   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpSetIpFailsToSend)
{
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);

   mock_slmp_send_port->will_fail_send = true;

   /* Master does set slave IP address. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_set_ip,
      SIZE_REQUEST_SET_IP);

   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now); /* Read back network settings */

   EXPECT_EQ (mock_interface->ip_address, new_ip);
   EXPECT_EQ (mock_interface->netmask, new_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_set_ip.master_ip_addr, remote_ip);
   EXPECT_EQ (mock_data.slave_cb_set_ip.new_ip_addr, new_ip);
   EXPECT_EQ (mock_data.slave_cb_set_ip.new_netmask, new_netmask);
   EXPECT_EQ (mock_data.slave_cb_set_ip.ip_setting_allowed, true);
   EXPECT_EQ (mock_data.slave_cb_set_ip.did_set_ip, true);
   EXPECT_TRUE (MacAddressMatch (
      &mock_data.slave_cb_set_ip.master_mac_addr,
      &remote_mac_addr));
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpSetIpSendWrongSize)
{
   const size_t wrong_send_size = 7;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);

   /* The UDP port just accepts to send fewer bytes */
   mock_slmp_send_port->use_modified_send_size_returnvalue = true;
   mock_slmp_send_port->modified_send_size_returnvalue     = wrong_send_size;

   /* Master does set slave IP address. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_set_ip,
      SIZE_REQUEST_SET_IP);

   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now); /* Read back network settings */

   EXPECT_EQ (mock_interface->ip_address, new_ip);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 1);
   EXPECT_EQ (mock_interface->netmask, new_netmask);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, wrong_send_size);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_set_ip.master_ip_addr, remote_ip);
   EXPECT_EQ (mock_data.slave_cb_set_ip.new_ip_addr, new_ip);
   EXPECT_EQ (mock_data.slave_cb_set_ip.new_netmask, new_netmask);
   EXPECT_EQ (mock_data.slave_cb_set_ip.ip_setting_allowed, true);
   EXPECT_EQ (mock_data.slave_cb_set_ip.did_set_ip, true);
   EXPECT_TRUE (MacAddressMatch (
      &mock_data.slave_cb_set_ip.master_mac_addr,
      &remote_mac_addr));
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpSetIpFailsToSetIpAddress)
{
   cl_mock_error_response_result_t result;
   clal_clear_memory (&result, sizeof (result));

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);

   mock_data.will_fail_set_ip_addr = true;

   /* Master does set slave IP address. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_set_ip,
      SIZE_REQUEST_SET_IP);

   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now); /* Read back network settings */

   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 1);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_RESPONSE_ERROR);
   EXPECT_EQ (
      mock_slmp_send_port->remote_destination_ip,
      CL_IPADDR_LOCAL_BROADCAST);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, CL_SLMP_PORT);

   /* Study callbacks */
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_set_ip.master_ip_addr, remote_ip);
   EXPECT_EQ (mock_data.slave_cb_set_ip.new_ip_addr, new_ip);
   EXPECT_EQ (mock_data.slave_cb_set_ip.new_netmask, new_netmask);
   EXPECT_EQ (mock_data.slave_cb_set_ip.ip_setting_allowed, true);
   EXPECT_EQ (mock_data.slave_cb_set_ip.did_set_ip, false);
   EXPECT_TRUE (MacAddressMatch (
      &mock_data.slave_cb_set_ip.master_mac_addr,
      &remote_mac_addr));

   /* Study error response frame */
   ASSERT_EQ (mock_analyze_fake_error_response (mock_slmp_send_port, &result), 0);
   EXPECT_EQ (result.serial, slmp_serial);
   EXPECT_EQ (
      result.length,
      SIZE_RESPONSE_ERROR - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.end_code, CL_SLMP_ENDCODE_COMMAND_REQUEST_MSG);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_NODE_IPADDRESS_SET);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_NODE_IPADDRESS_SET);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpSetIpNotAllowed)
{
   cl_mock_error_response_result_t result;
   clal_clear_memory (&result, sizeof (result));

   cls.config.num_occupied_stations = 2;
   cls.config.ip_setting_allowed    = false;
   cls.config.vendor_code           = 0x3456;
   cls.config.model_code            = 0x789ABCDE;
   cls.config.equipment_ver         = 0xF012;
   cls.config.state_cb              = my_slave_state_ind;
   cls.config.node_search_cb        = my_slave_node_search_ind;
   cls.config.set_ip_cb             = my_slave_set_ip_ind;
   cls.config.cb_arg                = &mock_data;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);

   /* Master does set slave IP address. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_set_ip,
      SIZE_REQUEST_SET_IP);

   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now); /* Read back network settings */

   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_RESPONSE_ERROR);

   /* Study callbacks */
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_set_ip.master_ip_addr, remote_ip);
   EXPECT_EQ (mock_data.slave_cb_set_ip.new_ip_addr, new_ip);
   EXPECT_EQ (mock_data.slave_cb_set_ip.new_netmask, new_netmask);
   EXPECT_EQ (mock_data.slave_cb_set_ip.ip_setting_allowed, false);
   EXPECT_EQ (mock_data.slave_cb_set_ip.did_set_ip, false);
   EXPECT_TRUE (MacAddressMatch (
      &mock_data.slave_cb_set_ip.master_mac_addr,
      &remote_mac_addr));

   /* Study error response frame */
   ASSERT_EQ (mock_analyze_fake_error_response (mock_slmp_send_port, &result), 0);
   EXPECT_EQ (result.serial, slmp_serial);
   EXPECT_EQ (
      result.length,
      SIZE_RESPONSE_ERROR - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.end_code, CL_SLMP_ENDCODE_COMMAND_REQUEST_MSG);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_NODE_IPADDRESS_SET);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_NODE_IPADDRESS_SET);
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpSetIpNoCallback)
{
   cl_mock_set_ip_response_result_t result;
   clal_clear_memory (&result, sizeof (result));

   cls.config.num_occupied_stations = 2;
   cls.config.ip_setting_allowed    = true;
   cls.config.vendor_code           = 0x3456;
   cls.config.model_code            = 0x789ABCDE;
   cls.config.equipment_ver         = 0xF012;
   cls.config.state_cb              = my_slave_state_ind;
   cls.config.node_search_cb        = my_slave_node_search_ind;
   cls.config.set_ip_cb             = nullptr;
   cls.config.cb_arg                = &mock_data;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);

   /* Master does set slave IP address. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_SLMP_PORT,
      (uint8_t *)&request_set_ip,
      SIZE_REQUEST_SET_IP);

   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now); /* Read back network settings */

   EXPECT_EQ (mock_interface->ip_address, new_ip);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 1);
   EXPECT_EQ (mock_interface->netmask, new_netmask);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, SIZE_RESPONSE_SET_IP);

   /* Study callbacks */
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0); /* No callback registered */

   /* Study response frame */
   ASSERT_EQ (mock_analyze_fake_set_ip_response (mock_slmp_send_port, &result), 0);
   EXPECT_EQ (result.serial, slmp_serial);
   EXPECT_EQ (
      result.length,
      SIZE_RESPONSE_SET_IP - CL_SLMP_RESP_HEADER_LENGTH_OFFSET);
   EXPECT_EQ (result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_TRUE (MacAddressMatch (&result.master_mac_addr, &remote_mac_addr));
}

TEST_F (SlaveIntegrationTestNotConnected, SlmpSetIpAddressMasterIpAddressUnequal)
{
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);

   /* Master does set slave IP address, but the master IP address in IP header
      does not match the master IP address in payload.
      Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_slmp_port,
      my_ip,
      my_ifindex,
      0x0102030D, /* IP 1.2.3.13 */
      CL_SLMP_PORT,
      (uint8_t *)&request_set_ip,
      SIZE_REQUEST_SET_IP);

   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now); /* Read back network settings */

   EXPECT_EQ (mock_interface->ip_address, my_ip);
   EXPECT_EQ (mock_interface->netmask, my_netmask);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_slmp_send_port->output_data_size, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);
}
