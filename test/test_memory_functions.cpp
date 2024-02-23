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

class ClalUnitTest : public UnitTest
{
};

/* Mandatory naming for death tests */
class ClalDeathTest : public ClalUnitTest
{
};

/************************ Tests ******************************************/

/**
 * Test platform-specific function for clearing copying
 *
 * @req REQ_CL_STDLIB_01
 */
TEST_F (ClalUnitTest, ClalClearMemory)
{
   uint8_t i;
   uint8_t buffer[10];
   for (i = 0; i < (uint8_t)NELEMENTS (buffer); i++)
   {
      buffer[i] = i + 1;
   }
   EXPECT_EQ (buffer[0], 0x01);
   EXPECT_EQ (buffer[1], 0x02);
   EXPECT_EQ (buffer[2], 0x03);
   EXPECT_EQ (buffer[3], 0x04);
   EXPECT_EQ (buffer[4], 0x05);
   EXPECT_EQ (buffer[5], 0x06);
   EXPECT_EQ (buffer[6], 0x07);
   EXPECT_EQ (buffer[7], 0x08);
   EXPECT_EQ (buffer[8], 0x09);
   EXPECT_EQ (buffer[9], 0x0A);

   clal_clear_memory (buffer, 3U);
   EXPECT_EQ (buffer[0], 0x00);
   EXPECT_EQ (buffer[1], 0x00);
   EXPECT_EQ (buffer[2], 0x00);
   EXPECT_EQ (buffer[3], 0x04);
   EXPECT_EQ (buffer[4], 0x05);
   EXPECT_EQ (buffer[5], 0x06);
   EXPECT_EQ (buffer[6], 0x07);
   EXPECT_EQ (buffer[7], 0x08);
   EXPECT_EQ (buffer[8], 0x09);
   EXPECT_EQ (buffer[9], 0x0A);

   clal_clear_memory (buffer + 5U, 0U);
   EXPECT_EQ (buffer[0], 0x00);
   EXPECT_EQ (buffer[1], 0x00);
   EXPECT_EQ (buffer[2], 0x00);
   EXPECT_EQ (buffer[3], 0x04);
   EXPECT_EQ (buffer[4], 0x05);
   EXPECT_EQ (buffer[5], 0x06);
   EXPECT_EQ (buffer[6], 0x07);
   EXPECT_EQ (buffer[7], 0x08);
   EXPECT_EQ (buffer[8], 0x09);
   EXPECT_EQ (buffer[9], 0x0A);

   clal_clear_memory (buffer + 5U, 2U);
   EXPECT_EQ (buffer[0], 0x00);
   EXPECT_EQ (buffer[1], 0x00);
   EXPECT_EQ (buffer[2], 0x00);
   EXPECT_EQ (buffer[3], 0x04);
   EXPECT_EQ (buffer[4], 0x05);
   EXPECT_EQ (buffer[5], 0x00);
   EXPECT_EQ (buffer[6], 0x00);
   EXPECT_EQ (buffer[7], 0x08);
   EXPECT_EQ (buffer[8], 0x09);
   EXPECT_EQ (buffer[9], 0x0A);
}

TEST_F (ClalDeathTest, ClalDeathClearMemoryNull)
{
#ifdef __rtk__
   GTEST_SKIP()
      << "Skipping as RT-Kernel compiler does not support death tests";
#else
   /* Should assert when pointer is NULL */
   EXPECT_DEATH ({ clal_clear_memory (nullptr, 1U); }, "");

#endif // __rtk__
}

/**
 * Test platform-specific function for memory copying
 *
 * @req REQ_CL_STDLIB_01
 */
