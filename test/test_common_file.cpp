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

#include "utils_for_testing.h"
#include "mocks.h"

#include "common/cl_file.h"

#include <gtest/gtest.h>

#define TEST_FILE_DATA_SIZE         10
#define TEST_FILE_FILENAME          "my_filename.bin"
#define TEST_FILE_DIRECTORY         "my_directory"
#define TEST_FILE_DIRECTORY_LINUX   "/my_directory/"
#define TEST_FILE_DIRECTORY_WINDOWS "C:\\my_directory\\"
#define TEST_FILE_OUTPUTSTRING_SIZE 40

class FileUnitTest : public UnitTest
{
};

/**
 * Verify file path joining
 *
 */
TEST_F (FileUnitTest, FileJoinDirectoryFilenameLinux)
{
   char outputstring[TEST_FILE_OUTPUTSTRING_SIZE] = {0}; /** Terminated */
   int res                                        = 0;

   /* Happy case */
   res = cl_file_join_directory_filename (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, TEST_FILE_DIRECTORY "/" TEST_FILE_FILENAME), 0);

   res = cl_file_join_directory_filename (
      TEST_FILE_DIRECTORY_LINUX,
      TEST_FILE_FILENAME,
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (
      strcmp (outputstring, TEST_FILE_DIRECTORY_LINUX TEST_FILE_FILENAME),
      0);

   /* Missing filename or outputbuffer */
   res = cl_file_join_directory_filename (
      TEST_FILE_DIRECTORY_LINUX,
      nullptr,
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, -1);
   res = cl_file_join_directory_filename (
      TEST_FILE_DIRECTORY_LINUX,
      TEST_FILE_FILENAME,
      nullptr,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, -1);

   /* Too small outputbuffer */
   res = cl_file_join_directory_filename (
      TEST_FILE_DIRECTORY_LINUX,
      TEST_FILE_FILENAME,
      outputstring,
      0,
      false);
   EXPECT_EQ (res, -1);
   res = cl_file_join_directory_filename (
      TEST_FILE_DIRECTORY_LINUX,
      TEST_FILE_FILENAME,
      outputstring,
      4,
      false);
   EXPECT_EQ (res, -1);

   /* Buffer size limits */
   EXPECT_EQ (
      cl_file_join_directory_filename ("a", "b", outputstring, 3, false),
      -1);
   EXPECT_EQ (
      cl_file_join_directory_filename ("a", "b", outputstring, 4, false),
      0);
   EXPECT_EQ (
      cl_file_join_directory_filename ("a/", "b", outputstring, 3, false),
      -1);
   EXPECT_EQ (
      cl_file_join_directory_filename ("a/", "b", outputstring, 4, false),
      0);
   EXPECT_EQ (
      cl_file_join_directory_filename ("a\\", "b", outputstring, 4, false),
      -1);
   EXPECT_EQ (
      cl_file_join_directory_filename ("a\\", "b", outputstring, 5, false),
      0);
   EXPECT_EQ (
      cl_file_join_directory_filename ("\\", "b", outputstring, 3, false),
      -1);
   EXPECT_EQ (
      cl_file_join_directory_filename ("\\", "b", outputstring, 4, false),
      0);
   EXPECT_EQ (
      cl_file_join_directory_filename ("", "b", outputstring, 1, false),
      -1);
   EXPECT_EQ (cl_file_join_directory_filename ("", "b", outputstring, 2, false), 0);
   EXPECT_EQ (
      cl_file_join_directory_filename (nullptr, "b", outputstring, 1, false),
      -1);
   EXPECT_EQ (
      cl_file_join_directory_filename (nullptr, "b", outputstring, 2, false),
      0);

   /* Too short filename */
   res = cl_file_join_directory_filename (
      TEST_FILE_DIRECTORY_LINUX,
      "",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, -1);

   /* Joining directory and filename */
   res = cl_file_join_directory_filename (
      "abc",
      "def",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "abc/def"), 0);

   res = cl_file_join_directory_filename (
      "abc/",
      "def",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "abc/def"), 0);

   res = cl_file_join_directory_filename (
      "abc/",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "abc/def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "/abc/",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "/abc/def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "/",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "/def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "a",
      "d",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "a/d"), 0);

   res = cl_file_join_directory_filename (
      "a",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "a/def.ghi"), 0);

   res = cl_file_join_directory_filename (
      ".",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "./def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "..",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "../def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "/",
      "d",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "/d"), 0);

   /* No directory given (nullptr or empty string) */
   res = cl_file_join_directory_filename (
      nullptr,
      "d",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "d"), 0);

   res = cl_file_join_directory_filename (
      "",
      "d",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "d"), 0);

   res = cl_file_join_directory_filename (
      nullptr,
      "def",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "def"), 0);

   res = cl_file_join_directory_filename (
      "",
      "def",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "def"), 0);

   res = cl_file_join_directory_filename (
      nullptr,
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      false);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "def.ghi"), 0);
}

