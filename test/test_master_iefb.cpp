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

#include "mocks.h"
#include "utils_for_testing.h"

#include <gtest/gtest.h>

/* TODO Suggestions for more integration tests:
 *    -  Use 3 groups of slaves.
 *       Verify that same parameterID is used in all groups.
 *       Disable a single group and verify that communication
 *       continues to the other groups.
 *    -  Use two masters simultaneously on different simulated
 *       network interfaces.
 *       Verify the sent and received cyclic data for both masters.
 *       Verify that the parameterID values are independent.
 */

/* For test fixtures suitable for master integration testing, see
 * utils_for_testing.h */

/**
 * Validate frame sequence number rotation
 *
 * @req REQ_CLM_SEQUENCE_02
 *
 */
TEST_F (UnitTest, MasterUpdateFrameSequenceNo)
{
   uint16_t i = 0;

   EXPECT_EQ (i, 0);
   clm_iefb_update_frame_sequence_no (&i);
   EXPECT_EQ (i, 1);
   clm_iefb_update_frame_sequence_no (&i);
   EXPECT_EQ (i, 2);
   clm_iefb_update_frame_sequence_no (&i);
   EXPECT_EQ (i, 3);
   clm_iefb_update_frame_sequence_no (&i);
   EXPECT_EQ (i, 4);
   clm_iefb_update_frame_sequence_no (&i);
   EXPECT_EQ (i, 5);
   clm_iefb_update_frame_sequence_no (&i);
   EXPECT_EQ (i, 6);

   i = 0xFFFD;

   EXPECT_EQ (i, 0xFFFD);
   clm_iefb_update_frame_sequence_no (&i);
   EXPECT_EQ (i, 0xFFFE);
   clm_iefb_update_frame_sequence_no (&i);
   EXPECT_EQ (i, 0xFFFF);
   clm_iefb_update_frame_sequence_no (&i);
   EXPECT_EQ (i, 1);
   clm_iefb_update_frame_sequence_no (&i);
   EXPECT_EQ (i, 2);
   clm_iefb_update_frame_sequence_no (&i);
}

TEST_F (UnitTest, StatemachinesShouldHandleLastState)
{
   /* Should not act on dummy event.
      Prove this by not crashing on nullptr arguments */
   clm_iefb_group_fsm_event (nullptr, 0, nullptr, CLM_GROUP_EVENT_LAST);
   clm_iefb_device_fsm_event (nullptr, 0, nullptr, nullptr, CLM_DEVICE_EVENT_LAST);
}

/**
 * Verify calculation of MasterLocalUnitInfo
 *
 * @req REQ_CL_PROTOCOL_13
 * @req REQ_CL_PROTOCOL_14
 * @req REQ_CL_PROTOCOL_15
 * @req REQ_CL_PROTOCOL_16
 * @req REQ_CL_PROTOCOL_56
 *
 */
TEST_F (UnitTest, CalcMasterLocalUnitInfo)
{
   /* No validation of protocol version */

   /* Protocol version 1 */
   EXPECT_EQ (
      clm_iefb_calc_master_local_unit_info (1, true, true),
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      clm_iefb_calc_master_local_unit_info (1, true, false),
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      clm_iefb_calc_master_local_unit_info (1, false, true),
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED);
   EXPECT_EQ (
      clm_iefb_calc_master_local_unit_info (1, false, false),
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED);

   /* Protocol version 2 */
   EXPECT_EQ (
      clm_iefb_calc_master_local_unit_info (2, true, true),
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      clm_iefb_calc_master_local_unit_info (2, true, false),
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      clm_iefb_calc_master_local_unit_info (2, false, true),
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED_BY_USER);
   EXPECT_EQ (
      clm_iefb_calc_master_local_unit_info (2, false, false),
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED);
}

TEST_F (UnitTest, CalcOccupiedPerGroup)
{
   clm_group_setting_t group_setting = {};

   /* Compile the tests with sufficiently large memory area */
   ASSERT_GE (CLM_MAX_OCCUPIED_STATIONS_PER_GROUP, 3);

   group_setting.parallel_off_timeout_count             = 3;
   group_setting.timeout_value                          = 500;
   group_setting.num_slave_devices                      = 2;
   group_setting.slave_devices[0].slave_id              = 0x02030405;
   group_setting.slave_devices[0].num_occupied_stations = 3;
   group_setting.slave_devices[1].slave_id              = 0x02030406;
   group_setting.slave_devices[1].num_occupied_stations = 7;

   EXPECT_EQ (clm_iefb_calc_occupied_per_group (&group_setting), 10);

   group_setting.num_slave_devices = 1;
   EXPECT_EQ (clm_iefb_calc_occupied_per_group (&group_setting), 3);

   group_setting.num_slave_devices = 0;
   EXPECT_EQ (clm_iefb_calc_occupied_per_group (&group_setting), 0);

   group_setting.num_slave_devices = 3;
   EXPECT_EQ (clm_iefb_calc_occupied_per_group (&group_setting), 10);
}

TEST_F (UnitTest, CalcSlaveStationNumber)
{
   clm_group_setting_t group_setting = {};
   uint16_t slave_station_no         = 0;

   /* Compile the tests with sufficiently large memory area */
   ASSERT_GE (CLM_MAX_OCCUPIED_STATIONS_PER_GROUP, 4);

   group_setting.parallel_off_timeout_count             = 3;
   group_setting.timeout_value                          = 500;
   group_setting.num_slave_devices                      = 3;
   group_setting.slave_devices[0].slave_id              = 0x02030405;
   group_setting.slave_devices[0].num_occupied_stations = 3;
   group_setting.slave_devices[1].slave_id              = 0x02030406;
   group_setting.slave_devices[1].num_occupied_stations = 7;
   group_setting.slave_devices[2].slave_id              = 0x02030407;
   group_setting.slave_devices[2].num_occupied_stations = 2;

   EXPECT_EQ (
      clm_iefb_calc_slave_station_no (&group_setting, 0, &slave_station_no),
      0);
   EXPECT_EQ (slave_station_no, 1);

   EXPECT_EQ (
      clm_iefb_calc_slave_station_no (&group_setting, 1, &slave_station_no),
      0);
   EXPECT_EQ (slave_station_no, 4);

   EXPECT_EQ (
      clm_iefb_calc_slave_station_no (&group_setting, 2, &slave_station_no),
      0);
   EXPECT_EQ (slave_station_no, 11);

   /* Invalid slave device index */
   EXPECT_EQ (
      clm_iefb_calc_slave_station_no (&group_setting, 3, &slave_station_no),
      -1);
}

TEST_F (UnitTest, SlaveDeviceStatistics)
{
   clm_slave_device_statistics_t stat = {};
   const uint16_t max_samples         = 5;

   /* Initialize */
   clm_iefb_statistics_clear (&stat);

   /* Response time statistics */
   EXPECT_EQ (stat.measured_time.number_of_samples, 0U);
   EXPECT_EQ (stat.measured_time.max, 0U);
   EXPECT_EQ (stat.measured_time.sum, 0U);
   EXPECT_EQ (stat.measured_time.average, 0U);
   clm_iefb_statistics_update_response_time (&stat, max_samples, 100);
   EXPECT_EQ (stat.measured_time.number_of_samples, 1U);
   EXPECT_EQ (stat.measured_time.min, 100U);
   EXPECT_EQ (stat.measured_time.max, 100U);
   EXPECT_EQ (stat.measured_time.sum, 100U);
   EXPECT_EQ (stat.measured_time.average, 100U);

   clm_iefb_statistics_update_response_time (&stat, max_samples, 300);
   EXPECT_EQ (stat.measured_time.number_of_samples, 2U);
   EXPECT_EQ (stat.measured_time.min, 100U);
   EXPECT_EQ (stat.measured_time.max, 300U);
   EXPECT_EQ (stat.measured_time.sum, 400U);
   EXPECT_EQ (stat.measured_time.average, 200U);

   clm_iefb_statistics_update_response_time (&stat, max_samples, 500);
   EXPECT_EQ (stat.measured_time.number_of_samples, 3U);
   EXPECT_EQ (stat.measured_time.min, 100U);
   EXPECT_EQ (stat.measured_time.max, 500U);
   EXPECT_EQ (stat.measured_time.sum, 900U);
   EXPECT_EQ (stat.measured_time.average, 300U);

   clm_iefb_statistics_update_response_time (&stat, max_samples, 300);
   EXPECT_EQ (stat.measured_time.number_of_samples, 4U);
   EXPECT_EQ (stat.measured_time.min, 100U);
   EXPECT_EQ (stat.measured_time.max, 500U);
   EXPECT_EQ (stat.measured_time.sum, 1200U);
   EXPECT_EQ (stat.measured_time.average, 300U);

   clm_iefb_statistics_update_response_time (&stat, max_samples, 50);
   EXPECT_EQ (stat.measured_time.number_of_samples, 5U);
   EXPECT_EQ (stat.measured_time.number_of_samples, max_samples);
   EXPECT_EQ (stat.measured_time.min, 50U);
   EXPECT_EQ (stat.measured_time.max, 500U);
   EXPECT_EQ (stat.measured_time.sum, 1250U);
   EXPECT_EQ (stat.measured_time.average, 250U);

   clm_iefb_statistics_update_response_time (&stat, max_samples, 200);
   EXPECT_EQ (stat.measured_time.number_of_samples, 5U);
   EXPECT_EQ (stat.measured_time.number_of_samples, max_samples);
   EXPECT_EQ (stat.measured_time.min, 50U);
   EXPECT_EQ (stat.measured_time.max, 500U);
   EXPECT_EQ (stat.measured_time.sum, 1250U);
   EXPECT_EQ (stat.measured_time.average, 250U);

   /* Clear statistics */
   clm_iefb_statistics_clear (&stat);
   EXPECT_EQ (stat.measured_time.number_of_samples, 0U);
   EXPECT_EQ (stat.measured_time.min, UINT32_MAX);
   EXPECT_EQ (stat.measured_time.max, 0U);
   EXPECT_EQ (stat.measured_time.sum, 0U);
   EXPECT_EQ (stat.measured_time.average, 0U);
   EXPECT_EQ (stat.number_of_sent_frames, 0U);
   EXPECT_EQ (stat.number_of_incoming_frames, 0U);
   EXPECT_EQ (stat.number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (stat.number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (stat.number_of_connects, 0U);
   EXPECT_EQ (stat.number_of_disconnects, 0U);
   EXPECT_EQ (stat.number_of_timeouts, 0U);

   /* Already large values in the response time statistics */
   stat.measured_time.sum = UINT32_MAX / 2;
   EXPECT_EQ (stat.measured_time.number_of_samples, 0U);
   clm_iefb_statistics_update_response_time (&stat, max_samples, 200);
   EXPECT_EQ (stat.measured_time.number_of_samples, 0U);
}

TEST_F (UnitTest, CalcSlaveDeviceIndex)
{
   clm_group_setting_t group_setting = {};
   uint16_t slave_device_index       = 0;

   /* Compile the tests with sufficiently large memory area */
   ASSERT_GE (CLM_MAX_OCCUPIED_STATIONS_PER_GROUP, 4);

   group_setting.parallel_off_timeout_count             = 3;
   group_setting.timeout_value                          = 500;
   group_setting.num_slave_devices                      = 3;
   group_setting.slave_devices[0].slave_id              = 0x02030405;
   group_setting.slave_devices[0].num_occupied_stations = 3;
   group_setting.slave_devices[1].slave_id              = 0x02030406;
   group_setting.slave_devices[1].num_occupied_stations = 7;
   group_setting.slave_devices[2].slave_id              = 0x02030407;
   group_setting.slave_devices[2].num_occupied_stations = 2;

   EXPECT_EQ (
      clm_iefb_calc_slave_device_index (
         &group_setting,
         0x02030405,
         &slave_device_index),
      0);
   EXPECT_EQ (slave_device_index, 0);

   EXPECT_EQ (
      clm_iefb_calc_slave_device_index (
         &group_setting,
         0x02030406,
         &slave_device_index),
      0);
   EXPECT_EQ (slave_device_index, 1);

   EXPECT_EQ (
      clm_iefb_calc_slave_device_index (
         &group_setting,
         0x02030407,
         &slave_device_index),
      0);
   EXPECT_EQ (slave_device_index, 2);

   /* Unknown IP address */
   EXPECT_EQ (
      clm_iefb_calc_slave_device_index (
         &group_setting,
         0x02030408,
         &slave_device_index),
      -1);
}

/**
 * Validate update slave IDs
 *
 * @req REQ_CL_PROTOCOL_34
 *
 */
TEST_F (UnitTest, UpdateSlaveIds)
{
   /* Compile the tests with sufficiently large memory area */
   ASSERT_GE (CLM_MAX_OCCUPIED_STATIONS_PER_GROUP, 8);

   uint8_t buffer[100] = {0};
   clm_group_data_t group_data;
   clm_group_setting_t group_setting;
   const uint16_t num_slave_devices = 3;
   int i                            = 0;

   clal_clear_memory (&group_data, sizeof (group_data));
   clal_clear_memory (&group_setting, sizeof (group_setting));

   group_setting.num_slave_devices                      = num_slave_devices;
   group_setting.slave_devices[0].num_occupied_stations = 1;
   group_setting.slave_devices[0].slave_id              = 0x01020304;
   group_setting.slave_devices[1].num_occupied_stations = 2;
   group_setting.slave_devices[1].slave_id              = 0x01020305;
   group_setting.slave_devices[2].num_occupied_stations = 5;
   group_setting.slave_devices[2].slave_id              = 0x01020306;
   group_data.group_index                               = 0;
   group_data.total_occupied                            = 8;
   group_data.cyclic_transmission_state =
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF;
   group_data.slave_devices[0].device_index     = 0;
   group_data.slave_devices[0].slave_station_no = 1;
   group_data.slave_devices[0].enabled          = true;
   group_data.slave_devices[0].device_state     = CLM_DEVICE_STATE_LISTEN;
   group_data.slave_devices[1].device_index     = 1;
   group_data.slave_devices[1].slave_station_no = 2;
   group_data.slave_devices[1].enabled          = true;
   group_data.slave_devices[1].device_state     = CLM_DEVICE_STATE_LISTEN;
   group_data.slave_devices[2].device_index     = 2;
   group_data.slave_devices[2].slave_station_no = 4;
   group_data.slave_devices[2].enabled          = true;
   group_data.slave_devices[2].device_state     = CLM_DEVICE_STATE_LISTEN;

   clm_iefb_initialise_request_slave_ids ((uint32_t *)buffer, &group_setting);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[0]), 0x01020304U);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[4]), 0x01020305U);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[8]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[12]), 0x01020306U);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[16]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[20]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[24]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[28]), 0xFFFFFFFFU);

   /* There should be no changes yet */
   for (i = 0; i < num_slave_devices; i++)
   {
      clm_iefb_update_request_slave_id (
         (uint32_t *)buffer,
         group_data.slave_devices[i].slave_station_no,
         group_data.slave_devices[i].enabled,
         group_setting.slave_devices[i].slave_id);
   }
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[0]), 0x01020304U);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[4]), 0x01020305U);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[8]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[12]), 0x01020306U);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[16]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[20]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[24]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[28]), 0xFFFFFFFFU);

   /* Disable slave device number 3 */
   group_data.slave_devices[2].enabled = false;
   for (i = 0; i < num_slave_devices; i++)
   {
      clm_iefb_update_request_slave_id (
         (uint32_t *)buffer,
         group_data.slave_devices[i].slave_station_no,
         group_data.slave_devices[i].enabled,
         group_setting.slave_devices[i].slave_id);
   }
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[0]), 0x01020304U);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[4]), 0x01020305U);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[8]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[12]), 0x00000000U);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[16]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[20]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[24]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[28]), 0xFFFFFFFFU);

   /* Enable slave device number 3 */
   group_data.slave_devices[2].enabled = true;
   for (i = 0; i < num_slave_devices; i++)
   {
      clm_iefb_update_request_slave_id (
         (uint32_t *)buffer,
         group_data.slave_devices[i].slave_station_no,
         group_data.slave_devices[i].enabled,
         group_setting.slave_devices[i].slave_id);
   }
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[0]), 0x01020304U);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[4]), 0x01020305U);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[8]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[12]), 0x01020306U);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[16]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[20]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[24]), 0xFFFFFFFFU);
   EXPECT_EQ (CC_FROM_LE32 (*(uint32_t *)&buffer[28]), 0xFFFFFFFFU);
}

/**
 * Verify mechanism to decide if we should wait for more slave responses
 *
 * @req REQ_CLM_TIMING_07
 * @req REQ_CLM_TIMING_08
 * @req REQ_CLM_TIMING_09
 * @req REQ_CLM_TIMING_10
 */
TEST_F (MasterUnitTest, CciefbReceivedFromAllDevices)
{
   const clm_group_setting_t * group_setting = &clm.config.hier.groups[gi];
   clm_group_data_t * group_data             = &clm.groups[gi];

   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);
   ASSERT_TRUE (group_data->slave_devices[sdi0].enabled);
   ASSERT_EQ (
      group_data->slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   ASSERT_TRUE (group_data->slave_devices[sdi].enabled);
   ASSERT_EQ (group_data->slave_devices[sdi].device_state, CLM_DEVICE_STATE_LISTEN);
   ASSERT_EQ (group_data->frame_sequence_no, 0);

   /* First link scan (frame sequence 0) */
   group_data->slave_devices[sdi0].transmission_bit = false;
   group_data->slave_devices[sdi0].device_state     = CLM_DEVICE_STATE_WAIT_TD;
   group_data->slave_devices[sdi].transmission_bit  = false;
   group_data->slave_devices[sdi].device_state      = CLM_DEVICE_STATE_WAIT_TD;
   EXPECT_FALSE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   group_data->slave_devices[sdi0].transmission_bit = false;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   group_data->slave_devices[sdi].transmission_bit = false;
   group_data->slave_devices[sdi].device_state     = CLM_DEVICE_STATE_WAIT_TD;
   EXPECT_FALSE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   group_data->slave_devices[sdi0].transmission_bit = false;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SUSPEND;
   group_data->slave_devices[sdi].transmission_bit = false;
   group_data->slave_devices[sdi].device_state = CLM_DEVICE_STATE_CYCLIC_SUSPEND;
   EXPECT_FALSE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   group_data->slave_devices[sdi0].transmission_bit = false;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SUSPEND;
   group_data->slave_devices[sdi].transmission_bit = false;
   group_data->slave_devices[sdi].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   EXPECT_TRUE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   group_data->slave_devices[sdi0].transmission_bit = false;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   group_data->slave_devices[sdi].transmission_bit = false;
   group_data->slave_devices[sdi].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   EXPECT_TRUE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   /* Consecutive link scans (frame sequence != 0) */
   group_data->frame_sequence_no = 1;

   group_data->slave_devices[sdi0].transmission_bit = true;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SENDING;
   group_data->slave_devices[sdi].transmission_bit = true;
   group_data->slave_devices[sdi].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   EXPECT_FALSE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   group_data->slave_devices[sdi0].transmission_bit = true;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SENDING;
   group_data->slave_devices[sdi].transmission_bit = false;
   group_data->slave_devices[sdi].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   EXPECT_FALSE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   group_data->slave_devices[sdi0].transmission_bit = true;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SENDING;
   group_data->slave_devices[sdi].transmission_bit = true;
   group_data->slave_devices[sdi].device_state = CLM_DEVICE_STATE_CYCLIC_SENDING;
   EXPECT_FALSE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   group_data->slave_devices[sdi0].transmission_bit = false;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   group_data->slave_devices[sdi].transmission_bit = false;
   group_data->slave_devices[sdi].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   EXPECT_FALSE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   group_data->slave_devices[sdi0].transmission_bit = true;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   group_data->slave_devices[sdi].transmission_bit = true;
   group_data->slave_devices[sdi].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   EXPECT_TRUE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   group_data->slave_devices[sdi0].transmission_bit = false;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   group_data->slave_devices[sdi].transmission_bit = true;
   group_data->slave_devices[sdi].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   EXPECT_TRUE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   group_data->slave_devices[sdi0].transmission_bit = false;
   group_data->slave_devices[sdi0].device_state     = CLM_DEVICE_STATE_WAIT_TD;
   group_data->slave_devices[sdi].transmission_bit  = false;
   group_data->slave_devices[sdi].device_state      = CLM_DEVICE_STATE_WAIT_TD;
   EXPECT_FALSE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   group_data->slave_devices[sdi0].transmission_bit = true;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   group_data->slave_devices[sdi].transmission_bit = false;
   group_data->slave_devices[sdi].device_state     = CLM_DEVICE_STATE_WAIT_TD;
   EXPECT_TRUE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   /* Consecutive link scans (frame sequence != 0), one slave disabled */

   group_data->slave_devices[sdi0].transmission_bit = false;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SUSPEND;
   group_data->slave_devices[sdi].transmission_bit = false;
   group_data->slave_devices[sdi].device_state     = CLM_DEVICE_STATE_WAIT_TD;
   EXPECT_FALSE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   group_data->slave_devices[sdi0].transmission_bit = false;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SUSPEND;
   group_data->slave_devices[sdi].transmission_bit = false;
   group_data->slave_devices[sdi].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   EXPECT_FALSE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   group_data->slave_devices[sdi0].transmission_bit = false;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SUSPEND;
   group_data->slave_devices[sdi].transmission_bit = true;
   group_data->slave_devices[sdi].device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   EXPECT_TRUE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   group_data->slave_devices[sdi0].transmission_bit = false;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SUSPEND;
   group_data->slave_devices[sdi].transmission_bit = true;
   group_data->slave_devices[sdi].device_state = CLM_DEVICE_STATE_CYCLIC_SENDING;
   EXPECT_FALSE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));

   /* Consecutive link scans (frame sequence != 0), all slaves disabled */
   group_data->slave_devices[sdi0].transmission_bit = false;
   group_data->slave_devices[sdi0].device_state = CLM_DEVICE_STATE_CYCLIC_SUSPEND;
   group_data->slave_devices[sdi].transmission_bit = false;
   group_data->slave_devices[sdi].device_state = CLM_DEVICE_STATE_CYCLIC_SUSPEND;
   EXPECT_FALSE (
      clm_iefb_group_have_received_from_all_devices (group_setting, group_data));
}

