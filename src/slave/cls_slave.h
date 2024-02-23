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

#ifndef CLS_SLAVE_H
#define CLS_SLAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/cl_types.h"

/**
 * Validate the slave configuration
 *
 * @req REQ_CL_PROTOCOL_36
 * @req REQ_CLS_CONFIGURATION_01
 *
 * @param cfg              c-link slave stack configuration
 * @return 0 for valid configuration, -1 for invalid.
 */
int cls_validate_config (const cls_cfg_t * cfg);

/**
 * Initialise c-link slave stack
 *
 * @param cls              c-link slave stack instance handle to be initialised
 * @param cfg              c-link slave configuration
 * @param now              timestamp in microseconds
 * @return 0 on success, or -1 on failure.
 *
 * @req REQ_CLS_CAPACITY_01
 * @req REQ_CLS_CAPACITY_02
 * @req REQ_CLS_CAPACITY_03
 * @req REQ_CLS_CAPACITY_04
 * @req REQ_CLS_CAPACITY_05
 *
 */
int cls_slave_init (cls_t * cls, const cls_cfg_t * cfg, uint32_t now);

/**
 * Exit c-link slave stack
 *
 * @param cls              c-link slave stack instance handle to be initialised
 * @return 0 on success, or -1 on failure.
 */
int cls_slave_exit (cls_t * cls);

/************ Internal functions made available for tests *****************/

void cls_slave_config_show (const cls_cfg_t * cfg);

void cls_slave_internals_show (cls_t * cls);

void cls_slave_cyclic_data_show (cls_t * cls, int indent_size);

#ifdef __cplusplus
}
#endif

#endif /* CLS_SLAVE_H */
