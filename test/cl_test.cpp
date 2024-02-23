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

#include <gtest/gtest.h>

int main (int argc, char * argv[])
{
   if (argc > 0)
   {
      ::testing::InitGoogleTest (&argc, argv);
   }
   else
   {
      ::testing::InitGoogleTest();
   }

   exit (RUN_ALL_TESTS());
}
