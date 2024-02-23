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

#include "utils_for_testing.h"

#include "osal_log.h"

#include <gtest/gtest.h>

// Test fixture

class LimiterUnitTest : public UnitTest
{
};

// Tests

TEST_F (LimiterUnitTest, RunLimiter)
{
   cl_limiter_t loglimiter;
   uint32_t now                       = 0;
   const int message_A                = 0;
   const int message_B                = 1;
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

   EXPECT_TRUE (cl_limiter_should_run_now (&loglimiter, message_A, now));
   EXPECT_EQ (loglimiter.number_of_calls, 1);
   EXPECT_EQ (loglimiter.number_of_outputs, 1);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Drop next similar message */
   EXPECT_FALSE (cl_limiter_should_run_now (&loglimiter, message_A, now));
   EXPECT_EQ (loglimiter.number_of_calls, 2);
   EXPECT_EQ (loglimiter.number_of_outputs, 1);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Drop next similar message, sent later */
   now += shorter_than_period;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   EXPECT_FALSE (cl_limiter_should_run_now (&loglimiter, message_A, now));
   EXPECT_EQ (loglimiter.number_of_calls, 3);
   EXPECT_EQ (loglimiter.number_of_outputs, 1);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Loglimiter times out */
   now += period + delta_time;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_FALSE (cl_timer_is_running (&loglimiter.timer));

   /* Next message is logged */
   EXPECT_TRUE (cl_limiter_should_run_now (&loglimiter, message_A, now));
   EXPECT_EQ (loglimiter.number_of_calls, 4);
   EXPECT_EQ (loglimiter.number_of_outputs, 2);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Drop next similar message, sent later */
   now += delta_time;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   EXPECT_FALSE (cl_limiter_should_run_now (&loglimiter, message_A, now));
   EXPECT_EQ (loglimiter.number_of_calls, 5);
   EXPECT_EQ (loglimiter.number_of_outputs, 2);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* A different message will be printed */
   EXPECT_TRUE (cl_limiter_should_run_now (&loglimiter, message_B, now));
   EXPECT_EQ (loglimiter.number_of_calls, 6);
   EXPECT_EQ (loglimiter.number_of_outputs, 3);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Repeated logging will be suppressed */
   now += shorter_than_period;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   EXPECT_FALSE (cl_limiter_should_run_now (&loglimiter, message_B, now));
   EXPECT_EQ (loglimiter.number_of_calls, 7);
   EXPECT_EQ (loglimiter.number_of_outputs, 3);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   now += shorter_than_period;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   EXPECT_FALSE (cl_limiter_should_run_now (&loglimiter, message_B, now));
   EXPECT_EQ (loglimiter.number_of_calls, 8);
   EXPECT_EQ (loglimiter.number_of_outputs, 3);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   now += shorter_than_period;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   EXPECT_FALSE (cl_limiter_should_run_now (&loglimiter, message_B, now));
   EXPECT_EQ (loglimiter.number_of_calls, 9);
   EXPECT_EQ (loglimiter.number_of_outputs, 3);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));
}

TEST_F (LimiterUnitTest, LimiterPeriodZero)
{
   const int message_A       = 0;
   uint32_t now              = 0;
   const uint32_t delta_time = 1000; /* 1 millisecond */
   cl_limiter_t loglimiter;

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

   EXPECT_TRUE (cl_limiter_should_run_now (&loglimiter, message_A, now));
   EXPECT_EQ (loglimiter.number_of_calls, 1);
   EXPECT_EQ (loglimiter.number_of_outputs, 1);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Next message is logged */
   EXPECT_TRUE (cl_limiter_should_run_now (&loglimiter, message_A, now));
   EXPECT_EQ (loglimiter.number_of_calls, 2);
   EXPECT_EQ (loglimiter.number_of_outputs, 2);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Timer times out immediately */
   now += delta_time;
   cl_limiter_periodic (&loglimiter, now);
   EXPECT_FALSE (cl_timer_is_running (&loglimiter.timer));

   /* Next message is logged */
   EXPECT_TRUE (cl_limiter_should_run_now (&loglimiter, message_A, now));
   EXPECT_EQ (loglimiter.number_of_calls, 3);
   EXPECT_EQ (loglimiter.number_of_outputs, 3);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));

   /* Next message is logged */
   EXPECT_TRUE (cl_limiter_should_run_now (&loglimiter, message_A, now));
   EXPECT_EQ (loglimiter.number_of_calls, 4);
   EXPECT_EQ (loglimiter.number_of_outputs, 4);
   EXPECT_TRUE (cl_timer_is_running (&loglimiter.timer));
}
