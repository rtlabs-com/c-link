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

#include "common/cl_util.h"

#include "cl_options.h"

#include "mocks.h"
#include "utils_for_testing.h"

#include <gtest/gtest.h>

/******************* Test fixture *************************************/

class UtilUnitTest : public UnitTest
{
};

/************************ Tests ******************************************/

TEST_F (UtilUnitTest, UtilIpToString)
{
   const cl_ipaddr_t remote_ip          = 0x01020304; /* IP 1.2.3.4 */
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0};        /** Terminated string */

   cl_util_ip_to_string (remote_ip, ip_string);
   EXPECT_EQ (strcmp (ip_string, "1.2.3.4"), 0);
}

TEST_F (UtilUnitTest, UtilCalculateBroadcastAddress)
{
   // clang-format off
   EXPECT_EQ (cl_util_calc_broadcast_address (0xFFFFFFFF, 0x01020304), 0x01020304U);
   EXPECT_EQ (cl_util_calc_broadcast_address (0xFFFFFF00, 0x01020304), 0x010203FFU);
   EXPECT_EQ (cl_util_calc_broadcast_address (0xFFFF0000, 0x01020304), 0x0102FFFFU);
   EXPECT_EQ (cl_util_calc_broadcast_address (0xFF000000, 0x01020304), 0x01FFFFFFU);
   EXPECT_EQ (cl_util_calc_broadcast_address (0x00000000, 0x01020304), 0xFFFFFFFFU);
   EXPECT_EQ (cl_util_calc_broadcast_address (0xFFFFFFFF, 0x00000000), 0x00000000U);
   EXPECT_EQ (cl_util_calc_broadcast_address (0xFFFFFF00, 0x00000000), 0x00000000U);
   EXPECT_EQ (cl_util_calc_broadcast_address (0xFFFF0000, 0x00000000), 0x00000000U);
   EXPECT_EQ (cl_util_calc_broadcast_address (0xFF000000, 0x00000000), 0x00000000U);
   EXPECT_EQ (cl_util_calc_broadcast_address (0x00000000, 0x00000000), 0x00000000U);
   // clang-format on
}

TEST_F (UtilUnitTest, UtilIsNetmaskValid)
{
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFFFFF));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFFFFE));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFFFFC));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFFFF8));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFFFF0));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFFFE0));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFFFC0));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFFF80));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFFF00));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFFE00));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFFC00));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFF800));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFF000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFE000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFFC000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFF8000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFF0000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFE0000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFFC0000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFF80000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFF00000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFE00000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFFC00000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFF800000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFF000000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFE000000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xFC000000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xF8000000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xF0000000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xE0000000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0xC0000000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0x80000000));
   EXPECT_TRUE (cl_utils_is_netmask_valid (0x00000000));

   EXPECT_FALSE (cl_utils_is_netmask_valid (0xFFFFFF0F));
   EXPECT_FALSE (cl_utils_is_netmask_valid (0xFFFFF0FF));
   EXPECT_FALSE (cl_utils_is_netmask_valid (0xFFFF0FFF));
   EXPECT_FALSE (cl_utils_is_netmask_valid (0xFFF0FFFF));
   EXPECT_FALSE (cl_utils_is_netmask_valid (0xFF0FFFFF));
   EXPECT_FALSE (cl_utils_is_netmask_valid (0xF0FFFFFF));
   EXPECT_FALSE (cl_utils_is_netmask_valid (0x0FFFFFFF));
   EXPECT_FALSE (cl_utils_is_netmask_valid (0xFF00FFFF));
   EXPECT_FALSE (cl_utils_is_netmask_valid (0xFFFF00FF));
   EXPECT_FALSE (cl_utils_is_netmask_valid (0xFFFAFFFF));
}

TEST_F (UtilUnitTest, UtilIsIpaddrValid)
{
   EXPECT_FALSE (cl_utils_is_ipaddr_range_valid (0x00000000));
   EXPECT_TRUE (cl_utils_is_ipaddr_range_valid (0x00000001));
   EXPECT_TRUE (cl_utils_is_ipaddr_range_valid (0xDFFFFFFE));
   EXPECT_FALSE (cl_utils_is_ipaddr_range_valid (0xDFFFFFFF));
   EXPECT_FALSE (cl_utils_is_ipaddr_range_valid (0xFFFFFFFE));
   EXPECT_FALSE (cl_utils_is_ipaddr_range_valid (0xFFFFFFFE));
   EXPECT_FALSE (cl_utils_is_ipaddr_range_valid (0xFFFFFFFF));
}

