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
#include "common/cl_literals.h"

#include "mocks.h"
#include "utils_for_testing.h"

#include <gtest/gtest.h>

// Test fixture

class LiteralsUnitTest : public UnitTest
{
};

// Tests

TEST_F (LiteralsUnitTest, LiteralsGetMasterState)
{
   // clang-format off
   EXPECT_STREQ (cl_literals_get_master_state (CLM_MASTER_STATE_DOWN),        "STATE_DOWN");
   EXPECT_STREQ (cl_literals_get_master_state (CLM_MASTER_STATE_STANDBY),     "STATE_STANDBY");
   EXPECT_STREQ (cl_literals_get_master_state (CLM_MASTER_STATE_ARBITRATION), "STATE_ARBITRATION");
   EXPECT_STREQ (cl_literals_get_master_state (CLM_MASTER_STATE_RUNNING),     "STATE_RUNNING");
   EXPECT_STREQ (cl_literals_get_master_state ((clm_master_state_t)123),      "unknown state");
   // clang-format on
}

TEST_F (LiteralsUnitTest, LiteralsGetGroupEvent)
{
   // clang-format off
   EXPECT_STREQ (cl_literals_get_group_event (CLM_GROUP_EVENT_NONE),             "EVENT_NONE");
   EXPECT_STREQ (cl_literals_get_group_event (CLM_GROUP_EVENT_STARTUP),          "EVENT_STARTUP");
   EXPECT_STREQ (cl_literals_get_group_event (CLM_GROUP_EVENT_PARAMETER_CHANGE), "EVENT_PARAMETER_CHANGE");
   EXPECT_STREQ (cl_literals_get_group_event (CLM_GROUP_EVENT_NEW_CONFIG),       "EVENT_NEW_CONFIG");
   EXPECT_STREQ (cl_literals_get_group_event (CLM_GROUP_EVENT_ARBITRATION_DONE), "EVENT_ARBITRATION_DONE");
   EXPECT_STREQ (cl_literals_get_group_event (CLM_GROUP_EVENT_REQ_FROM_OTHER),   "EVENT_REQ_FROM_OTHER");
   EXPECT_STREQ (cl_literals_get_group_event (CLM_GROUP_EVENT_MASTERDUPL_ALARM), "EVENT_MASTERDUPL_ALARM");
   EXPECT_STREQ (cl_literals_get_group_event (CLM_GROUP_EVENT_LINKSCAN_START),   "EVENT_LINKSCAN_START");
   EXPECT_STREQ (cl_literals_get_group_event (CLM_GROUP_EVENT_LINKSCAN_TIMEOUT), "EVENT_LINKSCAN_TIMEOUT");
   EXPECT_STREQ (cl_literals_get_group_event (CLM_GROUP_EVENT_LINKSCAN_COMPLETE),"EVENT_LINKSCAN_COMPLETE");
   EXPECT_STREQ (cl_literals_get_group_event (CLM_GROUP_EVENT_LAST),             "EVENT_LAST (dummy state)");
   EXPECT_STREQ (cl_literals_get_group_event ((clm_group_event_t)123),           "unknown event");
   // clang-format on
}

TEST_F (LiteralsUnitTest, LiteralsGetGroupState)
{
   // clang-format off
   EXPECT_STREQ (cl_literals_get_group_state (CLM_GROUP_STATE_MASTER_DOWN),            "STATE_MASTER_DOWN");
   EXPECT_STREQ (cl_literals_get_group_state (CLM_GROUP_STATE_MASTER_LISTEN),          "STATE_MASTER_LISTEN");
   EXPECT_STREQ (cl_literals_get_group_state (CLM_GROUP_STATE_MASTER_ARBITRATION),     "STATE_MASTER_ARBITRATION");
   EXPECT_STREQ (cl_literals_get_group_state (CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP),  "STATE_MASTER_LINK_SCAN_COMP");
   EXPECT_STREQ (cl_literals_get_group_state (CLM_GROUP_STATE_MASTER_LINK_SCAN),       "STATE_MASTER_LINK_SCAN");
   EXPECT_STREQ (cl_literals_get_group_state (CLM_GROUP_STATE_LAST),                   "STATE_LAST (dummy state)");
   EXPECT_STREQ (cl_literals_get_group_state ((clm_group_state_t)123),                 "unknown state");
   // clang-format on
}

