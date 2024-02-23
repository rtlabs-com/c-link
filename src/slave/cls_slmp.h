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

#ifndef CLS_SLMP_H
#define CLS_SLMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/cl_types.h"

/**
 * Initialise SLMP
 *
 * @param cls              c-link slave stack instance handle
 *
 * @return 0 on success, -1 on failure
 *
 * @req REQ_CL_UDP_02
 *
 */
int cls_slmp_init (cls_t * cls);

/**
 * Exit SLMP
 *
 * @param cls              c-link slave stack instance handle
 *
 */
void cls_slmp_exit (cls_t * cls);

/**
 * Execute SLMP internals.
 *
 * Use this function every tick.
 *
 * @param cls              c-link slave stack instance handle
 * @param now              timestamp in microseconds
 */
void cls_slmp_periodic (cls_t * cls, uint32_t now);

/************ Internal functions made available for tests *******************/

int cls_slmp_send_node_search_response (cls_t * cls);

#ifdef __cplusplus
}
#endif

#endif /* CLS_SLMP_H */
