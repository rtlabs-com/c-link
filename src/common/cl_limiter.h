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

#ifndef CL_LIMITER_H
#define CL_LIMITER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cl_timer.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct cl_limiter
{
   cl_timer_t timer;
   uint32_t period;
   int previous_message;

   /* Counter for tests. Overflows to be handled in test cases. */
   uint16_t number_of_calls;

   /* Counter for tests. Overflows to be handled in test cases. */
   uint16_t number_of_outputs;
} cl_limiter_t;

/**
 * Initialise the limiter instance
 *
 * Intended to limit logging, or to limit triggering of callbacks.
 *
 * The limiter will silently drop a message if the previous message
 * was of the same type and it arrived in the last \a period time.
 *
 * If lots of similar messages arrive, only the first is logged as the
 * timer is restarted on reception of a message.
 *
 * @param limiter     Limiter instance
 * @param period      Period, in microseconds. A value 0 is valid, and
 *                    causes all messages to be logged.
 */
void cl_limiter_init (cl_limiter_t * limiter, uint32_t period);

/**
 * Check if message should be logged (or a callback triggered).
 *
 * A different message type will be logged regardless of the timer.
 *
 * @param limiter       Limiter instance
 * @param message       Typically an enum describing the message.
 * @param now           Current timestamp, in microseconds.
 */
bool cl_limiter_should_run_now (cl_limiter_t * limiter, int message, uint32_t now);

/**
 * Periodic handling of the limiter.
 *
 * Stops the timer if no log messages have arrived (or callbacks triggered).
 *
 * @param limiter       Limiter instance
 * @param now           Current timestamp, in microseconds.
 */
void cl_limiter_periodic (cl_limiter_t * limiter, uint32_t now);

#ifdef __cplusplus
}
#endif

#endif /* CL_LIMITER_H */