TEST_F (LiteralsUnitTest, LiteralsGetDeviceState)
{
   // clang-format off
   EXPECT_STREQ (cl_literals_get_device_state (CLM_DEVICE_STATE_MASTER_DOWN),    "STATE_MASTER_DOWN");
   EXPECT_STREQ (cl_literals_get_device_state (CLM_DEVICE_STATE_LISTEN),         "STATE_LISTEN");
   EXPECT_STREQ (cl_literals_get_device_state (CLM_DEVICE_STATE_WAIT_TD),        "STATE_WAIT_TD");
   EXPECT_STREQ (cl_literals_get_device_state (CLM_DEVICE_STATE_CYCLIC_SUSPEND), "STATE_CYCLIC_SUSPEND");
   EXPECT_STREQ (cl_literals_get_device_state (CLM_DEVICE_STATE_CYCLIC_SENT),    "STATE_CYCLIC_SENT");
   EXPECT_STREQ (cl_literals_get_device_state (CLM_DEVICE_STATE_CYCLIC_SENDING), "STATE_CYCLIC_SENDING");
   EXPECT_STREQ (cl_literals_get_device_state (CLM_DEVICE_STATE_LAST),           "STATE_LAST (dummy state)");
   EXPECT_STREQ (cl_literals_get_device_state ((clm_device_state_t)123),         "unknown state");
   // clang-format on
}

TEST_F (LiteralsUnitTest, LiteralsGetDeviceEvent)
{
   // clang-format off
   EXPECT_STREQ (cl_literals_get_device_event (CLM_DEVICE_EVENT_NONE),                    "EVENT_NONE (dummy state)");
   EXPECT_STREQ (cl_literals_get_device_event (CLM_DEVICE_EVENT_GROUP_STARTUP),           "EVENT_GROUP_STARTUP");
   EXPECT_STREQ (cl_literals_get_device_event (CLM_DEVICE_EVENT_GROUP_STANDBY),           "EVENT_GROUP_STANDBY");
   EXPECT_STREQ (cl_literals_get_device_event (CLM_DEVICE_EVENT_GROUP_TIMEOUT),           "EVENT_GROUP_TIMEOUT");
   EXPECT_STREQ (cl_literals_get_device_event (CLM_DEVICE_EVENT_GROUP_ALL_RESPONDED),     "EVENT_GROUP_ALL_RESPONDED");
   EXPECT_STREQ (cl_literals_get_device_event (CLM_DEVICE_EVENT_RECEIVE_OK),              "EVENT_RECEIVE_OK");
   EXPECT_STREQ (cl_literals_get_device_event (CLM_DEVICE_EVENT_RECEIVE_ERROR),           "EVENT_RECEIVE_ERROR");
   EXPECT_STREQ (cl_literals_get_device_event (CLM_DEVICE_EVENT_SCAN_START_DEVICE_START), "EVENT_SCAN_START_DEVICE_START");
   EXPECT_STREQ (cl_literals_get_device_event (CLM_DEVICE_EVENT_SCAN_START_DEVICE_STOP),  "EVENT_SCAN_START_DEVICE_STOP");
   EXPECT_STREQ (cl_literals_get_device_event (CLM_DEVICE_EVENT_TIMEOUTCOUNTER_FULL),     "EVENT_TIMEOUTCOUNTER_FULL");
   EXPECT_STREQ (cl_literals_get_device_event (CLM_DEVICE_EVENT_TIMEOUTCOUNTER_NOT_FULL), "EVENT_TIMEOUTCOUNTER_NOT_FULL");
   EXPECT_STREQ (cl_literals_get_device_event (CLM_DEVICE_EVENT_SLAVE_DUPLICATION),       "EVENT_SLAVE_DUPLICATION");
   EXPECT_STREQ (cl_literals_get_device_event (CLM_DEVICE_EVENT_LAST),                    "EVENT_LAST (dummy state)");
   EXPECT_STREQ (cl_literals_get_device_event ((clm_device_event_t)123),                  "unknown event");
   // clang-format on
}

TEST_F (LiteralsUnitTest, LiteralsGetMasterErrorMessage)
{
   // clang-format off
   EXPECT_STREQ (cl_literals_get_master_error_message (CLM_ERROR_ARBITRATION_FAILED),                  "ARBITRATION_FAILED");
   EXPECT_STREQ (cl_literals_get_master_error_message (CLM_ERROR_SLAVE_DUPLICATION),                   "SLAVE_DUPLICATION");
   EXPECT_STREQ (cl_literals_get_master_error_message (CLM_ERROR_SLAVE_REPORTS_WRONG_NUMBER_OCCUPIED), "SLAVE_REPORTS_WRONG_NUMBER_OCCUPIED");
   EXPECT_STREQ (cl_literals_get_master_error_message (CLM_ERROR_SLAVE_REPORTS_MASTER_DUPLICATION),    "SLAVE_REPORTS_MASTER_DUPLICATION");
   EXPECT_STREQ (cl_literals_get_master_error_message ((clm_error_message_t)123),                      "unknown error");
   // clang-format on
}

TEST_F (LiteralsUnitTest, LiteralsGetMasterSetIpResult)
{
   // clang-format off
   EXPECT_STREQ (cl_literals_get_master_set_ip_result (CLM_MASTER_SET_IP_STATUS_SUCCESS), "STATUS_SUCCESS");
   EXPECT_STREQ (cl_literals_get_master_set_ip_result (CLM_MASTER_SET_IP_STATUS_ERROR),   "STATUS_ERROR");
   EXPECT_STREQ (cl_literals_get_master_set_ip_result (CLM_MASTER_SET_IP_STATUS_TIMEOUT), "STATUS_TIMEOUT");
   EXPECT_STREQ (cl_literals_get_master_set_ip_result ((clm_master_setip_status_t)123),   "unknown result");
   // clang-format on
}