/********************** Group state machine ****************************/

TEST_F (MasterUnitTest, CciefbGroupStatemachineInStateDown)
{
   const clm_group_event_t events_without_effect[] = {
      CLM_GROUP_EVENT_NONE,
      CLM_GROUP_EVENT_PARAMETER_CHANGE,
      CLM_GROUP_EVENT_NEW_CONFIG,
      CLM_GROUP_EVENT_ARBITRATION_DONE,
      CLM_GROUP_EVENT_REQ_FROM_OTHER,
      CLM_GROUP_EVENT_MASTERDUPL_ALARM,
      CLM_GROUP_EVENT_LINKSCAN_START,
      CLM_GROUP_EVENT_LINKSCAN_TIMEOUT,
      CLM_GROUP_EVENT_LINKSCAN_COMPLETE,
      CLM_GROUP_EVENT_LAST};
   size_t i;
   clm_group_data_t * group_data = &clm.groups[gi];

   clm_iefb_group_fsm_tables_init (&clm);
   clm_iefb_reflect_group_parameters (&clm, group_data);
   clm.config.state_cb = my_master_state_ind;
   clm.config.cb_arg   = cb_counters;

   /* Force state machine to desired state */
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_DOWN);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_DOWN);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 0);

   /* Events that should effect the state */
   clm_iefb_group_fsm_event (&clm, now, group_data, CLM_GROUP_EVENT_STARTUP);
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LISTEN);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_STANDBY);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_STANDBY);
   group_data->group_state = CLM_GROUP_STATE_MASTER_DOWN;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_DOWN);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      clm_iefb_group_fsm_event (&clm, now, group_data, events_without_effect[i]);
      ASSERT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_DOWN)
         << "Triggered by clm_group_event_t with value "
         << events_without_effect[i];
   }
}

TEST_F (MasterUnitTest, CciefbGroupStatemachineInStateListen)
{
   const clm_group_event_t events_without_effect[] = {
      CLM_GROUP_EVENT_NONE,
      CLM_GROUP_EVENT_STARTUP,
      CLM_GROUP_EVENT_ARBITRATION_DONE,
      CLM_GROUP_EVENT_REQ_FROM_OTHER,
      CLM_GROUP_EVENT_MASTERDUPL_ALARM,
      CLM_GROUP_EVENT_LINKSCAN_START,
      CLM_GROUP_EVENT_LINKSCAN_TIMEOUT,
      CLM_GROUP_EVENT_LINKSCAN_COMPLETE,
      CLM_GROUP_EVENT_LAST};
   size_t i;
   clm_group_data_t * group_data = &clm.groups[gi];

   clm_iefb_group_fsm_tables_init (&clm);
   clm_iefb_reflect_group_parameters (&clm, group_data);
   clm.config.state_cb = my_master_state_ind;
   clm.config.cb_arg   = cb_counters;

   /* Force state machine to desired state */
   group_data->group_state = CLM_GROUP_STATE_MASTER_LISTEN;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LISTEN);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_DOWN);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 0);

   /* Events that should effect the state */
   clm_iefb_group_fsm_event (&clm, now, group_data, CLM_GROUP_EVENT_NEW_CONFIG);
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   group_data->group_state = CLM_GROUP_STATE_MASTER_LISTEN;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LISTEN);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      clm_iefb_group_fsm_event (&clm, now, group_data, events_without_effect[i]);
      ASSERT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LISTEN)
         << "Triggered by clm_group_event_t with value "
         << events_without_effect[i];
   }
}

TEST_F (MasterUnitTest, CciefbGroupStatemachineInStateArbitration)
{
   const clm_group_event_t events_without_effect[] = {
      CLM_GROUP_EVENT_NONE,
      CLM_GROUP_EVENT_STARTUP,
      CLM_GROUP_EVENT_NEW_CONFIG,
      CLM_GROUP_EVENT_MASTERDUPL_ALARM,
      CLM_GROUP_EVENT_LINKSCAN_START,
      CLM_GROUP_EVENT_LINKSCAN_TIMEOUT,
      CLM_GROUP_EVENT_LINKSCAN_COMPLETE,
      CLM_GROUP_EVENT_LAST};
   size_t i;
   clm_group_data_t * group_data = &clm.groups[gi];

   clm_iefb_group_fsm_tables_init (&clm);
   clm_iefb_reflect_group_parameters (&clm, group_data);
   clm.config.state_cb = my_master_state_ind;
   clm.config.cb_arg   = cb_counters;

   /* Force state machine to desired state */
   group_data->group_state = CLM_GROUP_STATE_MASTER_ARBITRATION;
   clm.master_state        = CLM_MASTER_STATE_ARBITRATION;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 0);

   /* Events that should effect the state */
   clm_iefb_group_fsm_event (&clm, now, group_data, CLM_GROUP_EVENT_REQ_FROM_OTHER);
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LISTEN);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_STANDBY);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_STANDBY);
   group_data->group_state = CLM_GROUP_STATE_MASTER_ARBITRATION;
   clm.master_state        = CLM_MASTER_STATE_ARBITRATION;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_ARBITRATION);

   clm_iefb_group_fsm_event (
      &clm,
      now,
      group_data,
      CLM_GROUP_EVENT_PARAMETER_CHANGE);
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LISTEN);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_STANDBY);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_STANDBY);
   group_data->group_state = CLM_GROUP_STATE_MASTER_ARBITRATION;
   clm.master_state        = CLM_MASTER_STATE_ARBITRATION;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_ARBITRATION);

   clm_iefb_group_fsm_event (
      &clm,
      now,
      group_data,
      CLM_GROUP_EVENT_ARBITRATION_DONE);
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   group_data->group_state = CLM_GROUP_STATE_MASTER_ARBITRATION;
   clm.master_state        = CLM_MASTER_STATE_ARBITRATION;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_ARBITRATION);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      clm_iefb_group_fsm_event (&clm, now, group_data, events_without_effect[i]);
      ASSERT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_ARBITRATION)
         << "Triggered by clm_group_event_t with value "
         << events_without_effect[i];
   }
}

TEST_F (MasterUnitTest, CciefbGroupStatemachineInStateLinkScan)
{
   const clm_group_event_t events_without_effect[] = {
      CLM_GROUP_EVENT_NONE,
      CLM_GROUP_EVENT_STARTUP,
      CLM_GROUP_EVENT_NEW_CONFIG,
      CLM_GROUP_EVENT_ARBITRATION_DONE,
      CLM_GROUP_EVENT_REQ_FROM_OTHER,
      CLM_GROUP_EVENT_LINKSCAN_START,
      CLM_GROUP_EVENT_LAST};
   size_t i;
   clm_group_data_t * group_data       = &clm.groups[gi];
   clm_group_setting_t * group_setting = &clm.config.hier.groups[gi];

   clm_iefb_group_fsm_tables_init (&clm);
   clm_iefb_reflect_group_parameters (&clm, group_data);
   clm.config.state_cb    = my_master_state_ind;
   clm.config.linkscan_cb = my_master_linkscan_complete_ind;
   clm.config.cb_arg      = cb_counters;

   /* Force state machine to desired state */
   group_data->group_state = CLM_GROUP_STATE_MASTER_LINK_SCAN;
   clm.master_state        = CLM_MASTER_STATE_RUNNING;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (group_data->frame_sequence_no, 0);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 0);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);

   /* Events that should effect the state */
   group_setting->use_constant_link_scan_time = true;
   clm_iefb_group_fsm_event (
      &clm,
      now,
      group_data,
      CLM_GROUP_EVENT_LINKSCAN_COMPLETE);
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP);
   EXPECT_EQ (group_data->frame_sequence_no, 1);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 0);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);
   group_data->group_state = CLM_GROUP_STATE_MASTER_LINK_SCAN;
   clm.master_state        = CLM_MASTER_STATE_RUNNING;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);

   group_setting->use_constant_link_scan_time = false;
   clm_iefb_group_fsm_event (
      &clm,
      now,
      group_data,
      CLM_GROUP_EVENT_LINKSCAN_COMPLETE);
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (group_data->frame_sequence_no, 2);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 0);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);

   clm_iefb_group_fsm_event (
      &clm,
      now,
      group_data,
      CLM_GROUP_EVENT_LINKSCAN_TIMEOUT);
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (group_data->frame_sequence_no, 3);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 0);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);

   clm_iefb_group_fsm_event (
      &clm,
      now,
      group_data,
      CLM_GROUP_EVENT_MASTERDUPL_ALARM);
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LISTEN);
   EXPECT_EQ (group_data->frame_sequence_no, 3);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 1);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_STANDBY);
   group_data->group_state = CLM_GROUP_STATE_MASTER_LINK_SCAN;
   clm.master_state        = CLM_MASTER_STATE_RUNNING;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);

   clm_iefb_group_fsm_event (
      &clm,
      now,
      group_data,
      CLM_GROUP_EVENT_PARAMETER_CHANGE);
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LISTEN);
   EXPECT_EQ (group_data->frame_sequence_no, 3);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_STANDBY);
   group_data->group_state = CLM_GROUP_STATE_MASTER_LINK_SCAN;
   clm.master_state        = CLM_MASTER_STATE_RUNNING;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);

   /* Events that should not effect the state */
   group_data->frame_sequence_no = 0;
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      clm_iefb_group_fsm_event (&clm, now, group_data, events_without_effect[i]);
      ASSERT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN)
         << "Triggered by clm_group_event_t with value "
         << events_without_effect[i];
      EXPECT_EQ (group_data->frame_sequence_no, 0);
      EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);
   }
}

TEST_F (MasterUnitTest, CciefbGroupStatemachineInStateLinkScanComplete)
{
   const clm_group_event_t events_without_effect[] = {
      CLM_GROUP_EVENT_NONE,
      CLM_GROUP_EVENT_STARTUP,
      CLM_GROUP_EVENT_NEW_CONFIG,
      CLM_GROUP_EVENT_ARBITRATION_DONE,
      CLM_GROUP_EVENT_REQ_FROM_OTHER,
      CLM_GROUP_EVENT_LINKSCAN_TIMEOUT,
      CLM_GROUP_EVENT_LINKSCAN_COMPLETE,
      CLM_GROUP_EVENT_LAST};
   size_t i;
   clm_group_data_t * group_data = &clm.groups[gi];

   clm_iefb_group_fsm_tables_init (&clm);
   clm_iefb_reflect_group_parameters (&clm, group_data);
   clm.config.state_cb = my_master_state_ind;
   clm.config.cb_arg   = cb_counters;

   /* Force state machine to desired state */
   group_data->group_state = CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP;
   clm.master_state        = CLM_MASTER_STATE_RUNNING;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 0);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);

   /* Events that should effect the state */
   clm_iefb_group_fsm_event (&clm, now, group_data, CLM_GROUP_EVENT_LINKSCAN_START);
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 0);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);
   group_data->group_state = CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP;
   clm.master_state        = CLM_MASTER_STATE_RUNNING;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);

   clm_iefb_group_fsm_event (
      &clm,
      now,
      group_data,
      CLM_GROUP_EVENT_MASTERDUPL_ALARM);
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LISTEN);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_STANDBY);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_STANDBY);
   group_data->group_state = CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP;
   clm.master_state        = CLM_MASTER_STATE_RUNNING;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);

   clm_iefb_group_fsm_event (
      &clm,
      now,
      group_data,
      CLM_GROUP_EVENT_PARAMETER_CHANGE);
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LISTEN);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_STANDBY);
   group_data->group_state = CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP;
   clm.master_state        = CLM_MASTER_STATE_RUNNING;
   EXPECT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP);
   EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      clm_iefb_group_fsm_event (&clm, now, group_data, events_without_effect[i]);
      ASSERT_EQ (group_data->group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP)
         << "Triggered by clm_group_event_t with value "
         << events_without_effect[i];
      EXPECT_EQ (clm.master_state, CLM_MASTER_STATE_RUNNING);
   }
}

/********************** Slave device state machine *********************/

TEST_F (MasterUnitTest, CciefbDeviceStatemachineInStateDown)
{
   const clm_device_event_t events_without_effect[] = {
      CLM_DEVICE_EVENT_NONE,
      CLM_DEVICE_EVENT_GROUP_STANDBY,
      CLM_DEVICE_EVENT_GROUP_TIMEOUT,
      CLM_DEVICE_EVENT_GROUP_ALL_RESPONDED,
      CLM_DEVICE_EVENT_RECEIVE_OK,
      CLM_DEVICE_EVENT_RECEIVE_ERROR,
      CLM_DEVICE_EVENT_SCAN_START_DEVICE_START,
      CLM_DEVICE_EVENT_SCAN_START_DEVICE_STOP,
      CLM_DEVICE_EVENT_TIMEOUTCOUNTER_FULL,
      CLM_DEVICE_EVENT_TIMEOUTCOUNTER_NOT_FULL,
      CLM_DEVICE_EVENT_SLAVE_DUPLICATION,
      CLM_DEVICE_EVENT_LAST};
   size_t i;
   clm_group_data_t * gd         = &clm.groups[gi];
   clm_slave_device_data_t * sdd = &clm.groups[gi].slave_devices[sdi];
   sdd->slave_station_no         = 1;

   clm_iefb_device_fsm_tables_init (&clm);
   clm_iefb_reflect_group_parameters (&clm, gd);

   /* Force state machine to desired state */
   sdd->device_state = CLM_DEVICE_STATE_MASTER_DOWN;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_MASTER_DOWN);

   /* Events that should effect the state */
   sdd->timeout_count = 99;
   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_GROUP_STARTUP);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (sdd->timeout_count, 0);
   sdd->device_state = CLM_DEVICE_STATE_MASTER_DOWN;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_MASTER_DOWN);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      clm_iefb_device_fsm_event (&clm, now, gd, sdd, events_without_effect[i]);
      ASSERT_EQ (sdd->device_state, CLM_DEVICE_STATE_MASTER_DOWN)
         << "Triggered by clm_device_event_t with value "
         << events_without_effect[i];
   }
}

TEST_F (MasterUnitTest, CciefbDeviceStatemachineInStateListen)
{
   const clm_device_event_t events_without_effect[] = {
      CLM_DEVICE_EVENT_NONE,
      CLM_DEVICE_EVENT_GROUP_STARTUP,
      CLM_DEVICE_EVENT_GROUP_STANDBY,
      CLM_DEVICE_EVENT_GROUP_TIMEOUT,
      CLM_DEVICE_EVENT_GROUP_ALL_RESPONDED,
      CLM_DEVICE_EVENT_RECEIVE_OK,
      CLM_DEVICE_EVENT_RECEIVE_ERROR,
      CLM_DEVICE_EVENT_TIMEOUTCOUNTER_FULL,
      CLM_DEVICE_EVENT_TIMEOUTCOUNTER_NOT_FULL,
      CLM_DEVICE_EVENT_SLAVE_DUPLICATION,
      CLM_DEVICE_EVENT_LAST};
   size_t i;
   clm_group_data_t * gd         = &clm.groups[gi];
   clm_slave_device_data_t * sdd = &clm.groups[gi].slave_devices[sdi];
   sdd->slave_station_no         = 2;
   sdd->device_index             = 1;

   clm.config.connect_cb    = my_master_connect_ind;
   clm.config.disconnect_cb = my_master_disconnect_ind;
   clm.config.cb_arg        = cb_counters;
   clm.config.hier.groups[gi].slave_devices[sdi].slave_id = remote_ip;

   clm_iefb_device_fsm_tables_init (&clm);
   clm_iefb_reflect_group_parameters (&clm, gd);

   /* Force state machine to desired state */
   sdd->device_state     = CLM_DEVICE_STATE_LISTEN;
   sdd->transmission_bit = true;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);

   /* Events that should effect the state */
   clm_iefb_device_fsm_event (
      &clm,
      now,
      gd,
      sdd,
      CLM_DEVICE_EVENT_SCAN_START_DEVICE_START);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_id, remote_ip);
   sdd->device_state     = CLM_DEVICE_STATE_LISTEN;
   sdd->transmission_bit = true;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);

   clm_iefb_device_fsm_event (
      &clm,
      now,
      gd,
      sdd,
      CLM_DEVICE_EVENT_SCAN_START_DEVICE_STOP);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SUSPEND);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_id, remote_ip);
   sdd->device_state     = CLM_DEVICE_STATE_LISTEN;
   sdd->transmission_bit = true;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      clm_iefb_device_fsm_event (&clm, now, gd, sdd, events_without_effect[i]);
      ASSERT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN)
         << "Triggered by clm_device_event_t with value "
         << events_without_effect[i];
   }
}

TEST_F (MasterUnitTest, CciefbDeviceStatemachineInStateWaitTD)
{
   const clm_device_event_t events_without_effect[] = {
      CLM_DEVICE_EVENT_NONE,
      CLM_DEVICE_EVENT_GROUP_STARTUP,
      CLM_DEVICE_EVENT_SCAN_START_DEVICE_START,
      CLM_DEVICE_EVENT_SCAN_START_DEVICE_STOP,
      CLM_DEVICE_EVENT_TIMEOUTCOUNTER_FULL,
      CLM_DEVICE_EVENT_TIMEOUTCOUNTER_NOT_FULL,
      CLM_DEVICE_EVENT_LAST};
   size_t i;
   clm_group_data_t * gd = &clm.groups[gi];
   clm_slave_device_setting_t * device_setting =
      &clm.config.hier.groups[gi].slave_devices[sdi];
   clm_slave_device_data_t * sdd = &clm.groups[gi].slave_devices[sdi];

   sdd->device_index        = sdi;
   sdd->slave_station_no    = 1;
   device_setting->slave_id = remote_ip;
   clm.config.connect_cb    = my_master_connect_ind;
   clm.config.disconnect_cb = my_master_disconnect_ind;
   clm.config.cb_arg        = cb_counters;
   clm.config.hier.groups[gi].slave_devices[sdi].slave_id = remote_ip;

   clm_iefb_device_fsm_tables_init (&clm);
   clm_iefb_reflect_group_parameters (&clm, gd);

   /* Force state machine to desired state */
   sdd->device_state = CLM_DEVICE_STATE_WAIT_TD;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);

   /* Events that should effect the state */
   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_RECEIVE_ERROR);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   sdd->device_state = CLM_DEVICE_STATE_WAIT_TD;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_WAIT_TD);

   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_GROUP_TIMEOUT);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   sdd->device_state = CLM_DEVICE_STATE_WAIT_TD;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_WAIT_TD);

   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_GROUP_STANDBY);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   sdd->device_state = CLM_DEVICE_STATE_WAIT_TD;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_WAIT_TD);

   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_SLAVE_DUPLICATION);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   sdd->device_state = CLM_DEVICE_STATE_WAIT_TD;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_WAIT_TD);

   clm_iefb_device_fsm_event (
      &clm,
      now,
      gd,
      sdd,
      CLM_DEVICE_EVENT_GROUP_ALL_RESPONDED);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   sdd->device_state = CLM_DEVICE_STATE_WAIT_TD;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_WAIT_TD);

   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_RECEIVE_OK);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   sdd->device_state = CLM_DEVICE_STATE_WAIT_TD;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_WAIT_TD);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      clm_iefb_device_fsm_event (&clm, now, gd, sdd, events_without_effect[i]);
      ASSERT_EQ (sdd->device_state, CLM_DEVICE_STATE_WAIT_TD)
         << "Triggered by clm_device_event_t with value "
         << events_without_effect[i];
   }
}

