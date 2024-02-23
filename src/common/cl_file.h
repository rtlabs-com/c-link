
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

#ifndef CL_FILE_H
#define CL_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

/* Filename used by c-link stack. A filename may not be longer than
 * CL_MAX_FILENAME_SIZE (termination included).
 */
#define CLM_FILENAME_PARAM_NO "clm_data_param_no.bin"

/**
 * Load a binary file, and verify the file version.
 *
 * @param directory        Directory for files. Terminated string. NULL or
 *                         empty string is interpreted as current directory.
 * @param filename         File name. Terminated string.
 * @param p_object         Struct to load
 * @param size             Size of struct to load
 * @return  0  if the operation succeeded.
 *          -1 if not found or an error occurred (for example wrong version).
 */
int cl_file_load (
   const char * directory,
   const char * filename,
   void * p_object,
   size_t size);

/**
 * Save a binary file, and include version information.
 *
 * @param directory        Directory for files. Terminated string. NULL or
 *                         empty string is interpreted as current directory.
 * @param filename         File name. Terminated string.
 * @param p_object         Struct to save
 * @param size             Size of struct to save
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int cl_file_save (
   const char * directory,
   const char * filename,
   const void * p_object,
   size_t size);

/**
 * Save a binary file if modified, and include version information.
 *
 * No saving is done if the content would be the same. This reduces the flash
 * memory wear.
 *
 * @param directory        Directory for files. Terminated string. NULL or
 *                         empty string is interpreted as current directory.
 * @param filename         File name. Terminated string.
 * @param p_object         Struct to save
 * @param p_tempobject     Temporary buffer (of same size as object) for
 *                         loading existing file.
 * @param size             Size of struct to save
 * @return  2 First saving of file (no previous file with correct version found)
 *          1 Updated file
 *          0  No storing required (no changes)
 *          -1 if an error occurred.
 */
int cl_file_save_if_modified (
   const char * directory,
   const char * filename,
   const void * p_object,
   void * p_tempobject,
   size_t size);

/**
 * Clear a binary file.
 *
 * @param directory        Directory for files. Terminated string. NULL or
 *                         empty string is interpreted as current directory.
 * @param filename         File name. Terminated string.
 */
void cl_file_clear (const char * directory, const char * filename);

/************ Internal functions, made available for unit testing ************/

int cl_file_join_directory_filename (
   const char * directory,
   const char * filename,
   char * fullpath,
   size_t size,
   bool use_windows_path);

#ifdef __cplusplus
}
#endif

#endif /* CL_FILE_H */
