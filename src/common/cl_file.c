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

/**
 * @file
 * @brief Utility functions for reading and writing data from/to disk or flash.
 *
 * Adds the bytes "CLNK" and a version indicator to the beginning of the file
 * when writing. Checks the corresponding values when reading.
 *
 */

#ifdef UNIT_TEST
#define clal_clear_file mock_clal_clear_file
#define clal_load_file  mock_clal_load_file
#define clal_save_file  mock_clal_save_file
#endif

#include "cl_file.h"
#include "cl_options.h"
#include "cl_types.h"
#include "cl_util.h"
#include "osal_log.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define CL_FILE_MAGIC 0x434C4E4BU /* "CLNK" */

#ifndef CL_USE_BACKSLASH_PATH_SEPARATOR
#ifdef _WINDOWS
#define CL_USE_BACKSLASH_PATH_SEPARATOR true
#else
#define CL_USE_BACKSLASH_PATH_SEPARATOR false
#endif /* _WINDOWS */
#endif /* CL_USE_BACKSLASH_PATH_SEPARATOR */

/* Increase every time the saved contents have another format */
#define CL_FILE_VERSION 0x00000001U

/* The configurable constant CL_MAX_FILENAME_SIZE should be at least
 * as large as the longest filename used, including termination.
 */
CC_STATIC_ASSERT (CL_MAX_FILENAME_SIZE >= sizeof (CLM_FILENAME_PARAM_NO));

/**
 * @internal
 * Join directory and filename into a full path.
 *
 * If no directory is given, use only the filename.
 *
 * The behaviour is different on Windows from other operating systems,
 * as the path separator is a backslash instead of a forward slash.
 * Use a parameter for this, to be able to unittest the function behavior
 * regardless of which operating system the tests are running on.
 *
 * @param directory        Directory for files. Terminated string. NULL or
 *                         empty string is interpreted as current directory.
 * @param filename         File name. Terminated string.
 * @param fullpath         Resulting string. Terminated.
 * @param size             Size of outputbuffer.
 * @param use_windows_path True if the path contains backslashes instead of
 *                         forward slashes, for example on Windows.
 * @return  0  if the operation succeeded.
 *          -1 if not found or an error occurred.
 */
int cl_file_join_directory_filename (
   const char * directory,
   const char * filename,
   char * fullpath,
   size_t size,
   bool use_windows_path)
{
   const char * separator = (use_windows_path) ? "\\" : "/";

   if ((filename == NULL) || (fullpath == NULL) || (strlen (filename) < 1))
   {
      LOG_ERROR (
         CL_LOG,
         "FILE(%d): Filename and outputbuffer must be given!\n",
         __LINE__);
      return -1;
   }

   bool use_directory = (directory != NULL) && (strlen (directory) > 0);
   bool add_slash     = use_directory &&
                    (directory[strlen (directory) - 1] != separator[0]);
   size_t required_buffer_size = (use_directory ? strlen (directory) : 0) +
                                 (add_slash ? 1 : 0) + strlen (filename) + 1;

   if (required_buffer_size > size)
   {
      LOG_ERROR (
         CL_LOG,
         "FILE(%d): Too long directory and filename! Total required size is "
         "%zu but buffer size is %zu\n",
         __LINE__,
         required_buffer_size,
         size);
      return -1;
   }

   int num_written_characters = clal_snprintf (
      fullpath,
      size,
      "%s%s%s",
      (use_directory) ? directory : "",
      (add_slash) ? separator : separator + 1,
      filename);
   CC_ASSERT (num_written_characters == (int)(required_buffer_size - 1));
   (void)num_written_characters;

   return 0;
}