TEST_F (FileUnitTest, FileJoinDirectoryFilenameWindows)
{
   char outputstring[TEST_FILE_OUTPUTSTRING_SIZE] = {0}; /** Terminated */
   int res                                        = 0;

   /* Happy case */
   res = cl_file_join_directory_filename (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (
      strcmp (outputstring, TEST_FILE_DIRECTORY "\\" TEST_FILE_FILENAME),
      0);

   res = cl_file_join_directory_filename (
      TEST_FILE_DIRECTORY_WINDOWS,
      TEST_FILE_FILENAME,
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (
      strcmp (outputstring, TEST_FILE_DIRECTORY_WINDOWS TEST_FILE_FILENAME),
      0);

   /* Missing filename or outputbuffer */
   res = cl_file_join_directory_filename (
      TEST_FILE_DIRECTORY_WINDOWS,
      nullptr,
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, -1);
   res = cl_file_join_directory_filename (
      TEST_FILE_DIRECTORY_WINDOWS,
      TEST_FILE_FILENAME,
      nullptr,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, -1);

   /* Too small outputbuffer */
   res = cl_file_join_directory_filename (
      TEST_FILE_DIRECTORY_WINDOWS,
      TEST_FILE_FILENAME,
      outputstring,
      0,
      true);
   EXPECT_EQ (res, -1);
   res = cl_file_join_directory_filename (
      TEST_FILE_DIRECTORY_WINDOWS,
      TEST_FILE_FILENAME,
      outputstring,
      4,
      true);
   EXPECT_EQ (res, -1);

   /* Buffer size limits */
   EXPECT_EQ (
      cl_file_join_directory_filename ("a", "b", outputstring, 3, true),
      -1);
   EXPECT_EQ (cl_file_join_directory_filename ("a", "b", outputstring, 4, true), 0);
   EXPECT_EQ (
      cl_file_join_directory_filename ("a/", "b", outputstring, 4, true),
      -1);
   EXPECT_EQ (
      cl_file_join_directory_filename ("a/", "b", outputstring, 5, true),
      0);
   EXPECT_EQ (
      cl_file_join_directory_filename ("a\\", "b", outputstring, 3, true),
      -1);
   EXPECT_EQ (
      cl_file_join_directory_filename ("a\\", "b", outputstring, 4, true),
      0);
   EXPECT_EQ (
      cl_file_join_directory_filename ("\\", "b", outputstring, 2, true),
      -1);
   EXPECT_EQ (
      cl_file_join_directory_filename ("\\", "b", outputstring, 3, true),
      0);
   EXPECT_EQ (
      cl_file_join_directory_filename ("", "b", outputstring, 1, true),
      -1);
   EXPECT_EQ (cl_file_join_directory_filename ("", "b", outputstring, 2, true), 0);
   EXPECT_EQ (
      cl_file_join_directory_filename (nullptr, "b", outputstring, 1, true),
      -1);
   EXPECT_EQ (
      cl_file_join_directory_filename (nullptr, "b", outputstring, 2, true),
      0);

   /* Too short filename */
   res = cl_file_join_directory_filename (
      TEST_FILE_DIRECTORY_WINDOWS,
      "",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, -1);

   /* Joining directory and filename */
   res = cl_file_join_directory_filename (
      "abc",
      "def",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "abc\\def"), 0);

   res = cl_file_join_directory_filename (
      "abc\\",
      "def",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "abc\\def"), 0);

   res = cl_file_join_directory_filename (
      "abc\\",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "abc\\def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "\\abc\\",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "\\abc\\def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "\\",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "\\def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "C:\\",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "C:\\def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "C:\\abc",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "C:\\abc\\def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "C:\\abc\\",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "C:\\abc\\def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "a",
      "d",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "a\\d"), 0);

   res = cl_file_join_directory_filename (
      "a",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "a\\def.ghi"), 0);

   res = cl_file_join_directory_filename (
      ".",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, ".\\def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "..",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "..\\def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "\\",
      "d",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "\\d"), 0);

   /* No directory given (nullptr or empty string) */
   res = cl_file_join_directory_filename (
      nullptr,
      "d",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "d"), 0);

   res = cl_file_join_directory_filename (
      "",
      "d",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "d"), 0);

   res = cl_file_join_directory_filename (
      nullptr,
      "def",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "def"), 0);

   res = cl_file_join_directory_filename (
      "",
      "def",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "def"), 0);

   res = cl_file_join_directory_filename (
      nullptr,
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "def.ghi"), 0);

   res = cl_file_join_directory_filename (
      "",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE,
      true);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "def.ghi"), 0);
}

