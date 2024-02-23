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
#include "slave/cls_iefb.h"
#include "slave/cls_slave.h"

#include "mocks.h"
#include "utils_for_testing.h"

#include "osal_log.h"

#include <gtest/gtest.h>

class SlaveLoglimiterUnitTest : public UnitTest
{
};

// Tests

TEST_F (SlaveLoglimiterUnitTest, RunLoglimiter)
{
#if not LOG_WARNING_ENABLED(CL_CCIEFB_LOG)
   GTEST_SKIP() << "Skipping as log level WARNING not is enabled";
#endif

   cl_limiter_t loglimiter;

   uint32_t now                       = 0;
   const uint32_t period              = 100000; /* 100 milliseconds */
   const uint32_t delta_time          = 1000;   /* 1 millisecond */
   const uint32_t shorter_than_period = period - delta_time;

   /* Initialise */
   cl_limiter_init (&loglimiter, period);
   EXPECT_EQ (loglimiter.number_of_calls, 0);
   EXPECT_EQ (loglimiter.number_of_outputs, 0);
   EXPECT_FALSE (cl_timer_is_running (&loglimiter.timer));
   EXPECT_FALSE (cl_timer_is_expired (&loglimiter.timer, now));

   /* Log first message */
   now += delta_time;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_FALSE (cl_timer_is_running (&loglimiter.timer));

   cls_iefb_log_warning_once (
      &loglimiter,
      CLS_LOGLIMITER_MASTER_DUPLICATION,
      0x02030405,
      0,
      0,
      now);
   EXPECT_EQ (loglimiter.number_of_calls, 1);
   EXPECT_EQ (loglimiter.number_of_outputs, 1);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Drop next similar message */
   cls_iefb_log_warning_once (
      &loglimiter,
      CLS_LOGLIMITER_MASTER_DUPLICATION,
      0x02030406,
      1,
      2,
      now);
   EXPECT_EQ (loglimiter.number_of_calls, 2);
   EXPECT_EQ (loglimiter.number_of_outputs, 1);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Drop next similar message, sent later */
   now += shorter_than_period;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   cls_iefb_log_warning_once (
      &loglimiter,
      CLS_LOGLIMITER_MASTER_DUPLICATION,
      0x02030407,
      2,
      3,
      now);
   EXPECT_EQ (loglimiter.number_of_calls, 3);
   EXPECT_EQ (loglimiter.number_of_outputs, 1);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Loglimiter times out */
   now += period + delta_time;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_FALSE (cl_timer_is_running (&loglimiter.timer));

   /* Next message is logged */
   cls_iefb_log_warning_once (
      &loglimiter,
      CLS_LOGLIMITER_MASTER_DUPLICATION,
      0x02030408,
      3,
      4,
      now);
   EXPECT_EQ (loglimiter.number_of_calls, 4);
   EXPECT_EQ (loglimiter.number_of_outputs, 2);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Drop next similar message, sent later */
   now += delta_time;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   cls_iefb_log_warning_once (
      &loglimiter,
      CLS_LOGLIMITER_MASTER_DUPLICATION,
      0x02030409,
      4,
      5,
      now);
   EXPECT_EQ (loglimiter.number_of_calls, 5);
   EXPECT_EQ (loglimiter.number_of_outputs, 2);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* A different message will be printed */
   cls_iefb_log_warning_once (
      &loglimiter,
      CLS_LOGLIMITER_WRONG_NUMBER_OCCUPIED,
      0x00000000,
      5,
      6,
      now);
   EXPECT_EQ (loglimiter.number_of_calls, 6);
   EXPECT_EQ (loglimiter.number_of_outputs, 3);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Repeated logging will be suppressed */
   now += shorter_than_period;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   cls_iefb_log_warning_once (
      &loglimiter,
      CLS_LOGLIMITER_WRONG_NUMBER_OCCUPIED,
      0x00000001,
      6,
      7,
      now);
   EXPECT_EQ (loglimiter.number_of_calls, 7);
   EXPECT_EQ (loglimiter.number_of_outputs, 3);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   now += shorter_than_period;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   cls_iefb_log_warning_once (
      &loglimiter,
      CLS_LOGLIMITER_WRONG_NUMBER_OCCUPIED,
      0x00000002,
      7,
      8,
      now);
   EXPECT_EQ (loglimiter.number_of_calls, 8);
   EXPECT_EQ (loglimiter.number_of_outputs, 3);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   now += shorter_than_period;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   cls_iefb_log_warning_once (
      &loglimiter,
      CLS_LOGLIMITER_WRONG_NUMBER_OCCUPIED,
      0x00000003,
      8,
      9,
      now);
   EXPECT_EQ (loglimiter.number_of_calls, 9);
   EXPECT_EQ (loglimiter.number_of_outputs, 3);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Print a warning for wrong message type */
   cls_iefb_log_warning_once (
      &loglimiter,
      (cls_cciefb_logwarning_message_t)0x123,
      0x00000004,
      9,
      10,
      now);
   EXPECT_EQ (loglimiter.number_of_calls, 10);
   EXPECT_EQ (loglimiter.number_of_outputs, 4);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));
}

TEST_F (SlaveLoglimiterUnitTest, LoglimiterPeriodZero)
{
#if not LOG_WARNING_ENABLED(CL_CCIEFB_LOG)
   GTEST_SKIP() << "Skipping as log level WARNING not is enabled";
#endif

   cl_limiter_t loglimiter;

   uint32_t now              = 0;
   const uint32_t delta_time = 1000; /* 1 millisecond */

   /* Initialise */
   cl_limiter_init (&loglimiter, 0);
   EXPECT_EQ (loglimiter.number_of_calls, 0);
   EXPECT_EQ (loglimiter.number_of_outputs, 0);
   EXPECT_FALSE (cl_timer_is_running (&loglimiter.timer));
   EXPECT_FALSE (cl_timer_is_expired (&loglimiter.timer, now));

   /* Log first message */
   now += delta_time;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_FALSE (cl_timer_is_running (&loglimiter.timer));

   cls_iefb_log_warning_once (
      &loglimiter,
      CLS_LOGLIMITER_MASTER_DUPLICATION,
      0x02030405,
      0,
      0,
      now);
   EXPECT_EQ (loglimiter.number_of_calls, 1);
   EXPECT_EQ (loglimiter.number_of_outputs, 1);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Next message is logged */
   cls_iefb_log_warning_once (
      &loglimiter,
      CLS_LOGLIMITER_MASTER_DUPLICATION,
      0x02030406,
      1,
      2,
      now);
   EXPECT_EQ (loglimiter.number_of_calls, 2);
   EXPECT_EQ (loglimiter.number_of_outputs, 2);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Timer times out immediately */
   now += delta_time;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_FALSE (cl_timer_is_running (&loglimiter.timer));

   /* Next message is logged */
   cls_iefb_log_warning_once (
      &loglimiter,
      CLS_LOGLIMITER_MASTER_DUPLICATION,
      0x02030407,
      2,
      3,
      now);
   EXPECT_EQ (loglimiter.number_of_calls, 3);
   EXPECT_EQ (loglimiter.number_of_outputs, 3);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Next message is logged */
   cls_iefb_log_warning_once (
      &loglimiter,
      CLS_LOGLIMITER_MASTER_DUPLICATION,
      0x02030408,
      3,
      4,
      now);
   EXPECT_EQ (loglimiter.number_of_calls, 4);
   EXPECT_EQ (loglimiter.number_of_outputs, 4);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));
}

/* For test fixtures suitable for slave integration testing, see
 * utils_for_testing.h */

TEST_F (SlaveUnitTest, CciefbIsMasterIdCorrect)
{
   EXPECT_FALSE (cls_iefb_is_master_id_correct (&cls, 0x01020304)); /* 1.2.3.4
                                                                     */
   cls.master.master_id = 0x00000001; /* IP 0.0.0.1 */
   EXPECT_FALSE (cls_iefb_is_master_id_correct (&cls, 0x01020304)); /* 1.2.3.4
                                                                     */
   EXPECT_TRUE (cls_iefb_is_master_id_correct (&cls, 0x00000001)); /* 0.0.0.1 */
   EXPECT_FALSE (cls_iefb_is_master_id_correct (&cls, 0x00000002)); /* 0.0.0.2
                                                                     */
}

