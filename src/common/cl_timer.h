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

#ifndef CL_TIMER_H
#define CL_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define CL_TIMER_MICROSECONDS_PER_MILLISECOND 1000

typedef enum cl_timer_state
{
   CL_TIMER_STOPPED,
   CL_TIMER_RUNNING
} cl_timer_state_t;

typedef struct cl_timer
{
   cl_timer_state_t state;
   uint32_t period;
   uint32_t timestamp;
} cl_timer_t;

/**
 * Start the timer
 *
 * If the timer already is running, it will be overwritten with the new values.
 *
 * @param timer       Timer instance
 * @param period      Period, in microseconds. A value 0 is valid, and causes
 *                    the timer to be expired immediately.
 *                    Max value is UINT32_MAX/2.
 * @param now         Current timestamp, in microseconds
 */
void cl_timer_start (cl_timer_t * timer, uint32_t period, uint32_t now);

/**
 * Start the timer if not already running
 *
 * @param timer       Timer instance
 * @param period      Period, in microseconds. A value 0 is valid, and causes
 *                    the timer to be expired immediately.
 *                    Max value is UINT32_MAX/2.
 * @param now         Current timestamp, in microseconds
 */
void cl_timer_start_if_not_running (
   cl_timer_t * timer,
   uint32_t period,
   uint32_t now);

/**
 * Restart a running timer (using the same period)
 *
 * Do nothing if the timer is stopped.
 *
 * @param timer       Timer instance
 * @param now         Current timestamp, in microseconds
 */
void cl_timer_restart (cl_timer_t * timer, uint32_t now);

/**
 * Stop the timer. Clears the period.
 *
 * Can also be used to initialise the timer instance.
 *
 * Do nothing if the timer is stopped already.
 *
 * @param timer       Timer instance
 */
void cl_timer_stop (cl_timer_t * timer);

/**
 * Determine if timeout has expired
 *
 * This function returns true if the timer is running and time elapsed
 * between \a timer->timestamp and \a now is greater than \a timer->period.
 *
 * This function handles wrapping correctly.
 * Tolerant to negative time difference (returns false if \a now is less than
 * the time when the timer was started).
 *
 * You need to stop the timer manually when expired.
 *
 * @param timer       Timer instance
 * @param now         Current timestamp, in microseconds
 * @return true if the timer is running and the timeout has expired,
 *        and false otherwise
 */
bool cl_timer_is_expired (cl_timer_t * timer, uint32_t now);

/**
 * Check if the timer is running.
 *
 * @param timer       Timer instance
 * @return true if the timer is running, false otherwise
 */
bool cl_timer_is_running (cl_timer_t * timer);

/**
 * Show timer state, for debugging.
 *
 * @param timer       Timer instance
 * @param now         Current timestamp, in microseconds
 */
void cl_timer_show (cl_timer_t * timer, uint32_t now);

#ifdef __cplusplus
}
#endif

#endif /* CL_TIMER_H */