TEST_F (ClalUnitTest, ClalCopyMemory)
{
   uint8_t i;
   uint8_t source[10];
   uint8_t dest[sizeof (source)] = {0};
   for (i = 0; i < (uint8_t)NELEMENTS (source); i++)
   {
      source[i] = i + 1;
   }
   EXPECT_EQ (source[0], 0x01);
   EXPECT_EQ (source[1], 0x02);
   EXPECT_EQ (source[2], 0x03);
   EXPECT_EQ (source[3], 0x04);
   EXPECT_EQ (source[4], 0x05);
   EXPECT_EQ (source[5], 0x06);
   EXPECT_EQ (source[6], 0x07);
   EXPECT_EQ (source[7], 0x08);
   EXPECT_EQ (source[8], 0x09);
   EXPECT_EQ (source[9], 0x0A);
   EXPECT_EQ (dest[0], 0x00);
   EXPECT_EQ (dest[1], 0x00);
   EXPECT_EQ (dest[2], 0x00);
   EXPECT_EQ (dest[3], 0x00);
   EXPECT_EQ (dest[4], 0x00);
   EXPECT_EQ (dest[5], 0x00);
   EXPECT_EQ (dest[6], 0x00);
   EXPECT_EQ (dest[7], 0x00);
   EXPECT_EQ (dest[8], 0x00);
   EXPECT_EQ (dest[9], 0x00);

   clal_memcpy (dest, sizeof (dest), source, 0);
   EXPECT_EQ (dest[0], 0x00);
   EXPECT_EQ (dest[1], 0x00);
   EXPECT_EQ (dest[2], 0x00);
   EXPECT_EQ (dest[3], 0x00);
   EXPECT_EQ (dest[4], 0x00);
   EXPECT_EQ (dest[5], 0x00);
   EXPECT_EQ (dest[6], 0x00);
   EXPECT_EQ (dest[7], 0x00);
   EXPECT_EQ (dest[8], 0x00);
   EXPECT_EQ (dest[9], 0x00);

   clal_memcpy (dest, sizeof (dest), source, 4);
   EXPECT_EQ (dest[0], 0x01);
   EXPECT_EQ (dest[1], 0x02);
   EXPECT_EQ (dest[2], 0x03);
   EXPECT_EQ (dest[3], 0x04);
   EXPECT_EQ (dest[4], 0x00);
   EXPECT_EQ (dest[5], 0x00);
   EXPECT_EQ (dest[6], 0x00);
   EXPECT_EQ (dest[7], 0x00);
   EXPECT_EQ (dest[8], 0x00);
   EXPECT_EQ (dest[9], 0x00);

   clal_memcpy (dest + 5, 2, source + 1, 2);
   EXPECT_EQ (dest[0], 0x01);
   EXPECT_EQ (dest[1], 0x02);
   EXPECT_EQ (dest[2], 0x03);
   EXPECT_EQ (dest[3], 0x04);
   EXPECT_EQ (dest[4], 0x00);
   EXPECT_EQ (dest[5], 0x02);
   EXPECT_EQ (dest[6], 0x03);
   EXPECT_EQ (dest[7], 0x00);
   EXPECT_EQ (dest[8], 0x00);
   EXPECT_EQ (dest[9], 0x00);

   clal_memcpy (dest, sizeof (dest), source, sizeof (source));
   EXPECT_EQ (dest[0], 0x01);
   EXPECT_EQ (dest[1], 0x02);
   EXPECT_EQ (dest[2], 0x03);
   EXPECT_EQ (dest[3], 0x04);
   EXPECT_EQ (dest[4], 0x05);
   EXPECT_EQ (dest[5], 0x06);
   EXPECT_EQ (dest[6], 0x07);
   EXPECT_EQ (dest[7], 0x08);
   EXPECT_EQ (dest[8], 0x09);
   EXPECT_EQ (dest[9], 0x0A);
}

TEST_F (ClalDeathTest, ClalDeathMemcpyTooManyBytes)
{
#ifdef __rtk__
   GTEST_SKIP()
      << "Skipping as RT-Kernel compiler does not support death tests";
#else
   uint8_t source[10];
   uint8_t dest[sizeof (source)];
   ASSERT_EQ (sizeof (source), sizeof (dest));

   /* Should assert when trying to copy too many bytes */
   EXPECT_DEATH (
      { clal_memcpy (dest, sizeof (dest), source, sizeof (dest) + 1U); },
      "");
#endif // __rtk__
}

TEST_F (ClalDeathTest, ClalDeathMemcpyNullSrc)
{
#ifdef __rtk__
   GTEST_SKIP()
      << "Skipping as RT-Kernel compiler does not support death tests";
#else
   uint8_t source[10];
   uint8_t dest[sizeof (source)];
   ASSERT_EQ (sizeof (source), sizeof (dest));

   /* Should assert when source is NULL */
   EXPECT_DEATH (
      { clal_memcpy (dest, sizeof (dest), nullptr, sizeof (source)); },
      "");
#endif // __rtk__
}