TEST_F (UnitTest, CciefbFilterCallbackValues)
{
   bool filtered_connected_and_running         = false;
   bool filtered_stopped_by_user               = false;
   uint16_t filtered_protocol_ver              = 0;
   uint16_t filtered_master_application_status = 0;

   /* Stopped by error or user, protocol v 1 */
   cls_iefb_filter_master_state_callback_values (
      true, /* Connected */
      1,    /* Protocol version */
      0x00, /* master_application_status */
      &filtered_connected_and_running,
      &filtered_stopped_by_user,
      &filtered_protocol_ver,
      &filtered_master_application_status);
   EXPECT_EQ (filtered_connected_and_running, false);
   EXPECT_EQ (filtered_stopped_by_user, false);
   EXPECT_EQ (filtered_protocol_ver, 1);
   EXPECT_EQ (filtered_master_application_status, 0x00);

   /* Running, protocol v 1 */
   cls_iefb_filter_master_state_callback_values (
      true, /* Connected */
      1,    /* Protocol version */
      0x01, /* master_application_status */
      &filtered_connected_and_running,
      &filtered_stopped_by_user,
      &filtered_protocol_ver,
      &filtered_master_application_status);
   EXPECT_EQ (filtered_connected_and_running, true);
   EXPECT_EQ (filtered_stopped_by_user, false);
   EXPECT_EQ (filtered_protocol_ver, 1);
   EXPECT_EQ (filtered_master_application_status, 0x01);

   /* Stopped by user (invalid), protocol v 1 */
   cls_iefb_filter_master_state_callback_values (
      true, /* Connected */
      1,    /* Protocol version */
      0x02, /* master_application_status */
      &filtered_connected_and_running,
      &filtered_stopped_by_user,
      &filtered_protocol_ver,
      &filtered_master_application_status);
   EXPECT_EQ (filtered_connected_and_running, false);
   EXPECT_EQ (filtered_stopped_by_user, false);
   EXPECT_EQ (filtered_protocol_ver, 1);
   EXPECT_EQ (filtered_master_application_status, 0x02);

   /* Invalid value, protocol v 1 */
   cls_iefb_filter_master_state_callback_values (
      true, /* Connected */
      1,    /* Protocol version */
      0x03, /* master_application_status */
      &filtered_connected_and_running,
      &filtered_stopped_by_user,
      &filtered_protocol_ver,
      &filtered_master_application_status);
   EXPECT_EQ (filtered_connected_and_running, true);
   EXPECT_EQ (filtered_stopped_by_user, false);
   EXPECT_EQ (filtered_protocol_ver, 1);
   EXPECT_EQ (filtered_master_application_status, 0x03);

   /* Disconnected, protocol v 1 */
   cls_iefb_filter_master_state_callback_values (
      false, /* Connected */
      1,     /* Protocol version */
      0x01,  /* master_application_status */
      &filtered_connected_and_running,
      &filtered_stopped_by_user,
      &filtered_protocol_ver,
      &filtered_master_application_status);
   EXPECT_EQ (filtered_connected_and_running, false);
   EXPECT_EQ (filtered_stopped_by_user, false);
   EXPECT_EQ (filtered_protocol_ver, 0);
   EXPECT_EQ (filtered_master_application_status, 0x00);

   /* Stopped by error, protocol v 2*/
   cls_iefb_filter_master_state_callback_values (
      true, /* Connected */
      2,    /* Protocol version */
      0x00, /* master_application_status */
      &filtered_connected_and_running,
      &filtered_stopped_by_user,
      &filtered_protocol_ver,
      &filtered_master_application_status);
   EXPECT_EQ (filtered_connected_and_running, false);
   EXPECT_EQ (filtered_stopped_by_user, false);
   EXPECT_EQ (filtered_protocol_ver, 2);
   EXPECT_EQ (filtered_master_application_status, 0x00);

   /* Running, protocol v 2*/
   cls_iefb_filter_master_state_callback_values (
      true, /* Connected */
      2,    /* Protocol version */
      0x01, /* master_application_status */
      &filtered_connected_and_running,
      &filtered_stopped_by_user,
      &filtered_protocol_ver,
      &filtered_master_application_status);
   EXPECT_EQ (filtered_connected_and_running, true);
   EXPECT_EQ (filtered_stopped_by_user, false);
   EXPECT_EQ (filtered_protocol_ver, 2);
   EXPECT_EQ (filtered_master_application_status, 0x01);

   /* Stopped by user, protocol v 2*/
   cls_iefb_filter_master_state_callback_values (
      true, /* Connected */
      2,    /* Protocol version */
      0x02, /* master_application_status */
      &filtered_connected_and_running,
      &filtered_stopped_by_user,
      &filtered_protocol_ver,
      &filtered_master_application_status);
   EXPECT_EQ (filtered_connected_and_running, false);
   EXPECT_EQ (filtered_stopped_by_user, true);
   EXPECT_EQ (filtered_protocol_ver, 2);
   EXPECT_EQ (filtered_master_application_status, 0x02);

   /* Illegal value, protocol v 2*/
   cls_iefb_filter_master_state_callback_values (
      true, /* Connected */
      2,    /* Protocol version */
      0x03, /* master_application_status */
      &filtered_connected_and_running,
      &filtered_stopped_by_user,
      &filtered_protocol_ver,
      &filtered_master_application_status);
   EXPECT_EQ (filtered_connected_and_running, true);
   EXPECT_EQ (filtered_stopped_by_user, false);
   EXPECT_EQ (filtered_protocol_ver, 2);
   EXPECT_EQ (filtered_master_application_status, 0x03);

   /* Disconnected, protocol v 2*/
   cls_iefb_filter_master_state_callback_values (
      false, /* Connected */
      2,     /* Protocol version */
      0x01,  /* master_application_status */
      &filtered_connected_and_running,
      &filtered_stopped_by_user,
      &filtered_protocol_ver,
      &filtered_master_application_status);
   EXPECT_EQ (filtered_connected_and_running, false);
   EXPECT_EQ (filtered_stopped_by_user, false);
   EXPECT_EQ (filtered_protocol_ver, 0);
   EXPECT_EQ (filtered_master_application_status, 0x00);

   /* Running, protocol v 58 */
   cls_iefb_filter_master_state_callback_values (
      true, /* Connected */
      58,   /* Protocol version */
      0x01, /* master_application_status */
      &filtered_connected_and_running,
      &filtered_stopped_by_user,
      &filtered_protocol_ver,
      &filtered_master_application_status);
   EXPECT_EQ (filtered_connected_and_running, true);
   EXPECT_EQ (filtered_stopped_by_user, false);
   EXPECT_EQ (filtered_protocol_ver, 58);
   EXPECT_EQ (filtered_master_application_status, 0x01);

   /* Running, protocol v 0 */
   cls_iefb_filter_master_state_callback_values (
      true, /* Connected */
      0,    /* Protocol version */
      0x01, /* master_application_status */
      &filtered_connected_and_running,
      &filtered_stopped_by_user,
      &filtered_protocol_ver,
      &filtered_master_application_status);
   EXPECT_EQ (filtered_connected_and_running, true);
   EXPECT_EQ (filtered_stopped_by_user, false);
   EXPECT_EQ (filtered_protocol_ver, 0);
   EXPECT_EQ (filtered_master_application_status, 0x01);
}

TEST_F (UnitTest, CciefbShouldTriggerCallback)
{
   /* First run */
   EXPECT_TRUE (cls_iefb_should_trigger_master_state_callback (
      false,  /* executed_before */
      false,  /* connected_to_master */
      false,  /* previous_connected_to_master */
      7,      /* protocol_ver */
      7,      /* previous_protocol_ver */
      0x5152, /* master_application_status */
      0x5152 /* previous_master_application_status */));

   /* Connects to master */
   EXPECT_TRUE (cls_iefb_should_trigger_master_state_callback (
      true,   /* executed_before */
      true,   /* connected_to_master */
      false,  /* previous_connected_to_master */
      7,      /* protocol_ver */
      7,      /* previous_protocol_ver */
      0x5152, /* master_application_status */
      0x5152 /* previous_master_application_status */));

   EXPECT_FALSE (cls_iefb_should_trigger_master_state_callback (
      true,   /* executed_before */
      true,   /* connected_to_master */
      true,   /* previous_connected_to_master */
      7,      /* protocol_ver */
      7,      /* previous_protocol_ver */
      0x5152, /* master_application_status */
      0x5152 /* previous_master_application_status */));

   /* Disonnects from master */
   EXPECT_TRUE (cls_iefb_should_trigger_master_state_callback (
      true,   /* executed_before */
      false,  /* connected_to_master */
      true,   /* previous_connected_to_master */
      7,      /* protocol_ver */
      7,      /* previous_protocol_ver */
      0x5152, /* master_application_status */
      0x5152 /* previous_master_application_status */));

   EXPECT_FALSE (cls_iefb_should_trigger_master_state_callback (
      true,   /* executed_before */
      false,  /* connected_to_master */
      false,  /* previous_connected_to_master */
      7,      /* protocol_ver */
      7,      /* previous_protocol_ver */
      0x5152, /* master_application_status */
      0x5152 /* previous_master_application_status */));

   /* Changed protocol version  */
   EXPECT_TRUE (cls_iefb_should_trigger_master_state_callback (
      true,   /* executed_before */
      true,   /* connected_to_master */
      true,   /* previous_connected_to_master */
      8,      /* protocol_ver */
      7,      /* previous_protocol_ver */
      0x5152, /* master_application_status */
      0x5152 /* previous_master_application_status */));

   /* Changed master application status  */
   EXPECT_TRUE (cls_iefb_should_trigger_master_state_callback (
      true,   /* executed_before */
      true,   /* connected_to_master */
      true,   /* previous_connected_to_master */
      7,      /* protocol_ver */
      7,      /* previous_protocol_ver */
      0x5152, /* master_application_status */
      0x5153 /* previous_master_application_status */));
}

TEST_F (SlaveUnitTest, CliefCopyCyclicDataFromRequest)
{
   cls_cciefb_cyclic_request_info_t request            = {};
   uint8_t request_payload_copy[SIZE_REQUEST_3_SLAVES] = {};
   cl_ry_t * ry_in_message                             = nullptr;
   cl_rww_t * rww_in_message                           = nullptr;
   const uint16_t start_number = 2; /* Slave station number */

   /* Compile the tests with sufficiently large memory area */
   ASSERT_GE (CLS_MAX_OCCUPIED_STATIONS, 4);

   /* Prepare request buffer that we can modify */
   clal_memcpy (
      request_payload_copy,
      sizeof (request_payload_copy),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   /* Parse dummy request frame from the buffer */
   ASSERT_EQ (
      cl_iefb_parse_cyclic_request (
         (uint8_t *)&request_payload_copy,
         SIZE_REQUEST_3_SLAVES,
         remote_ip,
         123, /* remote_port */
         my_ip,
         &request),
      0);

   ry_in_message  = request.first_ry;
   rww_in_message = request.first_rww;

   ASSERT_EQ (config.num_occupied_stations, 2);
   ASSERT_EQ (
      CC_FROM_LE16 (request.full_headers->cyclic_data_header
                       .slave_total_occupied_station_count),
      3);

   /* Valid arguments, transmission bit on */
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, start_number, true),
      0);

   /* Data from request_payload_running, RY slave 2 */
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[0], 0xFF);
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[1], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[2], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[3], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[4], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[5], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[6], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[7], 0x00);

   /* Data from request_payload_running, RY slave 3 */
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[0], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[1], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[2], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[3], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[4], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[5], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[6], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[7], 0x08);

   /* Data from request_payload_running, RWw slave 2 */
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[0].words[0]), 0x0022);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[0].words[1]), 0x3300);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[0].words[2]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[0].words[3]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[0].words[31]), 0x0000);

   /* Data from request_payload_running, RWw slave 3 */
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[1].words[0]), 0x0044);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[1].words[1]), 0x5500);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[1].words[2]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[1].words[3]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[1].words[4]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[1].words[31]), 0x6600);

   /* Valid arguments, transmission bit off */
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, start_number, false),
      0);

   /* Data from request_payload_running, RY slave 2 */
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[0], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[1], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[2], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[3], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[4], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[5], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[6], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[0].bytes[7], 0x00);

   /* Data from request_payload_running, RY slave 3 */
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[0], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[1], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[2], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[3], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[4], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[5], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[6], 0x00);
   EXPECT_EQ (cls.cyclic_data_area.ry[1].bytes[7], 0x00);

   /* Data from request_payload_running, RWw slave 2 */
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[0].words[0]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[0].words[1]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[0].words[2]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[0].words[3]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[0].words[31]), 0x0000);

   /* Data from request_payload_running, RWw slave 3 */
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[1].words[0]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[1].words[1]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[1].words[2]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[1].words[3]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[1].words[4]), 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (cls.cyclic_data_area.rww[1].words[31]), 0x0000);

   /* Invalid total occupied station count in request */
   request.full_headers->cyclic_data_header.slave_total_occupied_station_count =
      CC_TO_LE16 (0); /* Invalid */
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, start_number, true),
      -1);

   request.full_headers->cyclic_data_header.slave_total_occupied_station_count =
      CC_TO_LE16 (17); /* Invalid */
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, start_number, true),
      -1);

   request.full_headers->cyclic_data_header.slave_total_occupied_station_count =
      CC_TO_LE16 (1); /* Less than what our station needs */
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, start_number, true),
      -1);

   request.full_headers->cyclic_data_header.slave_total_occupied_station_count =
      CC_TO_LE16 (3); /* OK */
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, start_number, true),
      0);

   /* Invalid start number */
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, 0, true),
      -1);
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, 17, true),
      -1);

   /* Invalid end number */
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, 3, true),
      -1);

   request.full_headers->cyclic_data_header.slave_total_occupied_station_count =
      CC_TO_LE16 (16);
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, 16, true),
      -1);

   request.full_headers->cyclic_data_header.slave_total_occupied_station_count =
      CC_TO_LE16 (5);
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, start_number, true),
      0);

   /* Invalid source memory areas, in struct describing the request */
   request.first_ry = nullptr;
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, start_number, true),
      -1);
   request.first_ry = ry_in_message;
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, start_number, true),
      0);

   request.first_rww = nullptr;
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, start_number, true),
      -1);
   request.first_rww = rww_in_message;
   EXPECT_EQ (
      cls_iefb_copy_cyclic_data_from_request (&cls, &request, start_number, true),
      0);
}

