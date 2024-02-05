/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2022 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#ifndef CL_OPTIONS_H
#define CL_OPTIONS_H

/* #undef LOG_ENABLE */

/* clang-format off */

#ifndef LOG_LEVEL
#define LOG_LEVEL (LOG_LEVEL_FATAL)
#endif

#ifndef CL_CCIEFB_LOG
#define CL_CCIEFB_LOG (LOG_STATE_ON)
#endif

#ifndef CL_SLMP_LOG
#define CL_SLMP_LOG (LOG_STATE_ON)
#endif

#ifndef CL_ETH_LOG
#define CL_ETH_LOG (LOG_STATE_ON)
#endif

#ifndef CL_CLAL_LOG
#define CL_CLAL_LOG (LOG_STATE_ON)
#endif

#ifndef CL_LOG
#define CL_LOG (LOG_STATE_ON)
#endif

#ifndef CL_MAX_DIRECTORYPATH_SIZE
/** Max directory path size, including termination */
#define CL_MAX_DIRECTORYPATH_SIZE (240)
#endif

#ifndef CLS_MAX_OCCUPIED_STATIONS
/** Max number of occupied slave stations. Compile time setting, allowed 1..16 */
#define CLS_MAX_OCCUPIED_STATIONS (4)
#endif

#ifndef CLM_MAX_GROUPS
/** Max number of groups. Compile time setting, allowed 1..64 */
#define CLM_MAX_GROUPS (5)
#endif

#ifndef CLM_MAX_OCCUPIED_STATIONS_PER_GROUP
/** Max occupied stations per group. Compile time setting, allowed 1..16 */
#define CLM_MAX_OCCUPIED_STATIONS_PER_GROUP (13)
#endif

#ifndef CLM_MAX_NODE_SEARCH_DEVICES
/** Max number of node search response stored in database. Compile time setting */
#define CLM_MAX_NODE_SEARCH_DEVICES (20)
#endif

/* clang-format on */

#endif /* CL_OPTIONS_H */
