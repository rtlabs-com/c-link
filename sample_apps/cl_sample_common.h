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

#ifndef CL_SAMPLE_COMMON_H
#define CL_SAMPLE_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Set LED state
 *
 * Hardware specific. Implemented in sample app main file for
 * each supported platform.
 *
 * @param user_arg      User argument
 * @param id            LED number, starting from 0.
 * @param led_state     LED state. Use true for on and false for off.
 */
void app_set_led (void * user_arg, uint16_t id, bool led_state);

/**
 * Read button state
 *
 * Hardware specific. Implemented in sample app main file for
 * each supported platform.
 *
 * @param user_arg      User argument
 * @param id            Button number, starting from 0.
 * @return true if button is pressed, false if not
 */
bool app_get_button (void * user_arg, uint16_t id);

/**
 * Find which directory should be used for storing files
 *
 * Typically used only on platforms where it is hardware
 * dependent.
 *
 * @param user_arg    User argument
 * @param strdest     Destination to where the directory path
 *                    is to be written.
 * @param maxsize     Max allowed path write size
 * @return 0 on success, -1 on failure
 */
int app_get_file_directory (void * user_arg, char * strdest, size_t maxsize);

#ifdef __cplusplus
}
#endif

#endif /* CL_SAMPLE_COMMON_H */