TEST_F (SlaveUnitTest, CciefbSetGetRxBits)
{
   cl_rx_t rx_areas[4]            = {};
   const uint16_t number_of_areas = NELEMENTS (rx_areas) & UINT16_MAX;
   uint16_t rxnumber              = 0;
   uint16_t areaindex             = 0;
   uint16_t byteindex             = 0;
   cl_cciefb_cyclic_resp_full_headers_t headers;

   clal_clear_memory (&headers, sizeof (headers));
   config.num_occupied_stations = 4;

   cls.cciefb_resp_frame_normal.full_headers = &headers;
   cls.cciefb_resp_frame_normal.first_rx     = rx_areas;

   /* Verify initial state */
   for (areaindex = 0; areaindex < number_of_areas; areaindex++)
   {
      for (byteindex = 0; byteindex < CL_BYTES_PER_BITAREA; byteindex++)
      {
         EXPECT_EQ (rx_areas[areaindex].bytes[byteindex], 0x00);
      }
   }

   /* Set RX0 */
   rxnumber = 0;
   cls_iefb_set_rx_bit (&cls, rxnumber, true);
   EXPECT_EQ (rx_areas[0].bytes[0], 0x01);
   EXPECT_EQ (rx_areas[0].bytes[1], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[2], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[3], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[4], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[5], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[6], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[7], 0x00);
   for (areaindex = 1; areaindex < number_of_areas; areaindex++)
   {
      for (byteindex = 0; byteindex < CL_BYTES_PER_BITAREA; byteindex++)
      {
         EXPECT_EQ (rx_areas[areaindex].bytes[byteindex], 0x00);
      }
   }
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber), true);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber + 1), false);

   cls_iefb_set_rx_bit (&cls, rxnumber, false);
   for (areaindex = 0; areaindex < number_of_areas; areaindex++)
   {
      for (byteindex = 0; byteindex < CL_BYTES_PER_BITAREA; byteindex++)
      {
         EXPECT_EQ (rx_areas[areaindex].bytes[byteindex], 0x00);
      }
   }
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber), false);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber + 1), false);

   /* Set RX1 */
   rxnumber = 1;
   cls_iefb_set_rx_bit (&cls, rxnumber, true);
   EXPECT_EQ (rx_areas[0].bytes[0], 0x02);
   EXPECT_EQ (rx_areas[0].bytes[1], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[2], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[3], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[4], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[5], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[6], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[7], 0x00);
   for (areaindex = 1; areaindex < number_of_areas; areaindex++)
   {
      for (byteindex = 0; byteindex < CL_BYTES_PER_BITAREA; byteindex++)
      {
         EXPECT_EQ (rx_areas[areaindex].bytes[byteindex], 0x00);
      }
   }
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber - 1), false);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber), true);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber + 1), false);

   cls_iefb_set_rx_bit (&cls, rxnumber, false);
   for (areaindex = 0; areaindex < number_of_areas; areaindex++)
   {
      for (byteindex = 0; byteindex < CL_BYTES_PER_BITAREA; byteindex++)
      {
         EXPECT_EQ (rx_areas[areaindex].bytes[byteindex], 0x00);
      }
   }
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber - 1), false);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber), false);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber + 1), false);

   /* Set RX63 */
   rxnumber = 63;
   cls_iefb_set_rx_bit (&cls, rxnumber, true);
   EXPECT_EQ (rx_areas[0].bytes[0], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[1], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[2], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[3], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[4], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[5], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[6], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[7], 0x80);
   for (areaindex = 1; areaindex < number_of_areas; areaindex++)
   {
      for (byteindex = 0; byteindex < CL_BYTES_PER_BITAREA; byteindex++)
      {
         EXPECT_EQ (rx_areas[areaindex].bytes[byteindex], 0x00);
      }
   }
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber - 1), false);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber), true);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber + 1), false);

   cls_iefb_set_rx_bit (&cls, rxnumber, false);
   for (areaindex = 0; areaindex < number_of_areas; areaindex++)
   {
      for (byteindex = 0; byteindex < CL_BYTES_PER_BITAREA; byteindex++)
      {
         EXPECT_EQ (rx_areas[areaindex].bytes[byteindex], 0x00);
      }
   }
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber - 1), false);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber), false);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber + 1), false);

   /* Set RX64 */
   rxnumber = 64;
   cls_iefb_set_rx_bit (&cls, rxnumber, true);
   EXPECT_EQ (rx_areas[0].bytes[0], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[1], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[2], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[3], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[4], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[5], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[6], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[7], 0x00);
   EXPECT_EQ (rx_areas[1].bytes[0], 0x01);
   EXPECT_EQ (rx_areas[1].bytes[1], 0x00);
   EXPECT_EQ (rx_areas[1].bytes[2], 0x00);
   EXPECT_EQ (rx_areas[1].bytes[3], 0x00);
   EXPECT_EQ (rx_areas[1].bytes[4], 0x00);
   EXPECT_EQ (rx_areas[1].bytes[5], 0x00);
   EXPECT_EQ (rx_areas[1].bytes[6], 0x00);
   EXPECT_EQ (rx_areas[1].bytes[7], 0x00);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber - 1), false);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber), true);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber + 1), false);

   cls_iefb_set_rx_bit (&cls, rxnumber, false);
   for (areaindex = 0; areaindex < number_of_areas; areaindex++)
   {
      for (byteindex = 0; byteindex < CL_BYTES_PER_BITAREA; byteindex++)
      {
         EXPECT_EQ (rx_areas[areaindex].bytes[byteindex], 0x00);
      }
   }
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber - 1), false);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber), false);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber + 1), false);

   /* Set RX65 */
   rxnumber = 65;
   cls_iefb_set_rx_bit (&cls, rxnumber, true);
   EXPECT_EQ (rx_areas[0].bytes[0], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[1], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[2], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[3], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[4], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[5], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[6], 0x00);
   EXPECT_EQ (rx_areas[0].bytes[7], 0x00);
   EXPECT_EQ (rx_areas[1].bytes[0], 0x02);
   EXPECT_EQ (rx_areas[1].bytes[1], 0x00);
   EXPECT_EQ (rx_areas[1].bytes[2], 0x00);
   EXPECT_EQ (rx_areas[1].bytes[3], 0x00);
   EXPECT_EQ (rx_areas[1].bytes[4], 0x00);
   EXPECT_EQ (rx_areas[1].bytes[5], 0x00);
   EXPECT_EQ (rx_areas[1].bytes[6], 0x00);
   EXPECT_EQ (rx_areas[1].bytes[7], 0x00);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber - 1), false);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber), true);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber + 1), false);

   cls_iefb_set_rx_bit (&cls, rxnumber, false);
   for (areaindex = 0; areaindex < number_of_areas; areaindex++)
   {
      for (byteindex = 0; byteindex < CL_BYTES_PER_BITAREA; byteindex++)
      {
         EXPECT_EQ (rx_areas[areaindex].bytes[byteindex], 0x00);
      }
   }
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber - 1), false);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber), false);
   EXPECT_EQ (cls_iefb_get_rx_bit (&cls, rxnumber + 1), false);
}

TEST_F (SlaveUnitTest, CciefbGetSetRwrValue)
{
   cl_rwr_t rwr_areas[4] = {};
   uint16_t rwr_number   = 0;
   cl_cciefb_cyclic_resp_full_headers_t headers;

   clal_clear_memory (&headers, sizeof (headers));
   cls.config.num_occupied_stations          = 4;
   cls.cciefb_resp_frame_normal.full_headers = &headers;
   cls.cciefb_resp_frame_normal.first_rwr    = rwr_areas;

   EXPECT_EQ (CC_FROM_LE16 (rwr_areas[0].words[0]), 0x0000U);

   /* Set RWr0 */
   rwr_number = 0;
   cls_iefb_set_rwr_value (&cls, rwr_number, 0x1234);
   EXPECT_EQ (CC_FROM_LE16 (rwr_areas[0].words[rwr_number]), 0x1234U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number), 0x1234U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number + 1), 0x0000U);

   cls_iefb_set_rwr_value (&cls, rwr_number, 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (rwr_areas[0].words[rwr_number]), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number + 1), 0x0000U);

   /* Set RWr1 */
   rwr_number = 1;
   cls_iefb_set_rwr_value (&cls, rwr_number, 0x1234);
   EXPECT_EQ (CC_FROM_LE16 (rwr_areas[0].words[rwr_number]), 0x1234U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number - 1), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number), 0x1234U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number + 1), 0x0000U);

   cls_iefb_set_rwr_value (&cls, rwr_number, 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (rwr_areas[0].words[rwr_number]), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number - 1), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number + 1), 0x0000U);

   /* Set RWr30 */
   rwr_number = 30;
   cls_iefb_set_rwr_value (&cls, rwr_number, 0x1234);
   EXPECT_EQ (CC_FROM_LE16 (rwr_areas[0].words[rwr_number]), 0x1234U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number - 1), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number), 0x1234U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number + 1), 0x0000U);

   cls_iefb_set_rwr_value (&cls, rwr_number, 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (rwr_areas[0].words[rwr_number]), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number - 1), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number + 1), 0x0000U);

   /* Set RWr31 */
   rwr_number = 31;
   cls_iefb_set_rwr_value (&cls, rwr_number, 0x1234);
   EXPECT_EQ (CC_FROM_LE16 (rwr_areas[0].words[rwr_number]), 0x1234U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number - 1), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number), 0x1234U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number + 1), 0x0000U);

   cls_iefb_set_rwr_value (&cls, rwr_number, 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (rwr_areas[0].words[rwr_number]), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number - 1), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number + 1), 0x0000U);

   /* Set RWr32 */
   rwr_number = 32;
   cls_iefb_set_rwr_value (&cls, rwr_number, 0x1234);
   EXPECT_EQ (CC_FROM_LE16 (rwr_areas[1].words[rwr_number % 32]), 0x1234U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number - 1), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number), 0x1234U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number + 1), 0x0000U);

   cls_iefb_set_rwr_value (&cls, rwr_number, 0x0000);
   EXPECT_EQ (CC_FROM_LE16 (rwr_areas[0].words[rwr_number % 32]), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number - 1), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number), 0x0000U);
   EXPECT_EQ (cls_iefb_get_rwr_value (&cls, rwr_number + 1), 0x0000U);
}

/*************************** State machine tests ***************************/