TEST_F (MasterUnitTest, CciefbDeviceStatemachineInStateCyclicSending)
{
   const clm_device_event_t events_without_effect[] = {
      CLM_DEVICE_EVENT_NONE,
      CLM_DEVICE_EVENT_GROUP_STARTUP,
      CLM_DEVICE_EVENT_GROUP_ALL_RESPONDED,
      CLM_DEVICE_EVENT_SCAN_START_DEVICE_START,
      CLM_DEVICE_EVENT_SCAN_START_DEVICE_STOP,
      CLM_DEVICE_EVENT_LAST};
   size_t i;
   clm_group_data_t * gd               = &clm.groups[gi];
   clm_slave_device_data_t * sdd       = &clm.groups[gi].slave_devices[sdi];
   clm_group_setting_t * group_setting = &clm.config.hier.groups[gi];
   clm_slave_device_setting_t * device_setting =
      &group_setting->slave_devices[sdi];

   sdd->slave_station_no                     = 1;
   group_setting->parallel_off_timeout_count = 3;
   device_setting->slave_id                  = remote_ip;
   sdd->device_index                         = sdi;
   clm.config.connect_cb                     = my_master_connect_ind;
   clm.config.disconnect_cb                  = my_master_disconnect_ind;
   clm.config.cb_arg                         = cb_counters;
   clm.config.hier.groups[gi].slave_devices[sdi].slave_id = remote_ip;

   clm_iefb_device_fsm_tables_init (&clm);
   clm_iefb_reflect_group_parameters (&clm, gd);

   /* Force state machine to desired state */
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SENDING;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);

   /* Events that should effect the state */
   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_RECEIVE_ERROR);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SENDING;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENDING);

   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_GROUP_STANDBY);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SENDING;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENDING);

   clm_iefb_device_fsm_event (
      &clm,
      now,
      gd,
      sdd,
      CLM_DEVICE_EVENT_TIMEOUTCOUNTER_FULL);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SENDING;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENDING);

   clm_iefb_device_fsm_event (
      &clm,
      now,
      gd,
      sdd,
      CLM_DEVICE_EVENT_TIMEOUTCOUNTER_NOT_FULL);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENT);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SENDING;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENDING);

   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_SLAVE_DUPLICATION);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SENDING;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENDING);

   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_RECEIVE_OK);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENT);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SENDING;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENDING);

   sdd->timeout_count = 0;
   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_GROUP_TIMEOUT);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (sdd->timeout_count, 1);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SENDING;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENDING);

   sdd->timeout_count = 3;
   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_GROUP_TIMEOUT);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (sdd->timeout_count, 0);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SENDING;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENDING);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      clm_iefb_device_fsm_event (&clm, now, gd, sdd, events_without_effect[i]);
      ASSERT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENDING)
         << "Triggered by clm_device_event_t with value "
         << events_without_effect[i];
   }
}

TEST_F (MasterUnitTest, CciefbDeviceStatemachineInStateCyclicSent)
{
   const clm_device_event_t events_without_effect[] = {
      CLM_DEVICE_EVENT_NONE,
      CLM_DEVICE_EVENT_GROUP_STARTUP,
      CLM_DEVICE_EVENT_GROUP_TIMEOUT,
      CLM_DEVICE_EVENT_GROUP_ALL_RESPONDED,
      CLM_DEVICE_EVENT_RECEIVE_OK,
      CLM_DEVICE_EVENT_RECEIVE_ERROR,
      CLM_DEVICE_EVENT_TIMEOUTCOUNTER_FULL,
      CLM_DEVICE_EVENT_TIMEOUTCOUNTER_NOT_FULL,
      CLM_DEVICE_EVENT_LAST};
   size_t i;
   clm_group_data_t * gd         = &clm.groups[gi];
   clm_slave_device_data_t * sdd = &clm.groups[gi].slave_devices[sdi];
   sdd->slave_station_no         = 1;
   sdd->device_index             = sdi;
   clm.config.connect_cb         = my_master_connect_ind;
   clm.config.disconnect_cb      = my_master_disconnect_ind;
   clm.config.cb_arg             = cb_counters;
   clm.config.hier.groups[gi].slave_devices[sdi].slave_id = remote_ip;

   clm_iefb_device_fsm_tables_init (&clm);
   clm_iefb_reflect_group_parameters (&clm, gd);

   /* Force state machine to desired state */
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);

   /* Events that should effect the state */
   clm_iefb_device_fsm_event (
      &clm,
      now,
      gd,
      sdd,
      CLM_DEVICE_EVENT_SCAN_START_DEVICE_START);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENT);

   clm_iefb_device_fsm_event (
      &clm,
      now,
      gd,
      sdd,
      CLM_DEVICE_EVENT_SCAN_START_DEVICE_STOP);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SUSPEND);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENT);

   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_SLAVE_DUPLICATION);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENT);

   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_GROUP_STANDBY);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SENT;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENT);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      clm_iefb_device_fsm_event (&clm, now, gd, sdd, events_without_effect[i]);
      ASSERT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SENT)
         << "Triggered by clm_device_event_t with value "
         << events_without_effect[i];
   }
}

TEST_F (MasterUnitTest, CciefbDeviceStatemachineInStateCyclicSuspend)
{
   const clm_device_event_t events_without_effect[] = {
      CLM_DEVICE_EVENT_NONE,
      CLM_DEVICE_EVENT_GROUP_STARTUP,
      CLM_DEVICE_EVENT_RECEIVE_OK,
      CLM_DEVICE_EVENT_RECEIVE_ERROR,
      CLM_DEVICE_EVENT_SCAN_START_DEVICE_START,
      CLM_DEVICE_EVENT_SCAN_START_DEVICE_STOP,
      CLM_DEVICE_EVENT_TIMEOUTCOUNTER_FULL,
      CLM_DEVICE_EVENT_TIMEOUTCOUNTER_NOT_FULL,
      CLM_DEVICE_EVENT_LAST};
   size_t i;
   clm_group_data_t * gd         = &clm.groups[gi];
   clm_slave_device_data_t * sdd = &clm.groups[gi].slave_devices[sdi];
   sdd->slave_station_no         = 1;

   clm_iefb_device_fsm_tables_init (&clm);
   clm_iefb_reflect_group_parameters (&clm, gd);

   /* Force state machine to desired state */
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SUSPEND;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SUSPEND);

   /* Events that should effect the state */
   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_GROUP_TIMEOUT);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SUSPEND;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SUSPEND);

   clm_iefb_device_fsm_event (
      &clm,
      now,
      gd,
      sdd,
      CLM_DEVICE_EVENT_GROUP_ALL_RESPONDED);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SUSPEND;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SUSPEND);

   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_GROUP_STANDBY);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SUSPEND;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SUSPEND);

   clm_iefb_device_fsm_event (&clm, now, gd, sdd, CLM_DEVICE_EVENT_SLAVE_DUPLICATION);
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_LISTEN);
   sdd->device_state = CLM_DEVICE_STATE_CYCLIC_SUSPEND;
   EXPECT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SUSPEND);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      clm_iefb_device_fsm_event (&clm, now, gd, sdd, events_without_effect[i]);
      ASSERT_EQ (sdd->device_state, CLM_DEVICE_STATE_CYCLIC_SUSPEND)
         << "Triggered by clm_device_event_t with value "
         << events_without_effect[i];
   }
}

/***********************************************************************/

/**
 * Verify master startup.
 *
 * Uses a single group.
 * Uses two slaves, with device index 0 and device index 1, occupying one
 * and two slave stations respectively.
 *
 * @req REQ_CL_PROTOCOL_20
 * @req REQ_CL_PROTOCOL_30
 * @req REQ_CL_PROTOCOL_35
 * @req REQ_CL_PROTOCOL_52
 * @req REQ_CL_UDP_01
 * @req REQ_CLM_ARBITR_01
 * @req REQ_CLM_ARBITR_02
 * @req REQ_CLM_ARBITR_03
 * @req REQ_CLM_DIAGNOSIS_01
 * @req REQ_CLM_PARAMETERID_01
 * @req REQ_CLM_SEQUENCE_01
 * @req REQ_CLM_SEQUENCE_03
 * @req REQ_CLM_UDP_01
 * @req REQ_CLM_VERSION_02
 *
 */
TEST_F (MasterIntegrationTestNotInitialised, CciefbStartup)
{
   const clm_slave_device_data_t * slave_device_connection_details = nullptr;
   const clm_slave_device_statistics_t * statistics                = nullptr;
   const clm_device_framevalues_t * framevalues                    = nullptr;
   uint16_t * p_file_parameter_no                                  = nullptr;
   const uint32_t magic   = CC_FROM_BE32 (0x434C4E4B); /* "CLNK" */
   const uint32_t version = CC_FROM_BE32 (0x00000001);
   cl_ipaddr_t resulting_ip;
   cl_mock_cyclic_request_result_t result;
   clm_master_status_details_t master_details;
   clm_group_status_details_t group_details;
   clal_clear_memory (&result, sizeof (result));
   clal_clear_memory (&master_details, sizeof (master_details));
   clal_clear_memory (&group_details, sizeof (group_details));

   ASSERT_FALSE (mock_data.udp_ports[0].in_use);
   ASSERT_FALSE (mock_data.udp_ports[1].in_use);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);

   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);

   ASSERT_TRUE (mock_data.udp_ports[0].in_use);
   ASSERT_TRUE (mock_data.udp_ports[0].is_open);
   ASSERT_TRUE (mock_data.udp_ports[1].in_use);
   ASSERT_TRUE (mock_data.udp_ports[1].is_open);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);

   ASSERT_EQ (&mock_data.udp_ports[0], mock_cciefb_port);
   ASSERT_EQ (&mock_data.udp_ports[1], mock_slmp_port);
   ASSERT_EQ (mock_cciefb_port->port_number, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_cciefb_port->ip_addr, my_ip);
   EXPECT_EQ (mock_cciefb_port->ifindex, my_ifindex);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   ASSERT_EQ (mock_slmp_port->port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_port->ip_addr, CL_IPADDR_ANY);
   EXPECT_EQ (mock_slmp_port->ifindex, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);

   slave_device_connection_details =
      clm_iefb_get_device_connection_details (&clm, gi, sdi);
   ASSERT_TRUE (slave_device_connection_details != nullptr);
   statistics  = &slave_device_connection_details->statistics;
   framevalues = &slave_device_connection_details->latest_frame;

   /* Check that file_size, magic and version have not been modified */
   EXPECT_EQ (storage_file->file_size, expected_filesize);
   EXPECT_EQ (memcmp (storage_file->file_content, &magic, 4), 0);
   EXPECT_EQ (memcmp (storage_file->file_content + 4, &version, 4), 0);
   /* Check that parameter_no has been incremented */
   p_file_parameter_no = (uint16_t *)&storage_file->file_content + 4;
   EXPECT_EQ (*p_file_parameter_no, parameter_no);
   EXPECT_EQ (clm.parameter_no, parameter_no);

   EXPECT_FALSE (framevalues->has_been_received);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (statistics->number_of_connects, 0U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);

   /* Run master stack, arbitration not done yet */
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (statistics->number_of_connects, 0U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   /* Run master stack, arbitration done.
      Master sends request. */
   now += longer_than_arbitration_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (statistics->number_of_connects, 0U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, broadcast_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (result.group_no, group_number);
   EXPECT_EQ (
      result.cyclic_transmission_state,
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.dl, SIZE_REQUEST_3_SLAVES - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   EXPECT_EQ (result.clock_info, mock_data.unix_timestamp_ms);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.parallel_off_timeout_count,
      config.hier.groups[gi].parallel_off_timeout_count);

   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 0, &resulting_ip),
      -1); /* Verify helper function */
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip_di0);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 3, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, UINT32_MAX);

   EXPECT_EQ (clm_iefb_get_master_status (&clm, &master_details), 0);
   EXPECT_EQ (master_details.master_state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (master_details.parameter_no, parameter_no);
   EXPECT_EQ (master_details.node_search_serial, CLM_SLMP_SERIAL_NONE);
   EXPECT_EQ (master_details.set_ip_request_serial, CLM_SLMP_SERIAL_NONE);

   EXPECT_EQ (clm_iefb_get_group_status (&clm, gi, &group_details), 0);
   EXPECT_EQ (
      group_details.cyclic_transmission_state,
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF);
   EXPECT_EQ (group_details.frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (group_details.group_index, gi);
   EXPECT_EQ (group_details.group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (group_details.timestamp_link_scan_start, now);
   EXPECT_EQ (group_details.total_occupied, 3);

   /* Slave (device index 1) responds */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);

   /* Analyze stored info from latest frame (slave device index 1) */
   EXPECT_TRUE (framevalues->has_been_received);
   EXPECT_EQ (framevalues->response_time, 1000U);
   EXPECT_EQ (framevalues->group_no, 1);
   EXPECT_EQ (framevalues->num_occupied_stations, 2);
   EXPECT_EQ (framevalues->protocol_ver, 2);
   EXPECT_EQ (framevalues->end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (framevalues->vendor_code, slave_vendor_code);
   EXPECT_EQ (framevalues->model_code, slave_model_code);
   EXPECT_EQ (framevalues->equipment_ver, slave_equipment_ver);
   EXPECT_EQ (
      framevalues->slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (framevalues->local_management_info, alarm_local_management_info);
   EXPECT_EQ (framevalues->slave_err_code, alarm_slave_err_code);
   EXPECT_EQ (framevalues->slave_id, remote_ip);
   EXPECT_EQ (framevalues->frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (statistics->number_of_sent_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (statistics->number_of_connects, 0U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (statistics->number_of_timeouts, 0U);
   EXPECT_EQ (statistics->measured_time.number_of_samples, 1U);
   EXPECT_EQ (statistics->measured_time.sum, tick_size);
   EXPECT_EQ (statistics->measured_time.min, tick_size);
   EXPECT_EQ (statistics->measured_time.max, tick_size);
   EXPECT_EQ (statistics->measured_time.average, tick_size);

   /* Slave (device index 0) responds.
      Master sends new request immediately. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_payload_di0,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 1);
   EXPECT_EQ (result.group_no, group_number);
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003); /* First two slaves */
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.dl, SIZE_REQUEST_3_SLAVES - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   EXPECT_EQ (result.clock_info, mock_data.unix_timestamp_ms);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.parallel_off_timeout_count,
      config.hier.groups[gi].parallel_off_timeout_count);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 6);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_2_SLAVES + SIZE_RESPONSE_1_SLAVE);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (statistics->measured_time.number_of_samples, 1U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_linkscan.group_index, gi);
   EXPECT_TRUE (cb_counters->master_cb_linkscan.success);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (statistics->number_of_connects, 1U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);

   /* Slave (device index 1) responds again (with new sequence number) */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += 3 * tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (framevalues->frame_sequence_no, frame_sequenceno_startup + 1);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);

   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (statistics->number_of_connects, 1U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (statistics->number_of_sent_frames, 2U);
   EXPECT_EQ (statistics->number_of_incoming_frames, 2U);
   EXPECT_EQ (statistics->measured_time.number_of_samples, 2U);
   EXPECT_EQ (statistics->measured_time.min, tick_size);
   EXPECT_EQ (statistics->measured_time.max, 3 * tick_size);
   EXPECT_EQ (statistics->measured_time.sum, 4 * tick_size);
   EXPECT_EQ (statistics->measured_time.average, 2 * tick_size);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 8);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      2 * SIZE_RESPONSE_2_SLAVES + SIZE_RESPONSE_1_SLAVE);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
}

/**
 * Maximum number of occupied slave capacity for master
 *
 * @req REQ_CLM_CAPACITY_01
 * @req REQ_CLM_CAPACITY_02
 * @req REQ_CLM_CAPACITY_03
 * @req REQ_CLM_CAPACITY_04
 * @req REQ_CLM_CAPACITY_05
 * @req REQ_CLM_GROUPS_01
 * @req REQ_CLM_CONFORMANCE_03
 *
 */
TEST_F (MasterIntegrationTestNotInitialised, CciefbMasterMaximumOccupiedSlaves)
{

#if CLM_MAX_OCCUPIED_STATIONS_PER_GROUP <                                      \
   CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP
   GTEST_SKIP()
      << "Skipping as CLM_MAX_OCCUPIED_STATIONS_PER_GROUP is set too low.";
#endif

   cl_mock_cyclic_request_result_t result;
   cl_ipaddr_t resulting_ip;
   config.hier.number_of_groups                                 = 4;
   config.hier.groups[0].timeout_value                          = timeout_value;
   config.hier.groups[0].parallel_off_timeout_count             = 3;
   config.hier.groups[0].num_slave_devices                      = 1;
   config.hier.groups[0].slave_devices[0].slave_id              = 0x0102030A;
   config.hier.groups[0].slave_devices[0].num_occupied_stations = 16;
   config.hier.groups[1].timeout_value                          = timeout_value;
   config.hier.groups[1].parallel_off_timeout_count             = 3;
   config.hier.groups[1].num_slave_devices                      = 1;
   config.hier.groups[1].slave_devices[0].slave_id              = 0x0102030B;
   config.hier.groups[1].slave_devices[0].num_occupied_stations = 16;
   config.hier.groups[2].timeout_value                          = timeout_value;
   config.hier.groups[2].parallel_off_timeout_count             = 3;
   config.hier.groups[2].num_slave_devices                      = 1;
   config.hier.groups[2].slave_devices[0].slave_id              = 0x0102030C;
   config.hier.groups[2].slave_devices[0].num_occupied_stations = 16;
   config.hier.groups[3].timeout_value                          = timeout_value;
   config.hier.groups[3].parallel_off_timeout_count             = 3;
   config.hier.groups[3].num_slave_devices                      = 1;
   config.hier.groups[3].slave_devices[0].slave_id              = 0x0102030D;
   config.hier.groups[3].slave_devices[0].num_occupied_stations = 16;
   uint8_t response_slave[SIZE_RESPONSE_16_SLAVES]              = {};

   clal_clear_memory (response_slave, SIZE_RESPONSE_16_SLAVES);
   clal_memcpy (
      response_slave,
      sizeof (response_slave),
      (uint8_t *)&response_payload_di0,
      SIZE_RESPONSE_16_SLAVES);
   response_slave[7]  = 0xB2; /* Length 1211-9=1202 */
   response_slave[8]  = 0x04;
   response_slave[51] = 0x0A; /* Slave IP address */
   response_slave[55] = 0x01; /* Group */
   response_slave[57] = 0x00; /* Sequence number */

   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);

   /* Run master stack, arbitration not done yet */
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[0].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (clm.groups[0].slave_devices[0].device_state, CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   /* Run master stack, arbitration done.
   Master sends requests, one frame for each group. */
   now += longer_than_arbitration_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[0].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 4 * SIZE_REQUEST_16_SLAVES);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   /* Analyze frame sent to last group */
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (result.group_no, 4); /* Group index 3 */
   EXPECT_EQ (
      result.cyclic_transmission_state,
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.dl, SIZE_REQUEST_16_SLAVES - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   EXPECT_EQ (result.clock_info, mock_data.unix_timestamp_ms);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, 16);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.parallel_off_timeout_count,
      config.hier.groups[gi].parallel_off_timeout_count);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, 0x0102030DU);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, UINT32_MAX);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 3, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, UINT32_MAX);

   /* Slave in group 1 (group index 0) responds. Master sends one request. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      0x0102030AU,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_slave,
      SIZE_RESPONSE_16_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (clm.groups[0].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[0].slave_devices[0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 5 * SIZE_REQUEST_16_SLAVES);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, 0x0102030AU);

   /* Slave in group 2 (group index 1) responds. Master sends one request. */
   response_slave[51] = 0x0B; /* Slave IP address */
   response_slave[55] = 0x02; /* Group */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      0x0102030BU,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_slave,
      SIZE_RESPONSE_16_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (clm.groups[0].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[0].slave_devices[0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 6 * SIZE_REQUEST_16_SLAVES);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, 0x0102030BU);
}

/**
 * Verify master startup where only one slave responds initially.
 *
 * Uses a single group.
 * Uses two slaves, with device index 0 and device index 1, occupying one
 * and two slave stations respectively.
 *
 * @req REQ_CL_PROTOCOL_27
 * @req REQ_CL_PROTOCOL_34
 * @req REQ_CL_STATUSBIT_01
 * @req REQ_CLM_ARBITR_01
 * @req REQ_CLM_ARBITR_02
 * @req REQ_CLM_COMMUNIC_03
 * @req REQ_CLM_COMMUNIC_04
 * @req REQ_CLM_CONFORMANCE_02
 * @req REQ_CLM_SEQUENCE_01
 * @req REQ_CLM_SEQUENCE_03
 * @req REQ_CLM_STATUSBIT_04
 * @req REQ_CLM_STATUSBIT_05
 * @req REQ_CLM_STATUSBIT_06
 * @req REQ_CLM_STATUSBIT_09
 * @req REQ_CLM_TIMING_05
 * @req REQ_CLM_TIMING_07
 * @req REQ_CLM_TIMING_10
 *
 */
