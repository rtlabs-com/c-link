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
 
#ifndef CL_VERSION_H
#define CL_VERSION_H

#define CLINK_GIT_REVISION "v1.12.0-3-gedbb2a6-dirty"

#if !defined(CL_VERSION_BUILD) && defined(CLINK_GIT_REVISION)
#define CL_VERSION_BUILD CLINK_GIT_REVISION
#endif

/* clang-format-off */

#define CL_VERSION_MAJOR 1
#define CL_VERSION_MINOR 12
#define CL_VERSION_PATCH 0

#if defined(CL_VERSION_BUILD)
#define CL_VERSION \
   "1.12.0+"CL_VERSION_BUILD
#else
#define CL_VERSION \
   "1.12.0"
#endif

/* clang-format-on */

#endif /* CL_VERSION_H */
