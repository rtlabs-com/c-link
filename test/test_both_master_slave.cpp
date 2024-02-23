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
#include "common/cl_file.h"
#include "common/cl_iefb.h"
#include "common/cl_util.h"
#include "master/clm_iefb.h"
#include "master/clm_master.h"
#include "slave/cls_slmp.h"

#include "mocks.h"
#include "utils_for_testing.h"

#include <gtest/gtest.h>

/* TODO Suggestions for more integration tests:
 *    -  Use a slave and a master in the same binary, where the
 *       slave communicates with a simulated PLC and the
 *       master communicates with an external slave.
 *       Verify that CCIEFB cyclic data can be sent from simulated
 *       PLC to slave to master and to the external slave.
 *       Verify cyclic data in the other direction.
 *    -  Use a slave and a master in the same binary, where the
 *       slave communicates with a simulated PLC and the
 *       master communicates with an external slave.
 *       Verify that the master responds to SLMP node search from
 *       the simulated PLC.
 *       Verify that the master can do node search to simulated
 *       external slaves.
 */

/* For test fixtures suitable for master integration testing, see
 * utils_for_testing.h */

/**
 * A simulated master communicates with a simulated slave via CCIEFB
 *
 * @req REQ_CL_DEPLOYMENT_01
 */
TEST_F (MasterUnitTest, CciefbCommunicationBetweenMasterAndSlave)
{
   ASSERT_GE (CLS_MAX_OCCUPIED_STATIONS, 1);

   cls_t cls                                     = {};
   cl_ipaddr_t slave_ip                          = 0x01080304; /* IP 1.8.3.4 */
   cl_ipaddr_t slave_broadcast                   = 0x010803FF; /* 1.8.3.255*/
   cl_ipaddr_t master_netmask                    = 0xFFFFFF00;
   uint16_t slave_di                             = 0;
   uint16_t address_rww                          = 2;
   uint16_t address_rwr                          = 3;
   uint16_t value_rww                            = 0x8765;
   uint16_t value_rwr                            = 0x5432;
   cl_mock_udp_port_t * slave_cciefb_port        = &mock_data.udp_ports[0];
   cl_mock_udp_port_t * slave_slmp_receive_port  = &mock_data.udp_ports[1];
   cl_mock_udp_port_t * master_cciefb_port       = &mock_data.udp_ports[2];
   cl_mock_udp_port_t * master_slmp_receive_port = &mock_data.udp_ports[3];

   /* Slave configuration */
   cls_cfg_t slave_config = {};
   clal_clear_memory (&slave_config, sizeof (slave_config));
   slave_config.num_occupied_stations = 1;
   slave_config.ip_setting_allowed    = true;
   slave_config.vendor_code           = 0x3456;
   slave_config.model_code            = 0x789ABCDE;
   slave_config.equipment_ver         = 0xF012;
   slave_config.connect_cb            = my_slave_connect_ind;
   slave_config.disconnect_cb         = my_slave_disconnect_ind;
   slave_config.state_cb              = my_slave_state_ind;
   slave_config.master_running_cb     = my_slave_master_running_ind;
   slave_config.node_search_cb        = my_slave_node_search_ind;
   slave_config.set_ip_cb             = my_slave_set_ip_ind;
   slave_config.cb_arg                = &mock_data;
   slave_config.iefb_ip_addr          = slave_broadcast;
   slave_cciefb_port->local_ip_addr   = slave_ip;

   /* Master configuration */
   config.hier.groups[0].num_slave_devices         = 1;
   config.hier.groups[0].slave_devices[0].slave_id = slave_ip;
   mock_data.interfaces[0].netmask                 = master_netmask;

   ASSERT_FALSE (mock_data.udp_ports[0].in_use);
   ASSERT_FALSE (mock_data.udp_ports[1].in_use);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);
   ASSERT_FALSE (mock_data.udp_ports[4].in_use);

   /* Start slave */
   (void)cls_slave_init (&cls, &slave_config, now);
   now += tick_size;
   cls_iefb_periodic (&cls, now);

   ASSERT_TRUE (mock_data.udp_ports[0].in_use);
   ASSERT_TRUE (mock_data.udp_ports[1].in_use);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);
   ASSERT_FALSE (mock_data.udp_ports[4].in_use);
   ASSERT_TRUE (slave_cciefb_port->is_open);
   ASSERT_TRUE (slave_slmp_receive_port->is_open);

   /* Set input cyclic data to master */
   cls_set_rwr_value (&cls, address_rwr, value_rwr);
   EXPECT_EQ (cls_get_rwr_value (&cls, address_rwr), value_rwr);

   /* Start master */
   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);
   now += tick_size;
   cls_iefb_periodic (&cls, now);
   clm_iefb_periodic (&clm, now);

   ASSERT_TRUE (mock_data.udp_ports[0].in_use);
   ASSERT_TRUE (mock_data.udp_ports[1].in_use);
   ASSERT_TRUE (mock_data.udp_ports[2].in_use);
   ASSERT_TRUE (mock_data.udp_ports[3].in_use);
   ASSERT_FALSE (mock_data.udp_ports[4].in_use);
   ASSERT_TRUE (master_cciefb_port->is_open);
   ASSERT_TRUE (master_slmp_receive_port->is_open);

   /* Set output cyclic data from master */
   clm_set_rww_value (&clm, gi, slave_di, address_rww, value_rww);
   EXPECT_EQ (clm_get_rww_value (&clm, gi, slave_di, address_rww), value_rww);

   /* Run master stack, arbitration done.
      Master sends request. */
   now += longer_than_arbitration_us;
   cls_iefb_periodic (&cls, now);
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (master_cciefb_port->output_data_size, SIZE_REQUEST_1_SLAVE);
   mock_transfer_udp_fakedata (slave_cciefb_port, master_cciefb_port);
   EXPECT_EQ (slave_cciefb_port->input_data_size, SIZE_REQUEST_1_SLAVE);

   /* Slave sends response */
   now += tick_size;
   cls_iefb_periodic (&cls, now);
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (slave_cciefb_port->output_data_size, SIZE_RESPONSE_1_SLAVE);
   mock_transfer_udp_fakedata (master_cciefb_port, slave_cciefb_port);
   EXPECT_EQ (master_cciefb_port->input_data_size, SIZE_RESPONSE_1_SLAVE);

   /* Master sends request */
   now += tick_size;
   cls_iefb_periodic (&cls, now);
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (master_cciefb_port->output_data_size, SIZE_REQUEST_1_SLAVE);
   mock_transfer_udp_fakedata (slave_cciefb_port, master_cciefb_port);
   EXPECT_EQ (slave_cciefb_port->input_data_size, SIZE_REQUEST_1_SLAVE);

   /* Slave sends response */
   now += tick_size;
   cls_iefb_periodic (&cls, now);
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (slave_cciefb_port->output_data_size, SIZE_RESPONSE_1_SLAVE);
   mock_transfer_udp_fakedata (master_cciefb_port, slave_cciefb_port);
   EXPECT_EQ (master_cciefb_port->input_data_size, SIZE_RESPONSE_1_SLAVE);

   /* Master sends request */
   now += tick_size;
   cls_iefb_periodic (&cls, now);
   clm_iefb_periodic (&clm, now);

   /* Validate transferred data */
   EXPECT_EQ (clm_get_rww_value (&clm, gi, slave_di, address_rww), value_rww);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, slave_di, address_rwr), value_rwr);

   EXPECT_EQ (cls_get_rwr_value (&cls, address_rwr), value_rwr);
   EXPECT_EQ (cls_get_rww_value (&cls, address_rww), value_rww);
}