/**
 * Verify the handling of binary file version and initial magic bytes
 *
 */
TEST_F (FileUnitTest, FileCheckMagicAndVersion)
{
   const uint8_t testdata[TEST_FILE_DATA_SIZE] =
      {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'};
   uint8_t retrieved[TEST_FILE_DATA_SIZE] = {0};
   uint8_t temporary_byte;
   int res = 0;
   int i;

   /* Save data together with file version information */
   res = cl_file_save (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &testdata,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (
      mock_data.storage_files[0].file_size,
      (size_t)(TEST_FILE_DATA_SIZE + 8));
   /* Implementation detail, but verifies mock functionality */

   /* Save: Validate mock (protection of max mocked file size) */
   res = cl_file_save (TEST_FILE_DIRECTORY, TEST_FILE_FILENAME, &testdata, 10000);
   EXPECT_EQ (res, -1);

   /* Load: Non-existent file */
   res = cl_file_load (
      TEST_FILE_DIRECTORY,
      "unknown_filename",
      &retrieved,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, -1);

   /* Retrieve data */
   res = cl_file_load (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &retrieved,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);
   for (i = 0; i < TEST_FILE_DATA_SIZE; i++)
   {
      EXPECT_EQ (testdata[i], retrieved[i]);
   }

   /* Invalid magic bytes in simulated file */
   temporary_byte = mock_data.storage_files[0].file_content[0];
   mock_data.storage_files[0].file_content[0] = 'A';
   res                                        = cl_file_load (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &retrieved,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, -1);
   mock_data.storage_files[0].file_content[0] = temporary_byte; /* Reset */

   /* Invalid file version in simulated file */
   temporary_byte = mock_data.storage_files[0].file_content[4];
   mock_data.storage_files[0].file_content[4] = 200;
   res                                        = cl_file_load (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &retrieved,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, -1);
   mock_data.storage_files[0].file_content[4] = temporary_byte; /* Reset */

   /* Verify that mock does not delete file for other name */
   cl_file_clear (TEST_FILE_DIRECTORY, "nonexistent file");
   EXPECT_GT (mock_data.storage_files[0].file_size, (size_t)1);

   /* Clear: Too long directory name.
             Fails to join path, no clearing done */
   char long_directory_name[CL_MAX_FILE_FULLPATH_SIZE];
   for (size_t ix = 0; ix < sizeof (long_directory_name); ix++)
   {
      if (ix == 0)
      {
         long_directory_name[ix] = '/';
      }
      else if (ix == sizeof (long_directory_name) - 1U)
      {
         long_directory_name[ix] = '\0';
      }
      else
      {
         long_directory_name[ix] = 'z';
      }
   }
   cl_file_clear (long_directory_name, TEST_FILE_FILENAME);
   EXPECT_GT (mock_data.storage_files[0].file_size, (size_t)1);

   /* Clear: Too long file name.
             Fails to join path, no clearing done */
   char long_file_name[CL_MAX_FILE_FULLPATH_SIZE + 1];
   for (size_t ix = 0; ix < sizeof (long_file_name); ix++)
   {
      if (ix == sizeof (long_file_name) - 1U)
      {
         long_file_name[ix] = '\0';
      }
      else
      {
         long_file_name[ix] = 'z';
      }
   }
   cl_file_clear ("", long_file_name);
   EXPECT_GT (mock_data.storage_files[0].file_size, (size_t)1);

   /* Clear: Wrong directory, no clearing done in TEST_FILE_DIRECTORY */
   cl_file_clear (nullptr, TEST_FILE_FILENAME);
   cl_file_clear ("", TEST_FILE_FILENAME);
   EXPECT_GT (mock_data.storage_files[0].file_size, (size_t)1);

   /* Check that it is OK when we use correct sample data again
      (the file is still available) */
   res = cl_file_load (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &retrieved,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);

   /* Verify that the mock can delete file entry
      (also verifies mock functionality) */
   cl_file_clear (TEST_FILE_DIRECTORY, TEST_FILE_FILENAME);
   EXPECT_EQ (mock_data.storage_files[0].file_size, (size_t)0);
   res = cl_file_load (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &retrieved,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, -1);

   /* Save to current directory (also verifies mock functionality) */
   res =
      cl_file_save (nullptr, TEST_FILE_FILENAME, &testdata, TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (
      mock_data.storage_files[0].file_size,
      (size_t)(TEST_FILE_DATA_SIZE + 8));

   /* Retrieve data from current directory */
   res =
      cl_file_load (nullptr, TEST_FILE_FILENAME, &retrieved, TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);
   for (i = 0; i < TEST_FILE_DATA_SIZE; i++)
   {
      EXPECT_EQ (testdata[i], retrieved[i]);
   }

   res = cl_file_load ("", TEST_FILE_FILENAME, &retrieved, TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);
   for (i = 0; i < TEST_FILE_DATA_SIZE; i++)
   {
      EXPECT_EQ (testdata[i], retrieved[i]);
   }

   /* Delete file in current directory (also verifies mock functionality) */
   cl_file_clear (nullptr, TEST_FILE_FILENAME);
   EXPECT_EQ (mock_data.storage_files[0].file_size, (size_t)0);
   res =
      cl_file_load (nullptr, TEST_FILE_FILENAME, &retrieved, TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, -1);
}

/**
 * Verify that the file contents are saved if modified
 *
 */
TEST_F (FileUnitTest, FileCheckSaveIfModified)
{
   uint8_t testdata[TEST_FILE_DATA_SIZE] =
      {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'};
   uint8_t tempobject[TEST_FILE_DATA_SIZE]; /* Not initialized on purpose */
   uint8_t retrieved[TEST_FILE_DATA_SIZE] = {0};
   int res                                = 0;
   int i;

   cl_file_clear (TEST_FILE_DIRECTORY, TEST_FILE_FILENAME);

   /* Missing filename */
   res = cl_file_save_if_modified (
      TEST_FILE_DIRECTORY,
      nullptr,
      &testdata,
      &tempobject,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, -1);

   /* First saving */
   res = cl_file_save_if_modified (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &testdata,
      &tempobject,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 2);

   /* No update */
   res = cl_file_save_if_modified (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &testdata,
      &tempobject,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);

   res = cl_file_save_if_modified (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &testdata,
      &tempobject,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);

   /* Updated data */
   testdata[0] = 's';
   res         = cl_file_save_if_modified (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &testdata,
      &tempobject,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 1);

   res = cl_file_save_if_modified (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &testdata,
      &tempobject,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);

   /* Verify updated data */
   res = cl_file_load (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &retrieved,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);

   EXPECT_EQ (retrieved[0], 's');
   for (i = 1; i < TEST_FILE_DATA_SIZE; i++)
   {
      EXPECT_EQ (retrieved[i], testdata[i]);
   }
}

/**
 * Verify mocked file handling
 *
 */
TEST_F (FileUnitTest, VerifyFileMocks)
{
   uint8_t loadbuffer1[50];
   uint8_t loadbuffer2[50];
   uint8_t result_uint8   = 0;
   uint16_t result_uint16 = 0;
   uint32_t result_uint32 = 0;
   int res                = 0;
   const uint32_t value_a = 0x11;
   const uint64_t value_b = 0x22;
   const uint16_t value_c = 0x33;
   const uint8_t value_d  = 0x44;
   const uint32_t value_e = 0x55;

   EXPECT_EQ (NELEMENTS (mock_data.storage_files), 2U);
   EXPECT_EQ (mock_data.storage_files[0].in_use, false);
   EXPECT_EQ (mock_data.storage_files[1].in_use, false);

   /* Trying to save a too large file, compared to how much available
      space there is in the mock */
   res = mock_clal_save_file ("file1", &value_a, 10000, nullptr, 0);
   EXPECT_EQ (res, -1);
   EXPECT_EQ (mock_data.storage_files[0].in_use, false);
   EXPECT_EQ (mock_data.storage_files[1].in_use, false);

   /* Too long path */
   res = mock_clal_save_file (
      "file567890123456789012345678901234567890",
      &value_a,
      sizeof (value_a),
      nullptr,
      0);
   EXPECT_EQ (res, -1);
   EXPECT_EQ (mock_data.storage_files[0].in_use, false);
   EXPECT_EQ (mock_data.storage_files[1].in_use, false);

   /* Store data */
   res = mock_clal_save_file (
      "file1",
      &value_a,
      sizeof (value_a),
      &value_b,
      sizeof (value_b));
   EXPECT_EQ (res, 0);
   EXPECT_EQ (mock_data.storage_files[0].in_use, true);
   EXPECT_EQ (mock_data.storage_files[1].in_use, false);

   res = mock_clal_save_file (
      "file2",
      &value_c,
      sizeof (value_c),
      &value_d,
      sizeof (value_d));
   EXPECT_EQ (res, 0);
   EXPECT_EQ (mock_data.storage_files[0].in_use, true);
   EXPECT_EQ (mock_data.storage_files[1].in_use, true);

   /* Overwrite */
   res = mock_clal_save_file (
      "file1",
      &value_a,
      sizeof (value_a),
      &value_b,
      sizeof (value_b));
   EXPECT_EQ (res, 0);
   EXPECT_EQ (mock_data.storage_files[0].in_use, true);
   EXPECT_EQ (mock_data.storage_files[1].in_use, true);

   /* Disk is full */
   res = mock_clal_save_file ("file3", &value_e, sizeof (value_e), nullptr, 0);
   EXPECT_EQ (res, -1);

   /* Remove one file */
   mock_clal_clear_file ("unknownfile");
   EXPECT_EQ (mock_data.storage_files[0].in_use, true);
   EXPECT_EQ (mock_data.storage_files[1].in_use, true);

   mock_clal_clear_file ("file1");
   EXPECT_EQ (mock_data.storage_files[0].in_use, false);
   EXPECT_EQ (mock_data.storage_files[1].in_use, true);

   /* Retry saving file */
   res = mock_clal_save_file ("file3", &value_e, sizeof (value_e), nullptr, 0);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (mock_data.storage_files[0].in_use, true);
   EXPECT_EQ (mock_data.storage_files[1].in_use, true);

   /* Verify disk state */
   EXPECT_EQ (strcmp (mock_data.storage_files[0].file_fullpath, "file3"), 0);
   EXPECT_EQ (mock_data.storage_files[0].file_size, sizeof (value_e));
   EXPECT_EQ (
      memcmp (
         mock_data.storage_files[0].file_content,
         &value_e,
         mock_data.storage_files[0].file_size),
      0);

   EXPECT_EQ (strcmp (mock_data.storage_files[1].file_fullpath, "file2"), 0);
   EXPECT_EQ (
      mock_data.storage_files[1].file_size,
      sizeof (value_c) + sizeof (value_d));
   EXPECT_EQ (
      memcmp (mock_data.storage_files[1].file_content, &value_c, sizeof (value_c)),
      0);
   EXPECT_EQ (
      memcmp (
         (uint8_t *)mock_data.storage_files[1].file_content + sizeof (value_c),
         &value_d,
         sizeof (value_d)),
      0);

   /* Read deleted file*/
   res = mock_clal_load_file ("file1", loadbuffer1, sizeof (value_e), nullptr, 0);
   EXPECT_EQ (res, -1);

   /* Read missing file */
   res =
      mock_clal_load_file ("missing", loadbuffer1, sizeof (value_e), nullptr, 0);
   EXPECT_EQ (res, -1);

   /* Ask for more data than what is available in the file */
   res =
      mock_clal_load_file ("file3", loadbuffer1, sizeof (value_e) + 1U, nullptr, 0);
   EXPECT_EQ (res, -1);

   /* Read data */
   res = mock_clal_load_file ("file3", loadbuffer1, sizeof (value_e), nullptr, 0);
   EXPECT_EQ (res, 0);
   result_uint32 = *((uint32_t *)loadbuffer1);
   EXPECT_EQ (result_uint32, value_e);

   res = mock_clal_load_file (
      "file2",
      loadbuffer1,
      sizeof (value_c),
      loadbuffer2,
      sizeof (value_d));
   EXPECT_EQ (res, 0);
   result_uint16 = *((uint16_t *)loadbuffer1);
   EXPECT_EQ (result_uint16, value_c);
   result_uint8 = *((uint8_t *)loadbuffer2);
   EXPECT_EQ (result_uint8, value_d);
}
