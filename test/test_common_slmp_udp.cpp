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
#include "common/cl_slmp_udp.h"
#include "common/cl_types.h"

#include "mocks.h"
#include "utils_for_testing.h"
#include <gtest/gtest.h>

#include "slave/cls_slave.h"
#include "slave/cls_slmp.h"

#include "common/cl_util.h"

// Test fixture
class SlmpUdpIntegrationTest : public UnitTest
{
 public:
   const cl_ipaddr_t my_ip                  = 0x01020306; /* IP 1.2.3.6 */
   const int my_ifindex                     = 4;
   const cl_ipaddr_t remote_ip              = 0x01020304; /* IP 1.2.3.4 */
   const uint16_t remote_port               = 0x0304;
   const size_t message_size                = 10;
   cl_mock_udp_port_t * mock_slmp_send_port = &mock_data.udp_ports[0];
};

TEST_F (SlmpUdpIntegrationTest, SlmpUdpSend)
{
   int my_socket       = 0;
   uint8_t buffer[100] = {0};
   buffer[0]           = 'A';
   buffer[1]           = 'B';

   ASSERT_FALSE (mock_data.udp_ports[0].in_use);
   ASSERT_FALSE (mock_data.udp_ports[0].is_open);
   ASSERT_FALSE (mock_data.udp_ports[1].in_use);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);

   EXPECT_EQ (
      cl_slmp_udp_send (
         &my_socket,
         true,
         buffer,
         sizeof (buffer),
         my_ip,
         remote_ip,
         remote_port,
         message_size),
      0);

   EXPECT_TRUE (mock_data.udp_ports[0].in_use);
   EXPECT_FALSE (mock_data.udp_ports[0].is_open);
   EXPECT_FALSE (mock_data.udp_ports[1].in_use);
   EXPECT_FALSE (mock_data.udp_ports[2].in_use);
   EXPECT_FALSE (mock_data.udp_ports[3].in_use);
   EXPECT_TRUE (mock_slmp_send_port->in_use);
   EXPECT_FALSE (mock_slmp_send_port->is_open);
   EXPECT_EQ (mock_slmp_send_port->ip_addr, my_ip);
   EXPECT_EQ (mock_slmp_send_port->port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_send_port->ifindex, my_ifindex);
   EXPECT_EQ (mock_slmp_send_port->input_data_size, 0U);
   EXPECT_EQ (mock_slmp_send_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, message_size);
   EXPECT_EQ (mock_slmp_send_port->output_data_size, message_size);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, remote_port);
   EXPECT_EQ (mock_slmp_send_port->udp_output_buffer[0], 'A');
   EXPECT_EQ (mock_slmp_send_port->udp_output_buffer[1], 'B');
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_open, 1U);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_recv, 0U);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1U);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_close, 1U);
}

TEST_F (SlmpUdpIntegrationTest, SlmpUdpSendDoNotOpen)
{
   int my_socket       = 1;
   uint8_t buffer[100] = {0};
   buffer[0]           = 'A';
   buffer[1]           = 'B';

   mock_slmp_send_port->in_use        = true;
   mock_slmp_send_port->is_open       = true;
   mock_slmp_send_port->handle_number = my_socket;
   mock_slmp_send_port->ip_addr       = my_ip;
   mock_slmp_send_port->port_number   = CL_SLMP_PORT;
   mock_slmp_send_port->ifindex       = (uint16_t)my_ifindex;
   mock_slmp_send_port->buf_size = sizeof (mock_slmp_send_port->udp_input_buffer);
   mock_slmp_send_port->remote_source_ip        = CL_IPADDR_INVALID;
   mock_slmp_send_port->remote_source_port      = 0;
   mock_slmp_send_port->remote_destination_ip   = CL_IPADDR_INVALID;
   mock_slmp_send_port->remote_destination_port = 0;
   mock_slmp_send_port->input_data_size         = 0;
   mock_slmp_send_port->output_data_size        = 0;

   ASSERT_TRUE (mock_data.udp_ports[0].in_use);
   ASSERT_TRUE (mock_data.udp_ports[0].is_open);
   ASSERT_FALSE (mock_data.udp_ports[1].in_use);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);

   EXPECT_EQ (
      cl_slmp_udp_send (
         &my_socket,
         false,
         buffer,
         sizeof (buffer),
         my_ip,
         remote_ip,
         remote_port,
         message_size),
      0);

   EXPECT_TRUE (mock_data.udp_ports[0].in_use);
   EXPECT_TRUE (mock_data.udp_ports[0].is_open);
   EXPECT_FALSE (mock_data.udp_ports[1].in_use);
   EXPECT_FALSE (mock_data.udp_ports[2].in_use);
   EXPECT_FALSE (mock_data.udp_ports[3].in_use);
   EXPECT_TRUE (mock_slmp_send_port->in_use);
   EXPECT_TRUE (mock_slmp_send_port->is_open);
   EXPECT_EQ (mock_slmp_send_port->port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_send_port->ifindex, my_ifindex);
   EXPECT_EQ (mock_slmp_send_port->input_data_size, 0U);
   EXPECT_EQ (mock_slmp_send_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_slmp_send_port->total_sent_bytes, message_size);
   EXPECT_EQ (mock_slmp_send_port->output_data_size, message_size);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_slmp_send_port->remote_destination_port, remote_port);
   EXPECT_EQ (mock_slmp_send_port->udp_output_buffer[0], 'A');
   EXPECT_EQ (mock_slmp_send_port->udp_output_buffer[1], 'B');
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_open, 0U);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_recv, 0U);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_send, 1U);
   EXPECT_EQ (mock_slmp_send_port->number_of_calls_close, 0U);
}

TEST_F (SlmpUdpIntegrationTest, SlmpUdpSendTooLargeMessage)
{
   int my_socket       = 0;
   uint8_t buffer[100] = {0};

   EXPECT_EQ (
      cl_slmp_udp_send (
         &my_socket,
         true,
         buffer,
         sizeof (buffer),
         my_ip,
         remote_ip,
         remote_port,
         sizeof (buffer) + 1),
      -1);
}

TEST_F (SlmpUdpIntegrationTest, SlmpUdpSendFailToOpen)
{
   int my_socket                       = 0;
   uint8_t buffer[100]                 = {0};
   mock_slmp_send_port->will_fail_open = true;

   EXPECT_EQ (
      cl_slmp_udp_send (
         &my_socket,
         true,
         buffer,
         sizeof (buffer),
         my_ip,
         remote_ip,
         remote_port,
         message_size),
      -1);
}

TEST_F (SlmpUdpIntegrationTest, SlmpUdpSendFailToSend)
{
   int my_socket                       = 0;
   uint8_t buffer[100]                 = {0};
   mock_slmp_send_port->will_fail_send = true;

   EXPECT_EQ (
      cl_slmp_udp_send (
         &my_socket,
         true,
         buffer,
         sizeof (buffer),
         my_ip,
         remote_ip,
         remote_port,
         message_size),
      -1);
}

TEST_F (SlmpUdpIntegrationTest, SlmpUdpSendWrongNumberOfBytes)
{
   int my_socket                                           = 0;
   uint8_t buffer[100]                                     = {0};
   mock_slmp_send_port->use_modified_send_size_returnvalue = true;
   mock_slmp_send_port->modified_send_size_returnvalue     = 1;

   EXPECT_EQ (
      cl_slmp_udp_send (
         &my_socket,
         true,
         buffer,
         sizeof (buffer),
         my_ip,
         remote_ip,
         remote_port,
         message_size),
      -1);
}