TEST_F (SlaveUnitTest, CciefbCallbacksShouldHandleNullRequest)
{
   EXPECT_EQ (
      cls_iefb_slave_init (&cls, now, nullptr, CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);

   EXPECT_EQ (
      cls_iefb_handle_wrong_master (&cls, now, nullptr, CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);

   EXPECT_EQ (
      cls_iefb_handle_wrong_stationcount (&cls, now, nullptr, CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);

   EXPECT_EQ (
      cls_iefb_handle_incoming_when_disabled (
         &cls,
         now,
         nullptr,
         CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);

   EXPECT_EQ (
      cls_iefb_handle_new_or_updated_master (&cls, now, nullptr, CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);

   EXPECT_EQ (
      cls_iefb_on_entry_master_none (&cls, now, nullptr, CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);

   EXPECT_EQ (
      cls_iefb_handle_cyclic_event (&cls, now, nullptr, CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);

   EXPECT_EQ (
      cls_iefb_log_timeout_master (&cls, now, nullptr, CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);

   EXPECT_EQ (
      cls_iefb_log_ip_addr_updated (&cls, now, nullptr, CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);

   EXPECT_EQ (
      cls_iefb_log_slave_disabled (&cls, now, nullptr, CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);

   EXPECT_EQ (
      cls_iefb_log_slave_reenabled (&cls, now, nullptr, CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);

   EXPECT_EQ (
      cls_iefb_on_entry_master_none (&cls, now, nullptr, CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);

   EXPECT_EQ (
      cls_iefb_on_entry_wait_disabling_slave (
         &cls,
         now,
         nullptr,
         CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);

   EXPECT_EQ (
      cls_iefb_on_entry_slave_disabled (&cls, now, nullptr, CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);
}

TEST_F (SlaveUnitTest, CciefbHandleSlaveCyclicEventInvalidOccupied)
{
   cls_cciefb_cyclic_request_info_t request = {};
   cl_rww_t rww[4]                          = {};
   cl_cciefb_cyclic_req_full_headers_t headers;

   clal_clear_memory (&headers, sizeof (headers));
   request.full_headers = &headers;
   request.first_rww    = (cl_rww_t *)&rww[0];

   /* Invalid total occupied slave stations */
   headers.cyclic_data_header.cyclic_transmission_state          = 0xFFFF;
   cls.master.slave_station_no                                   = 1;
   headers.cyclic_data_header.slave_total_occupied_station_count = 0;
   EXPECT_EQ (
      cls_iefb_handle_cyclic_event (&cls, now, &request, CLS_SLAVE_EVENT_NONE),
      CLS_SLAVE_EVENT_NONE);
}

TEST_F (SlaveUnitTest, CciefbStatemachineInStateDown)
{
   const cls_slave_event_t events_without_effect[] = {
      CLS_SLAVE_EVENT_NONE,
      CLS_SLAVE_EVENT_CYCLIC_NEW_MASTER,
      CLS_SLAVE_EVENT_CYCLIC_CORRECT_MASTER,
      CLS_SLAVE_EVENT_CYCLIC_WRONG_MASTER,
      CLS_SLAVE_EVENT_CYCLIC_WRONG_STATIONCOUNT,
      CLS_SLAVE_EVENT_CYCLIC_INCOMING_WHEN_DISABLED,
      CLS_SLAVE_EVENT_TIMEOUT_MASTER,
      CLS_SLAVE_EVENT_REENABLE_SLAVE,
      CLS_SLAVE_EVENT_DISABLE_SLAVE,
      CLS_SLAVE_EVENT_DISABLE_SLAVE_WAIT_ENDED,
      CLS_SLAVE_EVENT_IP_UPDATED,
      CLS_SLAVE_EVENT_LAST};
   size_t i;
   cls_cciefb_cyclic_request_info_t request = {};
   cl_rww_t rww[4]                          = {};
   cl_cciefb_cyclic_req_full_headers_t headers;

   clal_clear_memory (&headers, sizeof (headers));
   request.full_headers = &headers;
   request.first_rww    = (cl_rww_t *)&rww[0];

   /* Force state machine to desired state */
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DOWN);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 0);
   cls_fsm_init (&cls, now);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   cls.state = CLS_SLAVE_STATE_SLAVE_DOWN;
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DOWN);

   /* Events that should effect the state */
   cls.master_state_callback_trigger_data.executed_before = false;
   cls_iefb_fsm_event (&cls, now, &request, CLS_SLAVE_EVENT_STARTUP);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, false);
   cls.state = CLS_SLAVE_STATE_SLAVE_DOWN;
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DOWN);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      cls_iefb_fsm_event (&cls, now, &request, events_without_effect[i]);
      ASSERT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DOWN)
         << "Triggered by cls_slave_event_t with value "
         << events_without_effect[i];
   }
}

TEST_F (SlaveIntegrationTestNotConnected, CciefbStatemachineInStateMasterNone)
{
   const cls_slave_event_t events_without_effect[] = {
      CLS_SLAVE_EVENT_NONE,
      CLS_SLAVE_EVENT_STARTUP,
      CLS_SLAVE_EVENT_CYCLIC_CORRECT_MASTER,
      CLS_SLAVE_EVENT_CYCLIC_WRONG_MASTER,
      CLS_SLAVE_EVENT_CYCLIC_WRONG_STATIONCOUNT,
      CLS_SLAVE_EVENT_CYCLIC_INCOMING_WHEN_DISABLED,
      CLS_SLAVE_EVENT_TIMEOUT_MASTER,
      CLS_SLAVE_EVENT_REENABLE_SLAVE,
      CLS_SLAVE_EVENT_DISABLE_SLAVE_WAIT_ENDED,
      CLS_SLAVE_EVENT_IP_UPDATED,
      CLS_SLAVE_EVENT_LAST};
   size_t i;
   cls_cciefb_cyclic_request_info_t request = {};

   /* Prepare request struct from buffer */
   ASSERT_EQ (
      cl_iefb_parse_cyclic_request (
         (uint8_t *)&request_payload_running,
         SIZE_REQUEST_3_SLAVES,
         remote_ip,
         123, /* remote_port */
         my_ip,
         &request),
      0);

   /* Verify that we are in desired state */
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, false);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);

   /* Events that should effect the state */
   cls_iefb_fsm_event (&cls, now, &request, CLS_SLAVE_EVENT_CYCLIC_NEW_MASTER);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.master_ip_addr, remote_ip);
   cls.state = CLS_SLAVE_STATE_MASTER_NONE;
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   cls_iefb_fsm_event (&cls, now, &request, CLS_SLAVE_EVENT_DISABLE_SLAVE);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DISABLED);
   cls.state = CLS_SLAVE_STATE_MASTER_NONE;
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   cls_iefb_fsm_event (
      &cls,
      now,
      &request,
      CLS_SLAVE_EVENT_CYCLIC_WRONG_STATIONCOUNT);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      cls_iefb_fsm_event (&cls, now, &request, events_without_effect[i]);
      ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE)
         << "Triggered by cls_slave_event_t with value "
         << events_without_effect[i];
   }
}

TEST_F (SlaveIntegrationTestConnected, CciefbStatemachineInStateMasterControl)
{
   const cls_slave_event_t events_without_effect[] = {
      CLS_SLAVE_EVENT_NONE,
      CLS_SLAVE_EVENT_STARTUP,
      CLS_SLAVE_EVENT_CYCLIC_NEW_MASTER,
      CLS_SLAVE_EVENT_CYCLIC_CORRECT_MASTER,
      CLS_SLAVE_EVENT_CYCLIC_WRONG_MASTER,
      CLS_SLAVE_EVENT_CYCLIC_INCOMING_WHEN_DISABLED,
      CLS_SLAVE_EVENT_REENABLE_SLAVE,
      CLS_SLAVE_EVENT_DISABLE_SLAVE_WAIT_ENDED,
      CLS_SLAVE_EVENT_LAST};
   size_t i;
   cls_cciefb_cyclic_request_info_t request = {};

   /* Prepare request struct from buffer */
   ASSERT_EQ (
      cl_iefb_parse_cyclic_request (
         (uint8_t *)&request_payload_running,
         SIZE_REQUEST_3_SLAVES,
         remote_ip,
         123, /* remote_port */
         my_ip,
         &request),
      0);

   /* Verify that we are in desired state */
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);

   /* Events that should effect the state */
   cls.master_state_callback_trigger_data.executed_before = false;
   cls_iefb_fsm_event (
      &cls,
      now,
      &request,
      CLS_SLAVE_EVENT_CYCLIC_WRONG_STATIONCOUNT);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   cls.state = CLS_SLAVE_STATE_MASTER_CONTROL;
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);

   cls.master_state_callback_trigger_data.executed_before = false;
   cls_iefb_fsm_event (&cls, now, &request, CLS_SLAVE_EVENT_TIMEOUT_MASTER);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 4);
   cls.state = CLS_SLAVE_STATE_MASTER_CONTROL;
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);

   cls.master_state_callback_trigger_data.executed_before = false;
   cls_iefb_fsm_event (&cls, now, &request, CLS_SLAVE_EVENT_IP_UPDATED);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 5);
   cls.state = CLS_SLAVE_STATE_MASTER_CONTROL;
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);

   cls.master_state_callback_trigger_data.executed_before = false;
   cls_iefb_fsm_event (&cls, now, &request, CLS_SLAVE_EVENT_DISABLE_SLAVE);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 4);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 6);
   cls.state = CLS_SLAVE_STATE_MASTER_CONTROL;
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);

   cls.master_state_callback_trigger_data.executed_before = false;
   cls_iefb_fsm_event (&cls, now, &request, CLS_SLAVE_EVENT_CYCLIC_NEW_MASTER);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 4);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 7);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      cls_iefb_fsm_event (&cls, now, &request, events_without_effect[i]);
      ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL)
         << "Triggered by cls_slave_event_t with value "
         << events_without_effect[i];
   }
}

TEST_F (SlaveIntegrationTestNotConnected, CciefbStatemachineInStateWaitDisablingSlave)
{
   const cls_slave_event_t events_without_effect[] = {
      CLS_SLAVE_EVENT_NONE,
      CLS_SLAVE_EVENT_STARTUP,
      CLS_SLAVE_EVENT_CYCLIC_NEW_MASTER,
      CLS_SLAVE_EVENT_CYCLIC_CORRECT_MASTER,
      CLS_SLAVE_EVENT_CYCLIC_WRONG_MASTER,
      CLS_SLAVE_EVENT_CYCLIC_WRONG_STATIONCOUNT,
      CLS_SLAVE_EVENT_CYCLIC_INCOMING_WHEN_DISABLED,
      CLS_SLAVE_EVENT_TIMEOUT_MASTER,
      CLS_SLAVE_EVENT_DISABLE_SLAVE,
      CLS_SLAVE_EVENT_LAST};
   size_t i;
   cls_cciefb_cyclic_request_info_t request = {};

   /* Prepare request struct from buffer */
   ASSERT_EQ (
      cl_iefb_parse_cyclic_request (
         (uint8_t *)&request_payload_running,
         SIZE_REQUEST_3_SLAVES,
         remote_ip,
         123, /* remote_port */
         my_ip,
         &request),
      0);

   /* Force state machine to desired state */
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, false);
   cls.state = CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE;
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);

   /* Events that should effect the state */
   cls.master_state_callback_trigger_data.executed_before = false;
   cls_iefb_fsm_event (
      &cls,
      now,
      &request,
      CLS_SLAVE_EVENT_DISABLE_SLAVE_WAIT_ENDED);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DISABLED);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   cls.state = CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE;
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);

   cls.master_state_callback_trigger_data.executed_before = false;
   cls_iefb_fsm_event (&cls, now, &request, CLS_SLAVE_EVENT_IP_UPDATED);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DISABLED);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   cls.state = CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE;
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);

   cls.master_state_callback_trigger_data.executed_before = false;
   cls_iefb_fsm_event (&cls, now, &request, CLS_SLAVE_EVENT_REENABLE_SLAVE);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   cls.state = CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE;
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      cls_iefb_fsm_event (&cls, now, &request, events_without_effect[i]);
      ASSERT_EQ (cls.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE)
         << "Triggered by cls_slave_event_t with value "
         << events_without_effect[i];
   }
}

