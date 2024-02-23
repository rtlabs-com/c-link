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
#include "common/cl_timer.h"

#include "mocks.h"
#include "utils_for_testing.h"

#include <gtest/gtest.h>

// Test fixture

class TimerUnitTest : public UnitTest
{
};

// Tests

TEST_F (TimerUnitTest, RunTimer)
{
   cl_timer_t timer;
   uint32_t now          = 0;
   const uint32_t period = 2000; /* 2 milliseconds */

   /* Initialise */
   cl_timer_stop (&timer);
   EXPECT_EQ (timer.state, CL_TIMER_STOPPED);
   EXPECT_FALSE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   /* Restart should only work if already started */
   cl_timer_restart (&timer, now);
   EXPECT_EQ (timer.state, CL_TIMER_STOPPED);
   EXPECT_FALSE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   /* Start timer */
   cl_timer_start (&timer, period, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += period / 4;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   /* Timer should expire */
   now += period;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_TRUE (cl_timer_is_expired (&timer, now));

   /* Still running and expired */
   now += period;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_TRUE (cl_timer_is_expired (&timer, now));

   /* Restart expired timer with same period */
   cl_timer_restart (&timer, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += 3 * period / 4;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   /* Restarting will delay the expiration */
   cl_timer_restart (&timer, now);

   now += 3 * period / 4;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += 3 * period / 4;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_TRUE (cl_timer_is_expired (&timer, now));

   /* Stop a running timer */
   cl_timer_restart (&timer, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += period / 4;
   cl_timer_stop (&timer);
   EXPECT_EQ (timer.state, CL_TIMER_STOPPED);
   EXPECT_FALSE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   /* A stopped timer will not expire */
   now += 2 * period;
   EXPECT_EQ (timer.state, CL_TIMER_STOPPED);
   EXPECT_FALSE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   /* A stopped timer must be started with cl_timer_start() */
   now += period / 4;
   cl_timer_restart (&timer, now);
   EXPECT_EQ (timer.state, CL_TIMER_STOPPED);
   EXPECT_FALSE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   cl_timer_start (&timer, period, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += period / 4;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += period;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_TRUE (cl_timer_is_expired (&timer, now));

   /* Start an already running timer, with other period */
   cl_timer_restart (&timer, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += period / 4;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   cl_timer_start (&timer, 3 * period, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += 2 * period;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += 2 * period;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_TRUE (cl_timer_is_expired (&timer, now));
   cl_timer_stop (&timer);
   EXPECT_EQ (timer.state, CL_TIMER_STOPPED);
   EXPECT_FALSE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   /* Starting a running timer will overwrite values */
   cl_timer_start (&timer, period, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += 3 * period / 4;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));
   cl_timer_start (&timer, period, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += 3 * period / 4;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += 3 * period / 4;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_TRUE (cl_timer_is_expired (&timer, now));

   /* Try starting a timer with zero period */
   now += period;
   cl_timer_start (&timer, 0, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_TRUE (cl_timer_is_expired (&timer, now));

   now += 4 * period;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_TRUE (cl_timer_is_expired (&timer, now));

   /* Manipulate internal state */
   cl_timer_start (&timer, period, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += period / 4;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   timer.period = 0;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_TRUE (cl_timer_is_expired (&timer, now));

   now += 4 * period;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_TRUE (cl_timer_is_expired (&timer, now));

   /* Start if not running */
   now += period;
   cl_timer_stop (&timer);
   EXPECT_EQ (timer.state, CL_TIMER_STOPPED);
   EXPECT_FALSE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   cl_timer_start_if_not_running (&timer, period, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += 3 * period / 4;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));
   cl_timer_start_if_not_running (&timer, period, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += 3 * period / 4;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_TRUE (cl_timer_is_running (&timer));
   EXPECT_TRUE (cl_timer_is_expired (&timer, now));
}

TEST_F (TimerUnitTest, TimerHandleCounterWrapping)
{
   cl_timer_t timer;
   const uint32_t period = 2000; /* 2 milliseconds */
   uint32_t now          = UINT32_MAX - 1;

   /* Handle wrapping of time counter */
   cl_timer_stop (&timer);
   EXPECT_EQ (timer.state, CL_TIMER_STOPPED);
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   cl_timer_start (&timer, period, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += period / 2;
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += period / 2;
   EXPECT_TRUE (cl_timer_is_expired (&timer, now));
}

TEST_F (TimerUnitTest, TimerHandleNegativeTimeDifferences)
{
   cl_timer_t timer;
   uint32_t now          = 0;
   const uint32_t period = 2000; /* 2 milliseconds */

   /* Should be tolerant to small negative time differences */
   now += period;
   cl_timer_stop (&timer);
   EXPECT_EQ (timer.state, CL_TIMER_STOPPED);
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now += period;
   cl_timer_start (&timer, period, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   now -= period / 4;
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   /* Handle large negative time differences */
   cl_timer_stop (&timer);
   cl_timer_start (&timer, period, (UINT32_MAX / 2) + 5);
   EXPECT_FALSE (cl_timer_is_expired (&timer, (UINT32_MAX / 2) + 5));
   EXPECT_FALSE (cl_timer_is_expired (&timer, (UINT32_MAX / 2)));
   EXPECT_FALSE (cl_timer_is_expired (&timer, 6));
   EXPECT_FALSE (cl_timer_is_expired (&timer, 5));
   EXPECT_FALSE (cl_timer_is_expired (&timer, 4));
   EXPECT_TRUE (cl_timer_is_expired (&timer, 3));
   EXPECT_TRUE (cl_timer_is_expired (&timer, 2));
   EXPECT_TRUE (cl_timer_is_expired (&timer, 1));
   EXPECT_TRUE (cl_timer_is_expired (&timer, 0));
   EXPECT_TRUE (cl_timer_is_expired (&timer, UINT32_MAX));
   EXPECT_TRUE (cl_timer_is_expired (&timer, UINT32_MAX - 1));

   /* Should be tolerant to negative time differences while wrapping */
   now = 5;
   cl_timer_stop (&timer);
   EXPECT_EQ (timer.state, CL_TIMER_STOPPED);
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));

   cl_timer_start (&timer, period, now);
   EXPECT_EQ (timer.state, CL_TIMER_RUNNING);
   EXPECT_FALSE (cl_timer_is_expired (&timer, now));
   EXPECT_TRUE (cl_timer_is_expired (&timer, 5 + period));
   EXPECT_FALSE (cl_timer_is_expired (&timer, 7));
   EXPECT_FALSE (cl_timer_is_expired (&timer, 6));
   EXPECT_FALSE (cl_timer_is_expired (&timer, 5));
   EXPECT_FALSE (cl_timer_is_expired (&timer, 4));
   EXPECT_FALSE (cl_timer_is_expired (&timer, 3));
   EXPECT_FALSE (cl_timer_is_expired (&timer, 2));
   EXPECT_FALSE (cl_timer_is_expired (&timer, 1));
   EXPECT_FALSE (cl_timer_is_expired (&timer, 0));
   EXPECT_FALSE (cl_timer_is_expired (&timer, UINT32_MAX));
   EXPECT_FALSE (cl_timer_is_expired (&timer, UINT32_MAX - 1));
}

TEST_F (TimerUnitTest, TimerShow)
{
   cl_timer_t timer;
   uint32_t now          = 0;
   const uint32_t period = 2000; /* 2 milliseconds */
   cl_timer_stop (&timer);

   /* Should not crash */
   cl_timer_show (&timer, now);
   cl_timer_start (&timer, period, now);
   cl_timer_show (&timer, now);
   now += 2 * period;
   cl_timer_show (&timer, now);
}