int cl_file_load (
   const char * directory,
   const char * filename,
   void * p_object,
   size_t size)
{
   cl_file_header_t file_header;
   char path[CL_MAX_FILE_FULLPATH_SIZE];
#if LOG_DEBUG_ENABLED(CL_LOG)
   uint32_t start_time_us = 0;
#endif

   if (
      cl_file_join_directory_filename (
         directory,
         filename,
         path,
         CL_MAX_FILE_FULLPATH_SIZE,
         CL_USE_BACKSLASH_PATH_SEPARATOR) != 0)
   {
      return -1;
   }

   /* Read file */
#if LOG_DEBUG_ENABLED(CL_LOG)
   start_time_us = os_get_current_time_us();
#endif
   if (clal_load_file (path, &file_header, sizeof (file_header), p_object, size) != 0)
   {
      return -1;
   }
#if LOG_DEBUG_ENABLED(CL_LOG)
   LOG_DEBUG (
      CL_LOG,
      "FILE(%d): Read file %s (access time %" PRIu32 " ms)\n",
      __LINE__,
      path,
      ((os_get_current_time_us() - start_time_us) / 1000));
#endif

   /* Check file magic bytes */
   if (CC_FROM_BE32 (file_header.magic) != CL_FILE_MAGIC)
   {
      LOG_ERROR (
         CL_LOG,
         "FILE(%d): Wrong file magic bytes in file %s\n",
         __LINE__,
         path);
      return -1;
   }

   /* Check file version */
   if (CC_FROM_BE32 (file_header.version) != CL_FILE_VERSION)
   {
      LOG_WARNING (
         CL_LOG,
         "FILE(%d): Wrong file version identifier in file %s  Expected %" PRIu32
         " but got %" PRIu32 ".\n",
         __LINE__,
         path,
         (uint32_t)CL_FILE_VERSION,
         file_header.version);
      return -1;
   }

   return 0;
}

int cl_file_save (
   const char * directory,
   const char * filename,
   const void * p_object,
   size_t size)
{
   char path[CL_MAX_FILE_FULLPATH_SIZE]; /**< Terminated string */
   cl_file_header_t file_header;
#if LOG_DEBUG_ENABLED(CL_LOG)
   uint32_t start_time_us = 0;
#endif

   if (
      cl_file_join_directory_filename (
         directory,
         filename,
         path,
         CL_MAX_FILE_FULLPATH_SIZE,
         CL_USE_BACKSLASH_PATH_SEPARATOR) != 0)
   {
      return -1;
   }

   file_header.magic   = CC_TO_BE32 (CL_FILE_MAGIC);
   file_header.version = CC_TO_BE32 (CL_FILE_VERSION);

#if LOG_DEBUG_ENABLED(CL_LOG)
   start_time_us = os_get_current_time_us();
#endif
   int ret =
      clal_save_file (path, &file_header, sizeof (file_header), p_object, size);
#if LOG_DEBUG_ENABLED(CL_LOG)
   LOG_DEBUG (
      CL_LOG,
      "FILE(%d): Saved file %s (access time %" PRIu32 " ms)\n",
      __LINE__,
      path,
      ((os_get_current_time_us() - start_time_us) / 1000));
#endif

   return ret;
}

int cl_file_save_if_modified (
   const char * directory,
   const char * filename,
   const void * p_object,
   void * p_tempobject,
   size_t size)
{
   bool save = false;
   int ret   = 0; /* Assume no changes */

   clal_clear_memory (p_tempobject, size);

   if (cl_file_load (directory, filename, p_tempobject, size) == 0)
   {
      if (memcmp (p_tempobject, p_object, size) != 0)
      {
         ret  = 1;
         save = true;
      }
   }
   else
   {
      ret  = 2;
      save = true;
   }

   if (save == true)
   {
      if (cl_file_save (directory, filename, p_object, size) != 0)
      {
         ret = -1;
      }
   }

   return ret;
}

void cl_file_clear (const char * directory, const char * filename)
{
   char path[CL_MAX_FILE_FULLPATH_SIZE];

   if (
      cl_file_join_directory_filename (
         directory,
         filename,
         path,
         CL_MAX_FILE_FULLPATH_SIZE,
         CL_USE_BACKSLASH_PATH_SEPARATOR) == 0)
   {
      clal_clear_file (path);
   }
}