TEST_F (SlaveIntegrationTestNotConnected, CciefbStatemachineInStateSlaveDisabled)
{
   const cls_slave_event_t events_without_effect[] = {
      CLS_SLAVE_EVENT_NONE,
      CLS_SLAVE_EVENT_STARTUP,
      CLS_SLAVE_EVENT_CYCLIC_NEW_MASTER,
      CLS_SLAVE_EVENT_CYCLIC_CORRECT_MASTER,
      CLS_SLAVE_EVENT_CYCLIC_WRONG_MASTER,
      CLS_SLAVE_EVENT_CYCLIC_WRONG_STATIONCOUNT,
      CLS_SLAVE_EVENT_CYCLIC_INCOMING_WHEN_DISABLED,
      CLS_SLAVE_EVENT_TIMEOUT_MASTER,
      CLS_SLAVE_EVENT_DISABLE_SLAVE,
      CLS_SLAVE_EVENT_DISABLE_SLAVE_WAIT_ENDED,
      CLS_SLAVE_EVENT_IP_UPDATED,
      CLS_SLAVE_EVENT_LAST};
   size_t i;
   cls_cciefb_cyclic_request_info_t request = {};

   /* Prepare request struct from buffer */
   ASSERT_EQ (
      cl_iefb_parse_cyclic_request (
         (uint8_t *)&request_payload_running,
         SIZE_REQUEST_3_SLAVES,
         remote_ip,
         123, /* remote_port */
         my_ip,
         &request),
      0);

   /* Force state machine to desired state */
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, false);
   cls.state = CLS_SLAVE_STATE_SLAVE_DISABLED;
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DISABLED);

   /* Events that should effect the state */
   cls.master_state_callback_trigger_data.executed_before = false;
   cls_iefb_fsm_event (&cls, now, &request, CLS_SLAVE_EVENT_REENABLE_SLAVE);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   cls.state = CLS_SLAVE_STATE_SLAVE_DISABLED;
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DISABLED);

   /* Events that should not effect the state */
   for (i = 0; i < NELEMENTS (events_without_effect); i++)
   {
      cls_iefb_fsm_event (&cls, now, &request, events_without_effect[i]);
      ASSERT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DISABLED)
         << "Triggered by cls_slave_event_t with value "
         << events_without_effect[i];
   }
}

/************************ Master not yet connected *************************/

/**
 * Verify slave startup
 *
 * @req REQ_CL_PROTOCOL_37
 * @req REQ_CL_PROTOCOL_52
 * @req REQ_CL_UDP_01
 * @req REQ_CLS_CONFORMANCE_01
 * @req REQ_CLS_CONTRMASTER_01
 * @req REQ_CLS_PARAMETERID_01
 * @req REQ_CLS_UDP_01
 * @req REQ_CLS_VERSION_02
 *
 */
TEST_F (SlaveUnitTest, CciefbStartup)
{
   uint64_t resulting_timestamp                      = 0;
   const cls_master_connection_t * master_connection = nullptr;
   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   ASSERT_FALSE (mock_data.udp_ports[0].in_use);
   ASSERT_FALSE (mock_data.udp_ports[1].in_use);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);

   ASSERT_EQ (cls_slave_init (&cls, &config, now), 0);

   ASSERT_TRUE (mock_data.udp_ports[0].in_use);
   ASSERT_TRUE (mock_data.udp_ports[0].is_open);
   ASSERT_TRUE (mock_data.udp_ports[1].in_use);
   ASSERT_TRUE (mock_data.udp_ports[1].is_open);
   ASSERT_FALSE (mock_data.udp_ports[2].in_use);
   ASSERT_FALSE (mock_data.udp_ports[3].in_use);

   ASSERT_EQ (&mock_data.udp_ports[0], mock_cciefb_port);
   ASSERT_EQ (&mock_data.udp_ports[1], mock_slmp_port);
   ASSERT_EQ (mock_cciefb_port->port_number, CL_CCIEFB_PORT);
   ASSERT_EQ (mock_cciefb_port->ip_addr, CL_IPADDR_ANY);
   ASSERT_EQ (mock_cciefb_port->ifindex, 0);
   ASSERT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   ASSERT_EQ (mock_slmp_port->port_number, CL_SLMP_PORT);
   ASSERT_EQ (mock_slmp_port->ip_addr, CL_IPADDR_ANY);
   ASSERT_EQ (mock_slmp_port->ifindex, 0);
   ASSERT_EQ (mock_slmp_port->number_of_calls_open, 1);

   master_connection = cls_get_master_connection_details (&cls);
   ASSERT_TRUE (master_connection != nullptr);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, 0);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);
   EXPECT_EQ (master_connection->clock_info, 0U);
   EXPECT_EQ (master_connection->clock_info_valid, false);
   EXPECT_EQ (master_connection->group_no, 0);
   EXPECT_EQ (master_connection->master_id, CL_IPADDR_INVALID);
   EXPECT_EQ (master_connection->parallel_off_timeout_count, 0);
   EXPECT_EQ (master_connection->parameter_no, 0);
   EXPECT_EQ (master_connection->protocol_ver, 0);
   EXPECT_EQ (master_connection->slave_station_no, 0);
   EXPECT_EQ (master_connection->timeout_value, 0);
   EXPECT_EQ (master_connection->total_occupied_station_count, 0);
   EXPECT_EQ (cls_iefb_get_master_timestamp (&cls, &resulting_timestamp), -1);

   /* Run slave stack */
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, (size_t)0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, (size_t)0);
   EXPECT_EQ (mock_slmp_port->number_of_calls_open, 1);

   /* Master connects, slave responds */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_initial,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, remote_protocol_ver);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.group_no, my_group_no);
   EXPECT_EQ (mock_data.slave_cb_connect.master_ip_addr, remote_ip);
   EXPECT_EQ (mock_data.slave_cb_connect.slave_station_no, my_slave_station_no);
   EXPECT_EQ (master_connection->clock_info_valid, true);
   EXPECT_EQ (master_connection->group_no, my_group_no);
   EXPECT_EQ (master_connection->master_id, remote_ip);
   EXPECT_EQ (master_connection->parallel_off_timeout_count, timeout_count);
   EXPECT_EQ (master_connection->parameter_no, parameter_no);
   EXPECT_EQ (master_connection->protocol_ver, remote_protocol_ver);
   EXPECT_EQ (master_connection->slave_station_no, my_slave_station_no);
   EXPECT_EQ (master_connection->timeout_value, timeout_ms);
   EXPECT_EQ (
      master_connection->total_occupied_station_count,
      total_occupied_stations);
   EXPECT_EQ (cls.master.parameter_no, parameter_no);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);

   EXPECT_EQ (cls_iefb_get_master_timestamp (&cls, &resulting_timestamp), 0);
   EXPECT_EQ (resulting_timestamp, 0xEFCDAB9078563407UL);

   /* Cyclic data from master */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);

   EXPECT_EQ (cls_iefb_get_master_timestamp (&cls, &resulting_timestamp), 0);
   EXPECT_EQ (resulting_timestamp, 0xEFCDAB9078563412UL);
}

/********* Integration tests with master not yet connected *****************/

/**
 * Verify master connection to slave
 *
 * @req REQ_CL_PROTOCOL_44
 * @req REQ_CL_PROTOCOL_46
 * @req REQ_CL_PROTOCOL_47
 * @req REQ_CL_PROTOCOL_51
 * @req REQ_CL_PROTOCOL_52
 * @req REQ_CLS_GROUPS_01
 * @req REQ_CLS_GROUPS_03
 * @req REQ_CLS_PARAMETERID_02
 * @req REQ_CLS_SEQUENCE_01
 * @req REQ_CLS_STATIONNUMBER_01
 * @req REQ_CLS_STATIONNUMBER_02
 * @req REQ_CLS_STATIONNUMBER_03
 *
 */
TEST_F (SlaveIntegrationTestNotConnected, CciefbConnectAndCyclic)
{
   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (cls.local_management_info, 0U);
   EXPECT_EQ (cls.master.slave_station_no, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, 0);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED);

   /* Master connects */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_initial,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, remote_protocol_ver);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (cls.master.master_id, remote_ip);
   EXPECT_EQ (cls.master.protocol_ver, my_protocol_ver);
   EXPECT_EQ (cls.master.group_no, my_group_no);
   EXPECT_EQ (cls.master.parameter_no, parameter_no);
   EXPECT_EQ (cls.master.timeout_value, timeout_ms);
   EXPECT_EQ (cls.master.parallel_off_timeout_count, timeout_count);
   EXPECT_EQ (cls.master.slave_station_no, my_slave_station_no);
   EXPECT_EQ (cls.master.total_occupied_station_count, total_occupied_stations);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.slave_id, my_ip);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);

   /* Cyclic data from master */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);

   now += tick_size;
   cls_iefb_periodic (&cls, now);
}

TEST_F (SlaveIntegrationTestNotConnected, CciefbNoConnectCallback)
{
   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (cls.local_management_info, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (cls.master.parameter_no, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   cls.config.connect_cb = nullptr;

   /* Master connects */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_initial,
      SIZE_REQUEST_3_SLAVES);
   now += tick_size;
   cls_iefb_periodic (&cls, now);

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, remote_protocol_ver);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
}

