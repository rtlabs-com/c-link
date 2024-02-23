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

/**
 * @file
 * @brief Implement timer functionality
 *
 * Intended to be useful for both master and slave.
 * The functions in this file should not have knowledge on master or slave
 * states or configuration structs.
 *
 * It is preferred that any logging is done when calling these functions
 * (not in this file, if avoidable).
 *
 * No mocking should be necessary for testing these functions.
 */

#include "common/cl_timer.h"

#include <inttypes.h>
#include <stdio.h>

void cl_timer_start (cl_timer_t * timer, uint32_t period, uint32_t now)
{
   timer->state     = CL_TIMER_RUNNING;
   timer->period    = period;
   timer->timestamp = now;
}

void cl_timer_start_if_not_running (cl_timer_t * timer, uint32_t period, uint32_t now)
{
   if (timer->state == CL_TIMER_RUNNING)
   {
      return;
   }

   cl_timer_start (timer, period, now);
}

void cl_timer_restart (cl_timer_t * timer, uint32_t now)
{
   if (timer->state == CL_TIMER_STOPPED)
   {
      return;
   }

   timer->timestamp = now;
}

void cl_timer_stop (cl_timer_t * timer)
{
   timer->state     = CL_TIMER_STOPPED;
   timer->period    = 0;
   timer->timestamp = 0;
}

bool cl_timer_is_expired (cl_timer_t * timer, uint32_t now)
{
   uint32_t delta;

   if (timer->state == CL_TIMER_STOPPED)
   {
      return false;
   }

   delta = now - timer->timestamp;
   if (delta > (UINT32_MAX >> 1U))
   {
      return false;
   }

   return delta >= timer->period;
}

bool cl_timer_is_running (cl_timer_t * timer)
{
   return timer->state == CL_TIMER_RUNNING;
}

void cl_timer_show (cl_timer_t * timer, uint32_t now)
{
   uint32_t delta;

   if (timer->state == CL_TIMER_STOPPED)
   {
      printf (
         "Timer is stopped. Now: %" PRIu32 " milliseconds.\n",
         now / CL_TIMER_MICROSECONDS_PER_MILLISECOND);
      return;
   }

   if (cl_timer_is_expired (timer, now))
   {
      printf (
         "Timer has expired. Now: %" PRIu32 " milliseconds.\n",
         now / CL_TIMER_MICROSECONDS_PER_MILLISECOND);
      return;
   }

   delta = (timer->timestamp + timer->period - now);
   printf (
      "Timer has period of %" PRIu32 " milliseconds. It will expire in %" PRIu32
      " milliseconds. Now: %" PRIu32 " milliseconds.\n",
      timer->period / CL_TIMER_MICROSECONDS_PER_MILLISECOND,
      delta / CL_TIMER_MICROSECONDS_PER_MILLISECOND,
      now / CL_TIMER_MICROSECONDS_PER_MILLISECOND);
}