TEST_F (MasterIntegrationTestNotInitialised, CciefbStartupOnlyOneSlaveResponds)
{
   cl_ipaddr_t resulting_ip;
   cl_mock_cyclic_request_result_t result;
   cl_rww_t * resulting_rww_area;
   uint16_t registernumber                                             = 4;
   uint16_t value_A                                                    = 789;
   uint16_t value_B                                                    = 890;
   uint16_t value_C                                                    = 0x0908;
   uint8_t response_di1_higher_sequence_number[SIZE_RESPONSE_2_SLAVES] = {};
   uint8_t response_di0_higher_sequence_number[SIZE_RESPONSE_1_SLAVE]  = {};

   clal_memcpy (
      response_di1_higher_sequence_number,
      sizeof (response_di1_higher_sequence_number),
      (uint8_t *)&response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   response_di1_higher_sequence_number[57] = 0x02;
   clal_memcpy (
      response_di0_higher_sequence_number,
      sizeof (response_di0_higher_sequence_number),
      (uint8_t *)&response_payload_di0,
      SIZE_RESPONSE_1_SLAVE);
   response_di0_higher_sequence_number[57] = 0x02;
   clal_clear_memory (&result, sizeof (result));

   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);

   ASSERT_EQ (mock_cciefb_port->port_number, CL_CCIEFB_PORT);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);

   /* Run master stack, arbitration not done yet */
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   /* Set output data in master for both slaves */
   clm_set_rww_value (&clm, gi, sdi0, registernumber, value_A);
   clm_set_rww_value (&clm, gi, sdi, registernumber, value_B);
   EXPECT_EQ (clm_get_rww_value (&clm, gi, sdi0, registernumber), value_A);
   EXPECT_EQ (clm_get_rww_value (&clm, gi, sdi, registernumber), value_B);

   /* Verify input data in master from both slaves */
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi0, registernumber), 0);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi, registernumber), 0);

   /* Run master stack, arbitration done.
      Master sends request. */
   now += longer_than_arbitration_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (result.group_no, group_number);
   EXPECT_EQ (
      result.cyclic_transmission_state,
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.parallel_off_timeout_count,
      config.hier.groups[gi].parallel_off_timeout_count);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip_di0);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 3, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, UINT32_MAX);
   /* Verify that data is zero in request */
   resulting_rww_area =
      mock_analyze_cyclic_request_rww (mock_cciefb_port, slaves_in_group, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), 0);
   resulting_rww_area =
      mock_analyze_cyclic_request_rww (mock_cciefb_port, slaves_in_group, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), 0);

   /* Slave (device index 1) responds */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi0, registernumber), 0);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi, registernumber), 0);

   /* No response from slave device index 0 (slave station 1) */
   now += tick_size;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);

   /* Wait for time out. Master sends new request. One slave is connected. */
   now += almost_timeout_us;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);

   now += 5 * tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 1);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for one slave only */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0002);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip);
   /* Verify that data is set for one slave only */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), 0);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi0, registernumber), 0);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi, registernumber), 0);

   /* Slave (device index 1) responds again.
      Master sends new request immediately */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 2);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for one slave only */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0002);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip);
   /* Verify that data is set for one slave only */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), 0);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi0, registernumber), 0);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi, registernumber), value_C);

   /* Slave device index 0 (slave station 1) returns with wrong sequence number.
      Master waits for other slave. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (result.cyclic_transmission_state, 0x0002);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi0, registernumber), 0);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi, registernumber), value_C);

   /* Slave device index 0 (slave station 1) sends again with wrong sequence
      number (however not same sequence number again).
      Master still waits for other slave.
      No slave duplication error callback is triggered. */
   response_di0_higher_sequence_number[57] = 0x09;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (result.cyclic_transmission_state, 0x0002);

   /* Slave (device index 1) responds again.
      Master sends new request immediately */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi0);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip_di0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 3);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip);
   /* Verify that data is set for both slaves */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi0, registernumber), 0);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi, registernumber), value_C);

   /* Slave (device index 1) responds, correct sequence number */
   response_di1_higher_sequence_number[57] = 0x03;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 3);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi0, registernumber), 0);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi, registernumber), value_C);

   /* No response from slave device index 0 (slave station 1).
      It misses deadline but will not be disconnected yet. */
   now += tick_size;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);

   /* Wait for time out. Master sends new request */
   now += almost_timeout_us;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);

   now += 5 * tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 1);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 5);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 4);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip);
   /* Verify that data is set for both slaves */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi0, registernumber), 0);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi, registernumber), value_C);

   /* Slave (device index 1) responds. Master waits for other slave. */
   response_di1_higher_sequence_number[57] = 0x04;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 5);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi0, registernumber), 0);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi, registernumber), value_C);

   /* Response from slave device index 0 (slave station 1).
      Master sends new request */
   response_di0_higher_sequence_number[57] = 0x04;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 6);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 5);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip);
   /* Verify that data is set for both slaves */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi0, registernumber), value_C);
   EXPECT_EQ (clm_get_rwr_value (&clm, gi, sdi, registernumber), value_C);
}

/**
 * No slave response at startup. Measure delay between requests.
 *
 * Uses a single group.
 * Uses two slaves, with device index 0 and device index 1, occupying one
 * and two slave stations respectively.
 *
 * @req REQ_CLM_CONFORMANCE_07
 * @req REQ_CLM_TIMING_05
 * @req REQ_CLM_TIMING_08
 *
 */
TEST_F (MasterIntegrationTestNotInitialised, CciefbStartupNoResponse)
{
   cl_ipaddr_t resulting_ip;
   cl_mock_cyclic_request_result_t result;
   clal_clear_memory (&result, sizeof (result));

   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);

   ASSERT_EQ (&mock_data.udp_ports[0], mock_cciefb_port);
   ASSERT_EQ (&mock_data.udp_ports[1], mock_slmp_port);
   ASSERT_EQ (mock_cciefb_port->port_number, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_cciefb_port->ip_addr, my_ip);
   EXPECT_EQ (mock_cciefb_port->ifindex, my_ifindex);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   ASSERT_EQ (mock_slmp_port->port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_port->ip_addr, CL_IPADDR_ANY);
   EXPECT_EQ (mock_slmp_port->ifindex, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);

   /* Run master stack, arbitration not done yet */
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   /* Run master stack, arbitration done.
      Master sends request. */
   now += longer_than_arbitration_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (result.group_no, group_number);
   EXPECT_EQ (
      result.cyclic_transmission_state,
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF);

   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.parallel_off_timeout_count,
      config.hier.groups[gi].parallel_off_timeout_count);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip_di0);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 3, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, UINT32_MAX);

   /* No response */
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   /* No response */
   now += almost_timeout_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   /* No response, master sends again after the wait */
   now += 5 * tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 1);
   EXPECT_EQ (result.group_no, group_number);
   EXPECT_EQ (
      result.cyclic_transmission_state,
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF);

   /* No response */
   now += almost_timeout_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 6);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   /* No response, master sends again after the wait */
   now += 5 * tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   /* No response */
   now += almost_timeout_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 8);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   /* No response, master sends again after the wait */
   now += 5 * tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 4 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
}

TEST_F (MasterIntegrationTestNotInitialised, CciefbStartupFailsOpenArbitrationSocket)
{
   clm_cfg_t config_separate_socket                  = {};
   cl_mock_udp_port_t * mock_cciefb_arbitration_port = &mock_data.udp_ports[1];

   clal_memcpy (
      &config_separate_socket,
      sizeof (config_separate_socket),
      &config,
      sizeof (config));
   config_separate_socket.use_separate_arbitration_socket = true;

   mock_cciefb_arbitration_port->will_fail_open = true;

   ASSERT_FALSE (mock_data.udp_ports[0].in_use);
   ASSERT_FALSE (mock_data.udp_ports[1].in_use);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);

   EXPECT_EQ (clm_master_init (&clm, &config_separate_socket, now), -1);
}

TEST_F (MasterIntegrationTestNotInitialised, CciefbFailsToReadParameterFile)
{
   uint16_t * p_file_parameter_no  = nullptr;
   const uint32_t magic            = CC_FROM_BE32 (0x434C4E4B); /* "CLNK" */
   const uint32_t version          = CC_FROM_BE32 (0x00000001);
   const uint16_t new_parameter_no = 1;

   /* Clear existing file (containing parameter number) */
   storage_file->file_size = (size_t)0;
   clal_clear_memory (
      &storage_file->file_content,
      sizeof (storage_file->file_content));

   /* Start master */
   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);
   EXPECT_EQ (clm.parameter_no, new_parameter_no);

   /* Verify updated file */
   EXPECT_EQ (storage_file->file_size, expected_filesize);
   EXPECT_EQ (memcmp (storage_file->file_content, &magic, 4), 0);
   EXPECT_EQ (memcmp (storage_file->file_content + 4, &version, 4), 0);
   p_file_parameter_no = (uint16_t *)&storage_file->file_content + 4;
   EXPECT_EQ (*p_file_parameter_no, new_parameter_no);
}

TEST_F (MasterIntegrationTestNotInitialised, CciefbNoLinkscanCallback)
{
   config.linkscan_cb = nullptr;

   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);
   ASSERT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   ASSERT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (clm.parameter_no, parameter_no);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Run master stack, arbitration done */
   now += longer_than_arbitration_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);

   /* Link scan times out */
   now += longer_than_timeout_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
}

TEST_F (MasterIntegrationTestNotInitialised, CciefbForceTransmissionBits)
{
   config.linkscan_cb = nullptr;

   cl_mock_cyclic_request_result_t result;
   clal_clear_memory (&result, sizeof (result));

   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);

   ASSERT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   ASSERT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   clm_iefb_force_cyclic_transmission_bit (&clm, gi, sdi, true);

   /* Run master stack, arbitration done */
   now += longer_than_arbitration_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (result.group_no, group_number);
   EXPECT_EQ (result.cyclic_transmission_state, 0x0002); /* Slave 1 */

   clm_iefb_force_cyclic_transmission_bit (&clm, gi, sdi, false);

   /* Time out, so a new frame will be sent*/
   now += longer_than_timeout_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 1);
   EXPECT_EQ (result.group_no, group_number);
   EXPECT_EQ (
      result.cyclic_transmission_state,
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
}

TEST_F (MasterIntegrationTestNotInitialised, CciefbFailsToSend)
{
   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);
   ASSERT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   ASSERT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   mock_cciefb_port->will_fail_send = true;

   /* Run master stack, arbitration done */
   now += longer_than_arbitration_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
}

TEST_F (MasterIntegrationTestNotInitialised, CciefbSentTooFewBytes)
{
   const ssize_t size_small_packet = 123;

   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);
   ASSERT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   ASSERT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   mock_cciefb_port->use_modified_send_size_returnvalue = true;
   mock_cciefb_port->modified_send_size_returnvalue     = size_small_packet;

   /* Run master stack, arbitration done */
   now += longer_than_arbitration_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)size_small_packet);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
}

/**
 * Verify failing arbitration
 *
 * @req REQ_CL_CAPACITY_02
 * @req REQ_CLM_ARBITR_02
 * @req REQ_CLM_CONFORMANCE_12
 *
 */
TEST_F (MasterIntegrationTestNotInitialised, CciefbArbitrationFails)
{
   const cl_ipaddr_t remote_ip_other_master = 0x0102030F; /* 1.2.3.15 */
   uint8_t request_payload_other_master[SIZE_REQUEST_3_SLAVES] = {0};

   /* Prepare request from other master */
   clal_memcpy (
      request_payload_other_master,
      sizeof (request_payload_other_master),
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_payload_other_master[47] = 0x0F; /* 1.2.3.15 */

   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);

   /* Run master stack, arbitration not done yet */
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   /* Some other master sends request */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_other_master,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_other_master,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_STANDBY);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_error.error_message,
      CLM_ERROR_ARBITRATION_FAILED);
   EXPECT_EQ (cb_counters->master_cb_error.ip_addr, remote_ip_other_master);
   EXPECT_EQ (cb_counters->master_cb_error.argument_2, 0);
}

/**
 * Verify failing arbitration, separate socket for arbitration
 */
TEST_F (MasterIntegrationTestNotInitialised, CciefbArbitrationFailsSeparateSocket)
{
   const cl_ipaddr_t remote_ip_other_master = 0x0102030F; /* 1.2.3.15 */
   cl_mock_udp_port_t * mock_cciefb_arbitration_port = &mock_data.udp_ports[1];
   uint8_t request_payload_other_master[SIZE_REQUEST_3_SLAVES] = {0};
   clm_cfg_t config_separate_socket                            = {};

   mock_cciefb_port = &mock_data.udp_ports[0];
   mock_slmp_port   = &mock_data.udp_ports[2];

   clal_memcpy (
      &config_separate_socket,
      sizeof (config_separate_socket),
      &config,
      sizeof (config));
   config_separate_socket.use_separate_arbitration_socket = true;

   /* Prepare request from other master */
   clal_memcpy (
      request_payload_other_master,
      sizeof (request_payload_other_master),
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_payload_other_master[47] = 0x0F; /* 1.2.3.15 */

   ASSERT_FALSE (mock_data.udp_ports[0].in_use);
   ASSERT_FALSE (mock_data.udp_ports[1].in_use);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);

   ASSERT_EQ (clm_master_init (&clm, &config_separate_socket, now), 0);

   ASSERT_TRUE (mock_data.udp_ports[0].in_use);
   ASSERT_TRUE (mock_data.udp_ports[1].in_use);
   ASSERT_TRUE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);

   EXPECT_EQ (mock_slmp_port->is_open, true);
   EXPECT_EQ (mock_slmp_port->port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_port->ip_addr, CL_IPADDR_ANY);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->port_number, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_cciefb_port->ip_addr, my_ip);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_arbitration_port->is_open, true);
   EXPECT_EQ (mock_cciefb_arbitration_port->port_number, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_cciefb_arbitration_port->ip_addr, broadcast_ip);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_recv, 0);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_arbitration_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_arbitration_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);

   /* Run master stack, arbitration not done yet */
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_arbitration_port->is_open, true);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_arbitration_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_arbitration_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_open, 1);

   /* Some other master sends request */
   mock_set_udp_fakedata (
      mock_cciefb_arbitration_port,
      remote_ip_other_master,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_other_master,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_arbitration_port->is_open, true);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_send, 0);
   EXPECT_EQ (
      mock_cciefb_arbitration_port->total_recv_bytes,
      SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_arbitration_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_arbitration_port->number_of_calls_open, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_STANDBY);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_error.error_message,
      CLM_ERROR_ARBITRATION_FAILED);
   EXPECT_EQ (cb_counters->master_cb_error.ip_addr, remote_ip_other_master);
   EXPECT_EQ (cb_counters->master_cb_error.argument_2, 0);
}

/**
 * Verify constant link time
 *
 * @req REQ_CL_PROTOCOL_24
 * @req REQ_CLM_CONFORMANCE_11
 * @req REQ_CLM_TIMING_01
 * @req REQ_CLM_TIMING_06
 *
 */
TEST_F (MasterIntegrationTestNotInitialised, CciefbConstantLinkTime)
{

   const uint32_t delta_time                          = 10000; /* 10 ms */
   config.hier.groups[gi].use_constant_link_scan_time = true;
   config.hier.groups[gi].slave_devices[sdi0].reserved_slave_device = true;
   cl_mock_cyclic_request_result_t result;
   clal_clear_memory (&result, sizeof (result));

   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (clm.parameter_no, parameter_no);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);

   /* Run master stack, arbitration not done yet */
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 1);

   /* Run master stack, arbitration done. Master sends request. */
   now += longer_than_arbitration_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SUSPEND);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (result.group_no, group_number);
   EXPECT_EQ (
      result.cyclic_transmission_state,
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.dl, SIZE_REQUEST_3_SLAVES - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   EXPECT_EQ (result.clock_info, mock_data.unix_timestamp_ms);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.parallel_off_timeout_count,
      config.hier.groups[gi].parallel_off_timeout_count);

   /* Slave (device index 1) responds. Master does not yet send. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);

   /* Wait almost the timeout. Master does not yet send. */
   now += timeout_value * 1000 - delta_time;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);

   /* Wait for constant link time to time out. Master will send next request.
    */
   now += 2 * delta_time;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SUSPEND);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 6);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_linkscan.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_linkscan.success, true);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 1);
   EXPECT_EQ (result.group_no, group_number);
   EXPECT_EQ (result.cyclic_transmission_state, 0x0002); /* Slave station 2
                                                          */
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.dl, SIZE_REQUEST_3_SLAVES - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   EXPECT_EQ (result.clock_info, mock_data.unix_timestamp_ms);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.parallel_off_timeout_count,
      config.hier.groups[gi].parallel_off_timeout_count);
}

/**
 * Use two masters in same binary
 *
 * @req REQ_CLM_DEPLOYMENT_01
 *
 */
TEST_F (MasterIntegrationTestNotInitialised, CciefbMultipleMastersStartup)
{
#ifdef __rtk__
   GTEST_SKIP() << "Skipping as we run on RT-Kernel";
#endif // __rtk__

   const cl_ipaddr_t my_ip2     = 0x01080304; /* IP 1.8.3.4 */
   const cl_ipaddr_t slave_ip2  = 0x01080305; /* IP 1.8.3.4 */
   const uint16_t parameter_no2 = 1;
   const int my_ifindex2        = 9;

   cl_ipaddr_t resulting_ip;
   cl_mock_cyclic_request_result_t result;
   clal_clear_memory (&result, sizeof (result));

   uint16_t * p_file_parameter_no             = nullptr;
   clm_t clm2                                 = {};
   clm_cfg_t config2                          = {};
   cl_mock_master_file_info_t * storage_file2 = &mock_data.storage_files[1];
   cl_mock_udp_port_t * mock_cciefb_port2     = &mock_data.udp_ports[2];
   cl_mock_udp_port_t * mock_slmp_port2       = &mock_data.udp_ports[3];
   cl_mock_master_callback_counters_t * cb_counters2 =
      &mock_data.master_cb_counters[1];

   clal_memcpy (&config2, sizeof (config2), &config, sizeof (config));
   config2.cb_arg                                      = cb_counters2;
   config2.master_id                                   = my_ip2;
   config2.hier.number_of_groups                       = 1;
   config2.hier.groups[0].num_slave_devices            = 1;
   config2.hier.groups[0].slave_devices[sdi0].slave_id = slave_ip2;
   config2.hier.groups[0].slave_devices[sdi0].num_occupied_stations = 1;
   (void)clal_copy_string (
      config2.file_directory,
      "directory2",
      sizeof (config.file_directory));

   ASSERT_FALSE (mock_data.udp_ports[0].in_use);
   ASSERT_FALSE (mock_data.udp_ports[1].in_use);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);
   ASSERT_FALSE (mock_data.udp_ports[4].in_use);
   EXPECT_EQ (storage_file->file_size, expected_filesize);
   EXPECT_EQ (storage_file2->file_size, (size_t)0);

   /* Start first master */
   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);
   ASSERT_TRUE (mock_data.udp_ports[0].in_use);
   ASSERT_TRUE (mock_data.udp_ports[1].in_use);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);
   ASSERT_FALSE (mock_data.udp_ports[4].in_use);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);

   /* Start second master */
   ASSERT_EQ (clm_master_init (&clm2, &config2, now), 0);
   ASSERT_TRUE (mock_data.udp_ports[0].in_use);
   ASSERT_TRUE (mock_data.udp_ports[0].is_open);
   ASSERT_TRUE (mock_data.udp_ports[1].in_use);
   ASSERT_TRUE (mock_data.udp_ports[1].is_open);
   ASSERT_TRUE (mock_data.udp_ports[2].in_use);
   ASSERT_TRUE (mock_data.udp_ports[2].is_open);
   ASSERT_TRUE (mock_data.udp_ports[3].in_use);
   ASSERT_TRUE (mock_data.udp_ports[3].is_open);
   ASSERT_FALSE (mock_data.udp_ports[4].in_use);
   EXPECT_EQ (mock_cciefb_port->port_number, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_cciefb_port->ip_addr, my_ip);
   EXPECT_EQ (mock_cciefb_port->ifindex, my_ifindex);
   EXPECT_EQ (mock_slmp_port->port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_port->ip_addr, CL_IPADDR_ANY);
   EXPECT_EQ (mock_slmp_port->ifindex, 0);
   EXPECT_EQ (mock_cciefb_port2->port_number, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_cciefb_port2->ip_addr, my_ip2);
   EXPECT_EQ (mock_cciefb_port2->ifindex, my_ifindex2);
   EXPECT_EQ (mock_slmp_port2->port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_port2->ip_addr, CL_IPADDR_ANY);
   EXPECT_EQ (mock_slmp_port2->ifindex, 0);
   EXPECT_EQ (cb_counters2->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters2->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);

   /* Check stored files */
   EXPECT_EQ (clm.parameter_no, parameter_no);
   EXPECT_EQ (storage_file->file_size, expected_filesize);
   p_file_parameter_no = (uint16_t *)&storage_file->file_content + 4;
   EXPECT_EQ (*p_file_parameter_no, parameter_no);

   EXPECT_EQ (clm2.parameter_no, parameter_no2);
   EXPECT_EQ (storage_file2->file_size, expected_filesize);
   p_file_parameter_no = (uint16_t *)&storage_file2->file_content + 4;
   EXPECT_EQ (*p_file_parameter_no, parameter_no2);

   /* Run both masters, arbitration not done yet */
   now += tick_size;
   clm_iefb_periodic (&clm, now);
   clm_iefb_periodic (&clm2, now);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters2->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);

   /* Run master stacks, arbitration done. Both send requests */
   now += longer_than_arbitration_us;
   clm_iefb_periodic (&clm, now);
   clm_iefb_periodic (&clm2, now);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port2->total_sent_bytes, SIZE_REQUEST_1_SLAVE);

   /* Verify request sent from first master */
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (result.group_no, group_number);
   EXPECT_EQ (
      result.cyclic_transmission_state,
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.parameter_no, parameter_no);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip_di0);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, remote_ip);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 3, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, UINT32_MAX);

   /* Verify request sent from second master */
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port2, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (result.group_no, group_number);
   EXPECT_EQ (
      result.cyclic_transmission_state,
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF);
   EXPECT_EQ (result.master_ip_addr, my_ip2);
   EXPECT_EQ (result.slave_total_occupied_station_count, 1);
   EXPECT_EQ (result.parameter_no, parameter_no2);
   ASSERT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port2, 1, &resulting_ip),
      0);
   EXPECT_EQ (resulting_ip, slave_ip2);

   /* Both slaves belonging to first master respond. Master sends new request
    */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);
   clm_iefb_periodic (&clm2, now);

   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_payload_di0,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);
   clm_iefb_periodic (&clm2, now);

   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port2->total_sent_bytes, SIZE_REQUEST_1_SLAVE);

   // TODO (rtljobe): More communication and checks
}