TEST_F (SlaveIntegrationTestNotConnected, CciefbNoMasterTimestamp)
{
   uint64_t resulting_timestamp                                        = 0;
   uint8_t initial_request_no_timestamp[SIZE_REQUEST_3_SLAVES]         = {};
   uint8_t request_payload_running_no_timestamp[SIZE_REQUEST_3_SLAVES] = {};
   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   /* Prepare requests frame with no timestamp */
   clal_memcpy (
      initial_request_no_timestamp,
      sizeof (initial_request_no_timestamp),
      request_payload_initial,
      SIZE_REQUEST_3_SLAVES);
   initial_request_no_timestamp[39] = 0x00;
   initial_request_no_timestamp[40] = 0x00;
   initial_request_no_timestamp[41] = 0x00;
   initial_request_no_timestamp[42] = 0x00;
   initial_request_no_timestamp[43] = 0x00;
   initial_request_no_timestamp[44] = 0x00;
   initial_request_no_timestamp[45] = 0x00;
   initial_request_no_timestamp[46] = 0x00;
   clal_memcpy (
      request_payload_running_no_timestamp,
      sizeof (request_payload_running_no_timestamp),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_payload_running_no_timestamp[39] = 0x00;
   request_payload_running_no_timestamp[40] = 0x00;
   request_payload_running_no_timestamp[41] = 0x00;
   request_payload_running_no_timestamp[42] = 0x00;
   request_payload_running_no_timestamp[43] = 0x00;
   request_payload_running_no_timestamp[44] = 0x00;
   request_payload_running_no_timestamp[45] = 0x00;
   request_payload_running_no_timestamp[46] = 0x00;

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (cls.local_management_info, 0U);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, 0);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (cls_iefb_get_master_timestamp (&cls, &resulting_timestamp), -1);

   /* Master connects, no timestamp available. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&initial_request_no_timestamp,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, remote_protocol_ver);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.slave_id, my_ip);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
   EXPECT_EQ (cls_iefb_get_master_timestamp (&cls, &resulting_timestamp), -1);

   /* Cyclic data from master, with timestamp */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
   EXPECT_EQ (cls_iefb_get_master_timestamp (&cls, &resulting_timestamp), 0);
   EXPECT_EQ (resulting_timestamp, new_clock_info);

   /* Cyclic data from master, without timestamp */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running_no_timestamp,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
   EXPECT_EQ (cls_iefb_get_master_timestamp (&cls, &resulting_timestamp), -1);
}

/**
 * Request does not contain slave IP address
 *
 * @req REQ_CLS_COMMUNIC_01
 * @req REQ_CLS_GROUPS_01
 *
 */
TEST_F (SlaveIntegrationTestNotConnected, CciefbConnectWrongSlaveID)
{
   uint8_t initial_request_wrong_slaveid[SIZE_REQUEST_3_SLAVES] = {};

   /* Prepare initial request frame with wrong slave ID */
   clal_memcpy (
      initial_request_wrong_slaveid,
      sizeof (initial_request_wrong_slaveid),
      request_payload_initial,
      SIZE_REQUEST_3_SLAVES);
   initial_request_wrong_slaveid[74] = 0x0A;

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);

   /* Master tries to connect with wrong slave ID. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&initial_request_wrong_slaveid,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->output_data_size, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
}

/**
 * Master tries to connect to other group
 */
TEST_F (SlaveIntegrationTestNotConnected, CciefbConnectWrongGroup)
{
   uint8_t initial_request_wrong_group[SIZE_REQUEST_3_SLAVES] = {};

   /* Prepare initial request frame for other group */
   clal_memcpy (
      initial_request_wrong_group,
      sizeof (initial_request_wrong_group),
      request_payload_initial,
      SIZE_REQUEST_3_SLAVES);
   initial_request_wrong_group[74] = 0xAA;

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);

   /* Master tries to connect, but to other group. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&initial_request_wrong_group,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->output_data_size, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, CciefbConnectWrongCommand)
{
   uint8_t request_wrong_command[SIZE_REQUEST_3_SLAVES] = {};

   /* Prepare request with wrong command  */
   clal_memcpy (
      request_wrong_command,
      sizeof (request_wrong_command),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_wrong_command[11] = 0x12;

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);

   /* Master tries to connect, with wrong command. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_wrong_command,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->output_data_size, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, CciefbConnectWrongSubcommand)
{
   uint8_t request_wrong_subcommand[SIZE_REQUEST_3_SLAVES] = {};

   /* Prepare request with wrong subcommand  */
   clal_memcpy (
      request_wrong_subcommand,
      sizeof (request_wrong_subcommand),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_wrong_subcommand[13] = 0x12;

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);

   /* Master tries to connect, with wrong subcommand. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_wrong_subcommand,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->output_data_size, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, CciefbConnectInvalidHeader)
{
   uint8_t request_header_invalid[SIZE_REQUEST_3_SLAVES] = {};

   /* Prepare ill formatted request header */
   clal_memcpy (
      request_header_invalid,
      sizeof (request_header_invalid),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_header_invalid[0] = 0x12;

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);

   /* Master tries to connect, with ill formatted request header.
      Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_header_invalid,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->output_data_size, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, CciefbConnectInvalidCyclicHeader)
{
   uint8_t request_cyclic_header_invalid[SIZE_REQUEST_3_SLAVES] = {};

   /* Prepare ill formatted cyclic request header */
   clal_memcpy (
      request_cyclic_header_invalid,
      sizeof (request_cyclic_header_invalid),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_cyclic_header_invalid[65] = 0x12;

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);

   /* Master tries to connect, with ill formatted cyclic request header.
      Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_cyclic_header_invalid,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->output_data_size, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
}

TEST_F (SlaveIntegrationTestNotConnected, CciefbConnectTooShortFrame)
{
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);

   /* Master tries to connect, with too short request. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      4);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 4U);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->output_data_size, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
}

/**
 * Slave detects slave duplication
 *
 * @req REQ_CLS_CONFORMANCE_09
 * @req REQ_CLS_STATUSBIT_04
 *
 */
TEST_F (SlaveIntegrationTestNotConnected, CciefbSlaveDuplication)
{
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);

   /* Master tries to connect, with wrong slave state.
      Transmission bit is already on. "Slave duplication"
      Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->output_data_size, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 1);
   EXPECT_EQ (
      mock_data.slave_cb_error.error_message,
      CLS_ERROR_SLAVE_STATION_DUPLICATION);
   EXPECT_EQ (mock_data.slave_cb_error.ip_addr, CL_IPADDR_INVALID);
   EXPECT_EQ (mock_data.slave_cb_error.argument_2, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);

   /* Master sends again. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->output_data_size, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   /* Error callback is not triggered again */
   EXPECT_EQ (mock_data.slave_cb_error.calls, 1);
}

/**
 * Slave detects slave duplication, no error callback registered
 *
 */
TEST_F (SlaveIntegrationTestNotConnected, CciefbNoErrorCallback)
{
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);

   cls.config.error_cb = nullptr;

   /* Master tries to connect, with wrong slave state.
      Transmission bit is already on. "Slave duplication"
      Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->output_data_size, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
}

/**
 * Master tries to connect with wrong number of occupied slaves.
 *
 * @req REQ_CLS_CONFORMANCE_10
 */
TEST_F (SlaveIntegrationTestNotConnected, CciefbRunningWrongNumberOccupied)
{
   uint8_t request_initial_wrong_occupied[SIZE_REQUEST_3_SLAVES] = {};
   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   /* Prepare request with wrong number of occupied stations.
      One occupied station, but we expect two. */
   clal_memcpy (
      request_initial_wrong_occupied,
      sizeof (request_initial_wrong_occupied),
      request_payload_initial,
      SIZE_REQUEST_3_SLAVES);
   request_initial_wrong_occupied[75] = 0x01; /* Destroy 255.255.255.255 */

   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 0);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 0U);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 0U);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);

   /* Wrong number of stations from master.
      Respond with error frame */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_initial_wrong_occupied,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 1);
   EXPECT_EQ (
      mock_data.slave_cb_error.error_message,
      CLS_ERROR_WRONG_NUMBER_OCCUPIED);
   EXPECT_EQ (mock_data.slave_cb_error.ip_addr, CL_IPADDR_INVALID);
   EXPECT_EQ (mock_data.slave_cb_error.argument_2, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 3);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (
      cyclic_response_result.end_code,
      CL_SLMP_ENDCODE_CCIEFB_WRONG_NUMBER_OCCUPIED_STATIONS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
}

/************** Integration tests with master connected ********************/

/**
 * Verify watchdog for incoming requests
 *
 * @req REQ_CLS_CONFORMANCE_08
 * @req REQ_CLS_CONTRMASTER_02
 * @req REQ_CLS_GROUPS_01
 * @req REQ_CLS_TIMING_01
 *
 */
TEST_F (SlaveIntegrationTestConnected, CciefbMasterTimesOut)
{
   const cls_master_connection_t * master_connection = nullptr;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, remote_protocol_ver);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_connect.slave_station_no, my_slave_station_no);
   EXPECT_EQ (mock_data.slave_cb_connect.group_no, my_group_no);
   EXPECT_EQ (mock_data.slave_cb_connect.master_ip_addr, remote_ip);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (cls.master.parameter_no, parameter_no);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   master_connection = cls_get_master_connection_details (&cls);
   ASSERT_TRUE (master_connection != nullptr);
   EXPECT_EQ (master_connection->clock_info, new_clock_info);
   EXPECT_EQ (master_connection->clock_info_valid, true);
   EXPECT_EQ (master_connection->group_no, my_group_no);
   EXPECT_EQ (master_connection->master_id, remote_ip);
   EXPECT_EQ (master_connection->parallel_off_timeout_count, timeout_count);
   EXPECT_EQ (master_connection->parameter_no, parameter_no);
   EXPECT_EQ (master_connection->protocol_ver, remote_protocol_ver);
   EXPECT_EQ (master_connection->slave_station_no, my_slave_station_no);
   EXPECT_EQ (master_connection->timeout_value, timeout_ms);
   EXPECT_EQ (
      master_connection->total_occupied_station_count,
      total_occupied_stations);

   /* Master times out (very large values in incoming frame).
      Go to state MASTER_NONE. Send no frame. */
   now += longer_than_timeout_us;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, 0);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (cls.master.parameter_no, 0);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   EXPECT_EQ (master_connection->clock_info, 0U);
   EXPECT_EQ (master_connection->clock_info_valid, false);
   EXPECT_EQ (master_connection->group_no, 0);
   EXPECT_EQ (master_connection->master_id, 0U);
   EXPECT_EQ (master_connection->parallel_off_timeout_count, 0);
   EXPECT_EQ (master_connection->parameter_no, 0);
   EXPECT_EQ (master_connection->protocol_ver, 0);
   EXPECT_EQ (master_connection->slave_station_no, 0);
   EXPECT_EQ (master_connection->timeout_value, 0);
   EXPECT_EQ (master_connection->total_occupied_station_count, 0);
}

TEST_F (SlaveIntegrationTestConnected, CciefbNoDisconnectCallback)
{
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (cls.master.parameter_no, parameter_no);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   cls.config.disconnect_cb = nullptr;

   /* Master times out (very large values in incoming frame).
      Go to state MASTER_NONE. Send no frame. */
   now += longer_than_timeout_us;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (cls.master.parameter_no, 0);
}

/**
 * Request from wrong master
 *
 * @req REQ_CLS_CAPACITY_06
 * @req REQ_CLS_CONFORMANCE_07
 * @req REQ_CLS_ERROR_03
 * @req REQ_CLS_ERROR_07
 * @req REQ_CLS_ERROR_08
 *
 */