TEST_F (UtilUnitTest, UtilCopyMacAddress)
{
   const cl_macaddr_t original = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25};
   cl_macaddr_t destination    = {0};
   EXPECT_EQ (destination[0], 0x00);
   EXPECT_EQ (destination[1], 0x00);
   EXPECT_EQ (destination[2], 0x00);
   EXPECT_EQ (destination[3], 0x00);
   EXPECT_EQ (destination[4], 0x00);
   EXPECT_EQ (destination[5], 0x00);

   cl_util_copy_mac (&destination, &original);
   EXPECT_EQ (memcmp (&destination, &original, sizeof (original)), 0);

   cl_util_copy_mac_reverse (&destination, &original);
   EXPECT_EQ (destination[0], original[5]);
   EXPECT_EQ (destination[1], original[4]);
   EXPECT_EQ (destination[2], original[3]);
   EXPECT_EQ (destination[3], original[2]);
   EXPECT_EQ (destination[4], original[1]);
   EXPECT_EQ (destination[5], original[0]);
}

TEST_F (UtilUnitTest, UtilVerifyMacAddressMatcher)
{
   const cl_macaddr_t mac_a = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};
   const cl_macaddr_t mac_b = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};
   const cl_macaddr_t mac_c = {0x11, 0x12, 0x23, 0x14, 0x15, 0x16};
   const cl_macaddr_t mac_d = {0x16, 0x15, 0x14, 0x13, 0x12, 0x11};
   const cl_macaddr_t mac_e = {0x11, 0x12, 0x13, 0x13, 0x12, 0x11};

   /* Two similar but distinct buffers */
   EXPECT_TRUE (MacAddressMatch (&mac_a, &mac_b));
   EXPECT_TRUE (MacAddressMatch (&mac_b, &mac_a));

   /* A MAC address should match itself */
   EXPECT_TRUE (MacAddressMatch (&mac_a, &mac_a));
   EXPECT_TRUE (MacAddressMatch (&mac_b, &mac_b));
   EXPECT_TRUE (MacAddressMatch (&mac_c, &mac_c));
   EXPECT_TRUE (MacAddressMatch (&mac_d, &mac_d));
   EXPECT_TRUE (MacAddressMatch (&mac_e, &mac_e));

   /* Different addresses */
   EXPECT_FALSE (MacAddressMatch (&mac_a, &mac_c));
   EXPECT_FALSE (MacAddressMatch (&mac_a, &mac_d));
   EXPECT_FALSE (MacAddressMatch (&mac_a, &mac_e));

   /* Two buffers with similar but reversed bytes */
   EXPECT_TRUE (ReversedMacMatch (&mac_a, &mac_d));
   EXPECT_TRUE (ReversedMacMatch (&mac_d, &mac_a));

   /* Different addresses */
   EXPECT_FALSE (ReversedMacMatch (&mac_c, &mac_d)); /* one byte diff */
   EXPECT_FALSE (ReversedMacMatch (&mac_a, &mac_c));
   EXPECT_FALSE (ReversedMacMatch (&mac_a, &mac_e));

   /* These addresses are not reversed versions of themselves */
   EXPECT_FALSE (ReversedMacMatch (&mac_a, &mac_a));
   EXPECT_FALSE (ReversedMacMatch (&mac_b, &mac_b));
   EXPECT_FALSE (ReversedMacMatch (&mac_c, &mac_c));
   EXPECT_FALSE (ReversedMacMatch (&mac_d, &mac_d));
   EXPECT_FALSE (ReversedMacMatch (&mac_a, &mac_b));

   /* This MAC address is identical to a reversed version of itself */
   EXPECT_TRUE (ReversedMacMatch (&mac_e, &mac_e));
}

TEST_F (UtilUnitTest, UtilShowBytes)
{
   const uint8_t buffer[] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
      0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A,
      0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,
   };

   /* Should not crash */
   cl_util_buffer_show (buffer, 0, 0);

   /* Show bytes */
   cl_util_buffer_show (buffer, sizeof (buffer), 0);

   /* Use indentation */
   cl_util_buffer_show (buffer, sizeof (buffer), 10);
}
