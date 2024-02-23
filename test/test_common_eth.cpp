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

#include "common/cl_eth.h"

#include "cl_options.h"

#include "mocks.h"
#include "utils_for_testing.h"

#include <gtest/gtest.h>

// Test fixture

class EthUnitTest : public UnitTest
{
};

// Tests

/**
 * Verify reading network settings
 *
 */
TEST_F (EthUnitTest, EthInitExit)
{
   const cl_ipaddr_t ip_addr_0     = 0x01020306;
   const cl_ipaddr_t ip_addr_1     = 0x01020307;
   const cl_ipaddr_t ip_addr_wrong = 0x01020399;
   const int invalid_ifindex       = -123;
   int ifindex;
   cl_ipaddr_t netmask;
   cl_macaddr_t mac_address      = {};
   char ifname[CLAL_IFNAME_SIZE] = {};

   ASSERT_NE (mock_data.interfaces[0].mac_address[0], 0x00);

   /* Get network settings */
   EXPECT_EQ (
      cl_eth_get_network_settings (ip_addr_0, &ifindex, &netmask, &mac_address, ifname),
      0);
   EXPECT_EQ (ifindex, mock_data.interfaces[0].ifindex);
   EXPECT_EQ (netmask, mock_data.interfaces[0].netmask);
   EXPECT_TRUE (
      MacAddressMatch (&mac_address, &mock_data.interfaces[0].mac_address));
   EXPECT_NE (mac_address[0], 0x00);
   EXPECT_NE (mac_address[1], 0x00);
   EXPECT_NE (mac_address[2], 0x00);
   EXPECT_NE (mac_address[3], 0x00);
   EXPECT_NE (mac_address[4], 0x00);
   EXPECT_NE (mac_address[5], 0x00);
   EXPECT_EQ (strcmp (ifname, mock_data.interfaces[0].ifname), 0);

   EXPECT_EQ (
      cl_eth_get_network_settings (ip_addr_1, &ifindex, &netmask, &mac_address, ifname),
      0);
   EXPECT_EQ (ifindex, mock_data.interfaces[1].ifindex);
   EXPECT_EQ (netmask, mock_data.interfaces[1].netmask);
   EXPECT_TRUE (
      MacAddressMatch (&mac_address, &mock_data.interfaces[1].mac_address));
   EXPECT_NE (mac_address[0], 0x00);
   EXPECT_NE (mac_address[1], 0x00);
   EXPECT_NE (mac_address[2], 0x00);
   EXPECT_NE (mac_address[3], 0x00);
   EXPECT_NE (mac_address[4], 0x00);
   EXPECT_NE (mac_address[5], 0x00);
   EXPECT_EQ (strcmp (ifname, mock_data.interfaces[1].ifname), 0);

   /* Wrong IP address */
   mock_clear();
   mac_address[0] = 0x00;
   netmask        = CL_IPADDR_INVALID;
   ifindex        = invalid_ifindex;
   ifname[0]      = '\0';

   EXPECT_EQ (
      cl_eth_get_network_settings (
         ip_addr_wrong,
         &ifindex,
         &netmask,
         &mac_address,
         ifname),
      -1);

   /* Correct IP address, again */
   EXPECT_EQ (
      cl_eth_get_network_settings (ip_addr_0, &ifindex, &netmask, &mac_address, ifname),
      0);
   EXPECT_EQ (ifindex, mock_data.interfaces[0].ifindex);
   EXPECT_EQ (netmask, mock_data.interfaces[0].netmask);
   EXPECT_TRUE (
      MacAddressMatch (&mac_address, &mock_data.interfaces[0].mac_address));
   EXPECT_NE (mac_address[0], 0x00);
   EXPECT_NE (mac_address[1], 0x00);
   EXPECT_NE (mac_address[2], 0x00);
   EXPECT_NE (mac_address[3], 0x00);
   EXPECT_NE (mac_address[4], 0x00);
   EXPECT_NE (mac_address[5], 0x00);
   EXPECT_EQ (strcmp (ifname, mock_data.interfaces[0].ifname), 0);
}

/**
 * Verify error handling for reading network settings
 *
 */