TEST_F (SlaveIntegrationTestConnected, CciefbWrongMaster)
{
   const cl_ipaddr_t wrong_master_ip = 0x01020404; /* IP 1.2.4.4 */
   uint8_t request_wrong_master[SIZE_REQUEST_3_SLAVES] = {};
   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   /* Prepare request with wrong master */
   clal_memcpy (
      request_wrong_master,
      sizeof (request_wrong_master),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_wrong_master[48] = 0x04; /* Use wrong_master_ip */

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   /* Request from wrong master. Respond with error frame to that master.  */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      wrong_master_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_wrong_master,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 1);
   EXPECT_EQ (
      mock_data.slave_cb_error.error_message,
      CLS_ERROR_MASTER_STATION_DUPLICATION);
   EXPECT_EQ (mock_data.slave_cb_error.ip_addr, wrong_master_ip);
   EXPECT_EQ (mock_data.slave_cb_error.argument_2, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, wrong_master_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (
      cyclic_response_result.end_code,
      CL_SLMP_ENDCODE_CCIEFB_MASTER_DUPLICATION);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
   /* TODO verify that data in response is 0. REQ_CLS_ERROR_02 */

   /* TODO (rtljobe): Verify that communication with first master continue.
                      See REQ_CLS_CONFORMANCE_07 */
}

/**
 * Verify SlaveLocalUnitInfo
 *
 * @req REQ_CL_PROTOCOL_48
 * @req REQ_CL_PROTOCOL_49
 *
 */
TEST_F (SlaveIntegrationTestConnected, CciefbSlaveApplicationStatus)
{

   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   ASSERT_EQ (
      cls.slave_application_status,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   ASSERT_EQ (
      cls_iefb_get_slave_application_status (&cls),
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);

   /* Cyclic data from master */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);

   /* Set slave application status to stopped */
   cls_iefb_set_slave_application_status (
      &cls,
      CL_SLAVE_APPL_OPERATION_STATUS_STOPPED);
   ASSERT_EQ (cls.slave_application_status, CL_SLAVE_APPL_OPERATION_STATUS_STOPPED);
   ASSERT_EQ (
      cls_iefb_get_slave_application_status (&cls),
      CL_SLAVE_APPL_OPERATION_STATUS_STOPPED);

   /* Cyclic data from master */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 4 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_STOPPED);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);

   /* Set slave application status to running */
   cls_iefb_set_slave_application_status (
      &cls,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   ASSERT_EQ (
      cls.slave_application_status,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   ASSERT_EQ (
      cls_iefb_get_slave_application_status (&cls),
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);

   /* Cyclic data from master */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 5);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 5 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
}

/**
 * Updated local management info sent from slave
 *
 */
TEST_F (SlaveIntegrationTestConnected, CciefbChangedLocalManagementInfo)
{
   const uint32_t new_local_management = 0x23242526;
   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   ASSERT_EQ (
      cls_iefb_get_local_management_info (&cls),
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);

   /* Set local management info */
   cls_iefb_set_local_management_info (&cls, new_local_management);
   EXPECT_EQ (cls_iefb_get_local_management_info (&cls), new_local_management);

   /* Cyclic data from master */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (cyclic_response_result.local_management_info, new_local_management);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
}

TEST_F (SlaveIntegrationTestConnected, CciefbSearchParametersInvalidState)
{
   cls_cciefb_cyclic_request_info_t request = {};

   /* Prepare request struct from buffer */
   ASSERT_EQ (
      cl_iefb_parse_cyclic_request (
         (uint8_t *)&request_payload_initial,
         SIZE_REQUEST_3_SLAVES,
         remote_ip,
         123, /* remote_port */
         my_ip,
         &request),
      0);
   request.slave_ip_addr = CL_IPADDR_INVALID;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (cls_iefb_search_slave_parameters (&cls, now, &request), -1);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);

   cls.state = CLS_SLAVE_STATE_SLAVE_DOWN;
   EXPECT_EQ (cls_iefb_search_slave_parameters (&cls, now, &request), 0);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DOWN);

   cls.state = CLS_SLAVE_STATE_SLAVE_DISABLED;
   EXPECT_EQ (cls_iefb_search_slave_parameters (&cls, now, &request), 0);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DISABLED);

   cls.state = CLS_SLAVE_STATE_LAST;
   EXPECT_EQ (cls_iefb_search_slave_parameters (&cls, now, &request), 0);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_LAST);

   cls.state = CLS_SLAVE_STATE_MASTER_CONTROL;
   EXPECT_EQ (cls_iefb_search_slave_parameters (&cls, now, &request), -1);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
}

TEST_F (SlaveIntegrationTestConnected, CciefbSearchParametersWrongMaster)
{
   cls_cciefb_cyclic_request_info_t request = {};

   /* Prepare request struct from buffer */
   ASSERT_EQ (
      cl_iefb_parse_cyclic_request (
         (uint8_t *)&request_payload_initial,
         SIZE_REQUEST_3_SLAVES,
         remote_ip,
         123, /* remote_port */
         my_ip,
         &request),
      0);
   cls.master.master_id = 0x01020355;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (cls_iefb_search_slave_parameters (&cls, now, &request), 0);
   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
}

TEST_F (SlaveIntegrationTestConnected, CciefbWrongStateForIncomingFrames)
{
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);

   /* Force to other state */
   cls.state = CLS_SLAVE_STATE_SLAVE_DOWN;
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DOWN);

   /* Cyclic data from master. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DOWN);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);

   /* Force to other state */
   cls.state = CLS_SLAVE_STATE_LAST;
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_LAST);

   /* Cyclic data from master. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_LAST);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 6);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
}

TEST_F (SlaveIntegrationTestConnected, CciefbChangedSlaveErrorCode)
{
   const uint16_t new_slave_error_code = 0x3839;
   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   ASSERT_EQ (
      cls_get_slave_error_code (&cls),
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);

   /* Set slave error code */
   cls_set_slave_error_code (&cls, new_slave_error_code);
   EXPECT_EQ (cls_get_slave_error_code (&cls), new_slave_error_code);

   /* Cyclic data from master */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (cyclic_response_result.slave_err_code, new_slave_error_code);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
}

TEST_F (SlaveIntegrationTestConnected, CciefbFailsToSend)
{
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);

   /* Cyclic data from master */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   mock_cciefb_port->will_fail_send = true;

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
}

TEST_F (SlaveIntegrationTestConnected, CciefbSendsWrongSize)
{
   const size_t wrong_size = 7;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);

   /* Cyclic data from master */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   /* The UDP port just accepts to send fewer bytes */
   mock_cciefb_port->use_modified_send_size_returnvalue = true;
   mock_cciefb_port->modified_send_size_returnvalue     = wrong_size;

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (
      mock_cciefb_port->total_sent_bytes,
      2 * SIZE_RESPONSE_2_SLAVES + wrong_size);
}

/**
 * Request to other group
 *
 * @req REQ_CLS_PARAMETERID_05
 * @req REQ_CLS_GROUPS_02
 *
 */
TEST_F (SlaveIntegrationTestConnected, CciefbRunningOtherGroup)
{
   uint8_t request_other_group[SIZE_REQUEST_3_SLAVES] = {};

   /* Prepare request intended for other group, by changing group number */
   clal_memcpy (
      request_other_group,
      sizeof (request_other_group),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_other_group[51] = 0x04;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   /* Cyclic data intended for other group. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_other_group,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
}

/**
 * Slave responds with 'Wrong number of occupied stations' alarm
 *
 * @req REQ_CLS_ERROR_01
 * @req REQ_CLS_ERROR_10
 *
 */
TEST_F (SlaveIntegrationTestConnected, CciefbRunningWrongNumberOccupied)
{
   uint8_t request_wrong_occupied[SIZE_REQUEST_3_SLAVES] = {};
   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   /* Prepare request with wrong number of occupied stations */
   clal_memcpy (
      request_wrong_occupied,
      sizeof (request_wrong_occupied),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_wrong_occupied[75] = 0x01; /* Destroy 255.255.255.255 */
   request_wrong_occupied[59] = 0x78; /* New parameter counter */

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   /* Wrong number of stations from master.
      Respond with error frame and disconnect. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_wrong_occupied,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 1);
   EXPECT_EQ (
      mock_data.slave_cb_error.error_message,
      CLS_ERROR_WRONG_NUMBER_OCCUPIED);
   EXPECT_EQ (mock_data.slave_cb_error.ip_addr, CL_IPADDR_INVALID);
   EXPECT_EQ (mock_data.slave_cb_error.argument_2, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (
      cyclic_response_result.end_code,
      CL_SLMP_ENDCODE_CCIEFB_WRONG_NUMBER_OCCUPIED_STATIONS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
   /* TODO verify that data in response is 0. REQ_CLS_ERROR_02 */
}

TEST_F (SlaveIntegrationTestConnected, CciefbWrongTotalNumberOccupied)
{
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   /* Our slave station number is larger than the number of slaves in the
      incoming request */
   cls.master.slave_station_no = 5;

   /* Cyclic data. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
}

/**
 * Slave disconnects due to user action
 *
 * @req REQ_CLS_ERROR_05
 * @req REQ_CLS_ERROR_06
 * @req REQ_CLS_CONFORMANCE_05
 *
 */
TEST_F (SlaveIntegrationTestConnected, CciefbApplicationStopNoError)
{
   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   /* Application would like to stop (no error)  */
   cls_iefb_disable_slave (&cls, now, false);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);

   /* Cyclic data from master, respond with error frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, 0);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (
      cyclic_response_result.end_code,
      CL_SLMP_ENDCODE_CCIEFB_SLAVE_REQUESTS_DISCONNECT);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
   /* TODO verify that data in response is 0. REQ_CLS_ERROR_02 */

   /* Cyclic data from master, again. Respond with error frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 3);

   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);

   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 6);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 4 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 4 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   /* TODO verify that data in response is 0. REQ_CLS_ERROR_02 */

   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (
      cyclic_response_result.end_code,
      CL_SLMP_ENDCODE_CCIEFB_SLAVE_REQUESTS_DISCONNECT);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
   /* TODO verify that data in response is 0. REQ_CLS_ERROR_02 */

   /* Master tries to reconnect. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_initial,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 5 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 4 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);

   /* Wait until we reach state SLAVE_DISABLED */
   now += longer_than_timeout_us;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DISABLED);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 4);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_SLAVE_DISABLED);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 8);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 5 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 4 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);

   /* Cyclic data from master. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DISABLED);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 4);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 9);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 6 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 4 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);

   /* Master tries to reconnect. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_initial,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_SLAVE_DISABLED);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 4);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 10);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 7 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 4 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);

   /* Application re-enables slave  */
   cls_iefb_reenable_slave (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 5);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_NONE);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 1);

   /* Master tries to reconnect. Accept connection. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_initial,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 11);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 5);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 6);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 4);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, remote_protocol_ver);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 1);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 8 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 5 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
}

/**
 * Slave disconnects due to error
 *
 * @req REQ_CLS_ERROR_09
 *
 */
TEST_F (SlaveIntegrationTestConnected, CciefbApplicationStopError)
{
   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   /* Application would like to stop due to error  */
   cls_iefb_disable_slave (&cls, now, true);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);

   /* Cyclic data from master. Disconnects. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 1);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_CCIEFB_SLAVE_ERROR);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
   /* TODO verify that data in response is 0. REQ_CLS_ERROR_02 */
}

/**
 * Frame sequence restarts at zero
 *
 * @req REQ_CLS_PARAMETERID_04
 * @req REQ_CLS_STATUSBIT_04
 *
 */
