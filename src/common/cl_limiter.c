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
 * @brief Limit the number of times a warning message is logged or
 *        the number of times a callback is triggered
 *
 * Useful for warning or error messages that might be triggered repeatedly,
 * typically once per incoming frame.
 *
 * This avoids spamming the logfile.
 *
 * No mocking should be necessary for testing these functions.
 */

#include "cl_limiter.h"

#include "common/cl_timer.h"

#include <inttypes.h>

void cl_limiter_init (cl_limiter_t * limiter, uint32_t period)
{
   limiter->period            = period;
   limiter->number_of_calls   = 0;
   limiter->number_of_outputs = 0;
   limiter->previous_message  = 0;

   /* Initialise timer */
   cl_timer_stop (&limiter->timer);
}

bool cl_limiter_should_run_now (cl_limiter_t * limiter, int message, uint32_t now)
{
   bool was_running = cl_timer_is_running (&limiter->timer);

   limiter->number_of_calls++;
   cl_timer_start (&limiter->timer, limiter->period, now);

   if (was_running && limiter->period != 0 && message == limiter->previous_message)
   {
      return false;
   }

   limiter->number_of_outputs++;
   limiter->previous_message = message;
   return true;
}

void cl_limiter_periodic (cl_limiter_t * limiter, uint32_t now)
{
   if (cl_timer_is_expired (&limiter->timer, now))
   {
      cl_timer_stop (&limiter->timer);
   }
}