TEST_F (ClalDeathTest, ClalDeathMemcpyNullDest)
{
#ifdef __rtk__
   GTEST_SKIP()
      << "Skipping as RT-Kernel compiler does not support death tests";
#else
   uint8_t source[10];
   uint8_t dest[sizeof (source)];
   ASSERT_EQ (sizeof (source), sizeof (dest));

   /* Should assert when destination is NULL */
   EXPECT_DEATH (
      { clal_memcpy (nullptr, sizeof (dest), source, sizeof (source)); },
      "");
#endif // __rtk__
}

/**
 * Test platform-specific function for string copying
 *
 * @req REQ_CL_STDLIB_01
 */
TEST_F (ClalUnitTest, ClalCopyString)
{
   size_t i;
   char dest[10];
   for (i = 0; i < sizeof (dest); i++)
   {
      dest[i] = (char)(i + 1U);
   }
   EXPECT_EQ (dest[0], 0x01);
   EXPECT_EQ (dest[1], 0x02);
   EXPECT_EQ (dest[2], 0x03);
   EXPECT_EQ (dest[3], 0x04);
   EXPECT_EQ (dest[4], 0x05);
   EXPECT_EQ (dest[5], 0x06);
   EXPECT_EQ (dest[6], 0x07);
   EXPECT_EQ (dest[7], 0x08);
   EXPECT_EQ (dest[8], 0x09);
   EXPECT_EQ (dest[9], 0x0A);

   /* Buffer is too short, truncate. */
   EXPECT_EQ (clal_copy_string (dest, "ABC", 3U), -1);
   EXPECT_EQ (dest[0], 'A');
   EXPECT_EQ (dest[1], 'B');
   EXPECT_EQ (dest[2], '\0');
   EXPECT_EQ (dest[3], 0x04);
   EXPECT_EQ (dest[4], 0x05);
   EXPECT_EQ (dest[5], 0x06);
   EXPECT_EQ (dest[6], 0x07);
   EXPECT_EQ (dest[7], 0x08);
   EXPECT_EQ (dest[8], 0x09);
   EXPECT_EQ (dest[9], 0x0A);

   /* Empty string in buffer of size 1 */
   EXPECT_EQ (clal_copy_string (dest, "", 1U), 0);
   EXPECT_EQ (dest[0], '\0');
   EXPECT_EQ (dest[1], 'B');
   EXPECT_EQ (dest[2], '\0');
   EXPECT_EQ (dest[3], 0x04);
   EXPECT_EQ (dest[4], 0x05);
   EXPECT_EQ (dest[5], 0x06);
   EXPECT_EQ (dest[6], 0x07);
   EXPECT_EQ (dest[7], 0x08);
   EXPECT_EQ (dest[8], 0x09);
   EXPECT_EQ (dest[9], 0x0A);

   /* String fits buffer exactly */
   EXPECT_EQ (clal_copy_string (dest, "STUVX", 6U), 0);
   EXPECT_EQ (dest[0], 'S');
   EXPECT_EQ (dest[1], 'T');
   EXPECT_EQ (dest[2], 'U');
   EXPECT_EQ (dest[3], 'V');
   EXPECT_EQ (dest[4], 'X');
   EXPECT_EQ (dest[5], '\0');
   EXPECT_EQ (dest[6], 0x07);
   EXPECT_EQ (dest[7], 0x08);
   EXPECT_EQ (dest[8], 0x09);
   EXPECT_EQ (dest[9], 0x0A);

   EXPECT_EQ (clal_copy_string (dest + 7U, "Z", 2U), 0);
   EXPECT_EQ (dest[0], 'S');
   EXPECT_EQ (dest[1], 'T');
   EXPECT_EQ (dest[2], 'U');
   EXPECT_EQ (dest[3], 'V');
   EXPECT_EQ (dest[4], 'X');
   EXPECT_EQ (dest[5], '\0');
   EXPECT_EQ (dest[6], 0x07);
   EXPECT_EQ (dest[7], 'Z');
   EXPECT_EQ (dest[8], '\0');
   EXPECT_EQ (dest[9], 0x0A);

   /* Start with fresh buffer again */
   for (i = 0; i < sizeof (dest); i++)
   {
      dest[i] = (char)(i + 1U);
   }
   EXPECT_EQ (dest[0], 0x01);
   EXPECT_EQ (dest[1], 0x02);
   EXPECT_EQ (dest[2], 0x03);
   EXPECT_EQ (dest[3], 0x04);
   EXPECT_EQ (dest[4], 0x05);
   EXPECT_EQ (dest[5], 0x06);
   EXPECT_EQ (dest[6], 0x07);
   EXPECT_EQ (dest[7], 0x08);
   EXPECT_EQ (dest[8], 0x09);
   EXPECT_EQ (dest[9], 0x0A);

   /* Buffer is larger than string
      It is allowed to fill buffer with junk */
   EXPECT_EQ (clal_copy_string (dest, "JKLM", 8U), 0);
   EXPECT_EQ (dest[0], 'J');
   EXPECT_EQ (dest[1], 'K');
   EXPECT_EQ (dest[2], 'L');
   EXPECT_EQ (dest[3], 'M');
   EXPECT_EQ (dest[4], '\0');
   EXPECT_EQ (dest[8], 0x09);
   EXPECT_EQ (dest[9], 0x0A);
}