TEST_F (MasterIntegrationTestNoResponseYet, CciefbClearStatistics)
{
   /* Clear statistics for one slave device */
   clm.groups[gi].slave_devices[sdi0].statistics.number_of_connects   = 0x42;
   clm.groups[gi].slave_devices[sdi].statistics.number_of_connects    = 0x43;
   clm.groups[gi].slave_devices[sdi].statistics.number_of_disconnects = 0x44;
   clm.groups[gi].slave_devices[sdi].statistics.number_of_timeouts    = 0x45;
   clm.groups[gi].slave_devices[sdi].statistics.number_of_sent_frames = 0x46;
   clm.groups[gi].slave_devices[sdi].statistics.number_of_incoming_frames = 0x47;
   clm.groups[gi].slave_devices[sdi].statistics.number_of_incoming_invalid_frames =
      0x48;
   clm.groups[gi].slave_devices[sdi].statistics.number_of_incoming_alarm_frames =
      0x49;
   EXPECT_EQ (statistics->number_of_connects, 0x43U);
   clm_iefb_statistics_clear (&clm.groups[gi].slave_devices[sdi].statistics);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].statistics.number_of_connects,
      0x42U);
   EXPECT_EQ (statistics->number_of_connects, 0U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (statistics->number_of_timeouts, 0U);
   EXPECT_EQ (statistics->number_of_sent_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);

   /* Clear statistics for all slave devices */
   clm.groups[gi].slave_devices[sdi].statistics.number_of_connects    = 0x43;
   clm.groups[gi].slave_devices[sdi].statistics.number_of_disconnects = 0x44;
   clm.groups[gi].slave_devices[sdi].statistics.number_of_timeouts    = 0x45;
   clm.groups[gi].slave_devices[sdi].statistics.number_of_sent_frames = 0x46;
   clm.groups[gi].slave_devices[sdi].statistics.number_of_incoming_frames = 0x47;
   clm.groups[gi].slave_devices[sdi].statistics.number_of_incoming_invalid_frames =
      0x48;
   clm.groups[gi].slave_devices[sdi].statistics.number_of_incoming_alarm_frames =
      0x49;
   clm_iefb_statistics_clear_all (&clm);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].statistics.number_of_connects, 0U);
   EXPECT_EQ (statistics->number_of_connects, 0U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (statistics->number_of_timeouts, 0U);
   EXPECT_EQ (statistics->number_of_sent_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
}

/**
 * Parse cyclic data
 *
 * @req REQ_CLM_CONFORMANCE_02
 */
TEST_F (MasterIntegrationTestNoResponseYet, CciefbParseCyclicData)
{
   uint16_t resulting_occupied = 0;
   clm_device_framevalues_t * framevalues =
      &clm.groups[gi].slave_devices[sdi].latest_frame;
   const cl_rx_t * first_group_rx_area;
   const cl_rwr_t * first_group_rwr_area;
   const cl_rx_t * first_device_rx_area;
   const cl_rwr_t * first_device_rwr_area;

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_FALSE (framevalues->has_been_received);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (statistics->number_of_connects, 0U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (statistics->number_of_incoming_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (statistics->number_of_sent_frames, 1U);
   EXPECT_EQ (statistics->number_of_timeouts, 0U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   // TODO (rtljobe): Set outgoing data

   /* Slave device index 0 responds */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_payload_di0,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_RESPONSE_1_SLAVE);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);

   EXPECT_FALSE (framevalues->has_been_received);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);

   /* Slave device index 1 responds (no data storage as we are in WAIT_TD).
      Master sends new request. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_TRUE (framevalues->has_been_received);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (framevalues->group_no, 1);
   EXPECT_EQ (framevalues->num_occupied_stations, 2);
   EXPECT_EQ (framevalues->slave_id, remote_ip);
   EXPECT_EQ (framevalues->frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (statistics->number_of_connects, 1U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (statistics->number_of_sent_frames, 2U);
   EXPECT_EQ (statistics->number_of_timeouts, 0U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Slave device index 1 responds again. Store incoming data. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (statistics->number_of_connects, 1U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (statistics->number_of_incoming_frames, 2U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (statistics->number_of_sent_frames, 2U);
   EXPECT_EQ (statistics->number_of_timeouts, 0U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);

   /* Clear statistics */
   clm_iefb_statistics_clear (&clm.groups[gi].slave_devices[sdi].statistics);
   EXPECT_EQ (statistics->number_of_connects, 0U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (statistics->number_of_incoming_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (statistics->number_of_sent_frames, 0U);
   EXPECT_EQ (statistics->number_of_timeouts, 0U);

   /* Device RX memory area */
   resulting_occupied = 0;
   first_device_rx_area =
      clm_iefb_get_first_device_rx_area (&clm, gi, sdi, &resulting_occupied);
   EXPECT_EQ (resulting_occupied, 2);
   EXPECT_EQ (first_device_rx_area->bytes[0], 0x80);
   EXPECT_EQ (first_device_rx_area->bytes[1], 0x81);
   EXPECT_EQ (first_device_rx_area->bytes[2], 0x82);
   EXPECT_EQ (first_device_rx_area->bytes[3], 0x83);
   EXPECT_EQ (first_device_rx_area->bytes[4], 0x84);
   EXPECT_EQ (first_device_rx_area->bytes[5], 0x85);
   EXPECT_EQ (first_device_rx_area->bytes[6], 0x86);
   EXPECT_EQ (first_device_rx_area->bytes[7], 0x87);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[0], 0x88);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[1], 0x89);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[2], 0x8A);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[3], 0x8B);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[4], 0x8C);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[5], 0x8D);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[6], 0x8E);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[7], 0x8F);

   /* Device RWr memory area */
   resulting_occupied = 0;
   first_device_rwr_area =
      clm_iefb_get_first_device_rwr_area (&clm, gi, sdi, &resulting_occupied);
   EXPECT_EQ (resulting_occupied, 2);
   EXPECT_EQ (CC_FROM_LE16 (first_device_rwr_area->words[0]), 0x0100);
   EXPECT_EQ (CC_FROM_LE16 (first_device_rwr_area->words[1]), 0x0302);
   EXPECT_EQ (CC_FROM_LE16 (first_device_rwr_area->words[2]), 0x0504);
   EXPECT_EQ (CC_FROM_LE16 (first_device_rwr_area->words[31]), 0x3F3E);
   EXPECT_EQ (CC_FROM_LE16 ((first_device_rwr_area + 1)->words[0]), 0x4140);
   EXPECT_EQ (CC_FROM_LE16 ((first_device_rwr_area + 1)->words[1]), 0x4342);
   EXPECT_EQ (CC_FROM_LE16 ((first_device_rwr_area + 1)->words[2]), 0x4544);
   EXPECT_EQ (CC_FROM_LE16 ((first_device_rwr_area + 1)->words[31]), 0x7F7E);

   /* Group RX memory area */
   resulting_occupied = 0;
   first_group_rx_area =
      clm_iefb_get_first_rx_area (&clm, gi, &resulting_occupied);
   EXPECT_EQ (resulting_occupied, 3);
   EXPECT_EQ (first_group_rx_area->bytes[0], 0x00);
   EXPECT_EQ (first_group_rx_area->bytes[7], 0x00);
   EXPECT_EQ ((first_group_rx_area + 1)->bytes[0], 0x80);
   EXPECT_EQ ((first_group_rx_area + 1)->bytes[7], 0x87);
   EXPECT_EQ ((first_group_rx_area + 2)->bytes[0], 0x88);
   EXPECT_EQ ((first_group_rx_area + 2)->bytes[7], 0x8F);

   /* Group RWr memory area */
   resulting_occupied = 0;
   first_group_rwr_area =
      clm_iefb_get_first_rwr_area (&clm, gi, &resulting_occupied);
   EXPECT_EQ (resulting_occupied, 3);
   EXPECT_EQ (CC_FROM_LE16 (first_group_rwr_area->words[0]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (first_group_rwr_area->words[31]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 ((first_group_rwr_area + 1)->words[0]), 0x0100);
   EXPECT_EQ (CC_FROM_LE16 ((first_group_rwr_area + 1)->words[31]), 0x3F3E);
   EXPECT_EQ (CC_FROM_LE16 ((first_group_rwr_area + 2)->words[0]), 0x4140);
   EXPECT_EQ (CC_FROM_LE16 ((first_group_rwr_area + 2)->words[31]), 0x7F7E);

   /* RX bits. Value 0x80 for first byte in first occupied. */
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 0));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 1));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 2));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 3));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 4));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 5));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 6));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 7));
   /* RX bits. Value 0x87 for last byte in first occupied. */
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 56));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 57));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 58));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 59));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 60));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 61));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 62));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 63));
   /* RX bits. Value 0x88 for first byte in second occupied. */
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 64));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 65));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 66));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 67));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 68));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 69));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 70));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 71));
   /* RX bits. Value 0x8F for last byte in second occupied. */
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 120));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 121));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 122));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 123));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 124));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 125));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 126));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 127));

   /* RWr value */
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 0), 0x0100);
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 1), 0x0302);
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 2), 0x0504);
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 31), 0x3F3E);
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 32), 0x4140);
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 33), 0x4342);
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 34), 0x4544);
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 63), 0x7F7E);

   // TODO (rtljobe): Verify the cyclic data sent from the master
}

/**
 * Slave responds with alarm 'Wrong number of occupied slave stations'
 *
 * @req REQ_CL_PROTOCOL_50
 * @req REQ_CLM_DIAGNOSIS_01
 * @req REQ_CLM_DIAGNOSIS_02
 * @req REQ_CLM_ERROR_01
 * @req REQ_CLM_ERROR_06
 * @req REQ_CLS_CONFORMANCE_01
 *
 */
TEST_F (MasterIntegrationTestNoResponseYet, CciefbSlaveAlarmWrongNumberOccupied)
{
   cl_ipaddr_t resulting_ipaddr;
   clm_master_status_details_t master_details;
   clm_group_status_details_t group_details;
   const clm_slave_device_data_t * resulting_devicedata;
   cl_mock_cyclic_request_result_t result;
   uint8_t response_di1_alarm_wrong_occupied[SIZE_RESPONSE_2_SLAVES] = {};

   clal_clear_memory (&result, sizeof (result));
   clal_memcpy (
      response_di1_alarm_wrong_occupied,
      sizeof (response_di1_alarm_wrong_occupied),
      (uint8_t *)&response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   response_di1_alarm_wrong_occupied[13] = 0xE1;
   response_di1_alarm_wrong_occupied[14] = 0xCF;

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_FALSE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_FALSE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (statistics->number_of_connects, 0U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (statistics->number_of_incoming_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (statistics->number_of_sent_frames, 1U);
   EXPECT_EQ (statistics->number_of_timeouts, 0U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Slave device index 1 responds with alarm 'Wrong number of occupied slave
      stations' */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_alarm_wrong_occupied,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_FALSE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_FALSE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_error.error_message,
      CLM_ERROR_SLAVE_REPORTS_WRONG_NUMBER_OCCUPIED);
   EXPECT_EQ (cb_counters->master_cb_error.ip_addr, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_error.argument_2, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Application reads out diagnostic information */
   EXPECT_EQ (clm_get_master_status (&clm, &master_details), 0);
   EXPECT_EQ (master_details.master_state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (master_details.node_search_serial, -1);
   EXPECT_EQ (master_details.set_ip_request_serial, -1);
   EXPECT_EQ (master_details.parameter_no, parameter_no);
   EXPECT_EQ (clm_get_group_status (&clm, gi, &group_details), 0);
   EXPECT_EQ (group_details.group_index, gi);
   EXPECT_EQ (group_details.group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (group_details.timestamp_link_scan_start, now - 2 * tick_size);
   /* No transmission bits are on yet */
   EXPECT_EQ (group_details.cyclic_transmission_state, 0x0000);
   EXPECT_EQ (group_details.frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (group_details.total_occupied, slaves_in_group);
   resulting_devicedata = clm_get_device_connection_details (&clm, gi, sdi);
   EXPECT_NE (resulting_devicedata, nullptr);
   EXPECT_EQ (resulting_devicedata->device_index, sdi);
   EXPECT_EQ (resulting_devicedata->device_state, CLM_DEVICE_STATE_LISTEN);
   EXPECT_TRUE (resulting_devicedata->enabled);
   EXPECT_FALSE (resulting_devicedata->force_transmission_bit);
   EXPECT_FALSE (resulting_devicedata->transmission_bit);
   EXPECT_EQ (resulting_devicedata->slave_station_no, 2);
   EXPECT_EQ (resulting_devicedata->timeout_count, 0);
   EXPECT_EQ (
      resulting_devicedata->statistics.number_of_incoming_invalid_frames,
      0U);
   EXPECT_EQ (resulting_devicedata->statistics.number_of_connects, 0U);
   EXPECT_EQ (resulting_devicedata->statistics.number_of_disconnects, 0U);
   EXPECT_EQ (resulting_devicedata->statistics.number_of_incoming_frames, 1U);
   EXPECT_EQ (resulting_devicedata->statistics.number_of_incoming_alarm_frames, 1U);
   EXPECT_EQ (resulting_devicedata->statistics.number_of_sent_frames, 1U);
   EXPECT_EQ (resulting_devicedata->statistics.number_of_timeouts, 0U);
   EXPECT_EQ (resulting_devicedata->statistics.measured_time.number_of_samples, 1U);
   EXPECT_EQ (resulting_devicedata->statistics.measured_time.min, 2 * tick_size);
   EXPECT_EQ (resulting_devicedata->statistics.measured_time.max, 2 * tick_size);
   EXPECT_EQ (resulting_devicedata->statistics.measured_time.sum, 2 * tick_size);
   EXPECT_EQ (
      resulting_devicedata->statistics.measured_time.average,
      2 * tick_size);
   EXPECT_TRUE (resulting_devicedata->latest_frame.has_been_received);
   EXPECT_EQ (
      resulting_devicedata->latest_frame.end_code,
      CL_SLMP_ENDCODE_CCIEFB_WRONG_NUMBER_OCCUPIED_STATIONS);
   EXPECT_EQ (
      resulting_devicedata->latest_frame.equipment_ver,
      slave_equipment_ver);
   EXPECT_EQ (
      resulting_devicedata->latest_frame.frame_sequence_no,
      frame_sequenceno_startup);
   EXPECT_EQ (resulting_devicedata->latest_frame.group_no, group_number);
   EXPECT_EQ (
      resulting_devicedata->latest_frame.local_management_info,
      alarm_local_management_info);
   EXPECT_EQ (resulting_devicedata->latest_frame.model_code, slave_model_code);
   EXPECT_EQ (resulting_devicedata->latest_frame.num_occupied_stations, 2);
   EXPECT_EQ (
      resulting_devicedata->latest_frame.protocol_ver,
      CL_CCIEFB_MAX_SUPPORTED_PROTOCOL_VER);
   EXPECT_EQ (resulting_devicedata->latest_frame.response_time, 2 * tick_size);
   EXPECT_EQ (
      resulting_devicedata->latest_frame.slave_err_code,
      alarm_slave_err_code);
   EXPECT_EQ (resulting_devicedata->latest_frame.slave_id, remote_ip);
   EXPECT_EQ (
      resulting_devicedata->latest_frame.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (resulting_devicedata->latest_frame.vendor_code, slave_vendor_code);

   /* Slave device index 0 responds,
    * Master sends a new request immediately. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_payload_di0,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_FALSE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 1);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for one slave only */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0001);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);

   /* Slave device index 1 responds (again) with alarm 'Wrong number of occupied
      slave stations' */
   response_di1_alarm_wrong_occupied[57] = 0x01; /* Sequence number */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_alarm_wrong_occupied,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_FALSE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Slave device index 0 responds again,
    * Master sends a new request immediately. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 11);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      2 * SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_FALSE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 2);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for one slave only */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0001);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (result.parameter_no, parameter_no);
}

TEST_F (MasterIntegrationTestNoResponseYet, CciefbNoConnectCallback)
{
   clm.config.connect_cb = nullptr;
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);

   /* Slave device index 1 responds */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   /* Slave (device index 0) responds.
      Master sends new request immediately. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_payload_di0,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   /* Slave (device index 1) responds again (with new sequence number) */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += 3 * tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
}

TEST_F (MasterIntegrationTestNoResponseYet, CciefbGetFirstAreaValidGroup)
{
   const uint16_t expexted_occupied_in_group = 3;

   uint16_t resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)clm_iefb_get_first_rx_area (&clm, gi, &resulting_occupied),
      (void *)clm.groups[gi].memory_area.rx);
   EXPECT_EQ (resulting_occupied, expexted_occupied_in_group);

   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)clm_iefb_get_first_ry_area (&clm, gi, &resulting_occupied),
      (void *)clm.groups[gi].memory_area.ry);
   EXPECT_EQ (resulting_occupied, expexted_occupied_in_group);

   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)clm_iefb_get_first_rwr_area (&clm, gi, &resulting_occupied),
      (void *)clm.groups[gi].memory_area.rwr);
   EXPECT_EQ (resulting_occupied, expexted_occupied_in_group);

   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)clm_iefb_get_first_rww_area (&clm, gi, &resulting_occupied),
      (void *)clm.groups[gi].memory_area.rww);
   EXPECT_EQ (resulting_occupied, expexted_occupied_in_group);
}

TEST_F (MasterIntegrationTestNoResponseYet, CciefbGetFirstAreaInvalidGroup)
{
   const uint16_t invalid_gi = 99; /* Invalid group index */

   uint16_t resulting_occupied = UINT16_MAX;
   EXPECT_TRUE (
      clm_iefb_get_first_rx_area (&clm, invalid_gi, &resulting_occupied) ==
      nullptr);
   EXPECT_EQ (resulting_occupied, 0);

   resulting_occupied = UINT16_MAX;
   EXPECT_TRUE (
      clm_iefb_get_first_ry_area (&clm, invalid_gi, &resulting_occupied) ==
      nullptr);
   EXPECT_EQ (resulting_occupied, 0);

   resulting_occupied = UINT16_MAX;
   EXPECT_TRUE (
      clm_iefb_get_first_rwr_area (&clm, invalid_gi, &resulting_occupied) ==
      nullptr);
   EXPECT_EQ (resulting_occupied, 0);

   resulting_occupied = UINT16_MAX;
   EXPECT_TRUE (
      clm_iefb_get_first_rww_area (&clm, invalid_gi, &resulting_occupied) ==
      nullptr);
   EXPECT_EQ (resulting_occupied, 0);
}