TEST_F (SlaveIntegrationTestConnected, CciefbFrameSequenceRestartsAtZero)
{
   uint8_t request_sequence_zero[SIZE_REQUEST_3_SLAVES] = {};

   /* Prepare request with frame sequence number zero, but already running.
    */
   clal_memcpy (
      request_sequence_zero,
      sizeof (request_sequence_zero),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_sequence_zero[53] = 0x00; /* Request frame sequence no */
   request_sequence_zero[54] = 0x00;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   /* Request with frame sequence zero. Do a parameter search.
      Drop frame, as this indicates slave duplication (already running). */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_sequence_zero,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
}

/**
 * Slave is disabled (IP address 0.0.0.0 in request)
 *
 * @req REQ_CL_PROTOCOL_33
 * @req REQ_CLS_CONFORMANCE_04
 *
 */
TEST_F (SlaveIntegrationTestConnected, CciefbSlaveDisabled)
{
   uint8_t request_slave_disabled[SIZE_REQUEST_3_SLAVES] = {};

   /* Prepare request with slave IP address replaced by zeros */
   clal_memcpy (
      request_slave_disabled,
      sizeof (request_slave_disabled),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_slave_disabled[71] = 0x00;
   request_slave_disabled[72] = 0x00;
   request_slave_disabled[73] = 0x00;
   request_slave_disabled[74] = 0x00;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   /* Request with slave IP address zero. Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_slave_disabled,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
}

/**
 * New parameter number
 *
 * @req REQ_CLS_PARAMETERID_03
 *
 */
TEST_F (SlaveIntegrationTestConnected, CciefbNewParameterNumber)
{
   uint8_t request_new_parameter_number[SIZE_REQUEST_3_SLAVES] = {};
   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   /* Prepare request with with new parameter number */
   clal_memcpy (
      request_new_parameter_number,
      sizeof (request_new_parameter_number),
      request_payload_initial,
      SIZE_REQUEST_3_SLAVES);
   request_new_parameter_number[59] = 0x44;

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   /* Request with new parameter number. Do a parameter search.
      Respond with new settings */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_new_parameter_number,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
}

/**
 * Master state changes from stopped to running. Protocol version 2.
 *
 * @req REQ_CLS_CONFORMANCE_06
 * @req REQ_CLS_VERSION_04
 *
 */
TEST_F (SlaveIntegrationTestConnected, CciefbMasterStateChanged)
{
   uint8_t request_master_stopped[SIZE_REQUEST_3_SLAVES] = {};
   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   /* Prepare request with master station stopped */
   clal_memcpy (
      request_master_stopped,
      sizeof (request_master_stopped),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_master_stopped[35] = 0x00; /* Master station stopped */

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, remote_protocol_ver);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   /* Request with master = stopped. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_master_stopped,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, remote_protocol_ver);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);

   /* Request with master = running. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 4);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, remote_protocol_ver);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 6);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 4 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 4 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);

   // TODO (rtljobe): Request with master = stopped by user. REQ_CLS_VERSION_03
}

TEST_F (SlaveIntegrationTestConnected, CciefbMasterStoppedByUser)
{
   uint8_t request_master_stopped[SIZE_REQUEST_3_SLAVES] = {};
   cl_mock_cyclic_response_result_t cyclic_response_result;
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   /* Prepare request with master station stopped by user */
   clal_memcpy (
      request_master_stopped,
      sizeof (request_master_stopped),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_master_stopped[35] = 0x02; /* Master station stopped by user */

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, remote_protocol_ver);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   /* Request with master = stopped by user. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_master_stopped,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 3);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, remote_protocol_ver);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_STOPPED_BY_USER);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
}

TEST_F (SlaveIntegrationTestConnected, CciefbSlaveIdInvalid)
{
   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_to_master, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.connected_and_running, true);
   EXPECT_EQ (mock_data.slave_cb_master_running.stopped_by_user, false);
   EXPECT_EQ (mock_data.slave_cb_master_running.protocol_ver, remote_protocol_ver);
   EXPECT_EQ (
      mock_data.slave_cb_master_running.master_application_status,
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 2 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   /* Cyclic data. Force our IP address to be invalid. Drop frame
      (Later on we would time out as there are no received frames) */
   mock_set_udp_fakedata (
      mock_cciefb_port,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   mock_cciefb_port->ip_addr       = CL_IPADDR_INVALID;
   mock_cciefb_port->local_ip_addr = CL_IPADDR_INVALID;

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   /* Force our IP address back to the one used in tests */
   /* Cyclic data. We will respond. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 6);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 4 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);

   /* Force our IP address to be some other arbitrary IP address
      (Different from what is given in incoming frame)
      Drop frame. */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      0x0102030B,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 7);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 5 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
}

/**
 * Master turns off transmission bit for this slave.
 *
 * @req REQ_CLS_CYCLIC_01
 * @req REQ_CLS_CYCLIC_02
 * @req REQ_CLS_CYCLIC_03
 * @req REQ_CLS_STATUSBIT_01
 * @req REQ_CLS_STATUSBIT_02
 * @req REQ_CLS_STATUSBIT_03
 * @req REQ_CLS_STATUSBIT_05
 *
 * TODO (rtljobe): make sure all are implemented
 */
TEST_F (SlaveIntegrationTestConnected, CciefbMasterDisconnectsSlave)
{
   uint16_t registernumber_rwr   = 4;
   uint16_t registernumber_rww_A = 1;
   uint16_t registernumber_rww_B = 63;
   uint16_t registernumber_ry    = 123;
   uint16_t registernumber_rx    = 5;
   uint16_t value_rwr            = 0xBEEF;
   uint16_t value_rww_A          = 0x3300;
   uint16_t value_rww_B          = 0x6600;
   bool value_ry                 = true;
   bool value_rx                 = true;
   cl_rwr_t * resulting_rwr_area;
   cl_rx_t * resulting_rx_area;
   cl_mock_cyclic_response_result_t cyclic_response_result;
   uint8_t request_master_stops_slave[SIZE_REQUEST_3_SLAVES] = {};
   clal_clear_memory (&cyclic_response_result, sizeof (cyclic_response_result));

   clal_memcpy (
      request_master_stops_slave,
      sizeof (request_master_stops_slave),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   request_master_stops_slave[63] = 0x01; /* This slave is disconnected */
   request_master_stops_slave[53] += 1;   /* Sequence number */

   ASSERT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_state.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 2 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 4);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 2);

   /* Set input data in slave */
   cls_set_rwr_value (&cls, registernumber_rwr, value_rwr);
   EXPECT_EQ (cls_get_rwr_value (&cls, registernumber_rwr), value_rwr);
   cls_set_rx_bit (&cls, registernumber_rx, value_rx);
   EXPECT_EQ (cls_get_rx_bit (&cls, registernumber_rx), value_rx);

   /* Cyclic data from master, slave responds immediately */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_payload_running,
      SIZE_REQUEST_3_SLAVES);

   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 5);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 3);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 3 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 3 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no);
   /* Verify that data is set in response */
   resulting_rwr_area = mock_analyze_cyclic_response_rwr (mock_cciefb_port, 0);
   EXPECT_NE (resulting_rwr_area, nullptr);
   EXPECT_EQ (
      CC_FROM_LE16 (resulting_rwr_area->words[registernumber_rwr]),
      value_rwr);
   resulting_rx_area = mock_analyze_cyclic_response_rx (mock_cciefb_port, 2, 0);
   EXPECT_NE (resulting_rx_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rx_area->bytes[0]), 0x20); /* Bit 5 */

   /* Read out values from master */
   EXPECT_EQ (cls_get_rww_value (&cls, registernumber_rww_A), value_rww_A);
   EXPECT_EQ (cls_get_rww_value (&cls, registernumber_rww_B), value_rww_B);
   EXPECT_EQ (cls_get_ry_bit (&cls, registernumber_ry), value_ry);

   /* Master sends request with the slave disconnected,
    * slave responds immediately */
   mock_set_udp_fakedata_with_local_ipaddr (
      mock_cciefb_port,
      my_ip,
      my_ifindex,
      remote_ip,
      CL_CCIEFB_PORT,
      (uint8_t *)&request_master_stops_slave,
      SIZE_REQUEST_3_SLAVES);
   now += tick_size;
   cls_iefb_periodic (&cls, now);

   EXPECT_EQ (cls.state, CLS_SLAVE_STATE_MASTER_CONTROL);
   EXPECT_EQ (mock_data.slave_cb_state.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_error.calls, 0);
   EXPECT_EQ (mock_data.slave_cb_master_running.calls, 2);
   EXPECT_EQ (mock_data.slave_cb_connect.calls, 1);
   EXPECT_EQ (mock_data.slave_cb_disconnect.calls, 0);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_open, 1);
   EXPECT_EQ (mock_cciefb_port->is_open, true);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_recv, 6);
   EXPECT_EQ (mock_cciefb_port->number_of_calls_send, 4);
   EXPECT_EQ (mock_cciefb_port->total_recv_bytes, 4 * SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (mock_cciefb_port->total_sent_bytes, 4 * SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->output_data_size, SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (mock_cciefb_port->remote_destination_ip, remote_ip);
   EXPECT_EQ (mock_cciefb_port->remote_destination_port, CL_CCIEFB_PORT);
   ASSERT_EQ (
      mock_analyze_fake_cyclic_response (mock_cciefb_port, &cyclic_response_result),
      0);
   EXPECT_EQ (
      cyclic_response_result.dl,
      SIZE_RESPONSE_2_SLAVES - CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   EXPECT_EQ (cyclic_response_result.end_code, CL_SLMP_ENDCODE_SUCCESS);
   EXPECT_EQ (cyclic_response_result.vendor_code, config.vendor_code);
   EXPECT_EQ (cyclic_response_result.model_code, config.model_code);
   EXPECT_EQ (cyclic_response_result.equiment_version, config.equipment_ver);
   EXPECT_EQ (cyclic_response_result.slave_protocol_ver, my_protocol_ver);
   EXPECT_EQ (
      cyclic_response_result.slave_local_unit_info,
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cyclic_response_result.slave_err_code,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   EXPECT_EQ (
      cyclic_response_result.local_management_info,
      CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);
   EXPECT_EQ (cyclic_response_result.group_no, my_group_no);
   EXPECT_EQ (cyclic_response_result.frame_sequence_no, frame_sequence_no + 1);
   /* Verify that data in response is zero */
   resulting_rwr_area = mock_analyze_cyclic_response_rwr (mock_cciefb_port, 0);
   EXPECT_NE (resulting_rwr_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rwr_area->words[registernumber_rwr]), 0U);
   resulting_rx_area = mock_analyze_cyclic_response_rx (mock_cciefb_port, 2, 0);
   EXPECT_NE (resulting_rx_area, nullptr);
   EXPECT_EQ (CC_FROM_LE16 (resulting_rx_area->bytes[0]), 0);

   /* Read out values from master */
   EXPECT_EQ (cls_get_rww_value (&cls, registernumber_rww_A), 0U);
   EXPECT_EQ (cls_get_rww_value (&cls, registernumber_rww_B), 0U);
   EXPECT_EQ (cls_get_ry_bit (&cls, registernumber_ry), false);
}