TEST_F (LiteralsUnitTest, LiteralsGetSlaveErrorMessage)
{
   // clang-format off
   EXPECT_STREQ (cl_literals_get_slave_error_message (CLS_ERROR_SLAVE_STATION_DUPLICATION),   "SLAVE_STATION_DUPLICATION");
   EXPECT_STREQ (cl_literals_get_slave_error_message (CLS_ERROR_MASTER_STATION_DUPLICATION),  "MASTER_STATION_DUPLICATION");
   EXPECT_STREQ (cl_literals_get_slave_error_message (CLS_ERROR_WRONG_NUMBER_OCCUPIED),       "WRONG_NUMBER_OCCUPIED");
   EXPECT_STREQ (cl_literals_get_slave_error_message ((cls_error_message_t)123),              "unknown error");
   // clang-format on
}

TEST_F (LiteralsUnitTest, LiteralsGetSlaveState)
{
   // clang-format off
   EXPECT_STREQ (cl_literals_get_slave_state (CLS_SLAVE_STATE_SLAVE_DOWN),             "STATE_SLAVE_DOWN");
   EXPECT_STREQ (cl_literals_get_slave_state (CLS_SLAVE_STATE_MASTER_NONE),            "STATE_MASTER_NONE");
   EXPECT_STREQ (cl_literals_get_slave_state (CLS_SLAVE_STATE_MASTER_CONTROL),         "STATE_MASTER_CONTROL");
   EXPECT_STREQ (cl_literals_get_slave_state (CLS_SLAVE_STATE_SLAVE_DISABLED),         "STATE_SLAVE_DISABLED");
   EXPECT_STREQ (cl_literals_get_slave_state (CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE),   "STATE_WAIT_DISABLING_SLAVE");
   EXPECT_STREQ (cl_literals_get_slave_state (CLS_SLAVE_STATE_LAST),                   "STATE_LAST (dummy state)");
   EXPECT_STREQ (cl_literals_get_slave_state ((cls_slave_state_t)123),                 "unknown state");
   // clang-format on
}

TEST_F (LiteralsUnitTest, LiteralsGetSlaveEvent)
{
   // clang-format off
   EXPECT_STREQ (cl_literals_get_slave_event (CLS_SLAVE_EVENT_NONE),                            "EVENT_NONE (dummy state)");
   EXPECT_STREQ (cl_literals_get_slave_event (CLS_SLAVE_EVENT_STARTUP),                         "EVENT_STARTUP");
   EXPECT_STREQ (cl_literals_get_slave_event (CLS_SLAVE_EVENT_CYCLIC_NEW_MASTER),               "EVENT_CYCLIC_NEW_MASTER");
   EXPECT_STREQ (cl_literals_get_slave_event (CLS_SLAVE_EVENT_CYCLIC_CORRECT_MASTER),           "EVENT_CYCLIC_CORRECT_MASTER");
   EXPECT_STREQ (cl_literals_get_slave_event (CLS_SLAVE_EVENT_CYCLIC_WRONG_MASTER),             "EVENT_CYCLIC_WRONG_MASTER");
   EXPECT_STREQ (cl_literals_get_slave_event (CLS_SLAVE_EVENT_CYCLIC_WRONG_STATIONCOUNT),       "EVENT_CYCLIC_WRONG_STATIONCOUNT");
   EXPECT_STREQ (cl_literals_get_slave_event (CLS_SLAVE_EVENT_CYCLIC_INCOMING_WHEN_DISABLED),   "EVENT_CYCLIC_INCOMING_WHEN_DISABLED");
   EXPECT_STREQ (cl_literals_get_slave_event (CLS_SLAVE_EVENT_TIMEOUT_MASTER),                  "EVENT_TIMEOUT_MASTER");
   EXPECT_STREQ (cl_literals_get_slave_event (CLS_SLAVE_EVENT_REENABLE_SLAVE),                  "EVENT_REENABLE_SLAVE");
   EXPECT_STREQ (cl_literals_get_slave_event (CLS_SLAVE_EVENT_DISABLE_SLAVE),                   "EVENT_DISABLE_SLAVE");
   EXPECT_STREQ (cl_literals_get_slave_event (CLS_SLAVE_EVENT_DISABLE_SLAVE_WAIT_ENDED),        "EVENT_DISABLE_SLAVE_WAIT_ENDED");
   EXPECT_STREQ (cl_literals_get_slave_event (CLS_SLAVE_EVENT_IP_UPDATED),                      "EVENT_IP_UPDATED");
   EXPECT_STREQ (cl_literals_get_slave_event (CLS_SLAVE_EVENT_LAST),                            "EVENT_LAST (dummy state)");
   EXPECT_STREQ (cl_literals_get_slave_event ((cls_slave_event_t)123),                          "unknown event");
   // clang-format on
}