TEST_F (MasterIntegrationTestNoResponseYet, CciefbGetFirstDeviceValid)
{
   uint16_t resulting_occupied                 = UINT16_MAX;
   const uint16_t expexted_occupied_for_device = 1;

   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)
         clm_iefb_get_first_device_rx_area (&clm, gi, sdi0, &resulting_occupied),
      (void *)clm.groups[gi].memory_area.rx);
   EXPECT_EQ (resulting_occupied, expexted_occupied_for_device);

   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)
         clm_iefb_get_first_device_ry_area (&clm, gi, sdi0, &resulting_occupied),
      (void *)clm.groups[gi].memory_area.ry);
   EXPECT_EQ (resulting_occupied, expexted_occupied_for_device);

   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)
         clm_iefb_get_first_device_rwr_area (&clm, gi, sdi0, &resulting_occupied),
      (void *)clm.groups[gi].memory_area.rwr);
   EXPECT_EQ (resulting_occupied, expexted_occupied_for_device);

   resulting_occupied = UINT16_MAX;
   EXPECT_EQ (
      (void *)
         clm_iefb_get_first_device_rww_area (&clm, gi, sdi0, &resulting_occupied),
      (void *)clm.groups[gi].memory_area.rww);
   EXPECT_EQ (resulting_occupied, expexted_occupied_for_device);
}

TEST_F (MasterIntegrationTestNoResponseYet, CciefbGetFirstDeviceAreaInvalid)
{
   const uint16_t invalid_gi   = 99; /* Invalid group index */
   const uint16_t invalid_sdi  = 98; /* Invalid slave device index */
   uint16_t resulting_occupied = UINT16_MAX;

   /* Invalid group index */
   resulting_occupied = UINT16_MAX;
   EXPECT_TRUE (
      clm_iefb_get_first_device_rx_area (&clm, invalid_gi, 0, &resulting_occupied) ==
      nullptr);
   EXPECT_EQ (resulting_occupied, 0);

   resulting_occupied = UINT16_MAX;
   EXPECT_TRUE (
      clm_iefb_get_first_device_ry_area (&clm, invalid_gi, 0, &resulting_occupied) ==
      nullptr);
   EXPECT_EQ (resulting_occupied, 0);

   resulting_occupied = UINT16_MAX;
   EXPECT_TRUE (
      clm_iefb_get_first_device_rwr_area (&clm, invalid_gi, 0, &resulting_occupied) ==
      nullptr);
   EXPECT_EQ (resulting_occupied, 0);

   resulting_occupied = UINT16_MAX;
   EXPECT_TRUE (
      clm_iefb_get_first_device_rww_area (&clm, invalid_gi, 0, &resulting_occupied) ==
      nullptr);
   EXPECT_EQ (resulting_occupied, 0);

   /* Invalid slave device index */
   resulting_occupied = UINT16_MAX;
   EXPECT_TRUE (
      clm_iefb_get_first_device_rx_area (&clm, 0, invalid_sdi, &resulting_occupied) ==
      nullptr);
   EXPECT_EQ (resulting_occupied, 0);

   resulting_occupied = UINT16_MAX;
   EXPECT_TRUE (
      clm_iefb_get_first_device_ry_area (&clm, 0, invalid_sdi, &resulting_occupied) ==
      nullptr);
   EXPECT_EQ (resulting_occupied, 0);

   resulting_occupied = UINT16_MAX;
   EXPECT_TRUE (
      clm_iefb_get_first_device_rwr_area (&clm, 0, invalid_sdi, &resulting_occupied) ==
      nullptr);
   EXPECT_EQ (resulting_occupied, 0);

   resulting_occupied = UINT16_MAX;
   EXPECT_TRUE (
      clm_iefb_get_first_device_rww_area (&clm, 0, invalid_sdi, &resulting_occupied) ==
      nullptr);
   EXPECT_EQ (resulting_occupied, 0);
}

TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbParseCyclicData)
{
   uint16_t resulting_occupied = 0;
   clm_device_framevalues_t * framevalues =
      &clm.groups[gi].slave_devices[sdi].latest_frame;
   const cl_rx_t * first_group_rx_area;
   const cl_rwr_t * first_group_rwr_area;
   const cl_rx_t * first_device_rx_area;
   const cl_rwr_t * first_device_rwr_area;

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_TRUE (framevalues->has_been_received);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Slave responds */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      2 * SIZE_RESPONSE_2_SLAVES + SIZE_RESPONSE_1_SLAVE);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_TRUE (framevalues->has_been_received);
   EXPECT_EQ (framevalues->group_no, 1);
   EXPECT_EQ (framevalues->num_occupied_stations, 2);
   EXPECT_EQ (framevalues->slave_id, remote_ip);
   EXPECT_EQ (framevalues->frame_sequence_no, frame_sequenceno_startup + 1);

   /* Device RX memory area */
   resulting_occupied = 0;
   first_device_rx_area =
      clm_iefb_get_first_device_rx_area (&clm, gi, sdi, &resulting_occupied);
   EXPECT_EQ (resulting_occupied, 2);
   EXPECT_EQ (first_device_rx_area->bytes[0], 0x80);
   EXPECT_EQ (first_device_rx_area->bytes[1], 0x81);
   EXPECT_EQ (first_device_rx_area->bytes[2], 0x82);
   EXPECT_EQ (first_device_rx_area->bytes[3], 0x83);
   EXPECT_EQ (first_device_rx_area->bytes[4], 0x84);
   EXPECT_EQ (first_device_rx_area->bytes[5], 0x85);
   EXPECT_EQ (first_device_rx_area->bytes[6], 0x86);
   EXPECT_EQ (first_device_rx_area->bytes[7], 0x87);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[0], 0x88);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[1], 0x89);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[2], 0x8A);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[3], 0x8B);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[4], 0x8C);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[5], 0x8D);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[6], 0x8E);
   EXPECT_EQ ((first_device_rx_area + 1)->bytes[7], 0x8F);

   /* Device RWr memory area */
   resulting_occupied = 0;
   first_device_rwr_area =
      clm_iefb_get_first_device_rwr_area (&clm, gi, sdi, &resulting_occupied);
   EXPECT_EQ (resulting_occupied, 2);
   EXPECT_EQ (CC_FROM_LE16 (first_device_rwr_area->words[0]), 0x0100);
   EXPECT_EQ (CC_FROM_LE16 (first_device_rwr_area->words[1]), 0x0302);
   EXPECT_EQ (CC_FROM_LE16 (first_device_rwr_area->words[2]), 0x0504);
   EXPECT_EQ (CC_FROM_LE16 (first_device_rwr_area->words[31]), 0x3F3E);
   EXPECT_EQ (CC_FROM_LE16 ((first_device_rwr_area + 1)->words[0]), 0x4140);
   EXPECT_EQ (CC_FROM_LE16 ((first_device_rwr_area + 1)->words[1]), 0x4342);
   EXPECT_EQ (CC_FROM_LE16 ((first_device_rwr_area + 1)->words[2]), 0x4544);
   EXPECT_EQ (CC_FROM_LE16 ((first_device_rwr_area + 1)->words[31]), 0x7F7E);

   /* Group RX memory area */
   resulting_occupied = 0;
   first_group_rx_area =
      clm_iefb_get_first_rx_area (&clm, gi, &resulting_occupied);
   EXPECT_EQ (resulting_occupied, 3);
   EXPECT_EQ (first_group_rx_area->bytes[0], 0x00);
   EXPECT_EQ (first_group_rx_area->bytes[7], 0x00);
   EXPECT_EQ ((first_group_rx_area + 1)->bytes[0], 0x80);
   EXPECT_EQ ((first_group_rx_area + 1)->bytes[7], 0x87);
   EXPECT_EQ ((first_group_rx_area + 2)->bytes[0], 0x88);
   EXPECT_EQ ((first_group_rx_area + 2)->bytes[7], 0x8F);

   /* Group RWr memory area */
   resulting_occupied = 0;
   first_group_rwr_area =
      clm_iefb_get_first_rwr_area (&clm, gi, &resulting_occupied);
   EXPECT_EQ (resulting_occupied, 3);
   EXPECT_EQ (CC_FROM_LE16 (first_group_rwr_area->words[0]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (first_group_rwr_area->words[31]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 ((first_group_rwr_area + 1)->words[0]), 0x0100);
   EXPECT_EQ (CC_FROM_LE16 ((first_group_rwr_area + 1)->words[31]), 0x3F3E);
   EXPECT_EQ (CC_FROM_LE16 ((first_group_rwr_area + 2)->words[0]), 0x4140);
   EXPECT_EQ (CC_FROM_LE16 ((first_group_rwr_area + 2)->words[31]), 0x7F7E);

   /* RX bits. Value 0x80 for first byte in first occupied. */
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 0));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 1));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 2));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 3));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 4));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 5));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 6));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 7));
   /* RX bits. Value 0x87 for last byte in first occupied. */
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 56));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 57));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 58));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 59));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 60));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 61));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 62));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 63));
   /* RX bits. Value 0x88 for first byte in second occupied. */
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 64));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 65));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 66));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 67));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 68));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 69));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 70));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 71));
   /* RX bits. Value 0x8F for last byte in second occupied. */
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 120));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 121));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 122));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 123));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 124));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 125));
   EXPECT_FALSE (clm_iefb_get_rx_bit (&clm, gi, sdi, 126));
   EXPECT_TRUE (clm_iefb_get_rx_bit (&clm, gi, sdi, 127));

   /* RWr value */
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 0), 0x0100);
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 1), 0x0302);
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 2), 0x0504);
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 31), 0x3F3E);
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 32), 0x4140);
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 33), 0x4342);
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 34), 0x4544);
   EXPECT_EQ (clm_iefb_get_rwr_value (&clm, gi, sdi, 63), 0x7F7E);
}

/**
 * Disable transmission status for one of the two slaves.
 *
 * @req REQ_CL_PROTOCOL_33
 * @req REQ_CLM_CONFORMANCE_04
 * @req REQ_CLM_PARAMETERID_03
 * @req REQ_CLM_STATUSBIT_03
 * @req REQ_CLM_STATUSBIT_08
 *
 */
TEST_F (
   MasterIntegrationTestBothDevicesResponded,
   CciefbStateTransitionToCyclicSuspend)
{
   cl_mock_cyclic_request_result_t result;
   clal_clear_memory (&result, sizeof (result));
   cl_ipaddr_t resulting_ipaddr;
   cl_rww_t * resulting_rww_area;
   uint16_t registernumber                                             = 3;
   uint16_t value_A                                                    = 0xCAFE;
   uint16_t value_B                                                    = 0xBABE;
   uint8_t response_di1_higher_sequence_number[SIZE_RESPONSE_2_SLAVES] = {};
   uint8_t response_di0_higher_sequence_number[SIZE_RESPONSE_1_SLAVE]  = {};

   clal_memcpy (
      response_di1_higher_sequence_number,
      sizeof (response_di1_higher_sequence_number),
      (uint8_t *)&response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   response_di1_higher_sequence_number[57] = 0x02;
   clal_memcpy (
      response_di0_higher_sequence_number,
      sizeof (response_di0_higher_sequence_number),
      (uint8_t *)&response_payload_di0,
      SIZE_RESPONSE_1_SLAVE);
   response_di0_higher_sequence_number[57] = 0x02;

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_TRUE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (clm.parameter_no, parameter_no);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Set output data in master for both slaves */
   clm_set_rww_value (&clm, gi, sdi0, registernumber, value_A);
   clm_set_rww_value (&clm, gi, sdi, registernumber, value_B);
   EXPECT_EQ (clm_get_rww_value (&clm, gi, sdi0, registernumber), value_A);
   EXPECT_EQ (clm_get_rww_value (&clm, gi, sdi, registernumber), value_B);

   /* Slave di1 (device index 1, here slave station 2) responds */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request immediately. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_TRUE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 2);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.dl, SIZE_REQUEST_3_SLAVES - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   EXPECT_EQ (result.clock_info, mock_data.unix_timestamp_ms);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      result.parallel_off_timeout_count,
      config.hier.groups[gi].parallel_off_timeout_count);
   /* IP addresses in request */
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Data is available for both slaves in the request */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);

   /* Slave di1 (device index 1, here slave station 2) responds */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   ASSERT_TRUE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);

   /* Disable slave di1 */
   clm_iefb_set_slave_communication_status (&clm, gi, sdi, false);

   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   ASSERT_TRUE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request immediately. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SUSPEND);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 3);
   /* Transmission bit is still on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   /* Parameter number does not change. */
   EXPECT_EQ (result.parameter_no, parameter_no);
   /* IP address for slave station 2 is still valid */
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Data is still set for both slaves */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);

   /* Slave device index 0 (slave station 1) responds.
         Master sends new request immediately. */
   response_di0_higher_sequence_number[57] = 0x03;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SUSPEND);
   EXPECT_FALSE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 5);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_id, remote_ip);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 4);
   /* Transmission bit is on for slave station 1 only */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0001);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   /* Parameter number does not change. */
   EXPECT_EQ (result.parameter_no, parameter_no);
   /* IP address for slave station 2 is set to 0.0.0.0 in request */
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, CL_IPADDR_INVALID);
   /* Data is set only for one slave in the request */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), 0);

   /* Enable slave di1 */
   clm_iefb_set_slave_communication_status (&clm, gi, sdi, true);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request immediately. */
   response_di0_higher_sequence_number[57] = 0x04;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 6);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 5);
   /* Transmission bit is on for slave station 1 only so far */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0001);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   /* Parameter number does not change. */
   EXPECT_EQ (result.parameter_no, parameter_no);
   /* IP addresses in request */
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Data is set only for one slave in the request */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), 0);

   /* Slave device index 0 (slave station 1) responds.
      Slave device index 1 (slave station 2) has not yet responded.
      Master sends new request immediately. */
   response_di0_higher_sequence_number[57] = 0x05;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 7);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 6);
   /* Transmission bit is still on for only one slave */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0001);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   /* Parameter number does not change. */
   EXPECT_EQ (result.parameter_no, parameter_no);
   /* IP addresses in request */
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Data is set only for one slave in the request */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), 0);

   /* Slave device index 1 (slave station 2) responds too late,
      now with wrong sequence number.
      However the master will consider it reconnected.
      Master waits for device index 0. */
   response_di1_higher_sequence_number[57] = 0x05;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 7);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request immediately. */
   response_di0_higher_sequence_number[57] = 0x06;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 8);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 7);
   /* Transmission bits are on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   /* Parameter number does not change. */
   EXPECT_EQ (result.parameter_no, parameter_no);
   /* IP addresses in request */
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Verify that data is set for both slaves */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);
}

/**
 * Master application is stopped
 *
 * Setting the master application status should not affect the connection or the
 * data sent to the slaves.
 *
 * @req REQ_CLM_COMMUNIC_01
 * @req REQ_CLM_CONFORMANCE_01
 * @req REQ_CL_PROTOCOL_13
 * @req REQ_CL_PROTOCOL_14
 * @req REQ_CL_PROTOCOL_16
 *
 */
TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbMasterApplicationStopped)
{
   cl_mock_cyclic_request_result_t result;
   clal_clear_memory (&result, sizeof (result));
   cl_ipaddr_t resulting_ipaddr;
   cl_rww_t * resulting_rww_area;
   uint16_t registernumber                                             = 4;
   uint16_t value_A                                                    = 789;
   uint16_t value_B                                                    = 890;
   uint8_t response_di1_higher_sequence_number[SIZE_RESPONSE_2_SLAVES] = {};
   uint8_t response_di0_higher_sequence_number[SIZE_RESPONSE_1_SLAVE]  = {};

   clal_memcpy (
      response_di1_higher_sequence_number,
      sizeof (response_di1_higher_sequence_number),
      (uint8_t *)&response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   response_di1_higher_sequence_number[57] = 0x02;
   clal_memcpy (
      response_di0_higher_sequence_number,
      sizeof (response_di0_higher_sequence_number),
      (uint8_t *)&response_payload_di0,
      SIZE_RESPONSE_1_SLAVE);
   response_di0_higher_sequence_number[57] = 0x02;

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.parameter_no, parameter_no);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Set output data in master for both slaves */
   clm_set_rww_value (&clm, gi, sdi0, registernumber, value_A);
   clm_set_rww_value (&clm, gi, sdi, registernumber, value_B);
   EXPECT_EQ (clm_get_rww_value (&clm, gi, sdi0, registernumber), value_A);
   EXPECT_EQ (clm_get_rww_value (&clm, gi, sdi, registernumber), value_B);

   /* Slave di1 (device index 1, here slave station 2) responds */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 2);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.dl, SIZE_REQUEST_3_SLAVES - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   EXPECT_EQ (result.clock_info, mock_data.unix_timestamp_ms);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   /* Parameter number does not change. */
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Verify that data is set for both slaves */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);

   /* Set master application as stopped by user */
   clm_set_master_application_status (&clm, false, true);
   EXPECT_EQ (clm_get_master_application_status (&clm), 0x0002);

   /* Slave di1 (device index 1, here slave station 2) responds */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 3);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   /* Parameter number does not change. */
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED_BY_USER);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Verify that data is set for both slaves */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);

   /* Slave di1 (device index 1, here slave station 2) responds */
   response_di1_higher_sequence_number[57] = 0x03;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request. */
   response_di0_higher_sequence_number[57] = 0x03;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 5);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 4);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   /* Parameter number does not change. */
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED_BY_USER);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Verify that data is set for both slaves */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);

   /* Set master application as stopped by error */
   clm_set_master_application_status (&clm, false, false);
   EXPECT_EQ (clm_get_master_application_status (&clm), 0x0000);

   /* Slave di1 (device index 1, here slave station 2) responds */
   response_di1_higher_sequence_number[57] = 0x04;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 5);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request. */
   response_di0_higher_sequence_number[57] = 0x04;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 6);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 5);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   /* Parameter number does not change. */
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Verify that data is set for both slaves */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);

   /* Set master application as running */
   clm_set_master_application_status (&clm, true, false);
   EXPECT_EQ (clm_get_master_application_status (&clm), 0x0001);

   /* Slave di1 (device index 1, here slave station 2) responds */
   response_di1_higher_sequence_number[57] = 0x05;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 6);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request. */
   response_di0_higher_sequence_number[57] = 0x05;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 7);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 6);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   /* Parameter number does not change. */
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Verify that data is set for both slaves */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);
}

/** Master updates configuration
 *
 * @req REQ_CLM_PARAMETERID_02
 *
 */
TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbNewConfig)
{
   cl_ipaddr_t new_slave_ip = 0x01020355;
   cl_ipaddr_t resulting_ipaddr;
   cl_mock_cyclic_request_result_t result;
   clal_clear_memory (&result, sizeof (result));

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);

   /* Stop the master */
   EXPECT_EQ (clm_master_exit (&clm), 0);

   /* Modify configuration*/
   config.hier.groups[0].slave_devices[1].slave_id = new_slave_ip;

   /* Start the master again */
   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);

   /* Master sends request */
   now += longer_than_arbitration_us;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);

   now += tick_size;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);

   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is off for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0000);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   /* Parameter number is increased. */
   EXPECT_EQ (result.parameter_no, parameter_no + 1);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, new_slave_ip);
}

/**
 * One slave is lost
 *
 * @req REQ_CL_PROTOCOL_27
 * @req REQ_CL_STATUSBIT_01
 * @req REQ_CLM_COMMUNIC_04
 * @req REQ_CLM_CONFORMANCE_06
 * @req REQ_CLM_CONFORMANCE_08
 * @req REQ_CLM_STATUSBIT_01
 * @req REQ_CLM_STATUSBIT_02
 * @req REQ_CLM_STATUSBIT_06
 * @req REQ_CLM_STATUSBIT_07
 * @req REQ_CLM_TIMING_09
 * @req REQ_CLM_TIMING_10
 *
 */
TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbOneSlaveLost)
{
   cl_mock_cyclic_request_result_t result;
   clal_clear_memory (&result, sizeof (result));
   cl_ipaddr_t resulting_ipaddr;
   cl_rww_t * resulting_rww_area;
   uint16_t registernumber                                             = 4;
   uint16_t value_A                                                    = 789;
   uint16_t value_B                                                    = 890;
   uint8_t response_di1_higher_sequence_number[SIZE_RESPONSE_2_SLAVES] = {};
   uint8_t response_di0_higher_sequence_number[SIZE_RESPONSE_1_SLAVE]  = {};

   clal_memcpy (
      response_di1_higher_sequence_number,
      sizeof (response_di1_higher_sequence_number),
      (uint8_t *)&response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   response_di1_higher_sequence_number[57] = 0x02;
   clal_memcpy (
      response_di0_higher_sequence_number,
      sizeof (response_di0_higher_sequence_number),
      (uint8_t *)&response_payload_di0,
      SIZE_RESPONSE_1_SLAVE);
   response_di0_higher_sequence_number[57] = 0x02;

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.parameter_no, parameter_no);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Set output data in master for both slaves */
   clm_set_rww_value (&clm, gi, sdi0, registernumber, value_A);
   clm_set_rww_value (&clm, gi, sdi, registernumber, value_B);
   EXPECT_EQ (clm_get_rww_value (&clm, gi, sdi0, registernumber), value_A);
   EXPECT_EQ (clm_get_rww_value (&clm, gi, sdi, registernumber), value_B);

   /* Slave di1 (device index 1, here slave station 2) responds */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request immediately. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 2);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.dl, SIZE_REQUEST_3_SLAVES - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   EXPECT_EQ (result.clock_info, mock_data.unix_timestamp_ms);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Verify that data is set for both slaves */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);

   /* Slave di1 (device index 1, here slave station 2) responds */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);

   /* No response from slave device index 0 (slave station 1) */
   now += tick_size;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);

   /* Wait for time out. Master sends new request */
   now += almost_timeout_us;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);

   now += 5 * tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 1);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 3);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.dl, SIZE_REQUEST_3_SLAVES - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   EXPECT_EQ (result.clock_info, mock_data.unix_timestamp_ms);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Verify that data is set for both slaves */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);

   /* Slave di1 (device index 1, here slave station 2) responds */
   response_di1_higher_sequence_number[57] = 0x03;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 1);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);

   /* Wait for time out. Master sends new request */
   now += almost_timeout_us;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);

   now += 5 * tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 2);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 5);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 4);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Verify that data is set for both slaves */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);

   /* Slave di1 (device index 1, here slave station 2) responds */
   response_di1_higher_sequence_number[57] = 0x04;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 2);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 5);

   /* Wait for time out. Master disconnect slave and sends new request */
   now += almost_timeout_us;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 5);

   now += 5 * tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 6);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 5);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is off for one of the slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0002);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Verify that data is set for one slave only */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), 0);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);

   /* Slave di1 (device index 1, here slave station 2) responds
      Master sends request immediately. */
   response_di1_higher_sequence_number[57] = 0x05;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 7);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 6);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is off for one of the slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0002);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Verify that data is set for one slave only */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), 0);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);

   /* Slave device index 0 (slave station 1) returns with wrong sequence number
      (however not the last one, as that would give slave duplication).
      Master waits for other slave. */
   response_di0_higher_sequence_number[57] = 0x09;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 7);

   /* Slave di1 (device index 1, here slave station 2) responds
      Master sends request immediately. */
   response_di1_higher_sequence_number[57] = 0x06;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 8);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 7);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves again */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   /* Verify that data is set for both slaves */
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 1);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_A);
   resulting_rww_area = mock_analyze_cyclic_request_rww (mock_cciefb_port, 3, 2);
   EXPECT_NE (resulting_rww_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rww_area->words[registernumber]), value_B);
}

/**
 * Master detects slave duplication. Only a single slave in the config.
 *
 * @req REQ_CLM_ERROR_05
 * @req REQ_CLM_CONFORMANCE_14
 */
TEST_F (MasterIntegrationTestNotInitialised, CciefbSlaveIdDuplicationSingleSlave)
{
   const clm_slave_device_data_t * slave_device_connection_details = nullptr;
   const clm_slave_device_statistics_t * statistics                = nullptr;
   const clm_device_framevalues_t * framevalues                    = nullptr;
   // cl_ipaddr_t resulting_ip;
   cl_mock_cyclic_request_result_t result;
   clm_master_status_details_t master_details;
   clm_group_status_details_t group_details;
   clal_clear_memory (&result, sizeof (result));
   clal_clear_memory (&master_details, sizeof (master_details));
   clal_clear_memory (&group_details, sizeof (group_details));

   config.hier.groups[0].num_slave_devices = 1;

   ASSERT_EQ (clm_master_init (&clm, &config, now), 0);

   ASSERT_EQ (&mock_data.udp_ports[0], mock_cciefb_port);
   ASSERT_EQ (&mock_data.udp_ports[1], mock_slmp_port);
   ASSERT_EQ (mock_cciefb_port->port_number, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_cciefb_port->ip_addr, my_ip);
   EXPECT_EQ (mock_cciefb_port->ifindex, my_ifindex);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   ASSERT_EQ (mock_slmp_port->port_number, CL_SLMP_PORT);
   EXPECT_EQ (mock_slmp_port->ip_addr, CL_IPADDR_ANY);
   EXPECT_EQ (mock_slmp_port->ifindex, 0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);

   slave_device_connection_details =
      clm_iefb_get_device_connection_details (&clm, gi, sdi0);
   ASSERT_TRUE (slave_device_connection_details != nullptr);
   statistics  = &slave_device_connection_details->statistics;
   framevalues = &slave_device_connection_details->latest_frame;

   EXPECT_FALSE (framevalues->has_been_received);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (statistics->number_of_connects, 0U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);

   /* Run master stack, arbitration not done yet */
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_ARBITRATION);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_ARBITRATION);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (statistics->number_of_connects, 0U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   /* Run master stack, arbitration done.
      Master sends request. */
   now += longer_than_arbitration_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);

   EXPECT_EQ (cb_counters->master_cb_connect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (statistics->number_of_connects, 0U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_REQUEST_1_SLAVE);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, broadcast_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (result.group_no, group_number);
   EXPECT_EQ (
      result.cyclic_transmission_state,
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.dl, SIZE_REQUEST_1_SLAVE - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   EXPECT_EQ (result.clock_info, mock_data.unix_timestamp_ms);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, 1);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.parallel_off_timeout_count,
      config.hier.groups[gi].parallel_off_timeout_count);

   EXPECT_EQ (clm_iefb_get_master_status (&clm, &master_details), 0);
   EXPECT_EQ (master_details.master_state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (master_details.parameter_no, parameter_no);
   EXPECT_EQ (master_details.node_search_serial, CLM_SLMP_SERIAL_NONE);
   EXPECT_EQ (master_details.set_ip_request_serial, CLM_SLMP_SERIAL_NONE);

   EXPECT_EQ (clm_iefb_get_group_status (&clm, gi, &group_details), 0);
   EXPECT_EQ (
      group_details.cyclic_transmission_state,
      CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_CYCLIC_TR_STATE_ALL_OFF);
   EXPECT_EQ (group_details.frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (group_details.group_index, gi);
   EXPECT_EQ (group_details.group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (group_details.timestamp_link_scan_start, now);
   EXPECT_EQ (group_details.total_occupied, 1);

   /* Slave (device index 0) responds.
      Master sends new request immediately. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_payload_di0,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 1);
   EXPECT_EQ (result.group_no, group_number);
   EXPECT_EQ (result.cyclic_transmission_state, 0x0001);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_RESPONSE_1_SLAVE);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_1_SLAVE);

   /* Analyze stored info from latest frame (slave device) */
   EXPECT_TRUE (framevalues->has_been_received);
   EXPECT_EQ (framevalues->response_time, 1000U);
   EXPECT_EQ (framevalues->group_no, 1);
   EXPECT_EQ (framevalues->num_occupied_stations, 1);
   EXPECT_EQ (framevalues->protocol_ver, 2);
   EXPECT_EQ (framevalues->end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (framevalues->vendor_code, 0x6789);
   EXPECT_EQ (framevalues->model_code, 0x3456789AU);
   EXPECT_EQ (framevalues->equipment_ver, 0xBCDEU);
   EXPECT_EQ (
      framevalues->slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (framevalues->local_management_info, 0x33343536U);
   EXPECT_EQ (framevalues->slave_err_code, 0x6162);
   EXPECT_EQ (framevalues->slave_id, remote_ip_di0);
   EXPECT_EQ (framevalues->frame_sequence_no, frame_sequenceno_startup);
   EXPECT_EQ (statistics->number_of_sent_frames, 2U);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (statistics->number_of_connects, 1U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (statistics->number_of_timeouts, 0U);
   EXPECT_EQ (statistics->measured_time.number_of_samples, 1U);
   EXPECT_EQ (statistics->measured_time.sum, tick_size);
   EXPECT_EQ (statistics->measured_time.min, tick_size);
   EXPECT_EQ (statistics->measured_time.max, tick_size);
   EXPECT_EQ (statistics->measured_time.average, tick_size);

   /* Slave (device index 0) responds again (with new sequence number).
      Master sends new request immediately. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 2);
   EXPECT_EQ (result.group_no, group_number);
   EXPECT_EQ (result.cyclic_transmission_state, 0x0001);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.dl, SIZE_REQUEST_1_SLAVE - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   EXPECT_EQ (result.clock_info, mock_data.unix_timestamp_ms);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, 1);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.parallel_off_timeout_count,
      config.hier.groups[gi].parallel_off_timeout_count);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 6);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_RESPONSE_1_SLAVE);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_REQUEST_1_SLAVE);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (statistics->measured_time.number_of_samples, 2U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi0);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip_di0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_linkscan.group_index, gi);
   EXPECT_TRUE (cb_counters->master_cb_linkscan.success);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (statistics->number_of_connects, 1U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);

   /* Slave (device index 0) responds again with previous sequence number.
      Master detects slave duplication. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (framevalues->frame_sequence_no, frame_sequenceno_startup + 1);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_error.error_message,
      CLM_ERROR_SLAVE_DUPLICATION);
   EXPECT_EQ (cb_counters->master_cb_error.ip_addr, remote_ip_di0);
   EXPECT_EQ (cb_counters->master_cb_error.argument_2, 0);
   EXPECT_EQ (statistics->number_of_connects, 1U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (statistics->number_of_sent_frames, 3U);
   EXPECT_EQ (statistics->number_of_incoming_frames, 3U);
   EXPECT_EQ (statistics->measured_time.number_of_samples, 2U);
   EXPECT_EQ (statistics->measured_time.min, tick_size);
   EXPECT_EQ (statistics->measured_time.max, tick_size);
   EXPECT_EQ (statistics->measured_time.sum, 2 * tick_size);
   EXPECT_EQ (statistics->measured_time.average, tick_size);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 8);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_RESPONSE_1_SLAVE);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_REQUEST_1_SLAVE);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);

   /* Slave (device index 0) responds again with previous sequence number.
      No additional slave duplication error triggered. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (framevalues->frame_sequence_no, frame_sequenceno_startup + 1);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);
   EXPECT_EQ (statistics->number_of_connects, 1U);
   EXPECT_EQ (statistics->number_of_disconnects, 0U);
   EXPECT_EQ (statistics->number_of_sent_frames, 3U);
   EXPECT_EQ (statistics->number_of_incoming_frames, 4U);
   EXPECT_EQ (statistics->measured_time.number_of_samples, 2U);
   EXPECT_EQ (statistics->measured_time.min, tick_size);
   EXPECT_EQ (statistics->measured_time.max, tick_size);
   EXPECT_EQ (statistics->measured_time.sum, 2 * tick_size);
   EXPECT_EQ (statistics->measured_time.average, tick_size);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 10);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 4 * SIZE_RESPONSE_1_SLAVE);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_REQUEST_1_SLAVE);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);

   /* No further response from slave device  */
   now += tick_size;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);

   /* Wait for time out. Master sends new request */
   now += almost_timeout_us - 2 * tick_size;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);

   now += 5 * tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);

   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 3);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is off */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0000);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (framevalues->frame_sequence_no, frame_sequenceno_startup + 1);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_error.error_message,
      CLM_ERROR_SLAVE_DUPLICATION);
   EXPECT_EQ (cb_counters->master_cb_error.ip_addr, remote_ip_di0);
   EXPECT_EQ (cb_counters->master_cb_error.argument_2, 0);
   EXPECT_EQ (statistics->number_of_connects, 1U);
   EXPECT_EQ (statistics->number_of_disconnects, 1U);
   EXPECT_EQ (statistics->number_of_sent_frames, 4U);
   EXPECT_EQ (statistics->number_of_incoming_frames, 4U);
   EXPECT_EQ (statistics->measured_time.number_of_samples, 2U);
   EXPECT_EQ (statistics->measured_time.min, tick_size);
   EXPECT_EQ (statistics->measured_time.max, tick_size);
   EXPECT_EQ (statistics->measured_time.sum, 2 * tick_size);
   EXPECT_EQ (statistics->measured_time.average, tick_size);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 13);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 4 * SIZE_RESPONSE_1_SLAVE);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 4 * SIZE_REQUEST_1_SLAVE);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
}

/**
 * Master detects slave duplication. Multiple slaves in config.
 *
 * @req REQ_CLM_ERROR_05
 * @req REQ_CLM_CONFORMANCE_14
 *
 */
TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbSlaveStationDuplication)
{
   cl_mock_cyclic_request_result_t result;
   cl_ipaddr_t resulting_ipaddr;
   uint8_t response_di1_higher_sequence_number[SIZE_RESPONSE_2_SLAVES] = {};
   uint8_t response_di0_higher_sequence_number[SIZE_RESPONSE_1_SLAVE]  = {};

   clal_clear_memory (&result, sizeof (result));
   clal_memcpy (
      response_di0_higher_sequence_number,
      sizeof (response_di0_higher_sequence_number),
      (uint8_t *)&response_payload_di0,
      SIZE_RESPONSE_1_SLAVE);
   response_di0_higher_sequence_number[57] = 0x02;
   clal_memcpy (
      response_di1_higher_sequence_number,
      sizeof (response_di1_higher_sequence_number),
      (uint8_t *)&response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   response_di1_higher_sequence_number[57] = 0x02;

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_TRUE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (clm.parameter_no, parameter_no);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);

   /* Slave di1 (device index 1, here slave station 2) responds */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);

   /* Slave di1 (device index 1, here slave station 2) responds again.
      This is slave duplication.
      Master disconnects the slave. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_error.error_message,
      CLM_ERROR_SLAVE_DUPLICATION);
   EXPECT_EQ (cb_counters->master_cb_error.ip_addr, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_error.argument_2, 0);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request immediately. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_FALSE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_id, remote_ip);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 2);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for one slave only */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0001);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.dl, SIZE_REQUEST_3_SLAVES - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   EXPECT_EQ (result.clock_info, mock_data.unix_timestamp_ms);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);

   /* Slave di1 (device index 1, here slave station 2) responds */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);

   /* Slave di1 (device index 1, here slave station 2) responds again.
      This is slave duplication (again).
      Master will force the slave back to state CLM_DEVICE_STATE_LISTEN. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   /* Error callback is limited to triggering once */
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request immediately. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_FALSE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 3);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for one slave only */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0001);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.dl, SIZE_REQUEST_3_SLAVES - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   EXPECT_EQ (result.clock_info, mock_data.unix_timestamp_ms);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);

   /* Slave di1 (device index 1, here slave station 2) responds.
      No longer slave duplication. */
   response_di1_higher_sequence_number[57] = 0x03;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request immediately. */
   response_di0_higher_sequence_number[57] = 0x03;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 5);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 4);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves again */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);

   /* Slave di1 (device index 1, here slave station 2) responds.
         Again, no longer slave duplication. */
   response_di1_higher_sequence_number[57] = 0x04;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 5);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request immediately. */
   response_di0_higher_sequence_number[57] = 0x04;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_higher_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 6);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 5);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for both slaves */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);
}

TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbTooShortResponse)
{
   const size_t size_small_response = 9; /* Less than header size */

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);

   /* Slave responds with too short frame. Drop frame. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      size_small_response);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES + size_small_response);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
}

TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbTooShortResponseValidHeader)
{
   const size_t size_small_response = SIZE_RESPONSE_2_SLAVES - 1U;
   uint8_t response_too_short[SIZE_RESPONSE_2_SLAVES] = {};

   /* Prepare response with modified length field in header */
   clal_memcpy (
      response_too_short,
      sizeof (response_too_short),
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   response_too_short[7] = response_too_short[7] - 1U; /* Length */

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);

   /* Slave responds with too short frame. Drop frame. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_too_short,
      size_small_response);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES + size_small_response);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
}

TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbInvalidHeader)
{
   uint8_t response_invalid_header[SIZE_RESPONSE_2_SLAVES] = {0};

   /* Prepare response with invalid header */
   clal_memcpy (
      response_invalid_header,
      sizeof (response_invalid_header),
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   response_invalid_header[1] = 0x12;

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);

   /* Slave responds. Invalid protocol version, so drop frame */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_invalid_header,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
}

/**
 * Verify dropping frames from disabled slaves
 *
 * @req REQ_CLM_COMMUNIC_02
 */
TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbSlaveDisabled)
{
   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   clm.groups[gi].slave_devices[sdi].enabled = false;

   /* Slave responds. The slave is disabled, so drop frame. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (statistics->number_of_incoming_frames, 2U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
}

/**
 * Drop response with wrong sequence number
 *
 * Note that the sequence number must be different than previous frame,
 * or an slave duplication error will be triggered instead.
 *
 * @req REQ_CLM_STATUSBIT_04
 *
 */
TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbWrongSequenceNumber)
{
   uint8_t response_di1_higher_sequence_number[SIZE_RESPONSE_2_SLAVES] = {};
   clal_memcpy (
      response_di1_higher_sequence_number,
      sizeof (response_di1_higher_sequence_number),
      (uint8_t *)&response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   response_di1_higher_sequence_number[57] = 0x08;

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);

   /* Slave responds. Wrong sequence number, so drop frame. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_higher_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (statistics->number_of_incoming_frames, 2U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
}

TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbMasterIpInvalid)
{
   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);

   clm.config.master_id = CL_IPADDR_INVALID;

   /* Slave responds. Master IP address is 0.0.0.0, so drop frame */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
}

TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbInvalidProtocolVersion)
{
   uint8_t response_invalid_protocol[SIZE_RESPONSE_2_SLAVES] = {0};

   /* Prepare response with invalid protocol version number */
   clal_memcpy (
      response_invalid_protocol,
      sizeof (response_invalid_protocol),
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   response_invalid_protocol[11] = 0x07;

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);

   /* Slave responds. Invalid protocol version, so drop frame */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_invalid_protocol,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
}

/**
 * Verify group number in response
 *
 * @req REQ_CLM_GROUPS_02
 *
 */
TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbInvalidGroup)
{
   uint8_t response_invalid_group[SIZE_RESPONSE_2_SLAVES] = {0};

   /* Prepare response with wrong group number */
   clal_memcpy (
      response_invalid_group,
      sizeof (response_invalid_group),
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   response_invalid_group[55] = 0x02;

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);

   /* Slave responds. Invalid group number, so drop frame */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_invalid_group,
      SIZE_RESPONSE_2_SLAVES);

   /* Run master stack */
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
}

TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbWrongSlaveIp)
{
   uint8_t response_wrong_slave_ip[SIZE_RESPONSE_2_SLAVES] = {0};

   /* Prepare response with IP not in group */
   clal_memcpy (
      response_wrong_slave_ip,
      sizeof (response_wrong_slave_ip),
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   response_wrong_slave_ip[51] = 0x07; /* IP 1.2.3.7 */

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);

   /* Slave responds. Wrong IP number (not in group), so drop frame */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      0x01020307, /* IP 1.2.3.7 */
      CL_CCIEFB_PORT,
      (uint8_t *)&response_wrong_slave_ip,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
}

/**
 * Slave sends modified 'slave error code' and 'local management info'.
 */
TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbCustomSlaveInfo)
{
   uint8_t response_custom_slave_info[SIZE_RESPONSE_2_SLAVES] = {0};

   /* Prepare response with custom slave info */
   clal_memcpy (
      response_custom_slave_info,
      sizeof (response_custom_slave_info),
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   response_custom_slave_info[45] = 0x3A; /* Slave error code */

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.group_index, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.slave_device_index, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.end_code, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.slave_err_code, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.local_management_info, 0U);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.group_index, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.slave_device_index, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.end_code, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.slave_err_code, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.local_management_info, 0U);

   /* Slave device index 1 (slave station 2) responds.
    * Master triggers callback. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_custom_slave_info,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (statistics->number_of_incoming_frames, 2U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.slave_device_index, sdi);
   EXPECT_EQ (
      cb_counters->master_cb_changed_slave_info.end_code,
      CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.slave_err_code, 0x383AU);
   EXPECT_EQ (
      cb_counters->master_cb_changed_slave_info.local_management_info,
      alarm_local_management_info);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request immediately. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 11);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      2 * SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.calls, 1);

   /* Slave device index 1 (slave station 2) responds (new sequence number). */
   response_custom_slave_info[47] = 0x27; /* Local management info */
   response_custom_slave_info[57] = 0x02; /* Sequence number */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_custom_slave_info,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 13);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      2 * SIZE_RESPONSE_1_SLAVE + 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.slave_device_index, sdi);
   EXPECT_EQ (
      cb_counters->master_cb_changed_slave_info.end_code,
      CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.slave_err_code, 0x383AU);
   EXPECT_EQ (
      cb_counters->master_cb_changed_slave_info.local_management_info,
      0x23242527U);
}