TEST_F (EthUnitTest, EthClalErrors)
{
   const cl_ipaddr_t ip_addr_0 = 0x01020306;
   const int invalid_ifindex   = -123;
   int ifindex;
   cl_ipaddr_t netmask;
   cl_macaddr_t mac_address      = {};
   char ifname[CLAL_IFNAME_SIZE] = {};

   ASSERT_NE (mock_data.interfaces[0].mac_address[0], 0x00);

   /* Read out network settings */
   EXPECT_EQ (
      cl_eth_get_network_settings (ip_addr_0, &ifindex, &netmask, &mac_address, ifname),
      0);
   EXPECT_EQ (ifindex, mock_data.interfaces[0].ifindex);
   EXPECT_EQ (netmask, mock_data.interfaces[0].netmask);
   EXPECT_TRUE (
      MacAddressMatch (&mac_address, &mock_data.interfaces[0].mac_address));
   EXPECT_NE (mac_address[0], 0x00);
   EXPECT_NE (mac_address[1], 0x00);
   EXPECT_NE (mac_address[2], 0x00);
   EXPECT_NE (mac_address[3], 0x00);
   EXPECT_NE (mac_address[4], 0x00);
   EXPECT_NE (mac_address[5], 0x00);
   EXPECT_EQ (strcmp (ifname, mock_data.interfaces[0].ifname), 0);

   /* CLAL can not read interface index */
   mock_clear();
   mac_address[0]                   = 0x00;
   netmask                          = CL_IPADDR_INVALID;
   ifindex                          = invalid_ifindex;
   ifname[0]                        = '\0';
   mock_data.will_fail_read_ifindex = true;

   EXPECT_EQ (
      cl_eth_get_network_settings (ip_addr_0, &ifindex, &netmask, &mac_address, ifname),
      -1);

   mock_clear();
   EXPECT_EQ (
      cl_eth_get_network_settings (ip_addr_0, &ifindex, &netmask, &mac_address, ifname),
      0);
   EXPECT_EQ (ifindex, mock_data.interfaces[0].ifindex);
   EXPECT_EQ (netmask, mock_data.interfaces[0].netmask);
   EXPECT_TRUE (
      MacAddressMatch (&mac_address, &mock_data.interfaces[0].mac_address));
   EXPECT_NE (mac_address[0], 0x00);
   EXPECT_EQ (strcmp (ifname, mock_data.interfaces[0].ifname), 0);

   /* CLAL can not read netmask */
   mock_clear();
   mac_address[0]                   = 0x00;
   netmask                          = CL_IPADDR_INVALID;
   ifindex                          = invalid_ifindex;
   ifname[0]                        = '\0';
   mock_data.will_fail_read_netmask = true;

   EXPECT_EQ (
      cl_eth_get_network_settings (ip_addr_0, &ifindex, &netmask, &mac_address, ifname),
      -1);

   mock_clear();
   EXPECT_EQ (
      cl_eth_get_network_settings (ip_addr_0, &ifindex, &netmask, &mac_address, ifname),
      0);
   EXPECT_EQ (ifindex, mock_data.interfaces[0].ifindex);
   EXPECT_EQ (netmask, mock_data.interfaces[0].netmask);
   EXPECT_TRUE (
      MacAddressMatch (&mac_address, &mock_data.interfaces[0].mac_address));
   EXPECT_NE (mac_address[0], 0x00);
   EXPECT_EQ (strcmp (ifname, mock_data.interfaces[0].ifname), 0);

   /* CLAL can not read MAC address */
   mock_clear();
   mac_address[0]                    = 0x00;
   netmask                           = CL_IPADDR_INVALID;
   ifindex                           = invalid_ifindex;
   ifname[0]                         = '\0';
   mock_data.will_fail_read_mac_addr = true;

   EXPECT_EQ (
      cl_eth_get_network_settings (ip_addr_0, &ifindex, &netmask, &mac_address, ifname),
      -1);

   /* CLAL can not read interface name */
   mock_clear();
   mac_address[0]                  = 0x00;
   netmask                         = CL_IPADDR_INVALID;
   ifindex                         = invalid_ifindex;
   ifname[0]                       = '\0';
   mock_data.will_fail_read_ifname = true;

   EXPECT_EQ (
      cl_eth_get_network_settings (ip_addr_0, &ifindex, &netmask, &mac_address, ifname),
      0);
   EXPECT_EQ (ifindex, mock_data.interfaces[0].ifindex);
   EXPECT_EQ (netmask, mock_data.interfaces[0].netmask);
   EXPECT_TRUE (
      MacAddressMatch (&mac_address, &mock_data.interfaces[0].mac_address));
   EXPECT_NE (mac_address[0], 0x00);
   EXPECT_NE (mac_address[1], 0x00);
   EXPECT_NE (mac_address[2], 0x00);
   EXPECT_NE (mac_address[3], 0x00);
   EXPECT_NE (mac_address[4], 0x00);
   EXPECT_NE (mac_address[5], 0x00);
   EXPECT_EQ (strcmp (ifname, "UNKNOWN"), 0);

   mock_clear();
   EXPECT_EQ (
      cl_eth_get_network_settings (ip_addr_0, &ifindex, &netmask, &mac_address, ifname),
      0);
   EXPECT_EQ (ifindex, mock_data.interfaces[0].ifindex);
   EXPECT_EQ (netmask, mock_data.interfaces[0].netmask);
   EXPECT_TRUE (
      MacAddressMatch (&mac_address, &mock_data.interfaces[0].mac_address));
   EXPECT_NE (mac_address[0], 0x00);
   EXPECT_EQ (strcmp (ifname, mock_data.interfaces[0].ifname), 0);
}

