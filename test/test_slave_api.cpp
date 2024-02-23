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
class SlaveApiUnitTest : public UnitTest
{
 protected:
   void SetUp() override

   {
      UnitTest::SetUp();

      ASSERT_GE (CLS_MAX_OCCUPIED_STATIONS, 3);
   }
};

// Tests

TEST_F (SlaveApiUnitTest, GetClsVersion)
{
   EXPECT_GE (strlen (cl_version()), 3U);
}

TEST_F (SlaveApiUnitTest, ClsInit)
{
   cls_cfg_t config                    = {};
   cls_t * cls                         = nullptr;
   uint64_t master_timestamp           = 0;
   uint16_t rx_number                  = 0;
   uint16_t ry_number                  = 0;
   uint16_t rwr_number                 = 0;
   uint16_t rww_number                 = 0;
   const uint16_t new_rwr_value        = 0x1234;
   const uint16_t new_slave_error_code = 0x9192;
   const uint32_t new_management_info  = 0x06070809U;

   config.num_occupied_stations = 3;

   cls = cls_init (&config);
   ASSERT_TRUE (cls != nullptr);

   cls_handle_periodic (cls);

   /* Not yet any connection from master */
   EXPECT_EQ (cls_get_master_timestamp (cls, &master_timestamp), -1);

   /* Set and read back local management info */
   EXPECT_EQ (cls_get_local_management_info (cls), 0U);
   cls_set_local_management_info (cls, new_management_info);
   EXPECT_EQ (cls_get_local_management_info (cls), new_management_info);

   /* Set and read back slave application status */
   EXPECT_EQ (
      cls_get_slave_application_status (cls),
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   cls_set_slave_application_status (cls, CL_SLAVE_APPL_OPERATION_STATUS_STOPPED);
   EXPECT_EQ (
      cls_get_slave_application_status (cls),
      CL_SLAVE_APPL_OPERATION_STATUS_STOPPED);
   cls_set_slave_application_status (cls, CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      cls_get_slave_application_status (cls),
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);

   /* Set and read back slave error code */
   EXPECT_EQ (cls_get_slave_error_code (cls), 0U);
   cls_set_slave_error_code (cls, new_slave_error_code);
   EXPECT_EQ (cls_get_slave_error_code (cls), new_slave_error_code);

   /* Set and read back RX bits */
   rx_number = 0;
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), false);
   cls_set_rx_bit (cls, rx_number, true);
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), true);
   cls_set_rx_bit (cls, rx_number, false);
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), false);

   rx_number = 63;
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), false);
   cls_set_rx_bit (cls, rx_number, true);
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), true);
   cls_set_rx_bit (cls, rx_number, false);
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), false);

   rx_number = 64;
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), false);
   cls_set_rx_bit (cls, rx_number, true);
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), true);
   cls_set_rx_bit (cls, rx_number, false);
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), false);

   rx_number = 127;
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), false);
   cls_set_rx_bit (cls, rx_number, true);
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), true);
   cls_set_rx_bit (cls, rx_number, false);
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), false);

   rx_number = 128;
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), false);
   cls_set_rx_bit (cls, rx_number, true);
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), true);
   cls_set_rx_bit (cls, rx_number, false);
   EXPECT_EQ (cls_get_rx_bit (cls, rx_number), false);

   /* Set and read back RWr word */
   rwr_number = 0;
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), 0);
   cls_set_rwr_value (cls, rwr_number, new_rwr_value);
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), new_rwr_value);
   cls_set_rwr_value (cls, rwr_number, 0);
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), 0);

   rwr_number = 31;
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), 0);
   cls_set_rwr_value (cls, rwr_number, new_rwr_value);
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), new_rwr_value);
   cls_set_rwr_value (cls, rwr_number, 0);
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), 0);

   rwr_number = 32;
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), 0);
   cls_set_rwr_value (cls, rwr_number, new_rwr_value);
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), new_rwr_value);
   cls_set_rwr_value (cls, rwr_number, 0);
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), 0);

   rwr_number = 63;
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), 0);
   cls_set_rwr_value (cls, rwr_number, new_rwr_value);
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), new_rwr_value);
   cls_set_rwr_value (cls, rwr_number, 0);
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), 0);

   rwr_number = 64;
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), 0);
   cls_set_rwr_value (cls, rwr_number, new_rwr_value);
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), new_rwr_value);
   cls_set_rwr_value (cls, rwr_number, 0);
   EXPECT_EQ (cls_get_rwr_value (cls, rwr_number), 0);

   /* Read RY bits */
   ry_number = 0;
   EXPECT_EQ (cls_get_ry_bit (cls, ry_number), false);

   /* Read RWw registers */
   rww_number = 0;
   EXPECT_EQ (cls_get_rww_value (cls, rww_number), 0x0000);

   /* Find memory areas */
   // TODO (rtljobe): Implement more detailed tests
   EXPECT_TRUE (cls_get_first_rx_area (cls) != nullptr);
   EXPECT_TRUE (cls_get_first_ry_area (cls) != nullptr);
   EXPECT_TRUE (cls_get_first_rwr_area (cls) != nullptr);
   EXPECT_TRUE (cls_get_first_rww_area (cls) != nullptr);

   /* Stop communication, no error */
   cls_stop_cyclic_data (cls, false);

   /* Stop communication due to error */
   cls_stop_cyclic_data (cls, true);

   /* Restart communication */
   cls_restart_cyclic_data (cls);

   /* Close down the stack */
   EXPECT_EQ (cls_exit (cls), 0);
   free (cls);
}

TEST_F (SlaveApiUnitTest, ClsInitFails)
{
   cls_cfg_t config               = {};
   cls_t * cls                    = nullptr;
   config.num_occupied_stations   = 3;
   const cls_cfg_t default_config = config;

   cls = cls_init (&config);
   ASSERT_TRUE (cls != nullptr);
   free (cls);

   /* Invalid configuration */
   mock_clear();
   config.num_occupied_stations = 0;
   cls                          = cls_init (&config);
   ASSERT_TRUE (cls == nullptr);

   mock_clear();
   config = default_config;
   cls    = cls_init (&config);
   ASSERT_TRUE (cls != nullptr);
   free (cls);

   /* calloc fails */
   mock_clear();
   mock_data.will_fail_calloc = true;
   cls                        = cls_init (&config);
   ASSERT_TRUE (cls == nullptr);

   mock_clear();
   cls = cls_init (&config);
   ASSERT_TRUE (cls != nullptr);
   free (cls);
}

TEST_F (SlaveApiUnitTest, ClsIsNull)
{
   const cls_cfg_t config = {};
   uint64_t timestamp     = 0;

   ASSERT_EQ (cls_init_only (nullptr, &config), -1);
   ASSERT_EQ (cls_exit (nullptr), -1);

   ASSERT_TRUE (cls_get_first_ry_area (nullptr) == nullptr);
   ASSERT_TRUE (cls_get_first_rx_area (nullptr) == nullptr);
   ASSERT_TRUE (cls_get_first_rww_area (nullptr) == nullptr);
   ASSERT_TRUE (cls_get_first_rwr_area (nullptr) == nullptr);

   ASSERT_TRUE (cls_get_master_connection_details (nullptr) == nullptr);

   ASSERT_EQ (cls_get_master_timestamp (nullptr, &timestamp), -1);
}
