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

#ifndef CLM_MASTER_H
#define CLM_MASTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/cl_types.h"

/**
 * Initialise c-link master stack
 *
 * @param clm              c-link master stack instance handle to be initialised
 * @param cfg              c-link master configuration. Contents will be copied.
 * @param now              timestamp in microseconds
 * @return 0 on success, or -1 on failure.
 *
 * @req REQ_CLM_CAPACITY_01
 * @req REQ_CLM_CAPACITY_02
 * @req REQ_CLM_CAPACITY_03
 * @req REQ_CLM_CAPACITY_04
 *
 */
int clm_master_init (clm_t * clm, const clm_cfg_t * cfg, uint32_t now);

/**
 * Exit c-link master stack
 *
 * @param clm              c-link master stack instance handle
 * @return 0 on success, or -1 on failure.
 */
int clm_master_exit (clm_t * clm);

/************ Internal functions made available for tests *****************/

int clm_validate_config (const clm_cfg_t * cfg);

int clm_validate_config_duplicates (const clm_cfg_t * cfg);

int clm_master_eth_init (clm_t * clm);

void clm_master_eth_exit (clm_t * clm);

void clm_master_config_show (const clm_cfg_t * cfg);

void clm_master_internals_show (clm_t * clm);

void clm_master_cyclic_data_show (
   clm_t * clm,
   int indent_size,
   uint16_t num_occupied_stations,
   uint16_t group_index,
   uint16_t slave_device_index);

#ifdef __cplusplus
}
#endif

#endif /* CLM_MASTER_H */