/**
 * Verify network setting adjustments
 *
 */
TEST_F (EthUnitTest, EthSetNetworkSettings)
{
   const cl_ipaddr_t ip      = 0x01020304;
   const cl_ipaddr_t netmask = 0xFFFFFF00;
   const int ifindex         = 4;

   EXPECT_EQ (cl_eth_set_network_settings (ifindex, ip, netmask), 0);

   /* Invalid Ethernet interface index */
   EXPECT_EQ (cl_eth_set_network_settings (-3, ip, netmask), -1);
   EXPECT_EQ (cl_eth_set_network_settings (777, ip, netmask), -1);

   EXPECT_EQ (cl_eth_set_network_settings (ifindex, ip, netmask), 0);
}

/**
 * Verify mocking of network interfaces
 *
 */
TEST_F (EthUnitTest, VerifyInterfaceMocks)
{
   cl_macaddr_t resulting_macaddr          = {0};
   int resulting_ifindex                   = 0;
   char resulting_ifname[CLAL_IFNAME_SIZE] = {0};
   uint32_t resulting_netmask              = 0;

   const cl_ipaddr_t new_ip_addr = 0x02030405;
   const cl_ipaddr_t new_netmask = 0xFF000000;

   const int ifindex = 4;
   ASSERT_EQ (ifindex, mock_data.interfaces[0].ifindex);
   const cl_ipaddr_t netmask = 0xFFFF0000;
   ASSERT_EQ (netmask, mock_data.interfaces[0].netmask);
   const cl_ipaddr_t ip_addr = 0x01020306;
   ASSERT_EQ (ip_addr, mock_data.interfaces[0].ip_address);

   const int ifindex_b = 7;
   ASSERT_EQ (ifindex_b, mock_data.interfaces[1].ifindex);
   const cl_ipaddr_t ip_addr_b = 0x01020307;
   ASSERT_EQ (ip_addr_b, mock_data.interfaces[1].ip_address);
   const cl_ipaddr_t netmask_b = 0xFFFFFF00;
   ASSERT_EQ (netmask_b, mock_data.interfaces[1].netmask);

   /* Should not crash */
   cl_mock_show_mocked_interfaces();
   cl_mock_show_mocked_udp_ports();

   /* Helper function to find mocked interface */
   EXPECT_EQ (mock_helper_find_interface (ifindex), &mock_data.interfaces[0]);
   EXPECT_EQ (mock_helper_find_interface (0), nullptr);
   EXPECT_EQ (mock_helper_find_interface (-8), nullptr);
   EXPECT_EQ (mock_helper_find_interface (1234), nullptr);

   /* Read ifindex */
   EXPECT_EQ (mock_clal_get_ifindex (ip_addr, &resulting_ifindex), 0);
   EXPECT_EQ (resulting_ifindex, ifindex);
   mock_data.will_fail_read_ifindex = true;
   EXPECT_EQ (mock_clal_get_ifindex (ip_addr, &resulting_ifindex), -1);
   mock_data.will_fail_read_ifindex = false;
   EXPECT_EQ (mock_clal_get_ifindex (ip_addr, &resulting_ifindex), 0);
   EXPECT_EQ (mock_clal_get_ifindex (ip_addr + 10, &resulting_ifindex), -1);

   /* Read MAC address */
   EXPECT_EQ (mock_clal_get_mac_address (ifindex, &resulting_macaddr), 0);
   EXPECT_TRUE (
      MacAddressMatch (&resulting_macaddr, &mock_data.interfaces[0].mac_address));
   mock_data.will_fail_read_mac_addr = true;
   EXPECT_EQ (mock_clal_get_mac_address (ifindex, &resulting_macaddr), -1);
   mock_data.will_fail_read_mac_addr = false;
   EXPECT_EQ (mock_clal_get_mac_address (ifindex, &resulting_macaddr), 0);
   EXPECT_EQ (mock_clal_get_mac_address (0, &resulting_macaddr), -1);

   /* Read ifname */
   EXPECT_EQ (mock_clal_get_ifname (ifindex, resulting_ifname), 0);
   EXPECT_EQ (strcmp (resulting_ifname, mock_data.interfaces[0].ifname), 0);
   mock_data.will_fail_read_ifname = true;
   EXPECT_EQ (mock_clal_get_ifname (ifindex, resulting_ifname), -1);
   mock_data.will_fail_read_ifname = false;
   EXPECT_EQ (mock_clal_get_ifname (ifindex, resulting_ifname), 0);
   EXPECT_EQ (mock_clal_get_ifname (0, resulting_ifname), -1);

   /* Read netmask */
   EXPECT_EQ (mock_clal_get_netmask (ifindex, &resulting_netmask), 0);
   EXPECT_EQ (resulting_netmask, netmask);
   mock_data.will_fail_read_netmask = true;
   EXPECT_EQ (mock_clal_get_netmask (ifindex, &resulting_netmask), -1);
   mock_data.will_fail_read_netmask = false;
   EXPECT_EQ (mock_clal_get_netmask (ifindex, &resulting_netmask), 0);
   EXPECT_EQ (mock_clal_get_netmask (0, &resulting_netmask), -1);

   /* Set IP address and netmask */
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 0);
   EXPECT_EQ (mock_data.interfaces[0].ip_address, ip_addr);
   EXPECT_EQ (mock_data.interfaces[0].netmask, netmask);
   EXPECT_EQ (
      mock_clal_set_ip_address_netmask (ifindex, new_ip_addr, new_netmask),
      0);
   EXPECT_EQ (mock_data.interfaces[0].ip_address, new_ip_addr);
   EXPECT_EQ (mock_data.interfaces[0].netmask, new_netmask);
   EXPECT_EQ (mock_clal_get_ifindex (ip_addr, &resulting_ifindex), -1);
   EXPECT_EQ (mock_clal_get_ifindex (new_ip_addr, &resulting_ifindex), 0);
   EXPECT_EQ (resulting_ifindex, ifindex);

   mock_data.will_fail_set_ip_addr = true;
   EXPECT_EQ (mock_clal_set_ip_address_netmask (ifindex, ip_addr, netmask), -1);
   EXPECT_EQ (mock_data.interfaces[0].ip_address, new_ip_addr);
   EXPECT_EQ (mock_data.interfaces[0].netmask, new_netmask);
   mock_data.will_fail_set_ip_addr = false;
   EXPECT_EQ (mock_clal_set_ip_address_netmask (ifindex, ip_addr, netmask), 0);
   EXPECT_EQ (mock_data.interfaces[0].ip_address, ip_addr);
   EXPECT_EQ (mock_data.interfaces[0].netmask, netmask);
   EXPECT_EQ (mock_clal_set_ip_address_netmask (0, new_ip_addr, new_netmask), -1);
   EXPECT_EQ (mock_data.number_of_calls_set_ip_address_netmask, 3);

   /* Verify independent operation for two interfaces */
   mock_clear();
   EXPECT_EQ (mock_clal_get_ifindex (ip_addr, &resulting_ifindex), 0);
   EXPECT_EQ (resulting_ifindex, ifindex);
   EXPECT_EQ (mock_clal_get_mac_address (ifindex, &resulting_macaddr), 0);
   EXPECT_TRUE (
      MacAddressMatch (&resulting_macaddr, &mock_data.interfaces[0].mac_address));
   EXPECT_EQ (mock_clal_get_ifname (ifindex, resulting_ifname), 0);
   EXPECT_EQ (strcmp (resulting_ifname, mock_data.interfaces[0].ifname), 0);
   EXPECT_EQ (mock_clal_get_netmask (ifindex, &resulting_netmask), 0);
   EXPECT_EQ (resulting_netmask, netmask);

   EXPECT_EQ (mock_clal_get_ifindex (ip_addr_b, &resulting_ifindex), 0);
   EXPECT_EQ (resulting_ifindex, ifindex_b);
   EXPECT_EQ (mock_clal_get_ifname (ifindex_b, resulting_ifname), 0);
   EXPECT_EQ (strcmp (resulting_ifname, mock_data.interfaces[1].ifname), 0);
   EXPECT_EQ (mock_clal_get_netmask (ifindex_b, &resulting_netmask), 0);
   EXPECT_EQ (resulting_netmask, netmask_b);

   EXPECT_EQ (
      mock_clal_set_ip_address_netmask (ifindex, new_ip_addr, new_netmask),
      0);

   EXPECT_EQ (mock_clal_get_ifindex (ip_addr, &resulting_ifindex), -1);
   EXPECT_EQ (mock_clal_get_ifindex (new_ip_addr, &resulting_ifindex), 0);
   EXPECT_EQ (resulting_ifindex, ifindex);
   EXPECT_EQ (mock_clal_get_ifname (ifindex, resulting_ifname), 0);
   EXPECT_EQ (strcmp (resulting_ifname, mock_data.interfaces[0].ifname), 0);
   EXPECT_EQ (mock_clal_get_netmask (ifindex, &resulting_netmask), 0);
   EXPECT_EQ (resulting_netmask, new_netmask);

   EXPECT_EQ (mock_clal_get_ifindex (ip_addr_b, &resulting_ifindex), 0);
   EXPECT_EQ (resulting_ifindex, ifindex_b);
   EXPECT_EQ (mock_clal_get_ifname (ifindex_b, resulting_ifname), 0);
   EXPECT_EQ (strcmp (resulting_ifname, mock_data.interfaces[1].ifname), 0);
   EXPECT_EQ (mock_clal_get_netmask (ifindex_b, &resulting_netmask), 0);
   EXPECT_EQ (resulting_netmask, netmask_b);
}