/**
 * A simulated master communicates with a simulated slave via SLMP
 *
 * Performs node search and sets IP address of slave
 *
 * @req REQ_CL_DEPLOYMENT_01
 *
 */
TEST_F (MasterUnitTest, SlmpCommunicationBetweenMasterAndSlave)
{
   ASSERT_GE (CLS_MAX_OCCUPIED_STATIONS, 1);

   cls_t cls                   = {};
   uint16_t slave_ifindex      = 9;
   cl_ipaddr_t slave_ip        = 0x01080304; /* IP 1.8.3.4 */
   cl_ipaddr_t slave_netmask   = 0xFFFFFF00;
   cl_ipaddr_t slave_broadcast = 0x010803FF; /* 1.8.3.255*/
   cl_ipaddr_t slave_new_ip    = 0x01080309;
   cl_ipaddr_t master_netmask  = 0xFFFFFF00;
   auto * master_macaddr = (cl_macaddr_t *)mock_data.interfaces[0].mac_address;
   auto * slave_macaddr  = (cl_macaddr_t *)mock_data.interfaces[1].mac_address;
   cl_mock_udp_port_t * slave_cciefb_port        = &mock_data.udp_ports[0];
   cl_mock_udp_port_t * slave_slmp_receive_port  = &mock_data.udp_ports[1];
   cl_mock_udp_port_t * master_cciefb_port       = &mock_data.udp_ports[2];
   cl_mock_udp_port_t * master_slmp_receive_port = &mock_data.udp_ports[3];
   cl_mock_udp_port_t * master_slmp_send_port    = &mock_data.udp_ports[4];
   cl_mock_udp_port_t * slave_slmp_send_port     = &mock_data.udp_ports[5];
   cl_mock_udp_port_t * slave_new_slmp_send_port = &mock_data.udp_ports[6];

   /* Slave configuration */
   cls_cfg_t slave_config = {};
   clal_clear_memory (&slave_config, sizeof (slave_config));
   slave_config.num_occupied_stations     = 1;
   slave_config.ip_setting_allowed        = true;
   slave_config.vendor_code               = 0x3456;
   slave_config.model_code                = 0x789ABCDE;
   slave_config.equipment_ver             = 0xF012;
   slave_config.connect_cb                = my_slave_connect_ind;
   slave_config.disconnect_cb             = my_slave_disconnect_ind;
   slave_config.state_cb                  = my_slave_state_ind;
   slave_config.master_running_cb         = my_slave_master_running_ind;
   slave_config.node_search_cb            = my_slave_node_search_ind;
   slave_config.set_ip_cb                 = my_slave_set_ip_ind;
   slave_config.cb_arg                    = &mock_data;
   slave_config.iefb_ip_addr              = slave_broadcast;
   slave_cciefb_port->local_ip_addr       = slave_ip;
   slave_slmp_receive_port->local_ip_addr = slave_ip;
   slave_slmp_receive_port->local_ifindex = slave_ifindex;

   /* Master configuration */
   config.callback_time_node_search                = 2000;
   config.callback_time_set_ip                     = 500;
   config.hier.groups[0].num_slave_devices         = 1;
   config.hier.groups[0].slave_devices[0].slave_id = slave_ip;
   mock_data.interfaces[0].netmask                 = master_netmask;
   master_slmp_receive_port->local_ip_addr         = my_ip;
   master_slmp_receive_port->local_ifindex         = (uint16_t)my_ifindex;

   ASSERT_FALSE (mock_data.udp_ports[0].in_use);
   ASSERT_FALSE (mock_data.udp_ports[1].in_use);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);
   ASSERT_FALSE (mock_data.udp_ports[4].in_use);

   /* Start slave */
   (void)cls_slave_init (&cls, &slave_config, now);
   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now);

   EXPECT_TRUE (mock_data.udp_ports[0].in_use);
   EXPECT_TRUE (mock_data.udp_ports[1].in_use);
   EXPECT_FALSE (mock_data.udp_ports[2].in_use);
   EXPECT_FALSE (mock_data.udp_ports[3].in_use);
   EXPECT_FALSE (mock_data.udp_ports[4].in_use);
   EXPECT_TRUE (slave_cciefb_port->is_open);
   EXPECT_TRUE (slave_slmp_receive_port->is_open);

   /* Start master */
   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);
   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now);
   clm_slmp_periodic (&clm, now);
   clm_iefb_periodic (&clm, now);

   EXPECT_TRUE (mock_data.udp_ports[0].in_use);
   EXPECT_TRUE (mock_data.udp_ports[1].in_use);
   EXPECT_TRUE (mock_data.udp_ports[2].in_use);
   EXPECT_TRUE (mock_data.udp_ports[3].in_use);
   EXPECT_FALSE (mock_data.udp_ports[4].in_use);
   EXPECT_TRUE (master_cciefb_port->is_open);
   EXPECT_TRUE (master_slmp_receive_port->is_open);
   EXPECT_FALSE (master_slmp_send_port->is_open);

   /* Run master stack, arbitration done.
      Master sends request. */
   now += longer_than_arbitration_us;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now);
   clm_slmp_periodic (&clm, now);
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (master_cciefb_port->output_data_size, SIZE_REQUEST_1_SLAVE);

   /* Trigger node search from master */
   EXPECT_EQ (clm_slmp_perform_node_search (&clm, now), 0);
   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now);
   clm_slmp_periodic (&clm, now);
   clm_iefb_periodic (&clm, now);

   EXPECT_TRUE (master_slmp_send_port->in_use);
   EXPECT_FALSE (master_slmp_send_port->is_open);
   EXPECT_EQ (master_slmp_send_port->output_data_size, SIZE_REQUEST_NODE_SEARCH);
   mock_transfer_udp_fakedata (slave_slmp_receive_port, master_slmp_send_port);
   EXPECT_EQ (slave_slmp_receive_port->input_data_size, SIZE_REQUEST_NODE_SEARCH);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 0);

   /* Slave receives node search, no response is sent yet. */
   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now);
   clm_slmp_periodic (&clm, now);
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.master_ip_addr, my_ip);
   EXPECT_EQ (mock_data.slave_cb_nodesearch.master_ip_addr, my_ip);
   EXPECT_TRUE (MacAddressMatch (
      &mock_data.slave_cb_nodesearch.master_mac_addr,
      master_macaddr));

   /* Slave sends response after delay */
   now += long_enough_for_nodesearch;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now);
   clm_slmp_periodic (&clm, now);
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (slave_slmp_send_port->output_data_size, SIZE_RESPONSE_NODE_SEARCH);
   mock_transfer_udp_fakedata (master_slmp_receive_port, slave_slmp_send_port);
   EXPECT_EQ (master_slmp_receive_port->input_data_size, SIZE_RESPONSE_NODE_SEARCH);

   /* Master receives node search response */
   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now);
   clm_slmp_periodic (&clm, now);
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);

   /* Node search finished callback has been triggered on master */
   now += longer_than_arbitration_us;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now);
   clm_slmp_periodic (&clm, now);
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.count, 1);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.stored, 1);
   EXPECT_EQ (cb_counters->master_cb_node_search.db.entries[0].slave_id, slave_ip);
   EXPECT_EQ (
      cb_counters->master_cb_node_search.db.entries[0].slave_netmask,
      slave_netmask);
   EXPECT_TRUE (MacAddressMatch (
      &cb_counters->master_cb_node_search.db.entries[0].slave_mac_addr,
      slave_macaddr));
   EXPECT_EQ (
      cb_counters->master_cb_node_search.db.entries[0].vendor_code,
      slave_config.vendor_code);
   EXPECT_EQ (
      cb_counters->master_cb_node_search.db.entries[0].model_code,
      slave_config.model_code);
   EXPECT_EQ (
      cb_counters->master_cb_node_search.db.entries[0].equipment_ver,
      slave_config.equipment_ver);

   /* Master sends Set IP command */
   EXPECT_EQ (
      clm_slmp_perform_set_ipaddr_request (
         &clm,
         now,
         slave_macaddr,
         slave_new_ip,
         slave_netmask),
      0);
   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now);
   clm_slmp_periodic (&clm, now);
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (master_slmp_send_port->output_data_size, SIZE_REQUEST_SET_IP);
   mock_transfer_udp_fakedata (slave_slmp_receive_port, master_slmp_send_port);
   EXPECT_EQ (slave_slmp_receive_port->input_data_size, SIZE_REQUEST_SET_IP);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 0);

   /* Slave receives 'set IP', response with error (not allowed in config) */
   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now);
   clm_slmp_periodic (&clm, now);
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (mock_data.slave_cb_set_ip.calls, 1);
   EXPECT_TRUE (MacAddressMatch (
      &mock_data.slave_cb_set_ip.master_mac_addr,
      master_macaddr));
   EXPECT_EQ (mock_data.slave_cb_set_ip.master_ip_addr, my_ip);
   EXPECT_EQ (mock_data.slave_cb_set_ip.new_ip_addr, slave_new_ip);
   EXPECT_EQ (mock_data.slave_cb_set_ip.new_netmask, slave_netmask);
   EXPECT_EQ (mock_data.slave_cb_set_ip.ip_setting_allowed, true);
   EXPECT_EQ (mock_data.slave_cb_set_ip.did_set_ip, true);

   EXPECT_EQ (slave_new_slmp_send_port->output_data_size, SIZE_RESPONSE_SET_IP);
   mock_transfer_udp_fakedata (master_slmp_receive_port, slave_new_slmp_send_port);
   EXPECT_EQ (master_slmp_receive_port->input_data_size, SIZE_RESPONSE_SET_IP);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Master receives set IP error response */
   now += tick_size;
   cls_slmp_periodic (&cls, now);
   cls_iefb_periodic (&cls, now);
   clm_slmp_periodic (&clm, now);
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_set_ip.status,
      CLM_MASTER_SET_IP_STATUS_SUCCESS);
}
