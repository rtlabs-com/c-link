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

#ifndef CL_LITERALS_H
#define CL_LITERALS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/cl_types.h"

/**
 * Get a description for master state
 *
 * @param state      State to describe
 * @return description
 */
const char * cl_literals_get_master_state (clm_master_state_t state);

/**
 * Get a description for group event
 *
 * @param event      Event to describe
 * @return description
 */
const char * cl_literals_get_group_event (clm_group_event_t event);

/**
 * Get a description for group state
 *
 * @param state      State to describe
 * @return description
 */
const char * cl_literals_get_group_state (clm_group_state_t state);

/**
 * Get a description for device state
 *
 * @param state      State to describe
 * @return description
 */
const char * cl_literals_get_device_state (clm_device_state_t state);

/**
 * Get a description for device event
 *
 * @param event      Event to describe
 * @return description
 */
const char * cl_literals_get_device_event (clm_device_event_t event);

/**
 * Get a description for a master error message
 *
 * @param message      Error message enum
 * @return description
 */
const char * cl_literals_get_master_error_message (clm_error_message_t message);

/**
 * Get a description for a master 'Set IP' result
 *
 * @param message      Error message enum
 * @return description
 */
const char * cl_literals_get_master_set_ip_result (
   clm_master_setip_status_t message);

/**
 * Get a description for a slave error message
 *
 * @param message      Error message enum
 * @return description
 */
const char * cl_literals_get_slave_error_message (cls_error_message_t message);

/**
 * Get a description for slave state
 *
 * @param state      State to describe
 * @return description
 */
const char * cl_literals_get_slave_state (cls_slave_state_t state);

/**
 * Get a description for slave event
 *
 * @param event      Event to describe
 * @return description
 */
const char * cl_literals_get_slave_event (cls_slave_event_t event);

#ifdef __cplusplus
}
#endif

#endif /* CL_LITERALS_H */