/**
 * Verify mocking of UDP ports
 *
 */
TEST_F (EthUnitTest, VerifyUdpMocks)
{
   int handle_a;
   int handle_b;
   uint32_t resulting_ip               = CL_IPADDR_INVALID;
   uint32_t resulting_local_ip         = CL_IPADDR_INVALID;
   uint16_t resulting_port             = 0;
   int resulting_ifindex               = 0;
   uint8_t receive_buffer[100]         = {0};
   cl_mock_udp_port_t * mock_slmp_port = &mock_data.udp_ports[0];

   const cl_ipaddr_t ip_address        = 0x01020307U;
   const cl_ipaddr_t broadcast_address = 0x010203FFU;
   const cl_ipaddr_t remote_ip         = 0x02030405U;
   const uint16_t remote_port          = 0xabcdU;
   const size_t datasize_in            = 5U;
   const size_t datasize_out           = 7U;

   EXPECT_FALSE (mock_data.udp_ports[0].in_use);
   EXPECT_FALSE (mock_data.udp_ports[1].in_use);
   EXPECT_FALSE (mock_data.udp_ports[2].in_use);
   EXPECT_FALSE (mock_data.udp_ports[3].in_use);
   EXPECT_FALSE (mock_slmp_port->is_open);

   /* Fails to open */
   mock_slmp_port->in_use      = true;
   mock_slmp_port->is_open     = false;
   mock_slmp_port->ip_addr     = ip_address;
   mock_slmp_port->port_number = CL_SLMP_PORT;
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 0U);
   mock_slmp_port->will_fail_open = true;
   EXPECT_EQ (mock_clal_udp_open (ip_address, CL_SLMP_PORT), -1);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1U);
   mock_slmp_port->will_fail_open = false;

   /* Open a socket */
   handle_a = mock_clal_udp_open (ip_address, CL_SLMP_PORT);
   EXPECT_EQ (handle_a, 1);
   EXPECT_TRUE (mock_data.udp_ports[0].in_use);
   EXPECT_TRUE (mock_data.udp_ports[0].is_open);
   EXPECT_FALSE (mock_data.udp_ports[1].in_use);
   EXPECT_FALSE (mock_data.udp_ports[1].is_open);
   EXPECT_FALSE (mock_data.udp_ports[2].in_use);
   EXPECT_FALSE (mock_data.udp_ports[2].is_open);
   EXPECT_FALSE (mock_data.udp_ports[3].in_use);
   EXPECT_FALSE (mock_data.udp_ports[3].is_open);
   EXPECT_EQ (mock_data.udp_ports[0].handle_number, handle_a);
   EXPECT_EQ (mock_data.udp_ports[0].ip_addr, ip_address);
   EXPECT_EQ (mock_data.udp_ports[0].port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_data.udp_ports[0].ifindex, 7);
   EXPECT_TRUE (mock_slmp_port->is_open);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 2U);

   /* Find simulated UDP port via handle number */
   EXPECT_EQ (mock_find_simulated_udp_port (handle_a), mock_slmp_port);
   EXPECT_EQ (mock_find_simulated_udp_port (-123), nullptr);
   EXPECT_EQ (mock_find_simulated_udp_port (12345), nullptr);
   EXPECT_EQ (
      mock_find_simulated_udp_port (NELEMENTS (mock_data.udp_ports) + 1U),
      nullptr);

   /* Already open */
   EXPECT_EQ (mock_clal_udp_open (ip_address, CL_SLMP_PORT), -1);

   /* Open more sockets */
   handle_b = mock_clal_udp_open (ip_address, CL_CCIEFB_PORT);
   EXPECT_EQ (handle_b, 2);
   EXPECT_EQ (mock_clal_udp_open (CL_IPADDR_ANY, CL_SLMP_PORT), 3);
   EXPECT_EQ (mock_clal_udp_open (CL_IPADDR_ANY, CL_SLMP_PORT), 4);
   EXPECT_EQ (mock_clal_udp_open (CL_IPADDR_ANY, CL_SLMP_PORT), 5);
   EXPECT_EQ (mock_clal_udp_open (CL_IPADDR_ANY, CL_CCIEFB_PORT), 6);
   EXPECT_EQ (mock_clal_udp_open (CL_IPADDR_ANY, CL_CCIEFB_PORT), 7);
   EXPECT_EQ (mock_clal_udp_open (broadcast_address, CL_CCIEFB_PORT), 8);
   EXPECT_EQ (mock_clal_udp_open (broadcast_address, CL_CCIEFB_PORT), 9);
   EXPECT_EQ (mock_clal_udp_open (broadcast_address, CL_CCIEFB_PORT), 10);

   /* No more sockets available */
   EXPECT_EQ (mock_clal_udp_open (CL_IPADDR_ANY, 1234), -1);

   /* Verify sockets datastructure */
   EXPECT_TRUE (mock_data.udp_ports[0].in_use);
   EXPECT_TRUE (mock_data.udp_ports[0].is_open);
   EXPECT_EQ (mock_data.udp_ports[0].handle_number, handle_a);
   EXPECT_EQ (mock_data.udp_ports[0].ip_addr, ip_address);
   EXPECT_EQ (mock_data.udp_ports[0].port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_data.udp_ports[0].ifindex, 7);
   EXPECT_TRUE (mock_data.udp_ports[1].in_use);
   EXPECT_TRUE (mock_data.udp_ports[1].is_open);
   EXPECT_EQ (mock_data.udp_ports[1].handle_number, handle_b);
   EXPECT_EQ (mock_data.udp_ports[1].ip_addr, ip_address);
   EXPECT_EQ (mock_data.udp_ports[1].port_number, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_data.udp_ports[1].ifindex, 7);
   EXPECT_TRUE (mock_data.udp_ports[2].in_use);
   EXPECT_TRUE (mock_data.udp_ports[2].is_open);
   EXPECT_EQ (mock_data.udp_ports[2].handle_number, 3);
   EXPECT_EQ (mock_data.udp_ports[2].ip_addr, CL_IPADDR_ANY);
   EXPECT_EQ (mock_data.udp_ports[2].port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_data.udp_ports[2].ifindex, 0);
   EXPECT_TRUE (mock_data.udp_ports[3].in_use);
   EXPECT_TRUE (mock_data.udp_ports[3].is_open);
   EXPECT_EQ (mock_data.udp_ports[3].handle_number, 4);
   EXPECT_EQ (mock_data.udp_ports[3].ip_addr, CL_IPADDR_ANY);
   EXPECT_EQ (mock_data.udp_ports[3].port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_data.udp_ports[3].ifindex, 0);
   EXPECT_TRUE (mock_data.udp_ports[7].in_use);
   EXPECT_TRUE (mock_data.udp_ports[7].is_open);
   EXPECT_EQ (mock_data.udp_ports[7].handle_number, 8);
   EXPECT_EQ (mock_data.udp_ports[7].ip_addr, broadcast_address);
   EXPECT_EQ (mock_data.udp_ports[7].port_number, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_data.udp_ports[7].ifindex, 7);

   /* Send data */
   EXPECT_EQ (mock_slmp_port->output_data_size, 0U);
   EXPECT_EQ (mock_slmp_port->remote_destination_ip, CL_IPADDR_INVALID);
   EXPECT_EQ (mock_slmp_port->remote_destination_ip, CL_IPADDR_INVALID);
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_slmp_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_slmp_port->output_data_size, 0U);
   EXPECT_EQ (
      mock_clal_udp_sendto (
         handle_a,
         remote_ip,
         remote_port,
         (uint8_t *)"JKLMNOPQRST",
         datasize_out),
      (ssize_t)datasize_out);
   EXPECT_EQ (mock_slmp_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_slmp_port->remote_destination_port, remote_port);
   EXPECT_EQ (mock_slmp_port->output_data_size, datasize_out);
   EXPECT_EQ (mock_slmp_port->udp_output_buffer[0], 'J');
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_slmp_port->total_sent_bytes, datasize_out);
   EXPECT_EQ (mock_slmp_port->output_data_size, datasize_out);

   /* Sending via wrong socket */
   EXPECT_EQ (
      mock_clal_udp_sendto (
         123,
         remote_ip,
         remote_port,
         (uint8_t *)"JKLMNOPQRST",
         datasize_out),
      -1);

   /* Fails to send */
   mock_slmp_port->output_data_size = 0U;
   mock_slmp_port->will_fail_send   = true;
   EXPECT_EQ (
      mock_clal_udp_sendto (
         handle_a,
         remote_ip,
         remote_port,
         (uint8_t *)"JKLMNOPQRST",
         datasize_out),
      -1);
   EXPECT_EQ (mock_slmp_port->output_data_size, 0U);

   mock_slmp_port->will_fail_send = false;
   EXPECT_EQ (
      mock_clal_udp_sendto (
         handle_a,
         remote_ip,
         remote_port,
         (uint8_t *)"MNOPQRSTUVXY",
         datasize_out),
      (ssize_t)datasize_out);
   EXPECT_EQ (mock_slmp_port->output_data_size, datasize_out);
   EXPECT_EQ (mock_slmp_port->udp_output_buffer[0], 'M');
   EXPECT_EQ (mock_slmp_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_slmp_port->total_sent_bytes, 2 * datasize_out);

   /* Read data (no data available) */
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 0);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_slmp_port->input_data_size, 0U);
   EXPECT_EQ (
      mock_clal_udp_recvfrom (
         handle_a,
         &resulting_ip,
         &resulting_port,
         receive_buffer,
         sizeof (receive_buffer)),
      0);
   EXPECT_EQ (resulting_ip, CL_IPADDR_INVALID);
   EXPECT_EQ (resulting_port, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_slmp_port->input_data_size, 0U);

   /* Read data (invalid socket) */
   EXPECT_EQ (
      mock_clal_udp_recvfrom (
         0,
         &resulting_ip,
         &resulting_port,
         receive_buffer,
         sizeof (receive_buffer)),
      -1);
   EXPECT_EQ (resulting_ip, CL_IPADDR_INVALID);
   EXPECT_EQ (resulting_port, 0);

   /* Populate the mocked port with incoming data */
   EXPECT_EQ (mock_slmp_port->remote_source_ip, CL_IPADDR_INVALID);
   EXPECT_EQ (mock_slmp_port->remote_source_port, 0);
   EXPECT_EQ (mock_slmp_port->input_data_size, 0U);
   mock_set_udp_fakedata (
      mock_slmp_port,
      remote_ip,
      remote_port,
      (uint8_t *)"DEFGHIJKLMN",
      datasize_in);
   EXPECT_EQ (mock_slmp_port->remote_source_ip, remote_ip);
   EXPECT_EQ (mock_slmp_port->remote_source_port, remote_port);
   EXPECT_EQ (mock_slmp_port->input_data_size, datasize_in);
   EXPECT_EQ (mock_slmp_port->udp_input_buffer[0], 'D');
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_slmp_port->input_data_size, datasize_in);

   /* Read data */
   EXPECT_EQ (
      mock_clal_udp_recvfrom (
         handle_a,
         &resulting_ip,
         &resulting_port,
         receive_buffer,
         sizeof (receive_buffer)),
      (ssize_t)datasize_in);
   EXPECT_EQ (receive_buffer[0], 'D');
   EXPECT_EQ (resulting_ip, remote_ip);
   EXPECT_EQ (resulting_port, remote_port);
   EXPECT_EQ (mock_slmp_port->input_data_size, 0U);
   EXPECT_EQ (mock_slmp_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_slmp_port->total_recv_bytes, datasize_in);
   EXPECT_EQ (mock_slmp_port->input_data_size, 0U);

   /* Read data with ifindex */
   mock_slmp_port->input_data_size = datasize_in;
   resulting_ip                    = CL_IPADDR_INVALID;
   resulting_port                  = 0;
   EXPECT_EQ (
      mock_clal_udp_recvfrom_with_ifindex (
         handle_a,
         &resulting_ip,
         &resulting_port,
         &resulting_local_ip,
         &resulting_ifindex,
         receive_buffer,
         sizeof (receive_buffer)),
      (ssize_t)datasize_in);
   EXPECT_EQ (receive_buffer[0], 'D');
   EXPECT_EQ (resulting_ip, remote_ip);
   EXPECT_EQ (resulting_port, remote_port);
   EXPECT_EQ (resulting_local_ip, ip_address);
   EXPECT_EQ (resulting_ifindex, 7);
   EXPECT_EQ (mock_slmp_port->input_data_size, 0U);

   /* Fail to read data with ifindex  */
   mock_slmp_port->input_data_size = datasize_in;
   mock_slmp_port->will_fail_recv  = true;
   resulting_ip                    = CL_IPADDR_INVALID;
   resulting_port                  = 0;
   EXPECT_EQ (
      mock_clal_udp_recvfrom_with_ifindex (
         handle_a,
         &resulting_ip,
         &resulting_port,
         &resulting_local_ip,
         &resulting_ifindex,
         receive_buffer,
         sizeof (receive_buffer)),
      -1);
   EXPECT_EQ (mock_slmp_port->input_data_size, datasize_in);
   mock_slmp_port->will_fail_recv = false;
   EXPECT_EQ (
      mock_clal_udp_recvfrom_with_ifindex (
         handle_a,
         &resulting_ip,
         &resulting_port,
         &resulting_local_ip,
         &resulting_ifindex,
         receive_buffer,
         sizeof (receive_buffer)),
      (ssize_t)datasize_in);
   EXPECT_EQ (resulting_ip, remote_ip);
   EXPECT_EQ (resulting_port, remote_port);
   EXPECT_EQ (mock_slmp_port->input_data_size, 0U);

   /* Read data with ifindex, small buffer */
   mock_slmp_port->input_data_size = datasize_in;
   resulting_ip                    = CL_IPADDR_INVALID;
   resulting_port                  = 0;
   EXPECT_EQ (
      mock_clal_udp_recvfrom_with_ifindex (
         handle_a,
         &resulting_ip,
         &resulting_port,
         &resulting_local_ip,
         &resulting_ifindex,
         receive_buffer,
         2),
      (ssize_t)2);
   EXPECT_EQ (receive_buffer[0], 'D');
   EXPECT_EQ (receive_buffer[1], 'E');
   EXPECT_EQ (resulting_ip, remote_ip);
   EXPECT_EQ (resulting_port, remote_port);
   EXPECT_EQ (resulting_local_ip, ip_address);
   EXPECT_EQ (resulting_ifindex, 7);
   EXPECT_EQ (mock_slmp_port->input_data_size, 0U);

   /* Trying to read form closed port */
   mock_slmp_port->input_data_size = datasize_in;
   mock_slmp_port->is_open         = false;
   resulting_ip                    = CL_IPADDR_INVALID;
   resulting_port                  = 0;
   EXPECT_EQ (
      mock_clal_udp_recvfrom_with_ifindex (
         handle_a,
         &resulting_ip,
         &resulting_port,
         &resulting_local_ip,
         &resulting_ifindex,
         receive_buffer,
         sizeof (receive_buffer)),
      -1);
   EXPECT_EQ (mock_slmp_port->input_data_size, datasize_in);
   mock_slmp_port->is_open = true;
   EXPECT_EQ (
      mock_clal_udp_recvfrom_with_ifindex (
         handle_a,
         &resulting_ip,
         &resulting_port,
         &resulting_local_ip,
         &resulting_ifindex,
         receive_buffer,
         sizeof (receive_buffer)),
      (ssize_t)datasize_in);
   EXPECT_EQ (resulting_ip, remote_ip);
   EXPECT_EQ (resulting_port, remote_port);
   EXPECT_EQ (mock_slmp_port->input_data_size, 0U);

   /* Close invalid socket */
   EXPECT_EQ (mock_slmp_port->number_of_calls_close, 0);
   EXPECT_TRUE (mock_data.udp_ports[0].is_open);
   EXPECT_TRUE (mock_data.udp_ports[1].is_open);
   EXPECT_TRUE (mock_data.udp_ports[2].is_open);
   EXPECT_TRUE (mock_data.udp_ports[3].is_open);
   mock_clal_udp_close (0);
   EXPECT_TRUE (mock_data.udp_ports[0].is_open);
   EXPECT_TRUE (mock_data.udp_ports[1].is_open);
   EXPECT_TRUE (mock_data.udp_ports[2].is_open);
   EXPECT_TRUE (mock_data.udp_ports[3].is_open);
   EXPECT_EQ (mock_slmp_port->number_of_calls_close, 0);

   /* Close socket (some mock info should remain) */
   mock_slmp_port->output_data_size = datasize_out;
   EXPECT_TRUE (mock_slmp_port->is_open);
   mock_clal_udp_close (handle_a);
   EXPECT_FALSE (mock_data.udp_ports[0].is_open);
   EXPECT_TRUE (mock_data.udp_ports[1].is_open);
   EXPECT_TRUE (mock_data.udp_ports[2].is_open);
   EXPECT_TRUE (mock_data.udp_ports[3].is_open);
   EXPECT_EQ (mock_slmp_port->number_of_calls_close, 1);
   EXPECT_FALSE (mock_slmp_port->is_open);
   EXPECT_EQ (mock_slmp_port->remote_source_ip, CL_IPADDR_INVALID);
   EXPECT_EQ (mock_slmp_port->remote_source_port, 0);
   EXPECT_EQ (mock_slmp_port->input_data_size, 0U);
   EXPECT_EQ (mock_slmp_port->output_data_size, datasize_out);
   EXPECT_EQ (mock_slmp_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_slmp_port->remote_destination_port, remote_port);

   /* Close other socket */
   mock_clal_udp_close (handle_b);
   EXPECT_FALSE (mock_data.udp_ports[0].is_open);
   EXPECT_FALSE (mock_data.udp_ports[1].is_open);
   EXPECT_TRUE (mock_data.udp_ports[2].is_open);
   EXPECT_TRUE (mock_data.udp_ports[3].is_open);
}