TEST_F (ClalDeathTest, ClalDeathCopyStringNullDest)
{
#ifdef __rtk__
   GTEST_SKIP()
      << "Skipping as RT-Kernel compiler does not support death tests";
#else

   /* Should assert when destination is NULL */
   EXPECT_DEATH ({ (void)clal_copy_string (nullptr, "JKLM", 100U); }, "");
#endif // __rtk__
}

TEST_F (ClalDeathTest, ClalDeathCopyStringNullSource)
{
#ifdef __rtk__
   GTEST_SKIP()
      << "Skipping as RT-Kernel compiler does not support death tests";
#else
   char dest[10];

   /* Should assert when source is NULL */
   EXPECT_DEATH ({ (void)clal_copy_string (dest, nullptr, sizeof (dest)); }, "");

#endif // __rtk__
}

/**
 * Test platform-specific function for string formatting
 *
 * @req REQ_CL_STDLIB_01
 */
TEST_F (ClalUnitTest, ClalSnprintf)
{
   size_t i;
   char dest[10];
   for (i = 0; i < sizeof (dest); i++)
   {
      dest[i] = (char)(i + 1U);
   }
   EXPECT_EQ (dest[0], 0x01);
   EXPECT_EQ (dest[1], 0x02);
   EXPECT_EQ (dest[2], 0x03);
   EXPECT_EQ (dest[3], 0x04);
   EXPECT_EQ (dest[4], 0x05);
   EXPECT_EQ (dest[5], 0x06);
   EXPECT_EQ (dest[6], 0x07);
   EXPECT_EQ (dest[7], 0x08);
   EXPECT_EQ (dest[8], 0x09);
   EXPECT_EQ (dest[9], 0x0A);

   /* Null pointer */
   EXPECT_EQ (clal_snprintf (nullptr, sizeof (dest), "A %d", 12), -1);
   EXPECT_EQ (dest[0], 0x01);
   EXPECT_EQ (dest[1], 0x02);
   EXPECT_EQ (dest[2], 0x03);
   EXPECT_EQ (dest[3], 0x04);
   EXPECT_EQ (dest[4], 0x05);
   EXPECT_EQ (dest[5], 0x06);
   EXPECT_EQ (dest[6], 0x07);
   EXPECT_EQ (dest[7], 0x08);
   EXPECT_EQ (dest[8], 0x09);
   EXPECT_EQ (dest[9], 0x0A);

   EXPECT_EQ (clal_snprintf (dest, sizeof (dest), nullptr, 12), -1);
   EXPECT_EQ (dest[0], 0x01);
   EXPECT_EQ (dest[1], 0x02);
   EXPECT_EQ (dest[2], 0x03);
   EXPECT_EQ (dest[3], 0x04);
   EXPECT_EQ (dest[4], 0x05);
   EXPECT_EQ (dest[5], 0x06);
   EXPECT_EQ (dest[6], 0x07);
   EXPECT_EQ (dest[7], 0x08);
   EXPECT_EQ (dest[8], 0x09);
   EXPECT_EQ (dest[9], 0x0A);

   /* Destination size is zero */
   EXPECT_EQ (clal_snprintf (dest, 0U, "A %d", 12), -1);
   EXPECT_EQ (dest[0], 0x01);
   EXPECT_EQ (dest[1], 0x02);
   EXPECT_EQ (dest[2], 0x03);
   EXPECT_EQ (dest[3], 0x04);
   EXPECT_EQ (dest[4], 0x05);
   EXPECT_EQ (dest[5], 0x06);
   EXPECT_EQ (dest[6], 0x07);
   EXPECT_EQ (dest[7], 0x08);
   EXPECT_EQ (dest[8], 0x09);
   EXPECT_EQ (dest[9], 0x0A);

   /* Truncate, destination size is one */
   EXPECT_EQ (clal_snprintf (dest, 1U, "A %d", 12), -1);
   EXPECT_EQ (dest[0], '\0');
   EXPECT_EQ (dest[1], 0x02);
   EXPECT_EQ (dest[2], 0x03);
   EXPECT_EQ (dest[3], 0x04);
   EXPECT_EQ (dest[4], 0x05);
   EXPECT_EQ (dest[5], 0x06);
   EXPECT_EQ (dest[6], 0x07);
   EXPECT_EQ (dest[7], 0x08);
   EXPECT_EQ (dest[8], 0x09);
   EXPECT_EQ (dest[9], 0x0A);

   /* Truncate */
   EXPECT_EQ (clal_snprintf (dest, 2U, "A %d", 12), -1);
   EXPECT_EQ (dest[0], 'A');
   EXPECT_EQ (dest[1], '\0');
   EXPECT_EQ (dest[2], 0x03);
   EXPECT_EQ (dest[3], 0x04);
   EXPECT_EQ (dest[4], 0x05);
   EXPECT_EQ (dest[5], 0x06);
   EXPECT_EQ (dest[6], 0x07);
   EXPECT_EQ (dest[7], 0x08);
   EXPECT_EQ (dest[8], 0x09);
   EXPECT_EQ (dest[9], 0x0A);

   /* Truncate */
   EXPECT_EQ (clal_snprintf (dest, 7U, "b %d", 456789), -1);
   EXPECT_EQ (dest[0], 'b');
   EXPECT_EQ (dest[1], ' ');
   EXPECT_EQ (dest[2], '4');
   EXPECT_EQ (dest[3], '5');
   EXPECT_EQ (dest[4], '6');
   EXPECT_EQ (dest[5], '7');
   EXPECT_EQ (dest[6], '\0');
   EXPECT_EQ (dest[7], 0x08);
   EXPECT_EQ (dest[8], 0x09);
   EXPECT_EQ (dest[9], 0x0A);

   /* Verify formatting of number*/
   EXPECT_EQ (clal_snprintf (dest, sizeof (dest), "A %d", 12), 4);
   EXPECT_EQ (dest[0], 'A');
   EXPECT_EQ (dest[1], ' ');
   EXPECT_EQ (dest[2], '1');
   EXPECT_EQ (dest[3], '2');
   EXPECT_EQ (dest[4], '\0');

   /* Verify formatting of string*/
   EXPECT_EQ (clal_snprintf (dest, sizeof (dest), "mn %s", "pq"), 5);
   EXPECT_EQ (dest[0], 'm');
   EXPECT_EQ (dest[1], 'n');
   EXPECT_EQ (dest[2], ' ');
   EXPECT_EQ (dest[3], 'p');
   EXPECT_EQ (dest[4], 'q');
   EXPECT_EQ (dest[5], '\0');

   /* No additional argument */
   EXPECT_EQ (clal_snprintf (dest, sizeof (dest), "efg"), 3);
   EXPECT_EQ (dest[0], 'e');
   EXPECT_EQ (dest[1], 'f');
   EXPECT_EQ (dest[2], 'g');
   EXPECT_EQ (dest[3], '\0');

   /* Empty string */
   dest[0] = 0x01;
   dest[1] = 0x02;
   dest[2] = 0x03;
   EXPECT_EQ (clal_snprintf (dest, 1U, ""), 0);
   EXPECT_EQ (dest[0], '\0');
   EXPECT_EQ (dest[1], 0x02);
   EXPECT_EQ (dest[2], 0x03);
}