/**
 * Slave sends an endcode indicating an error.
 *
 * @req REQ_CLM_ERROR_01
 * @req REQ_CLM_ERROR_06
 *
 */
TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbSlaveIndicatesError)
{
   uint8_t response_slave_error[SIZE_RESPONSE_2_SLAVES] = {0};

   /* Prepare response with custom slave info */
   clal_memcpy (
      response_slave_error,
      sizeof (response_slave_error),
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   response_slave_error[13] = 0xF0; /* End code */
   response_slave_error[14] = 0xCF;

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.group_index, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.slave_device_index, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.end_code, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.slave_err_code, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.local_management_info, 0U);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.calls, 0);

   /* Slave device index 1 (slave station 2) responds.
    * Master triggers callback. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_slave_error,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (statistics->number_of_incoming_frames, 2U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 1U);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_alarm.slave_device_index, sdi);
   EXPECT_EQ (
      cb_counters->master_cb_alarm.end_code,
      CL_SLMP_ENDCODE_CCIEFB_SLAVE_ERROR);
   EXPECT_EQ (cb_counters->master_cb_alarm.slave_err_code, alarm_slave_err_code);
   EXPECT_EQ (
      cb_counters->master_cb_alarm.local_management_info,
      alarm_local_management_info);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.calls, 0);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request immediately. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 11);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      2 * SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_id, remote_ip);

   /* Slave device index 1 (slave station 2) responds (new sequence number). */
   response_slave_error[57] = 0x02;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_slave_error,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 13);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      2 * SIZE_RESPONSE_1_SLAVE + 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 2);
}

/**
 * Slave sends an endcode indicating that it would like to disconnect
 *
 * @req REQ_CLM_CONFORMANCE_09
 * @req REQ_CLM_ERROR_01
 * @req REQ_CLM_ERROR_06
 *
 */
TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbSlaveWantsToDisconnect)
{
   uint8_t response_slave_disconnect[SIZE_RESPONSE_2_SLAVES] = {0};

   /* Prepare response with custom slave info */
   clal_memcpy (
      response_slave_disconnect,
      sizeof (response_slave_disconnect),
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   response_slave_disconnect[13] = 0xFF; /* End code */
   response_slave_disconnect[14] = 0xCF;

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.group_index, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.slave_device_index, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.end_code, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.slave_err_code, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.local_management_info, 0U);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.calls, 0);

   /* Slave device index 1 (slave station 2) responds.
    * Master triggers callback. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_slave_disconnect,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (statistics->number_of_incoming_frames, 2U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 1U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_alarm.slave_device_index, sdi);
   EXPECT_EQ (
      cb_counters->master_cb_alarm.end_code,
      CL_SLMP_ENDCODE_CCIEFB_SLAVE_REQUESTS_DISCONNECT);
   EXPECT_EQ (cb_counters->master_cb_alarm.slave_err_code, alarm_slave_err_code);
   EXPECT_EQ (
      cb_counters->master_cb_alarm.local_management_info,
      alarm_local_management_info);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.calls, 0);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request immediately. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 11);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      2 * SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_id, remote_ip);

   /* Slave device index 1 (slave station 2) responds (new sequence number). */
   response_slave_disconnect[57] = 0x02;
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_slave_disconnect,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 13);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      2 * SIZE_RESPONSE_1_SLAVE + 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_changed_slave_info.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 2);
}

TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbNoSlaveInfoCallbacks)
{
   cl_mock_cyclic_request_result_t result;
   cl_ipaddr_t resulting_ipaddr;

   clm.config.alarm_cb              = nullptr;
   clm.config.changed_slave_info_cb = nullptr;

   uint8_t response_custom_slave_info[SIZE_RESPONSE_2_SLAVES] = {0};

   /* Prepare response with custom slave info */
   clal_memcpy (
      response_custom_slave_info,
      sizeof (response_custom_slave_info),
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   response_custom_slave_info[13] = 0xF0; /* end code */
   response_custom_slave_info[14] = 0xCF;
   response_custom_slave_info[45] = 0x05; /* slave error */
   response_custom_slave_info[47] = 0x06; /* local management info */

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Slave responds with error. Custom slave info. Master disconnects. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_custom_slave_info,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (statistics->number_of_incoming_frames, 2U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 1U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);

   /* Slave device index 0 (slave station 1) responds.
      Master sends new request. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_FALSE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_disconnect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_id, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   ASSERT_EQ (mock_analyze_cyclic_request (mock_cciefb_port, &result), 0);
   EXPECT_EQ (result.frame_sequence_no, frame_sequenceno_startup + 2);
   EXPECT_EQ (result.group_no, group_number);
   /* Transmission bit is on for one slave only */
   EXPECT_EQ (result.cyclic_transmission_state, 0x0001);
   EXPECT_EQ (result.command, CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.sub_command, CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (result.dl, SIZE_REQUEST_3_SLAVES - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   EXPECT_EQ (result.clock_info, mock_data.unix_timestamp_ms);
   EXPECT_EQ (result.master_ip_addr, my_ip);
   EXPECT_EQ (result.master_protocol_ver, config.protocol_ver);
   EXPECT_EQ (result.slave_total_occupied_station_count, slaves_in_group);
   EXPECT_EQ (result.timeout_value, config.hier.groups[gi].timeout_value);
   /* Parameter number does not change. */
   EXPECT_EQ (result.parameter_no, parameter_no);
   EXPECT_EQ (
      result.master_local_unit_info,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (
      result.parallel_off_timeout_count,
      config.hier.groups[gi].parallel_off_timeout_count);
   /* IP addresses in request */
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 1, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip_di0);
   EXPECT_EQ (
      mock_analyze_cyclic_request_slaveid (mock_cciefb_port, 2, &resulting_ipaddr),
      0);
   EXPECT_EQ (resulting_ipaddr, remote_ip);
}

TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbSameIpAddressAsMaster)
{
   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);

   /* Slave responds from master IP address. Drop frame */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      my_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
}

/**
 * Timeout due to missing responses from slaves
 *
 * @req REQ_CLM_ERROR_07
 */
TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbMasterTimesOut)
{
   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   ASSERT_EQ (clm.groups[gi].slave_devices[sdi].timeout_count, 0);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (statistics->number_of_timeouts, 0U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_linkscan.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_linkscan.success, true);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Timeout. No frames from the devices */
   now += longer_than_timeout_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 1);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi].timeout_count, 1);

   now += longer_than_timeout_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 2);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi].timeout_count, 2);

   now += longer_than_timeout_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi].timeout_count, 0);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 10);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 5);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 5 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (statistics->number_of_timeouts, 1U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.group_index, gi);
   EXPECT_TRUE (
      cb_counters->master_cb_disconnect.slave_device_index == sdi ||
      cb_counters->master_cb_disconnect.slave_device_index == sdi0);
   EXPECT_TRUE (
      cb_counters->master_cb_disconnect.slave_id == remote_ip ||
      cb_counters->master_cb_disconnect.slave_id == remote_ip_di0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 4);
   EXPECT_EQ (cb_counters->master_cb_linkscan.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_linkscan.success, false);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
}

/**
 * One missing response from slave, but then more responses
 *
 * @req REQ_CLM_ERROR_07
 */
TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbMasterTimesOutOnce)
{
   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   ASSERT_EQ (clm.groups[gi].slave_devices[sdi].timeout_count, 0);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (statistics->number_of_timeouts, 0U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_linkscan.group_index, gi);

   /* Timeout. No frames from the devices */
   now += longer_than_timeout_us;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 1);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi].timeout_count, 1);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);

   /* Slave device index 0 responds */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   /* Check that timeout count has been reset for slave device index 0 */
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi0].timeout_count, 0);
   EXPECT_EQ (clm.groups[gi].slave_devices[sdi].timeout_count, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 10);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      2 * SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENT);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (statistics->number_of_timeouts, 0U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.group_index, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.slave_device_index, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_linkscan.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
}

TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbNoDisconnectCallback)
{
   clm.config.disconnect_cb = nullptr;

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_linkscan.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_linkscan.success, true);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Timeout. No frames from the devices */
   now += longer_than_timeout_us;
   clm_iefb_periodic (&clm, now);
   now += longer_than_timeout_us;
   clm_iefb_periodic (&clm, now);
   now += longer_than_timeout_us;
   clm_iefb_periodic (&clm, now);

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_WAIT_TD);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 4);
   EXPECT_EQ (cb_counters->master_cb_linkscan.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_linkscan.success, false);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
}

/**
 * Master receives 'master duplication alarm' from slave.
 *
 * Master completely stops to communicate.
 *
 * @req REQ_CLM_CONFORMANCE_13
 * @req REQ_CLM_DIAGNOSIS_01
 * @req REQ_CLM_DIAGNOSIS_02
 * @req REQ_CLM_ERROR_04
 * @req REQ_CLM_ERROR_06
 */
TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbMasterDuplicationAlarm)
{
   uint8_t response_master_duplication[SIZE_RESPONSE_2_SLAVES] = {0};
   const uint16_t custom_end_code = CL_SLMP_ENDCODE_CCIEFB_MASTER_DUPLICATION;
   clm_master_status_details_t master_details;
   clm_group_status_details_t group_details;
   const clm_slave_device_data_t * resulting_devicedata;

   /* Prepare response with "Master duplication" end code */
   clal_memcpy (
      response_master_duplication,
      sizeof (response_master_duplication),
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);
   response_master_duplication[13] = custom_end_code & UINT8_MAX;
   response_master_duplication[14] = custom_end_code >> 8;

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_linkscan.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_linkscan.success, true);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.group_index, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.slave_device_index, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.end_code, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.slave_err_code, 0);
   EXPECT_EQ (cb_counters->master_cb_alarm.local_management_info, 0U);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Slave device index 1 responds. "Master duplication" end code. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_master_duplication,
      SIZE_RESPONSE_2_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_EQ (statistics->number_of_incoming_frames, 2U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 1U);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 4);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_STANDBY);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_linkscan.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_linkscan.success, true);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);
   EXPECT_EQ (
      cb_counters->master_cb_error.error_message,
      CLM_ERROR_SLAVE_REPORTS_MASTER_DUPLICATION);
   EXPECT_EQ (cb_counters->master_cb_error.ip_addr, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_error.argument_2, 0);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 4);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);

   /* Slave device index 0 (slave station 1) responds.
      Master is still in standby. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip_di0,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di0_next_sequence_number,
      SIZE_RESPONSE_1_SLAVE);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LISTEN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_LISTEN);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   /* No disconnect callback has been triggered */
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 4);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_STANDBY);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 1);

   /* Application reads out diagnostic information */
   EXPECT_EQ (clm_get_master_status (&clm, &master_details), 0);
   EXPECT_EQ (master_details.master_state, CLM_MASTER_STATE_STANDBY);
   EXPECT_EQ (master_details.node_search_serial, -1);
   EXPECT_EQ (master_details.set_ip_request_serial, -1);
   EXPECT_EQ (master_details.parameter_no, parameter_no);
   EXPECT_EQ (clm_get_group_status (&clm, gi, &group_details), 0);
   EXPECT_EQ (group_details.group_index, gi);
   EXPECT_EQ (group_details.group_state, CLM_GROUP_STATE_MASTER_LISTEN);
   EXPECT_EQ (group_details.timestamp_link_scan_start, now - 2 * tick_size);
   /* Transmission bits are still on */
   EXPECT_EQ (group_details.cyclic_transmission_state, 0x0003);
   EXPECT_EQ (group_details.frame_sequence_no, frame_sequenceno_startup + 1);
   EXPECT_EQ (group_details.total_occupied, slaves_in_group);
   resulting_devicedata = clm_get_device_connection_details (&clm, gi, sdi);
   EXPECT_NE (resulting_devicedata, nullptr);
   EXPECT_EQ (resulting_devicedata->device_index, sdi);
   EXPECT_EQ (resulting_devicedata->device_state, CLM_DEVICE_STATE_LISTEN);
   EXPECT_TRUE (resulting_devicedata->enabled);
   EXPECT_FALSE (resulting_devicedata->force_transmission_bit);
   EXPECT_TRUE (resulting_devicedata->transmission_bit);
   EXPECT_EQ (resulting_devicedata->slave_station_no, 2);
   EXPECT_EQ (resulting_devicedata->timeout_count, 0);
   EXPECT_EQ (
      resulting_devicedata->statistics.number_of_incoming_invalid_frames,
      0U);
   EXPECT_EQ (resulting_devicedata->statistics.number_of_connects, 1U);
   EXPECT_EQ (resulting_devicedata->statistics.number_of_disconnects, 0U);
   EXPECT_EQ (resulting_devicedata->statistics.number_of_incoming_frames, 2U);
   EXPECT_EQ (resulting_devicedata->statistics.number_of_incoming_alarm_frames, 1U);
   EXPECT_EQ (resulting_devicedata->statistics.number_of_sent_frames, 2U);
   EXPECT_EQ (resulting_devicedata->statistics.number_of_timeouts, 0U);
   EXPECT_EQ (resulting_devicedata->statistics.measured_time.number_of_samples, 2U);
   EXPECT_EQ (resulting_devicedata->statistics.measured_time.min, tick_size);
   EXPECT_EQ (resulting_devicedata->statistics.measured_time.max, 3 * tick_size);
   EXPECT_EQ (resulting_devicedata->statistics.measured_time.sum, 4 * tick_size);
   EXPECT_EQ (
      resulting_devicedata->statistics.measured_time.average,
      2 * tick_size);
   EXPECT_TRUE (resulting_devicedata->latest_frame.has_been_received);
   EXPECT_EQ (
      resulting_devicedata->latest_frame.end_code,
      CL_SLMP_ENDCODE_CCIEFB_MASTER_DUPLICATION);
   EXPECT_EQ (
      resulting_devicedata->latest_frame.equipment_ver,
      slave_equipment_ver);
   EXPECT_EQ (
      resulting_devicedata->latest_frame.frame_sequence_no,
      frame_sequenceno_startup + 1);
   EXPECT_EQ (resulting_devicedata->latest_frame.group_no, group_number);
   EXPECT_EQ (
      resulting_devicedata->latest_frame.local_management_info,
      alarm_local_management_info);
   EXPECT_EQ (resulting_devicedata->latest_frame.model_code, slave_model_code);
   EXPECT_EQ (resulting_devicedata->latest_frame.num_occupied_stations, 2);
   EXPECT_EQ (
      resulting_devicedata->latest_frame.protocol_ver,
      CL_CCIEFB_MAX_SUPPORTED_PROTOCOL_VER);
   EXPECT_EQ (resulting_devicedata->latest_frame.response_time, tick_size);
   EXPECT_EQ (
      resulting_devicedata->latest_frame.slave_err_code,
      alarm_slave_err_code);
   EXPECT_EQ (resulting_devicedata->latest_frame.slave_id, remote_ip);
   EXPECT_EQ (
      resulting_devicedata->latest_frame.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (resulting_devicedata->latest_frame.vendor_code, slave_vendor_code);
}

/**
 * A slave responds with wrong number of occupied stations. Master drops frame.
 *
 * @req REQ_CLM_ERROR_02
 *
 */
TEST_F (MasterIntegrationTestBothDevicesResponded, CciefbWrongNumberOfOccupied)
{
   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (statistics->number_of_incoming_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 0U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);

   clm.config.hier.groups[gi].slave_devices[sdi].num_occupied_stations = 3;

   /* Slave responds. Invalid number of occupied stations (2) compared to
      config (which now says 3 occupied stations). Drop frame */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&response_di1_next_sequence_number,
      SIZE_RESPONSE_2_SLAVES);

   /* Run master stack */
   now += tick_size;
   clm_iefb_periodic (&clm, now);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi0].transmission_bit);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_TRUE (clm.groups[gi].slave_devices[sdi].transmission_bit);
   EXPECT_EQ (statistics->number_of_incoming_frames, 2U);
   EXPECT_EQ (statistics->number_of_incoming_invalid_frames, 1U);
   EXPECT_EQ (statistics->number_of_incoming_alarm_frames, 0U);
}

/**
 * Verify that request from other master is dropped when running.
 *
 * @req REQ_CLM_COMMUNIC_05
 * @req REQ_CLM_CONFORMANCE_10
 *
 */
TEST_F (
   MasterIntegrationTestBothDevicesResponded,
   CciefbRequestFromOtherMasterDuplication)
{
   uint8_t request_from_other_master[SIZE_REQUEST_3_SLAVES] = {0};

   /* Prepare request from other master */
   clal_memcpy (
      request_from_other_master,
      sizeof (request_from_other_master),
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_from_other_master[47] = 0x08; /* IP 1.2.3.8 */

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_linkscan.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_linkscan.success, true);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);

   /* Request from other master. Drop frame. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      0x01020308, /* IP 1.2.3.8 */
      CL_CCIEFB_PORT,
      (uint8_t *)&request_from_other_master,
      SIZE_REQUEST_3_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (cb_counters->master_cb_connect.calls, 2);
   EXPECT_EQ (cb_counters->master_cb_connect.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_device_index, sdi);
   EXPECT_EQ (cb_counters->master_cb_connect.slave_id, remote_ip);
   EXPECT_EQ (cb_counters->master_cb_disconnect.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_state.calls, 3);
   EXPECT_EQ (cb_counters->master_cb_state.state, CLM_MASTER_STATE_RUNNING);
   EXPECT_EQ (cb_counters->master_cb_linkscan.calls, 1);
   EXPECT_EQ (cb_counters->master_cb_linkscan.group_index, gi);
   EXPECT_EQ (cb_counters->master_cb_linkscan.success, true);
   EXPECT_EQ (cb_counters->master_cb_alarm.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_node_search.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_set_ip.calls, 0);
   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
}

TEST_F (
   MasterIntegrationTestBothDevicesResponded,
   CciefbRequestWrongCommandFromOtherMaster)
{
   uint8_t request_wrong_command[SIZE_REQUEST_3_SLAVES] = {0};

   /* Prepare request from other master */
   clal_memcpy (
      request_wrong_command,
      sizeof (request_wrong_command),
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_wrong_command[11] = 0x01; /* Wrong command */
   request_wrong_command[47] = 0x08; /* IP 1.2.3.8 */

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);

   /* Request from other master. Drop frame. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      0x01020308, /* IP 1.2.3.8 */
      CL_CCIEFB_PORT,
      (uint8_t *)&request_wrong_command,
      SIZE_REQUEST_3_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES + SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
}

TEST_F (
   MasterIntegrationTestBothDevicesResponded,
   CciefbRequestWrongSubcommandFromOtherMaster)
{
   uint8_t request_wrong_subcommand[SIZE_REQUEST_3_SLAVES] = {0};

   /* Prepare request from other master */
   clal_memcpy (
      request_wrong_subcommand,
      sizeof (request_wrong_subcommand),
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_wrong_subcommand[13] = 0x01; /* Wrong subcommand */
   request_wrong_subcommand[47] = 0x08; /* IP 1.2.3.8 */

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);

   /* Request from other master. Drop frame. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      0x01020308, /* IP 1.2.3.8 */
      CL_CCIEFB_PORT,
      (uint8_t *)&request_wrong_subcommand,
      SIZE_REQUEST_3_SLAVES);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES + SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
}

TEST_F (
   MasterIntegrationTestBothDevicesResponded,
   CciefbTooShortRequestFromOtherMaster)
{
   /* Less than request header size, but larger than response header size */
   const size_t size_short_request = 13;

   ASSERT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   ASSERT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);

   /* Other master sends request with too short frame. Drop frame. */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      size_short_request);
   now += tick_size;
   clm_iefb_periodic (&clm, now);

   EXPECT_EQ (cb_counters->master_cb_error.calls, 0);
   EXPECT_EQ (clm.groups[gi].group_state, CLM_GROUP_STATE_MASTER_LINK_SCAN);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi0].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (
      clm.groups[gi].slave_devices[sdi].device_state,
      CLM_DEVICE_STATE_CYCLIC_SENDING);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (
      mock_cciefb_port->total_recv_bytes,
      SIZE_RESPONSE_1_SLAVE + SIZE_RESPONSE_2_SLAVES + size_short_request);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_REQUEST_3_SLAVES);
}
