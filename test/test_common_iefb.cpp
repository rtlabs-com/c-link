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

#include "cl_options.h"
#include "common/cl_iefb.h"

#include "mocks.h"
#include "utils_for_testing.h"

#include <gtest/gtest.h>

// Test fixture

class IefbUnitTest : public UnitTest
{
};

/*********************** Parse and create frames *****************************/

/**
 * Validate slave end code
 *
 * @req REQ_CLS_ERROR_04
 *
 */
TEST_F (IefbUnitTest, CciefbValidateSlaveEndcode)
{
   // clang-format off
   EXPECT_TRUE (cl_is_slave_endcode_valid (CL_SLMP_ENDCODE_SUCCESS));
   EXPECT_TRUE (cl_is_slave_endcode_valid (CL_SLMP_ENDCODE_CCIEFB_MASTER_DUPLICATION));
   EXPECT_TRUE (cl_is_slave_endcode_valid (CL_SLMP_ENDCODE_CCIEFB_WRONG_NUMBER_OCCUPIED_STATIONS));
   EXPECT_TRUE (cl_is_slave_endcode_valid (CL_SLMP_ENDCODE_CCIEFB_SLAVE_ERROR));
   EXPECT_TRUE (cl_is_slave_endcode_valid (CL_SLMP_ENDCODE_CCIEFB_SLAVE_REQUESTS_DISCONNECT));
   EXPECT_FALSE (cl_is_slave_endcode_valid ((cl_slmp_error_codes_t)1));
   EXPECT_FALSE (cl_is_slave_endcode_valid ((cl_slmp_error_codes_t)2));
   EXPECT_FALSE (cl_is_slave_endcode_valid ((cl_slmp_error_codes_t)3));
   EXPECT_FALSE (cl_is_slave_endcode_valid ((cl_slmp_error_codes_t)UINT16_MAX));
   // clang-format on
}

/**
 * Validate cyclic request size
 *
 * @req REQ_CL_PROTOCOL_38
 * @req REQ_CL_PROTOCOL_39
 *
 */
TEST_F (IefbUnitTest, CciefbCalculateCyclicRequestSize)
{
   cl_calculate_cyclic_request_size (0); /* Invalid, should not crash */
   EXPECT_EQ (cl_calculate_cyclic_request_size (1), 143U); /* Wireshark */
   EXPECT_EQ (cl_calculate_cyclic_request_size (1), SIZE_REQUEST_1_SLAVE);
   EXPECT_EQ (cl_calculate_cyclic_request_size (2), 219U);
   EXPECT_EQ (cl_calculate_cyclic_request_size (3), 295U);
   EXPECT_EQ (cl_calculate_cyclic_request_size (3), SIZE_REQUEST_3_SLAVES);
   EXPECT_EQ (cl_calculate_cyclic_request_size (4), 371U);
   EXPECT_EQ (cl_calculate_cyclic_request_size (5), 447U);
   EXPECT_EQ (cl_calculate_cyclic_request_size (6), 523U); /* Wireshark */
   EXPECT_EQ (cl_calculate_cyclic_request_size (7), 599U);
   EXPECT_EQ (cl_calculate_cyclic_request_size (8), 675U);
   EXPECT_EQ (cl_calculate_cyclic_request_size (9), 751U);
   EXPECT_EQ (cl_calculate_cyclic_request_size (10), 827U);
   EXPECT_EQ (cl_calculate_cyclic_request_size (11), 903U);
   EXPECT_EQ (cl_calculate_cyclic_request_size (12), 979U);
   EXPECT_EQ (cl_calculate_cyclic_request_size (13), 1055U);
   EXPECT_EQ (cl_calculate_cyclic_request_size (14), 1131U);
   EXPECT_EQ (cl_calculate_cyclic_request_size (15), 1207U);
   EXPECT_EQ (cl_calculate_cyclic_request_size (16), 1283U);
   cl_calculate_cyclic_request_size (17); /* Invalid, should not crash */
}

TEST_F (IefbUnitTest, CciefbCalculateCyclicResponseSize)
{
   cl_calculate_cyclic_response_size (0); /* Invalid, should not crash */
   EXPECT_EQ (cl_calculate_cyclic_response_size (1), 131U); /* Wireshark */
   EXPECT_EQ (cl_calculate_cyclic_response_size (1), SIZE_RESPONSE_1_SLAVE);
   EXPECT_EQ (cl_calculate_cyclic_response_size (2), 203U);
   EXPECT_EQ (cl_calculate_cyclic_response_size (2), SIZE_RESPONSE_2_SLAVES);
   EXPECT_EQ (cl_calculate_cyclic_response_size (3), 275U);
   EXPECT_EQ (cl_calculate_cyclic_response_size (4), 347U);
   EXPECT_EQ (cl_calculate_cyclic_response_size (5), 419U);
   EXPECT_EQ (cl_calculate_cyclic_response_size (6), 491U);
   EXPECT_EQ (cl_calculate_cyclic_response_size (7), 563U);
   EXPECT_EQ (cl_calculate_cyclic_response_size (8), 635U);
   EXPECT_EQ (cl_calculate_cyclic_response_size (9), 707U);
   EXPECT_EQ (cl_calculate_cyclic_response_size (10), 779U);
   EXPECT_EQ (cl_calculate_cyclic_response_size (11), 851U);
   EXPECT_EQ (cl_calculate_cyclic_response_size (12), 923U);
   EXPECT_EQ (cl_calculate_cyclic_response_size (13), 995U);
   EXPECT_EQ (cl_calculate_cyclic_response_size (14), 1067U);
   EXPECT_EQ (cl_calculate_cyclic_response_size (15), 1139U);
   EXPECT_EQ (cl_calculate_cyclic_response_size (16), 1211U);
   cl_calculate_cyclic_response_size (17); /* Invalid, should not crash */
}

TEST_F (IefbUnitTest, CciefbCalculateNumberOfOccupied)
{
   EXPECT_EQ (cl_calculate_number_of_occupied_stations (0), 0U);
   EXPECT_EQ (cl_calculate_number_of_occupied_stations (130), 0U);
   EXPECT_EQ (cl_calculate_number_of_occupied_stations (131), 1U);
   EXPECT_EQ (cl_calculate_number_of_occupied_stations (132), 0U);
   EXPECT_EQ (cl_calculate_number_of_occupied_stations (202), 0U);
   EXPECT_EQ (cl_calculate_number_of_occupied_stations (203), 2U);
   EXPECT_EQ (cl_calculate_number_of_occupied_stations (204), 0U);
   EXPECT_EQ (cl_calculate_number_of_occupied_stations (1210), 0U);
   EXPECT_EQ (cl_calculate_number_of_occupied_stations (1211), 16U);
   EXPECT_EQ (cl_calculate_number_of_occupied_stations (1212), 0U);
   EXPECT_EQ (cl_calculate_number_of_occupied_stations (UINT32_MAX - 1), 0U);
   EXPECT_EQ (cl_calculate_number_of_occupied_stations (UINT32_MAX), 0U);
}

TEST_F (IefbUnitTest, CciefbSanityCyclicResponseSize)
{
   uint16_t number_of_occupied = 0;

   for (number_of_occupied = 1; number_of_occupied <= 16; number_of_occupied++)
   {
      EXPECT_EQ (
         cl_calculate_number_of_occupied_stations (
            cl_calculate_cyclic_response_size (number_of_occupied)),
         number_of_occupied);
   }
}

TEST_F (IefbUnitTest, CciefbValidateCyclicReqFrameSize)
{
   /* Protocol version */
   EXPECT_EQ (cl_iefb_validate_req_cyclic_frame_size (0, 523, 514, 6, 36), -1);
   EXPECT_EQ (cl_iefb_validate_req_cyclic_frame_size (1, 523, 514, 6, 36), 0);
   EXPECT_EQ (cl_iefb_validate_req_cyclic_frame_size (2, 523, 514, 6, 36), 0);
   EXPECT_EQ (cl_iefb_validate_req_cyclic_frame_size (3, 523, 514, 6, 36), -1);

   /* Wrong recv_len */
   EXPECT_EQ (cl_iefb_validate_req_cyclic_frame_size (2, 522, 514, 6, 36), -1);
   EXPECT_EQ (cl_iefb_validate_req_cyclic_frame_size (2, 524, 514, 6, 36), -1);

   /* Wrong dl */
   EXPECT_EQ (cl_iefb_validate_req_cyclic_frame_size (2, 523, 513, 6, 36), -1);
   EXPECT_EQ (cl_iefb_validate_req_cyclic_frame_size (2, 523, 515, 6, 36), -1);

   /* Wrong recv_len and dl, but their difference is OK */
   EXPECT_EQ (cl_iefb_validate_req_cyclic_frame_size (1, 524, 515, 6, 36), -1);

   /* Wrong cyclic offset */
   EXPECT_EQ (cl_iefb_validate_req_cyclic_frame_size (2, 523, 514, 6, 0), -1);
   EXPECT_EQ (cl_iefb_validate_req_cyclic_frame_size (2, 523, 514, 6, 35), -1);
   EXPECT_EQ (cl_iefb_validate_req_cyclic_frame_size (2, 523, 514, 6, 37), -1);
}

TEST_F (IefbUnitTest, CciefbParseRequestHeader)
{
   cl_cciefb_req_header_t * header = nullptr;

   uint8_t buffer[50] = {}; /* Larger than sizeof(cl_cciefb_req_header_t) */
   uint32_t ix;
   for (ix = 0; ix < sizeof (buffer); ix++)
   {
      buffer[ix] = (uint8_t)ix;
   }

   /* Parsing of valid content */
   EXPECT_EQ (
      cl_iefb_parse_request_header ((uint8_t *)&buffer, sizeof (buffer), &header),
      0);
   EXPECT_EQ (CC_FROM_BE16 (header->reserved1), 0x0001); /* Big endian */
   EXPECT_EQ (header->reserved2, 0x02);
   EXPECT_EQ (header->reserved3, 0x03);
   EXPECT_EQ (CC_FROM_LE16 (header->reserved4), 0x0504);
   EXPECT_EQ (header->reserved5, 0x06);
   EXPECT_EQ (CC_FROM_LE16 (header->dl), 0x0807);
   EXPECT_EQ (CC_FROM_LE16 (header->reserved6), 0x0A09);
   EXPECT_EQ (CC_FROM_LE16 (header->command), 0x0C0B);
   EXPECT_EQ (CC_FROM_LE16 (header->sub_command), 0x0E0D);

   /* Too short buffer.
      Buffer should be at least sizeof(cl_cciefb_req_header_t) which is 15 bytes
      according to BAP-C2010ENG-001-B */
   EXPECT_EQ (cl_iefb_parse_request_header ((uint8_t *)&buffer, 0, &header), -1);
   EXPECT_EQ (cl_iefb_parse_request_header ((uint8_t *)&buffer, 14, &header), -1);
   EXPECT_EQ (cl_iefb_parse_request_header ((uint8_t *)&buffer, 15, &header), 0);
}

TEST_F (IefbUnitTest, CciefbValidateRequestHeader)
{
   const uint16_t udp_len = 100; /* arbitrary value */

   /* Fixed values according to BAP-C2010ENG-001-B */
   const uint16_t dl             = udp_len - 9;
   cl_cciefb_req_header_t header = {
      .reserved1   = CC_TO_BE16 (0x5000), /* Big endian */
      .reserved2   = 0x00,
      .reserved3   = 0xFF,
      .reserved4   = CC_TO_LE16 (0x03FF),
      .reserved5   = 0x00,
      .dl          = CC_TO_LE16 (dl),
      .reserved6   = CC_TO_LE16 (0x0000),
      .command     = CC_TO_LE16 (0x0E70), /* CL_SLMP_COMMAND_CCIEFB_CYCLIC */
      .sub_command = CC_TO_LE16 (0x0000), /* CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC */
   };
   const cl_cciefb_req_header_t default_header = header;

   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), 0);

   /* Length */
   header.dl = CC_TO_LE16 (0x0000);
   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), 0);

   /* Reserved1 */
   header.reserved1 = CC_TO_BE16 (0x0000);
   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), 0);

   /* Reserved2 */
   header.reserved2 = 0x12;
   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), 0);

   /* Reserved3 */
   header.reserved3 = 0x12;
   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), 0);

   /* Reserved4 */
   header.reserved4 = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), 0);

   /* Reserved5 */
   header.reserved5 = 0x12;
   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), 0);

   /* Reserved6 */
   header.reserved6 = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_request_header (&header, udp_len), 0);
}

TEST_F (IefbUnitTest, CciefbSetTransmissionState)
{
   uint16_t trstate = 0;

   /* Turn on */
   cl_iefb_set_cyclic_transmission_state (&trstate, 0, true);
   EXPECT_EQ (trstate, 0x0000);
   trstate = 0;

   cl_iefb_set_cyclic_transmission_state (&trstate, 1, true);
   EXPECT_EQ (trstate, 0x0001);
   trstate = 0;

   cl_iefb_set_cyclic_transmission_state (&trstate, 2, true);
   EXPECT_EQ (trstate, 0x0002);
   trstate = 0;

   cl_iefb_set_cyclic_transmission_state (&trstate, 3, true);
   EXPECT_EQ (trstate, 0x0004);
   trstate = 0;

   cl_iefb_set_cyclic_transmission_state (&trstate, 15, true);
   EXPECT_EQ (trstate, 0x4000);
   trstate = 0;

   cl_iefb_set_cyclic_transmission_state (&trstate, 16, true);
   EXPECT_EQ (trstate, 0x8000);
   trstate = 0;

   cl_iefb_set_cyclic_transmission_state (&trstate, 17, true);
   EXPECT_EQ (trstate, 0x0000);
   trstate = 0;

   cl_iefb_set_cyclic_transmission_state (&trstate, 1, true);
   cl_iefb_set_cyclic_transmission_state (&trstate, 2, true);
   cl_iefb_set_cyclic_transmission_state (&trstate, 15, true);
   cl_iefb_set_cyclic_transmission_state (&trstate, 16, true);
   EXPECT_EQ (trstate, 0xC003);

   /* Turn off */
   trstate = 0xFFFF;

   cl_iefb_set_cyclic_transmission_state (&trstate, 0, false);
   EXPECT_EQ (trstate, 0xFFFF);
   trstate = 0xFFFF;

   cl_iefb_set_cyclic_transmission_state (&trstate, 1, false);
   EXPECT_EQ (trstate, 0xFFFE);
   trstate = 0xFFFF;

   cl_iefb_set_cyclic_transmission_state (&trstate, 2, false);
   EXPECT_EQ (trstate, 0xFFFD);
   trstate = 0xFFFF;

   cl_iefb_set_cyclic_transmission_state (&trstate, 3, false);
   EXPECT_EQ (trstate, 0xFFFB);
   trstate = 0xFFFF;

   cl_iefb_set_cyclic_transmission_state (&trstate, 15, false);
   EXPECT_EQ (trstate, 0xBFFF);
   trstate = 0xFFFF;

   cl_iefb_set_cyclic_transmission_state (&trstate, 16, false);
   EXPECT_EQ (trstate, 0x7FFF);
   trstate = 0xFFFF;

   cl_iefb_set_cyclic_transmission_state (&trstate, 17, false);
   EXPECT_EQ (trstate, 0xFFFF);
   trstate = 0xFFFF;

   cl_iefb_set_cyclic_transmission_state (&trstate, 1, false);
   cl_iefb_set_cyclic_transmission_state (&trstate, 2, false);
   cl_iefb_set_cyclic_transmission_state (&trstate, 15, false);
   cl_iefb_set_cyclic_transmission_state (&trstate, 16, false);
   EXPECT_EQ (trstate, 0x3FFC);
}

TEST_F (IefbUnitTest, CciefbExtractMyTransmissionState)
{
   /* Slave station 1 */
   EXPECT_FALSE (cl_iefb_extract_my_transmission_state (0x0000, 1));
   EXPECT_TRUE (cl_iefb_extract_my_transmission_state (0x0001, 1));
   EXPECT_FALSE (cl_iefb_extract_my_transmission_state (0x0002, 1));
   EXPECT_TRUE (cl_iefb_extract_my_transmission_state (0x0003, 1));
   EXPECT_FALSE (cl_iefb_extract_my_transmission_state (0xFFF0, 1));
   EXPECT_TRUE (cl_iefb_extract_my_transmission_state (0xFFFF, 1));

   /* Slave station 2 */
   EXPECT_FALSE (cl_iefb_extract_my_transmission_state (0x0000, 2));
   EXPECT_FALSE (cl_iefb_extract_my_transmission_state (0x0001, 2));
   EXPECT_TRUE (cl_iefb_extract_my_transmission_state (0x0002, 2));
   EXPECT_TRUE (cl_iefb_extract_my_transmission_state (0x0003, 2));
   EXPECT_FALSE (cl_iefb_extract_my_transmission_state (0xFFF0, 2));
   EXPECT_TRUE (cl_iefb_extract_my_transmission_state (0xFFFF, 2));

   /* Slave station 16 */
   EXPECT_FALSE (cl_iefb_extract_my_transmission_state (0x0000, 16));
   EXPECT_FALSE (cl_iefb_extract_my_transmission_state (0x0001, 16));
   EXPECT_TRUE (cl_iefb_extract_my_transmission_state (0x8000, 16));
   EXPECT_TRUE (cl_iefb_extract_my_transmission_state (0x9000, 16));
   EXPECT_FALSE (cl_iefb_extract_my_transmission_state (0x0FFF, 16));
   EXPECT_TRUE (cl_iefb_extract_my_transmission_state (0xFFFF, 16));

   /* Invalid slave station */
   EXPECT_FALSE (cl_iefb_extract_my_transmission_state (0x0000, 0));
   EXPECT_FALSE (cl_iefb_extract_my_transmission_state (0xFFFF, 0));
   EXPECT_FALSE (cl_iefb_extract_my_transmission_state (0x0000, 17));
   EXPECT_FALSE (cl_iefb_extract_my_transmission_state (0xFFFF, 17));
}

TEST_F (IefbUnitTest, CciefbBitCalculateAreaNumber)
{
   uint8_t bitmask       = 0x00;
   uint16_t byte_in_area = 0;

   /* First area */
   EXPECT_EQ (cl_iefb_bit_calculate_areanumber (0, &byte_in_area, &bitmask), 0);
   EXPECT_EQ (byte_in_area, 0);
   EXPECT_EQ (bitmask, 0x01);

   EXPECT_EQ (cl_iefb_bit_calculate_areanumber (1, &byte_in_area, &bitmask), 0);
   EXPECT_EQ (byte_in_area, 0);
   EXPECT_EQ (bitmask, 0x02);

   EXPECT_EQ (cl_iefb_bit_calculate_areanumber (2, &byte_in_area, &bitmask), 0);
   EXPECT_EQ (byte_in_area, 0);
   EXPECT_EQ (bitmask, 0x04);

   EXPECT_EQ (cl_iefb_bit_calculate_areanumber (3, &byte_in_area, &bitmask), 0);
   EXPECT_EQ (byte_in_area, 0);
   EXPECT_EQ (bitmask, 0x08);

   EXPECT_EQ (cl_iefb_bit_calculate_areanumber (7, &byte_in_area, &bitmask), 0);
   EXPECT_EQ (byte_in_area, 0);
   EXPECT_EQ (bitmask, 0x80);

   EXPECT_EQ (cl_iefb_bit_calculate_areanumber (8, &byte_in_area, &bitmask), 0);
   EXPECT_EQ (byte_in_area, 1);
   EXPECT_EQ (bitmask, 0x01);

   EXPECT_EQ (cl_iefb_bit_calculate_areanumber (62, &byte_in_area, &bitmask), 0);
   EXPECT_EQ (byte_in_area, 7);
   EXPECT_EQ (bitmask, 0x40);

   EXPECT_EQ (cl_iefb_bit_calculate_areanumber (63, &byte_in_area, &bitmask), 0);
   EXPECT_EQ (byte_in_area, 7);
   EXPECT_EQ (bitmask, 0x80);

   /* Second area */
   EXPECT_EQ (cl_iefb_bit_calculate_areanumber (64, &byte_in_area, &bitmask), 1);
   EXPECT_EQ (byte_in_area, 0);
   EXPECT_EQ (bitmask, 0x01);

   EXPECT_EQ (cl_iefb_bit_calculate_areanumber (65, &byte_in_area, &bitmask), 1);
   EXPECT_EQ (byte_in_area, 0);
   EXPECT_EQ (bitmask, 0x02);

   EXPECT_EQ (cl_iefb_bit_calculate_areanumber (126, &byte_in_area, &bitmask), 1);
   EXPECT_EQ (byte_in_area, 7);
   EXPECT_EQ (bitmask, 0x40);

   EXPECT_EQ (cl_iefb_bit_calculate_areanumber (127, &byte_in_area, &bitmask), 1);
   EXPECT_EQ (byte_in_area, 7);
   EXPECT_EQ (bitmask, 0x80);

   /* Third area */
   EXPECT_EQ (cl_iefb_bit_calculate_areanumber (128, &byte_in_area, &bitmask), 2);
   EXPECT_EQ (byte_in_area, 0);
   EXPECT_EQ (bitmask, 0x01);

   EXPECT_EQ (cl_iefb_bit_calculate_areanumber (129, &byte_in_area, &bitmask), 2);
   EXPECT_EQ (byte_in_area, 0);
   EXPECT_EQ (bitmask, 0x02);
}

TEST_F (IefbUnitTest, CciefbRegisterCalculateAreaNumber)
{
   uint16_t register_in_area = 0;

   /* First area */
   EXPECT_EQ (cl_iefb_register_calculate_areanumber (0, &register_in_area), 0);
   EXPECT_EQ (register_in_area, 0);

   EXPECT_EQ (cl_iefb_register_calculate_areanumber (1, &register_in_area), 0);
   EXPECT_EQ (register_in_area, 1);

   EXPECT_EQ (cl_iefb_register_calculate_areanumber (2, &register_in_area), 0);
   EXPECT_EQ (register_in_area, 2);

   EXPECT_EQ (cl_iefb_register_calculate_areanumber (30, &register_in_area), 0);
   EXPECT_EQ (register_in_area, 30);

   EXPECT_EQ (cl_iefb_register_calculate_areanumber (31, &register_in_area), 0);
   EXPECT_EQ (register_in_area, 31);

   /* Second area */
   EXPECT_EQ (cl_iefb_register_calculate_areanumber (32, &register_in_area), 1);
   EXPECT_EQ (register_in_area, 0);

   EXPECT_EQ (cl_iefb_register_calculate_areanumber (33, &register_in_area), 1);
   EXPECT_EQ (register_in_area, 1);

   EXPECT_EQ (cl_iefb_register_calculate_areanumber (63, &register_in_area), 1);
   EXPECT_EQ (register_in_area, 31);

   /* Third area */
   EXPECT_EQ (cl_iefb_register_calculate_areanumber (64, &register_in_area), 2);
   EXPECT_EQ (register_in_area, 0);

   EXPECT_EQ (cl_iefb_register_calculate_areanumber (65, &register_in_area), 2);
   EXPECT_EQ (register_in_area, 1);
}

/**
 * Test validation of cyclic request header
 *
 */
TEST_F (IefbUnitTest, CciefbValidateCyclicRequestHeader)
{

   /* Fixed values according to BAP-C2010ENG-001-B */
   cl_cciefb_cyclic_req_header_t header = {
      .protocol_ver            = CC_TO_LE16 (1),
      .reserved1               = CC_TO_LE16 (0x0000),
      .cyclic_info_offset_addr = CC_TO_LE16 (36),
      .reserved2               = {},
   };
   const cl_cciefb_cyclic_req_header_t default_header = header;

   EXPECT_EQ (cl_iefb_validate_cyclic_request_header (&header), 0);

   /* Protocol version */
   header.protocol_ver = CC_TO_LE16 (0);
   EXPECT_EQ (cl_iefb_validate_cyclic_request_header (&header), -1);
   header.protocol_ver = CC_TO_LE16 (3);
   EXPECT_EQ (cl_iefb_validate_cyclic_request_header (&header), -1);
   header.protocol_ver = CC_TO_LE16 (2);
   EXPECT_EQ (cl_iefb_validate_cyclic_request_header (&header), 0);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_cyclic_request_header (&header), 0);

   /* Reserved1 */
   header.reserved1 = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_iefb_validate_cyclic_request_header (&header), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_cyclic_request_header (&header), 0);

   /* Cyclic offset */
   header.cyclic_info_offset_addr = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_iefb_validate_cyclic_request_header (&header), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_cyclic_request_header (&header), 0);

   /* Reserved2 */
   header.reserved2[0] = 0x12;
   EXPECT_EQ (cl_iefb_validate_cyclic_request_header (&header), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_cyclic_request_header (&header), 0);
}

TEST_F (IefbUnitTest, CciefbValidateMasterStationNotification)
{
   /* Fixed values according to BAP-C2010ENG-001-B */
   cl_cciefb_master_station_notification_t notification = {
      .master_local_unit_info = CC_TO_LE16 (0x0001),
      .reserved               = CC_TO_LE16 (0x0000),
      .clock_info             = CC_TO_LE64 (0x1234567890123456),
   };
   const cl_cciefb_master_station_notification_t default_notification =
      notification;

   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 1), 0);

   /* Reserved */
   notification.reserved = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 1), -1);
   notification = default_notification;
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 1), 0);

   /* master_local_unit_info, protocol version 1 */
   notification.master_local_unit_info = CC_TO_LE16 (0x0000);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 1), 0);
   notification.master_local_unit_info = CC_TO_LE16 (0x0001);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 1), 0);
   notification.master_local_unit_info = CC_TO_LE16 (0x0002);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 1), -1);
   notification.master_local_unit_info = CC_TO_LE16 (0x0003);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 1), -1);
   notification.master_local_unit_info = CC_TO_LE16 (0x0004);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 1), -1);
   notification.master_local_unit_info = CC_TO_LE16 (0xFFFF);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 1), -1);

   /* master_local_unit_info, protocol version 2 */
   notification.master_local_unit_info = CC_TO_LE16 (0x0000);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 2), 0);
   notification.master_local_unit_info = CC_TO_LE16 (0x0001);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 2), 0);
   notification.master_local_unit_info = CC_TO_LE16 (0x0002);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 2), 0);
   notification.master_local_unit_info = CC_TO_LE16 (0x0003);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 2), 0);
   notification.master_local_unit_info = CC_TO_LE16 (0x0004);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 2), -1);
   notification.master_local_unit_info = CC_TO_LE16 (0xFFFF);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 2), -1);

   /* master_local_unit_info, invalid protocol version */
   notification.master_local_unit_info = CC_TO_LE16 (0x0000);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 0), -1);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 3), -1);
   EXPECT_EQ (cl_iefb_validate_master_station_notification (&notification, 4), -1);
}

/**
 * Test request data header validator
 *
 * @req REQ_CL_PROTOCOL_31
 *
 */
TEST_F (IefbUnitTest, CciefbValidateCyclicRequestDataHeader)
{
   const cl_ipaddr_t ip  = 1; /* 0.0.0.1 */
   const cl_ipaddr_t ip2 = 2; /* 0.0.0.2 */

   /* Fixed values according to BAP-C2010ENG-001-B */
   cl_cciefb_cyclic_req_data_header_t header = {
      .master_id                          = CC_TO_LE32 (ip),
      .group_no                           = 1,
      .reserved3                          = 0x00,
      .frame_sequence_no                  = CC_TO_LE16 (1),
      .timeout_value                      = CC_TO_LE16 (1),
      .parallel_off_timeout_count         = CC_TO_LE16 (1),
      .parameter_no                       = CC_TO_LE16 (1),
      .slave_total_occupied_station_count = CC_TO_LE16 (1),
      .cyclic_transmission_state          = CC_TO_LE16 (1),
      .reserved4                          = CC_TO_LE16 (0x0000)};
   const cl_cciefb_cyclic_req_data_header_t default_header = header;

   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), 0);

   /* All values are allowed for:
      - frame_sequence_no
      - timeout_value
      - parallel_off_timeout_count
      - parameter_no
      - cyclic_transmission_state */

   /* Master ID */
   header.master_id = CC_TO_LE32 (CL_IPADDR_INVALID);
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), -1);
   header.master_id = CC_TO_LE32 (ip2);
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), 0);
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip2), -1);
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), 0);

   /* Group no */
   header.group_no = 0;
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), -1);
   header.group_no = 65;
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), 0);

   /* Reserved3 */
   header.reserved3 = 0x12;
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), 0);

   /* Slave total occupied count */
   header.slave_total_occupied_station_count = CC_TO_LE16 (0);
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), -1);
   header.slave_total_occupied_station_count = CC_TO_LE16 (17);
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), 0);

   /* Reserved4 */
   header.reserved4 = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_req_cyclic_data_header (&header, ip), 0);
}

/**
 * Verify total timeout
 *
 * @req REQ_CL_PROTOCOL_25
 * @req REQ_CL_PROTOCOL_28
 * @req REQ_CL_PROTOCOL_29
 */
TEST_F (IefbUnitTest, CciefbCalculateTotalTimeout)
{
   EXPECT_EQ (cl_calculate_total_timeout_us (1, 1), 1000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (2, 1), 2000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (3, 1), 3000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (4, 1), 4000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (0xFFFF, 1), 65535000UL);
   EXPECT_EQ (cl_calculate_total_timeout_us (1, 2), 2000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (2, 2), 4000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (3, 2), 6000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (4, 2), 8000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (1, 3), 3000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (2, 3), 6000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (3, 3), 9000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (4, 3), 12000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (1, 0xFFFF), 65535000UL);
   EXPECT_EQ (cl_calculate_total_timeout_us (0xFFFF, 0xFFFF), 4294836225000UL);

   /* Default values */
   EXPECT_EQ (cl_calculate_total_timeout_us (0, 0), 1500000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (0, 1), 500000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (0, 2), 1000000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (0, 3), 1500000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (0, 4), 2000000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (1, 0), 3000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (2, 0), 6000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (3, 0), 9000U);
   EXPECT_EQ (cl_calculate_total_timeout_us (500, 0), 1500000U);
}

TEST_F (IefbUnitTest, CciefbGetSlaveId)
{
   uint8_t buffer[100];
   cl_ipaddr_t slave_id          = CL_IPADDR_INVALID;
   buffer[0]                     = 0x01; /* IP 4.3.2.1 */
   buffer[1]                     = 0x02;
   buffer[2]                     = 0x03;
   buffer[3]                     = 0x04;
   buffer[4]                     = 0x05; /* IP 8.7.6.5 */
   buffer[5]                     = 0x06;
   buffer[6]                     = 0x07;
   buffer[7]                     = 0x08;
   buffer[8]                     = 0x09; /* IP 12.11.10.9 */
   buffer[9]                     = 0x0A;
   buffer[10]                    = 0x0B;
   buffer[11]                    = 0x0C;
   buffer[12]                    = 0x0D; /* IP 16.15.14.13 */
   buffer[13]                    = 0x0E;
   buffer[14]                    = 0x0F;
   buffer[15]                    = 0x10;
   buffer[16]                    = 0x11; /* IP 20.19.18.17 */
   buffer[17]                    = 0x12;
   buffer[18]                    = 0x13;
   buffer[19]                    = 0x14;
   buffer[20]                    = 0xFF; /* IP 255.255.255.255 */
   buffer[21]                    = 0xFF;
   buffer[22]                    = 0xFF;
   buffer[23]                    = 0xFF;
   const uint16_t total_occupied = 6;

   /* Valid IP addresses */
   EXPECT_EQ (
      cl_iefb_request_get_slave_id ((uint32_t *)buffer, 1, total_occupied, &slave_id),
      0);
   EXPECT_EQ (slave_id, 0x04030201U); /* IP 4.3.2.1 */
   EXPECT_EQ (
      cl_iefb_request_get_slave_id ((uint32_t *)buffer, 2, total_occupied, &slave_id),
      0);
   EXPECT_EQ (slave_id, 0x08070605U); /* IP 8.7.6.5 */
   EXPECT_EQ (
      cl_iefb_request_get_slave_id ((uint32_t *)buffer, 3, total_occupied, &slave_id),
      0);
   EXPECT_EQ (slave_id, 0x0C0B0A09U); /* IP 12.11.10.9 */
   EXPECT_EQ (
      cl_iefb_request_get_slave_id ((uint32_t *)buffer, 4, total_occupied, &slave_id),
      0);
   EXPECT_EQ (slave_id, 0x100F0E0DU); /* IP 16.15.14.13 */
   EXPECT_EQ (
      cl_iefb_request_get_slave_id ((uint32_t *)buffer, 5, total_occupied, &slave_id),
      0);
   EXPECT_EQ (slave_id, 0x14131211U); /* IP 20.19.18.17 */
   EXPECT_EQ (
      cl_iefb_request_get_slave_id ((uint32_t *)buffer, 6, total_occupied, &slave_id),
      0);
   EXPECT_EQ (slave_id, UINT32_MAX); /* IP 255.255.255.255 */

   /* Wrong total occupied */
   EXPECT_EQ (
      cl_iefb_request_get_slave_id ((uint32_t *)buffer, 1, 0, &slave_id),
      -1);
   EXPECT_EQ (
      cl_iefb_request_get_slave_id ((uint32_t *)buffer, 1, 17, &slave_id),
      -1);

   /* Wrong slave station */
   EXPECT_EQ (
      cl_iefb_request_get_slave_id ((uint32_t *)buffer, 0, total_occupied, &slave_id),
      -1);
   EXPECT_EQ (
      cl_iefb_request_get_slave_id ((uint32_t *)buffer, 17, 16, &slave_id),
      -1);
   EXPECT_EQ (
      cl_iefb_request_get_slave_id (
         (uint32_t *)buffer,
         total_occupied + 1,
         total_occupied,
         &slave_id),
      -1);
}

/**
 * Verify parsing of slave IDs
 *
 * @req REQ_CL_PROTOCOL_34
 *
 */
TEST_F (IefbUnitTest, CciefbAnalyzeSlaveIds)
{
   bool found_my_slave_id            = false;
   uint16_t my_slave_station_no      = 0;
   uint16_t implied_occupation_count = 0;
   const cl_ipaddr_t ip              = 0x08070605; /* IP 8.7.6.5 */
   uint8_t buffer[100];
   buffer[0]                     = 0x01; /* IP 4.3.2.1 */
   buffer[1]                     = 0x02;
   buffer[2]                     = 0x03;
   buffer[3]                     = 0x04;
   buffer[4]                     = 0x05; /* IP 8.7.6.5 */
   buffer[5]                     = 0x06;
   buffer[6]                     = 0x07;
   buffer[7]                     = 0x08;
   buffer[8]                     = 0x09; /* IP 12.11.10.9 */
   buffer[9]                     = 0x0A;
   buffer[10]                    = 0x0B;
   buffer[11]                    = 0x0C;
   buffer[12]                    = 0x0D; /* IP 16.15.14.13 */
   buffer[13]                    = 0x0E;
   buffer[14]                    = 0x0F;
   buffer[15]                    = 0x10;
   buffer[16]                    = 0x11; /* IP 20.19.18.17 */
   buffer[17]                    = 0x12;
   buffer[18]                    = 0x13;
   buffer[19]                    = 0x14;
   const uint16_t total_occupied = 5;

   /* My occupation count = 1 */
   EXPECT_EQ (
      cl_iefb_analyze_slave_ids (
         ip,
         total_occupied,
         (uint32_t *)buffer,
         &found_my_slave_id,
         &my_slave_station_no,
         &implied_occupation_count),
      0);
   EXPECT_EQ (found_my_slave_id, true);
   EXPECT_EQ (my_slave_station_no, 2);
   EXPECT_EQ (implied_occupation_count, 1);

   /* Slave ID not in list */
   EXPECT_EQ (
      cl_iefb_analyze_slave_ids (
         0x08080808, /* IP 8.8.8.8 */
         total_occupied,
         (uint32_t *)buffer,
         &found_my_slave_id,
         &my_slave_station_no,
         &implied_occupation_count),
      0);
   EXPECT_EQ (found_my_slave_id, false);
   EXPECT_EQ (my_slave_station_no, 0);
   EXPECT_EQ (implied_occupation_count, 0);

   /* Invalid IP */
   EXPECT_EQ (
      cl_iefb_analyze_slave_ids (
         CL_IPADDR_INVALID,
         total_occupied,
         (uint32_t *)buffer,
         &found_my_slave_id,
         &my_slave_station_no,
         &implied_occupation_count),
      -1);
   EXPECT_EQ (found_my_slave_id, false);
   EXPECT_EQ (my_slave_station_no, 0);
   EXPECT_EQ (implied_occupation_count, 0);

   /* Wrong occupied stations */
   EXPECT_EQ (
      cl_iefb_analyze_slave_ids (
         ip,
         0,
         (uint32_t *)buffer,
         &found_my_slave_id,
         &my_slave_station_no,
         &implied_occupation_count),
      -1);
   EXPECT_EQ (found_my_slave_id, false);
   EXPECT_EQ (my_slave_station_no, 0);
   EXPECT_EQ (implied_occupation_count, 0);

   EXPECT_EQ (
      cl_iefb_analyze_slave_ids (
         ip,
         17,
         (uint32_t *)buffer,
         &found_my_slave_id,
         &my_slave_station_no,
         &implied_occupation_count),
      -1);
   EXPECT_EQ (found_my_slave_id, false);
   EXPECT_EQ (my_slave_station_no, 0);
   EXPECT_EQ (implied_occupation_count, 0);

   /* My occupation count = 3 (next two positions are IP 255.255.255.255) */
   buffer[8]  = 0xFF;
   buffer[9]  = 0xFF;
   buffer[10] = 0xFF;
   buffer[11] = 0xFF;
   buffer[12] = 0xFF;
   buffer[13] = 0xFF;
   buffer[14] = 0xFF;
   buffer[15] = 0xFF;
   EXPECT_EQ (
      cl_iefb_analyze_slave_ids (
         ip,
         total_occupied,
         (uint32_t *)buffer,
         &found_my_slave_id,
         &my_slave_station_no,
         &implied_occupation_count),
      0);
   EXPECT_EQ (found_my_slave_id, true);
   EXPECT_EQ (my_slave_station_no, 2);
   EXPECT_EQ (implied_occupation_count, 3);

   /* Error due to repeated slave ID */
   buffer[16] = 0x05; /* IP 8.7.6.5 */
   buffer[17] = 0x06;
   buffer[18] = 0x07;
   buffer[19] = 0x08;
   EXPECT_EQ (
      cl_iefb_analyze_slave_ids (
         ip,
         total_occupied,
         (uint32_t *)buffer,
         &found_my_slave_id,
         &my_slave_station_no,
         &implied_occupation_count),
      -1);
   EXPECT_EQ (found_my_slave_id, false);
   EXPECT_EQ (my_slave_station_no, 0);
   EXPECT_EQ (implied_occupation_count, 0);
}

/**
 * Validate generated request frame
 *
 * @req REQ_CL_PROTOCOL_02
 * @req REQ_CL_PROTOCOL_03
 * @req REQ_CL_PROTOCOL_04
 * @req REQ_CL_PROTOCOL_05
 * @req REQ_CL_PROTOCOL_06
 * @req REQ_CL_PROTOCOL_07
 * @req REQ_CL_PROTOCOL_08
 * @req REQ_CL_PROTOCOL_09
 * @req REQ_CL_PROTOCOL_10
 * @req REQ_CL_PROTOCOL_11
 * @req REQ_CL_PROTOCOL_12
 * @req REQ_CL_PROTOCOL_17
 * @req REQ_CL_PROTOCOL_18
 * @req REQ_CL_PROTOCOL_21
 * @req REQ_CL_PROTOCOL_23
 * @req REQ_CL_PROTOCOL_32
 * @req REQ_CL_PROTOCOL_37
 * @req REQ_CL_PROTOCOL_38
 * @req REQ_CL_PROTOCOL_39
 * @req REQ_CL_PROTOCOL_52
 * @req REQ_CL_PROTOCOL_56
 *
 */
TEST_F (IefbUnitTest, CciefbInitialiseUpdateCyclicRequestFrame)
{
   uint8_t buffer[400]                        = {};
   clm_cciefb_cyclic_request_info_t frameinfo = {};
   const uint16_t total_occupied_stations     = 3;
   const size_t len =
      cl_calculate_cyclic_request_size (total_occupied_stations) - (size_t)9;

   cl_iefb_initialise_request_frame (
      (uint8_t *)&buffer,
      sizeof (buffer),
      0x0002,       /* Protocol version */
      0x1234,       /* timeout_value */
      0x5678,       /* parallel_off_timeout_count */
      0x01020304UL, /* master_ip_addr 1.2.3.4 */
      0xBA,         /* group_no */
      total_occupied_stations,
      0xCDEF, /* parameter_no,*/
      &frameinfo);

   EXPECT_EQ (buffer[0], 0x50); /* Fixed values in response header. reserved1 */
   EXPECT_EQ (buffer[1], 0x00);
   EXPECT_EQ (buffer[2], 0x00); /* reserved2 */
   EXPECT_EQ (buffer[3], 0xFF); /* reserved3 */
   EXPECT_EQ (buffer[4], 0xFF); /* reserved4 */
   EXPECT_EQ (buffer[5], 0x03);
   EXPECT_EQ (buffer[6], 0x00);            /* reserved5 */
   EXPECT_EQ (buffer[7], len & UINT8_MAX); /* Length */
   EXPECT_EQ (buffer[8], len >> 8);
   EXPECT_EQ (buffer[9], 0x00); /* Fixed value reserved6 */
   EXPECT_EQ (buffer[10], 0x00);
   EXPECT_EQ (buffer[11], 0x70); /* Command */
   EXPECT_EQ (buffer[12], 0x0E);
   EXPECT_EQ (buffer[13], 0x00); /* Subcommand */
   EXPECT_EQ (buffer[14], 0x00);
   EXPECT_EQ (buffer[15], 0x02); /* Protocol version */
   EXPECT_EQ (buffer[16], 0x00);
   EXPECT_EQ (buffer[17], 0x00); /* CyclicTransmission reserved1 */
   EXPECT_EQ (buffer[18], 0x00);
   EXPECT_EQ (buffer[19], 0x24); /* Cyclic offset = 36 for protocol v1 and 2 */
   EXPECT_EQ (buffer[20], 0x00);
   EXPECT_EQ (buffer[21], 0x00); /* CyclicTransmission reserved2 */
   EXPECT_EQ (buffer[22], 0x00);
   EXPECT_EQ (buffer[23], 0x00);
   EXPECT_EQ (buffer[24], 0x00);
   EXPECT_EQ (buffer[25], 0x00);
   EXPECT_EQ (buffer[26], 0x00);
   EXPECT_EQ (buffer[27], 0x00);
   EXPECT_EQ (buffer[28], 0x00);
   EXPECT_EQ (buffer[29], 0x00);
   EXPECT_EQ (buffer[30], 0x00);
   EXPECT_EQ (buffer[31], 0x00);
   EXPECT_EQ (buffer[32], 0x00);
   EXPECT_EQ (buffer[33], 0x00);
   EXPECT_EQ (buffer[34], 0x00);
   EXPECT_EQ (buffer[35], 0x00); /* Master station running. */
   EXPECT_EQ (buffer[36], 0x00);
   EXPECT_EQ (buffer[37], 0x00); /* MasterNoticeInfo Reserved */
   EXPECT_EQ (buffer[38], 0x00);
   EXPECT_EQ (buffer[39], 0x00); /* Master unix timestamp with milliseconds */
   EXPECT_EQ (buffer[40], 0x00);
   EXPECT_EQ (buffer[41], 0x00);
   EXPECT_EQ (buffer[42], 0x00);
   EXPECT_EQ (buffer[43], 0x00);
   EXPECT_EQ (buffer[44], 0x00);
   EXPECT_EQ (buffer[45], 0x00);
   EXPECT_EQ (buffer[46], 0x00);
   EXPECT_EQ (buffer[47], 0x04); /* Master ID = IP 1.2.3.4  */
   EXPECT_EQ (buffer[48], 0x03);
   EXPECT_EQ (buffer[49], 0x02);
   EXPECT_EQ (buffer[50], 0x01);
   EXPECT_EQ (buffer[51], 0xBA); /* Group */
   EXPECT_EQ (buffer[52], 0x00); /* CyclicInfo reserved3 */
   EXPECT_EQ (buffer[53], 0x00); /* Frame sequence number */
   EXPECT_EQ (buffer[54], 0x00);
   EXPECT_EQ (buffer[55], 0x34); /* Timeout period */
   EXPECT_EQ (buffer[56], 0x12);
   EXPECT_EQ (buffer[57], 0x78); /* Timeout multiplier */
   EXPECT_EQ (buffer[58], 0x56);
   EXPECT_EQ (buffer[59], 0xEF); /* Parameter identifier  */
   EXPECT_EQ (buffer[60], 0xCD);
   EXPECT_EQ (buffer[61], 0x03); /* Total number of slave stations */
   EXPECT_EQ (buffer[62], 0x00);
   EXPECT_EQ (buffer[63], 0x00); /* Transmission status. None running */
   EXPECT_EQ (buffer[64], 0x00);
   EXPECT_EQ (buffer[65], 0x00); /* CyclicInfo reserved4 */
   EXPECT_EQ (buffer[66], 0x00);
   EXPECT_EQ (buffer[67], 0x00); /* IP slave 1 = 1.2.3.5 */
   EXPECT_EQ (buffer[68], 0x00);
   EXPECT_EQ (buffer[69], 0x00);
   EXPECT_EQ (buffer[70], 0x00);
   EXPECT_EQ (buffer[71], 0x00); /* IP slave 2 = 1.2.3.6, Occupies 2 stations */
   EXPECT_EQ (buffer[72], 0x00);
   EXPECT_EQ (buffer[73], 0x00);
   EXPECT_EQ (buffer[74], 0x00);
   EXPECT_EQ (buffer[75], 0x00);
   EXPECT_EQ (buffer[76], 0x00);
   EXPECT_EQ (buffer[77], 0x00);
   EXPECT_EQ (buffer[78], 0x00);
   EXPECT_EQ (buffer[79], 0x00); /* RWw slave 1 */

   cl_iefb_update_request_frame_headers (
      &frameinfo,
      0x4142,                                   /* frame_sequence_no */
      0x5152535455565758,                       /* clock_info */
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING, /* 0x0001 */
      0x6162 /*cyclic_transmission_state */);

   EXPECT_EQ (buffer[0], 0x50); /* Fixed values in response header */
   EXPECT_EQ (buffer[1], 0x00);
   EXPECT_EQ (buffer[2], 0x00);
   EXPECT_EQ (buffer[3], 0xFF);
   EXPECT_EQ (buffer[4], 0xFF);
   EXPECT_EQ (buffer[5], 0x03);
   EXPECT_EQ (buffer[6], 0x00);
   EXPECT_EQ (buffer[7], len & UINT8_MAX); /* Length for 3 occupied stations */
   EXPECT_EQ (buffer[8], len >> 8);
   EXPECT_EQ (buffer[9], 0x00); /* Fixed value */
   EXPECT_EQ (buffer[10], 0x00);
   EXPECT_EQ (buffer[11], 0x70); /* Command */
   EXPECT_EQ (buffer[12], 0x0E);
   EXPECT_EQ (buffer[13], 0x00); /* Subcommand */
   EXPECT_EQ (buffer[14], 0x00);
   EXPECT_EQ (buffer[15], 0x02); /* Protocol version */
   EXPECT_EQ (buffer[16], 0x00);
   EXPECT_EQ (buffer[17], 0x00); /* Reserved area */
   EXPECT_EQ (buffer[18], 0x00);
   EXPECT_EQ (buffer[19], 0x24); /* Cyclic offset = 36 for protocol v1 and 2 */
   EXPECT_EQ (buffer[20], 0x00);
   EXPECT_EQ (buffer[21], 0x00); /* Reserved area */
   EXPECT_EQ (buffer[22], 0x00);
   EXPECT_EQ (buffer[23], 0x00);
   EXPECT_EQ (buffer[24], 0x00);
   EXPECT_EQ (buffer[25], 0x00);
   EXPECT_EQ (buffer[26], 0x00);
   EXPECT_EQ (buffer[27], 0x00);
   EXPECT_EQ (buffer[28], 0x00);
   EXPECT_EQ (buffer[29], 0x00);
   EXPECT_EQ (buffer[30], 0x00);
   EXPECT_EQ (buffer[31], 0x00);
   EXPECT_EQ (buffer[32], 0x00);
   EXPECT_EQ (buffer[33], 0x00);
   EXPECT_EQ (buffer[34], 0x00);
   EXPECT_EQ (buffer[35], 0x01); /* Master station running. UPDATED */
   EXPECT_EQ (buffer[36], 0x00);
   EXPECT_EQ (buffer[37], 0x00); /* Reserved */
   EXPECT_EQ (buffer[38], 0x00);
   EXPECT_EQ (buffer[39], 0x58); /* Master unix timestamp. UPDATED */
   EXPECT_EQ (buffer[40], 0x57);
   EXPECT_EQ (buffer[41], 0x56);
   EXPECT_EQ (buffer[42], 0x55);
   EXPECT_EQ (buffer[43], 0x54);
   EXPECT_EQ (buffer[44], 0x53);
   EXPECT_EQ (buffer[45], 0x52);
   EXPECT_EQ (buffer[46], 0x51);
   EXPECT_EQ (buffer[47], 0x04); /* Master ID = IP 1.2.3.4  */
   EXPECT_EQ (buffer[48], 0x03);
   EXPECT_EQ (buffer[49], 0x02);
   EXPECT_EQ (buffer[50], 0x01);
   EXPECT_EQ (buffer[51], 0xBA); /* Group */
   EXPECT_EQ (buffer[52], 0x00); /* Reserved*/
   EXPECT_EQ (buffer[53], 0x42); /* Frame sequence number. UPDATED */
   EXPECT_EQ (buffer[54], 0x41);
   EXPECT_EQ (buffer[55], 0x34); /* Timeout period */
   EXPECT_EQ (buffer[56], 0x12);
   EXPECT_EQ (buffer[57], 0x78); /* Timeout multiplier */
   EXPECT_EQ (buffer[58], 0x56);
   EXPECT_EQ (buffer[59], 0xEF); /* Parameter identifier  */
   EXPECT_EQ (buffer[60], 0xCD);
   EXPECT_EQ (buffer[61], 0x03); /* Total number of slave stations */
   EXPECT_EQ (buffer[62], 0x00);
   EXPECT_EQ (buffer[63], 0x62); /* Transmission status. UPDATED */
   EXPECT_EQ (buffer[64], 0x61);
   EXPECT_EQ (buffer[65], 0x00); /* Reserved */
   EXPECT_EQ (buffer[66], 0x00);
   EXPECT_EQ (buffer[67], 0x00); /* IP slave 1 = 1.2.3.5 */
   EXPECT_EQ (buffer[68], 0x00);
   EXPECT_EQ (buffer[69], 0x00);
   EXPECT_EQ (buffer[70], 0x00);
   EXPECT_EQ (buffer[71], 0x00); /* IP slave 2 = 1.2.3.6, Occupies 2 stations */
   EXPECT_EQ (buffer[72], 0x00);
   EXPECT_EQ (buffer[73], 0x00);
   EXPECT_EQ (buffer[74], 0x00);
   EXPECT_EQ (buffer[75], 0x00);
   EXPECT_EQ (buffer[76], 0x00);
   EXPECT_EQ (buffer[77], 0x00);
   EXPECT_EQ (buffer[78], 0x00);
   EXPECT_EQ (buffer[79], 0x00); /* RWw slave 1 */
}

TEST_F (IefbUnitTest, CciefbParseRequestFullCyclicHeaders)
{
   cl_cciefb_cyclic_req_full_headers_t * header = nullptr;
   uint8_t buffer[100] = {}; /* sizeof(cl_cciefb_cyclic_req_full_headers_t)
                              */
   uint32_t ix;

   for (ix = 0; ix < sizeof (buffer); ix++)
   {
      buffer[ix] = (uint8_t)ix;
   }

   /* Parsing of valid content */
   EXPECT_EQ (
      cl_iefb_parse_req_full_cyclic_headers (
         (uint8_t *)&buffer,
         sizeof (buffer),
         &header),
      0);

   /* Request header */
   EXPECT_EQ (CC_FROM_BE16 (header->req_header.reserved1), 0x0001); /* BE */
   EXPECT_EQ (header->req_header.reserved2, 0x02);
   EXPECT_EQ (header->req_header.reserved3, 0x03);
   EXPECT_EQ (CC_FROM_LE16 (header->req_header.reserved4), 0x0504);
   EXPECT_EQ (header->req_header.reserved5, 0x06);
   EXPECT_EQ (CC_FROM_LE16 (header->req_header.dl), 0x0807);
   EXPECT_EQ (CC_FROM_LE16 (header->req_header.reserved6), 0x0A09);
   EXPECT_EQ (CC_FROM_LE16 (header->req_header.command), 0x0C0B);
   EXPECT_EQ (CC_FROM_LE16 (header->req_header.sub_command), 0x0E0D);

   EXPECT_EQ (CC_FROM_LE16 (header->cyclic_header.protocol_ver), 0x100F);
   EXPECT_EQ (CC_FROM_LE16 (header->cyclic_header.reserved1), 0x1211);
   EXPECT_EQ (CC_FROM_LE16 (header->cyclic_header.cyclic_info_offset_addr), 0x1413);
   for (ix = 0; ix < sizeof (header->cyclic_header.reserved2); ix++)
   {
      EXPECT_EQ (header->cyclic_header.reserved2[ix], ix + 0x15);
   }

   /* Master station notification */
   EXPECT_EQ (
      CC_FROM_LE16 (header->master_station_notification.master_local_unit_info),
      0x2423);
   EXPECT_EQ (CC_FROM_LE16 (header->master_station_notification.reserved), 0x2625);
   EXPECT_EQ (
      CC_FROM_LE64 (header->master_station_notification.clock_info),
      0x2E2D2C2B2A292827U);

   /* Cyclic data header */
   EXPECT_EQ (CC_FROM_LE32 (header->cyclic_data_header.master_id), 0x3231302FU);
   EXPECT_EQ (header->cyclic_data_header.group_no, 0x33U);
   EXPECT_EQ (header->cyclic_data_header.reserved3, 0x34U);
   EXPECT_EQ (CC_FROM_LE16 (header->cyclic_data_header.frame_sequence_no), 0x3635U);
   EXPECT_EQ (CC_FROM_LE16 (header->cyclic_data_header.timeout_value), 0x3837U);
   EXPECT_EQ (
      CC_FROM_LE16 (header->cyclic_data_header.parallel_off_timeout_count),
      0x3A39U);
   EXPECT_EQ (CC_FROM_LE16 (header->cyclic_data_header.parameter_no), 0x3C3BU);
   EXPECT_EQ (
      CC_FROM_LE16 (header->cyclic_data_header.slave_total_occupied_station_count),
      0x3E3DU);
   EXPECT_EQ (
      CC_FROM_LE16 (header->cyclic_data_header.cyclic_transmission_state),
      0x403FU);
   EXPECT_EQ (CC_FROM_LE16 (header->cyclic_data_header.reserved4), 0x4241U);

   /* Too short buffer.
   Buffer should be at least sizeof(cl_cciefb_cyclic_req_full_headers_t)
   which is 67 bytes according to BAP-C2010ENG-001-B */
   EXPECT_EQ (
      cl_iefb_parse_req_full_cyclic_headers ((uint8_t *)&buffer, 0, &header),
      -1);
   EXPECT_EQ (
      cl_iefb_parse_req_full_cyclic_headers ((uint8_t *)&buffer, 66, &header),
      -1);
   EXPECT_EQ (
      cl_iefb_parse_req_full_cyclic_headers ((uint8_t *)&buffer, 67, &header),
      0);
}

TEST_F (IefbUnitTest, CciefbValidateRequestFullCyclicHeaders)
{
   const cl_ipaddr_t ip   = 1;   /* 0.0.0.1 */
   const uint16_t udp_len = 143; /* For one occupied slave station */

   /* Fixed values according to BAP-C2010ENG-001-B */
   const uint16_t dl                           = udp_len - 9;
   cl_cciefb_cyclic_req_full_headers_t headers = {
      {
         .reserved1 = CC_TO_BE16 (0x5000), /* Big endian */
         .reserved2 = 0x00,
         .reserved3 = 0xFF,
         .reserved4 = CC_TO_LE16 (0x03FF),
         .reserved5 = 0x00,
         .dl        = CC_TO_LE16 (dl),
         .reserved6 = CC_TO_LE16 (0x0000),
         .command   = CC_TO_LE16 (0x0E70), /* CL_SLMP_COMMAND_CCIEFB_CYCLIC */
         /* CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC */
         .sub_command = CC_TO_LE16 (0x0000),
      },
      {
         .protocol_ver            = CC_TO_LE16 (1),
         .reserved1               = CC_TO_LE16 (0x0000),
         .cyclic_info_offset_addr = CC_TO_LE16 (36),
         .reserved2               = {},
      },
      {
         .master_local_unit_info = CC_TO_LE16 (0x0001),
         .reserved               = CC_TO_LE16 (0x0000),
         .clock_info             = CC_TO_LE64 (0x1234567890123456),
      },
      {.master_id                          = CC_TO_LE32 (ip),
       .group_no                           = 1,
       .reserved3                          = 0x00,
       .frame_sequence_no                  = CC_TO_LE16 (1),
       .timeout_value                      = CC_TO_LE16 (1),
       .parallel_off_timeout_count         = CC_TO_LE16 (1),
       .parameter_no                       = CC_TO_LE16 (1),
       .slave_total_occupied_station_count = CC_TO_LE16 (1),
       .cyclic_transmission_state          = CC_TO_LE16 (1),
       .reserved4                          = CC_TO_LE16 (0x0000)}};
   const cl_cciefb_cyclic_req_full_headers_t default_headers = headers;

   EXPECT_EQ (cl_iefb_validate_req_full_cyclic_headers (&headers, udp_len, ip), 0);

   /* The cl_cciefb_req_header_t is validated in another function */

   /* Example for cyclic request header */
   headers.cyclic_header.protocol_ver = CC_TO_LE16 (0);
   EXPECT_EQ (
      cl_iefb_validate_req_full_cyclic_headers (&headers, udp_len, ip),
      -1);
   headers = default_headers;
   EXPECT_EQ (cl_iefb_validate_req_full_cyclic_headers (&headers, udp_len, ip), 0);

   /* Example for master station notification */
   headers.master_station_notification.reserved = CC_TO_LE16 (0x1234);
   EXPECT_EQ (
      cl_iefb_validate_req_full_cyclic_headers (&headers, udp_len, ip),
      -1);
   headers = default_headers;
   EXPECT_EQ (cl_iefb_validate_req_full_cyclic_headers (&headers, udp_len, ip), 0);

   /* Example for cyclic data header */
   headers.cyclic_data_header.master_id = CC_TO_LE32 (CL_IPADDR_INVALID);
   EXPECT_EQ (
      cl_iefb_validate_req_full_cyclic_headers (&headers, udp_len, ip),
      -1);
   headers = default_headers;
   EXPECT_EQ (cl_iefb_validate_req_full_cyclic_headers (&headers, udp_len, ip), 0);

   /* Example for wrong frame size */
   EXPECT_EQ (
      cl_iefb_validate_req_full_cyclic_headers (&headers, udp_len + 1, ip),
      -1);
}

TEST_F (IefbUnitTest, CciefbParseRequestCyclicData)
{
   uint32_t * first_slave_id = nullptr;
   cl_rww_t * first_rww      = nullptr;
   cl_ry_t * first_ry        = nullptr;

   uint8_t buffer[3000] = {}; /* Should fit largest request frame */
   uint32_t ix;

   for (ix = 0; ix < sizeof (buffer); ix++)
   {
      buffer[ix] = (uint8_t)ix;
   }

   /* Parsing of valid content */
   EXPECT_EQ (
      cl_iefb_parse_request_cyclic_data (
         (uint8_t *)&buffer,
         sizeof (buffer),
         1,
         &first_slave_id,
         &first_rww,
         &first_ry),
      0);
   EXPECT_EQ (CC_FROM_LE32 (*first_slave_id), 0x46454443U);
   EXPECT_EQ (CC_FROM_LE16 (first_rww->words[0]), 0x4847U);
   EXPECT_EQ (CC_FROM_LE16 (first_rww->words[1]), 0x4A49U);
   EXPECT_EQ (CC_FROM_LE16 (first_rww->words[31]), 0x8685U);
   EXPECT_EQ (first_ry->bytes[0], 0x87);
   EXPECT_EQ (first_ry->bytes[1], 0x88);
   EXPECT_EQ (first_ry->bytes[2], 0x89);
   EXPECT_EQ (first_ry->bytes[3], 0x8A);
   EXPECT_EQ (first_ry->bytes[4], 0x8B);
   EXPECT_EQ (first_ry->bytes[5], 0x8C);
   EXPECT_EQ (first_ry->bytes[6], 0x8D);
   EXPECT_EQ (first_ry->bytes[7], 0x8E);

   /* Too short buffer.
   Buffer should be at least 143 bytes to fit one occupied slave station.
   See BAP-C2010ENG-001-B. */
   EXPECT_EQ (
      cl_iefb_parse_request_cyclic_data (
         (uint8_t *)&buffer,
         142,
         1,
         &first_slave_id,
         &first_rww,
         &first_ry),
      -1);
   EXPECT_EQ (
      cl_iefb_parse_request_cyclic_data (
         (uint8_t *)&buffer,
         143,
         1,
         &first_slave_id,
         &first_rww,
         &first_ry),
      0);

   /* Invalid total occupied stations */
   EXPECT_EQ (
      cl_iefb_parse_request_cyclic_data (
         (uint8_t *)&buffer,
         sizeof (buffer),
         0,
         &first_slave_id,
         &first_rww,
         &first_ry),
      -1);
   EXPECT_EQ (
      cl_iefb_parse_request_cyclic_data (
         (uint8_t *)&buffer,
         sizeof (buffer),
         17,
         &first_slave_id,
         &first_rww,
         &first_ry),
      -1);
   EXPECT_EQ (
      cl_iefb_parse_request_cyclic_data (
         (uint8_t *)&buffer,
         sizeof (buffer),
         16,
         &first_slave_id,
         &first_rww,
         &first_ry),
      0);
}

TEST_F (IefbUnitTest, CciefbRequestGetRww)
{
   cls_cciefb_cyclic_request_info_t request = {};
   cl_rww_t rww[4]                          = {};
   cl_cciefb_cyclic_req_full_headers_t headers;

   clal_clear_memory (&headers, sizeof (headers));
   request.first_rww    = (cl_rww_t *)&rww[0];
   request.full_headers = &headers;
   request.full_headers->cyclic_data_header.slave_total_occupied_station_count =
      CC_TO_LE16 (4);

   /* Valid pointers */
   EXPECT_EQ (cl_iefb_request_get_rww (&request, 1), (cl_rww_t *)&rww[0]);
   EXPECT_EQ (cl_iefb_request_get_rww (&request, 2), (cl_rww_t *)&rww[1]);
   EXPECT_EQ (cl_iefb_request_get_rww (&request, 3), (cl_rww_t *)&rww[2]);
   EXPECT_EQ (cl_iefb_request_get_rww (&request, 4), (cl_rww_t *)&rww[3]);

   /* Invalid absolute slave station */
   EXPECT_TRUE (cl_iefb_request_get_rww (&request, 0) == nullptr);
   EXPECT_TRUE (cl_iefb_request_get_rww (&request, 5) == nullptr);
   EXPECT_TRUE (cl_iefb_request_get_rww (&request, 17) == nullptr);
}

TEST_F (IefbUnitTest, CciefbRequestGetRy)
{
   cls_cciefb_cyclic_request_info_t request = {};
   cl_ry_t ry[4]                            = {};
   cl_cciefb_cyclic_req_full_headers_t headers;

   clal_clear_memory (&headers, sizeof (headers));
   request.first_ry     = (cl_ry_t *)&ry[0];
   request.full_headers = &headers;
   request.full_headers->cyclic_data_header.slave_total_occupied_station_count =
      CC_TO_LE16 (4);

   /* Valid pointers */
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 1), (cl_ry_t *)&ry[0]);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 2), (cl_ry_t *)&ry[1]);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 3), (cl_ry_t *)&ry[2]);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 4), (cl_ry_t *)&ry[3]);

   /* Invalid absolute slave station */
   EXPECT_TRUE (cl_iefb_request_get_ry (&request, 0) == nullptr);
   EXPECT_TRUE (cl_iefb_request_get_ry (&request, 5) == nullptr);
   EXPECT_TRUE (cl_iefb_request_get_ry (&request, 17) == nullptr);
}

/**
 * Validate generated response frame
 *
 * @req REQ_CL_PROTOCOL_02
 * @req REQ_CL_PROTOCOL_40
 * @req REQ_CL_PROTOCOL_41
 * @req REQ_CL_PROTOCOL_42
 * @req REQ_CL_PROTOCOL_43
 * @req REQ_CL_PROTOCOL_45
 * @req REQ_CL_PROTOCOL_50
 * @req REQ_CL_PROTOCOL_53
 * @req REQ_CL_PROTOCOL_56
 *
 */
TEST_F (IefbUnitTest, CciefbInitialiseUpdateCyclicResponseFrame)
{
   uint8_t buffer[400]                         = {};
   cls_cciefb_cyclic_response_info_t frameinfo = {};

   cl_iefb_initialise_cyclic_response_frame (
      (uint8_t *)&buffer,
      sizeof (buffer),
      2,          /* Occupied stations */
      0x1234,     /* Vendor code */
      0x56789ABC, /* Model code */
      0xCDEF,     /* Equipment version */
      &frameinfo);

   EXPECT_EQ (buffer[0], 0xD0); /* reserved1 */
   EXPECT_EQ (buffer[1], 0x00);
   EXPECT_EQ (buffer[2], 0x00);
   EXPECT_EQ (buffer[3], 0xFF);
   EXPECT_EQ (buffer[4], 0xFF);
   EXPECT_EQ (buffer[5], 0x03);
   EXPECT_EQ (buffer[6], 0x00);
   EXPECT_EQ (buffer[7], 194); /* Length for 2 occupied stations */
   EXPECT_EQ (buffer[8], 0x00);
   EXPECT_EQ (buffer[9], 0x00); /* reserved6 */
   EXPECT_EQ (buffer[10], 0x00);
   EXPECT_EQ (buffer[11], 2); /* Protocol version */
   EXPECT_EQ (buffer[12], 0x00);
   EXPECT_EQ (buffer[13], 0x00); /* End code */
   EXPECT_EQ (buffer[14], 0x00);
   EXPECT_EQ (buffer[15], 40); /* Cyclic offset*/
   EXPECT_EQ (buffer[16], 0x00);
   EXPECT_EQ (buffer[17], 0x00); /* OffsetAddrInfo reserved1 */
   EXPECT_EQ (buffer[18], 0x00);
   EXPECT_EQ (buffer[19], 0x00);
   EXPECT_EQ (buffer[20], 0x00);
   EXPECT_EQ (buffer[21], 0x00);
   EXPECT_EQ (buffer[22], 0x00);
   EXPECT_EQ (buffer[23], 0x00);
   EXPECT_EQ (buffer[24], 0x00);
   EXPECT_EQ (buffer[25], 0x00);
   EXPECT_EQ (buffer[26], 0x00);
   EXPECT_EQ (buffer[27], 0x00);
   EXPECT_EQ (buffer[28], 0x00);
   EXPECT_EQ (buffer[29], 0x00);
   EXPECT_EQ (buffer[30], 0x00);
   EXPECT_EQ (buffer[31], 0x34); /* Vendor code */
   EXPECT_EQ (buffer[32], 0x12);
   EXPECT_EQ (buffer[33], 0x00); /* SlaveNoticeInfo reserved1 */
   EXPECT_EQ (buffer[34], 0x00);
   EXPECT_EQ (buffer[35], 0xBC); /* Model code */
   EXPECT_EQ (buffer[36], 0x9A);
   EXPECT_EQ (buffer[37], 0x78);
   EXPECT_EQ (buffer[38], 0x56);
   EXPECT_EQ (buffer[39], 0xEF); /* Equipment version */
   EXPECT_EQ (buffer[40], 0xCD);
   EXPECT_EQ (buffer[41], 0x00); /* Reserved */
   EXPECT_EQ (buffer[42], 0x00);
   EXPECT_EQ (buffer[43], 0x01); /* Slave local unit info = operating */
   EXPECT_EQ (buffer[44], 0x00);
   EXPECT_EQ (buffer[45], 0x00); /* Slave err code */
   EXPECT_EQ (buffer[46], 0x00);
   EXPECT_EQ (buffer[47], 0x00); /* Local management info */
   EXPECT_EQ (buffer[48], 0x00);
   EXPECT_EQ (buffer[49], 0x00);
   EXPECT_EQ (buffer[50], 0x00);
   EXPECT_EQ (buffer[51], 0x00); /* Slave ID */
   EXPECT_EQ (buffer[52], 0x00);
   EXPECT_EQ (buffer[53], 0x00);
   EXPECT_EQ (buffer[54], 0x00);
   EXPECT_EQ (buffer[55], 0x00); /* Group */
   EXPECT_EQ (buffer[56], 0x00); /* CyclicInfo reserved2 */
   EXPECT_EQ (buffer[57], 0x00); /* Frame sequence number */
   EXPECT_EQ (buffer[58], 0x00);
   EXPECT_EQ (buffer[59], 0x00); /* RWr occupied 1 */

   cl_iefb_update_cyclic_response_frame (
      &frameinfo,
      0x01234567,                                /* slave_id */
      CL_SLMP_ENDCODE_DIVIDED_MESSAGE_DUPLICATE, /* end_code */
      0xBC,                                      /* group_no */
      0x2345,                                    /* frame_sequence_no */
      CL_SLAVE_APPL_OPERATION_STATUS_STOPPED,    /* slave_local_unit_info */
      0x789A,                                    /* slave_err_code */
      0x89ABCDEF                                 /* local_management_info */
   );

   EXPECT_EQ (buffer[0], 0xD0); /* Fixed values in response header */
   EXPECT_EQ (buffer[1], 0x00);
   EXPECT_EQ (buffer[2], 0x00);
   EXPECT_EQ (buffer[3], 0xFF);
   EXPECT_EQ (buffer[4], 0xFF);
   EXPECT_EQ (buffer[5], 0x03);
   EXPECT_EQ (buffer[6], 0x00);
   EXPECT_EQ (buffer[7], 194); /* Length for 2 occupied stations */
   EXPECT_EQ (buffer[8], 0x00);
   EXPECT_EQ (buffer[9], 0x00); /* Fixed value */
   EXPECT_EQ (buffer[10], 0x00);
   EXPECT_EQ (buffer[11], 2); /* Protocol version */
   EXPECT_EQ (buffer[12], 0x00);
   EXPECT_EQ (buffer[13], 0x41); /* End code UPDATED*/
   EXPECT_EQ (buffer[14], 0xCF);
   EXPECT_EQ (buffer[15], 40); /* Cyclic offset*/
   EXPECT_EQ (buffer[16], 0x00);
   EXPECT_EQ (buffer[17], 0x00); /*Reserved area */
   EXPECT_EQ (buffer[18], 0x00);
   EXPECT_EQ (buffer[19], 0x00);
   EXPECT_EQ (buffer[20], 0x00);
   EXPECT_EQ (buffer[21], 0x00);
   EXPECT_EQ (buffer[22], 0x00);
   EXPECT_EQ (buffer[23], 0x00);
   EXPECT_EQ (buffer[24], 0x00);
   EXPECT_EQ (buffer[25], 0x00);
   EXPECT_EQ (buffer[26], 0x00);
   EXPECT_EQ (buffer[27], 0x00);
   EXPECT_EQ (buffer[28], 0x00);
   EXPECT_EQ (buffer[29], 0x00);
   EXPECT_EQ (buffer[30], 0x00);
   EXPECT_EQ (buffer[31], 0x34); /* Vendor code */
   EXPECT_EQ (buffer[32], 0x12);
   EXPECT_EQ (buffer[33], 0x00); /* Reserved */
   EXPECT_EQ (buffer[34], 0x00);
   EXPECT_EQ (buffer[35], 0xBC); /* Model code */
   EXPECT_EQ (buffer[36], 0x9A);
   EXPECT_EQ (buffer[37], 0x78);
   EXPECT_EQ (buffer[38], 0x56);
   EXPECT_EQ (buffer[39], 0xEF); /* Equipment version */
   EXPECT_EQ (buffer[40], 0xCD);
   EXPECT_EQ (buffer[41], 0x00); /* Reserved */
   EXPECT_EQ (buffer[42], 0x00);
   EXPECT_EQ (buffer[43], 0x00); /* Slave local unit info UPDATED */
   EXPECT_EQ (buffer[44], 0x00);
   EXPECT_EQ (buffer[45], 0x9A); /* Slave err code UPDATED */
   EXPECT_EQ (buffer[46], 0x78);
   EXPECT_EQ (buffer[47], 0xEF); /* Local management info UPDATED*/
   EXPECT_EQ (buffer[48], 0xCD);
   EXPECT_EQ (buffer[49], 0xAB);
   EXPECT_EQ (buffer[50], 0x89);
   EXPECT_EQ (buffer[51], 0x67); /* Slave ID UPDATED */
   EXPECT_EQ (buffer[52], 0x45);
   EXPECT_EQ (buffer[53], 0x23);
   EXPECT_EQ (buffer[54], 0x01);
   EXPECT_EQ (buffer[55], 0xBC); /* Group UPDATED*/
   EXPECT_EQ (buffer[56], 0x00); /* Reserved */
   EXPECT_EQ (buffer[57], 0x45); /* Frame sequence number UPDATED */
   EXPECT_EQ (buffer[58], 0x23);
   EXPECT_EQ (buffer[59], 0x00); /* RWr occupied 1 */
}

TEST_F (IefbUnitTest, CciefbParseCyclicRequestFrame)
{
   cls_cciefb_cyclic_request_info_t request = {};
   const uint16_t expected_payload_length   = 295;
   const uint16_t expected_total_occupied   = 3;
   const uint16_t remote_port               = 123;
   const cl_ipaddr_t master_ip              = 0x01020304; /* IP 1.2.3.4 */
   const cl_ipaddr_t slave1_ip              = 0x01020305; /* IP 1.2.3.5 */
   const cl_ipaddr_t slave2_ip              = 0x01020306; /* IP 1.2.3.6 */
   const uint16_t group_number              = 1;
   const uint16_t timeout_value             = 0x0033;
   const uint16_t timeout_count             = 0x0055;
   const uint16_t parameter_no              = 0x8877;
   const uint16_t cyclic_transmission_state = 0x0003;
   uint8_t invalid_header_payload[SIZE_REQUEST_3_SLAVES] = {};
   cl_ipaddr_t slave_id                                  = CL_IPADDR_INVALID;
   const uint16_t frame_sequence_no                      = 0x2211;

   /* Prepare request frame with invalid header */
   clal_memcpy (
      invalid_header_payload,
      sizeof (invalid_header_payload),
      request_payload_running,
      SIZE_REQUEST_3_SLAVES);
   invalid_header_payload[15] = 7; /* Protocol version */

   /* Parse valid frame */
   EXPECT_EQ (
      cl_iefb_parse_cyclic_request (
         (uint8_t *)&request_payload_running,
         SIZE_REQUEST_3_SLAVES,
         master_ip,
         remote_port,
         slave_id,
         &request),
      0);

   EXPECT_EQ (request.remote_port, remote_port);
   EXPECT_EQ (request.remote_ip, master_ip);

   /* Request header */
   EXPECT_EQ (
      CC_FROM_LE16 (request.full_headers->req_header.dl),
      expected_payload_length - 9);
   EXPECT_EQ (
      CC_FROM_LE16 (request.full_headers->req_header.command),
      CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   EXPECT_EQ (
      CC_FROM_LE16 (request.full_headers->req_header.sub_command),
      CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);

   /* Cyclic header */
   EXPECT_EQ (CC_FROM_LE16 (request.full_headers->cyclic_header.protocol_ver), 2);

   /* Master station notification */
   EXPECT_EQ (
      CC_FROM_LE16 (
         request.full_headers->master_station_notification.master_local_unit_info),
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      CC_FROM_LE64 (request.full_headers->master_station_notification.clock_info),
      0xEFCDAB9078563412UL);

   /* Cyclic data header */
   EXPECT_EQ (
      CC_FROM_LE32 (request.full_headers->cyclic_data_header.master_id),
      master_ip);
   EXPECT_EQ (request.full_headers->cyclic_data_header.group_no, group_number);
   EXPECT_EQ (
      CC_FROM_LE16 (request.full_headers->cyclic_data_header.frame_sequence_no),
      frame_sequence_no);
   EXPECT_EQ (
      CC_FROM_LE16 (request.full_headers->cyclic_data_header.timeout_value),
      timeout_value);
   EXPECT_EQ (
      CC_FROM_LE16 (
         request.full_headers->cyclic_data_header.parallel_off_timeout_count),
      timeout_count);
   EXPECT_EQ (
      CC_FROM_LE16 (request.full_headers->cyclic_data_header.parameter_no),
      parameter_no);
   EXPECT_EQ (
      CC_FROM_LE16 (request.full_headers->cyclic_data_header
                       .slave_total_occupied_station_count),
      expected_total_occupied);
   EXPECT_EQ (
      CC_FROM_LE16 (
         request.full_headers->cyclic_data_header.cyclic_transmission_state),
      cyclic_transmission_state);

   /* Slave IP addresses */
   EXPECT_EQ (
      cl_iefb_request_get_slave_id (
         request.first_slave_id,
         1,
         expected_total_occupied,
         &slave_id),
      0);
   EXPECT_EQ (slave_id, slave1_ip);
   EXPECT_EQ (
      cl_iefb_request_get_slave_id (
         request.first_slave_id,
         2,
         expected_total_occupied,
         &slave_id),
      0);
   EXPECT_EQ (slave_id, slave2_ip);
   EXPECT_EQ (
      cl_iefb_request_get_slave_id (
         request.first_slave_id,
         3,
         expected_total_occupied,
         &slave_id),
      0);
   EXPECT_EQ (slave_id, CL_CCIEFB_MULTISTATION_INDICATOR); /* Used by prev */

   /* RWw slave 1 */
   EXPECT_EQ (CC_FROM_LE16 (request.first_rww->words[0]), 0x0011);
   EXPECT_EQ (CC_FROM_LE16 (request.first_rww->words[1]), 0x3412);
   EXPECT_EQ (CC_FROM_LE16 (request.first_rww->words[2]), 0x0000);
   EXPECT_EQ (
      CC_FROM_LE16 (cl_iefb_request_get_rww (&request, 1)->words[0]),
      0x0011);
   EXPECT_EQ (
      CC_FROM_LE16 (cl_iefb_request_get_rww (&request, 1)->words[1]),
      0x3412);
   EXPECT_EQ (
      CC_FROM_LE16 (cl_iefb_request_get_rww (&request, 1)->words[2]),
      0x000);

   /* RWw slave 2 */
   EXPECT_EQ (
      CC_FROM_LE16 (cl_iefb_request_get_rww (&request, 2)->words[0]),
      0x0022);
   EXPECT_EQ (
      CC_FROM_LE16 (cl_iefb_request_get_rww (&request, 2)->words[1]),
      0x3300);
   EXPECT_EQ (
      CC_FROM_LE16 (cl_iefb_request_get_rww (&request, 2)->words[2]),
      0x0000);

   /* RWw slave 3 */
   EXPECT_EQ (
      CC_FROM_LE16 (cl_iefb_request_get_rww (&request, 3)->words[0]),
      0x0044);
   EXPECT_EQ (
      CC_FROM_LE16 (cl_iefb_request_get_rww (&request, 3)->words[1]),
      0x5500);
   EXPECT_EQ (
      CC_FROM_LE16 (cl_iefb_request_get_rww (&request, 3)->words[2]),
      0x0000);
   EXPECT_EQ (
      CC_FROM_LE16 (cl_iefb_request_get_rww (&request, 3)->words[31]),
      0x6600);

   /* RY slave 1 */
   EXPECT_EQ (request.first_ry->bytes[0], 0x01);
   EXPECT_EQ (request.first_ry->bytes[1], 0x00);
   EXPECT_EQ (request.first_ry->bytes[2], 0x00);
   EXPECT_EQ (request.first_ry->bytes[3], 0x00);
   EXPECT_EQ (request.first_ry->bytes[4], 0x00);
   EXPECT_EQ (request.first_ry->bytes[5], 0x00);
   EXPECT_EQ (request.first_ry->bytes[6], 0x00);
   EXPECT_EQ (request.first_ry->bytes[7], 0x00);

   EXPECT_EQ (cl_iefb_request_get_ry (&request, 1)->bytes[0], 0x01);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 1)->bytes[1], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 1)->bytes[2], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 1)->bytes[3], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 1)->bytes[4], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 1)->bytes[5], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 1)->bytes[6], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 1)->bytes[7], 0x00);

   /* RY slave 2 */
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 2)->bytes[0], 0xFF);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 2)->bytes[1], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 2)->bytes[2], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 2)->bytes[3], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 2)->bytes[4], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 2)->bytes[5], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 2)->bytes[6], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 2)->bytes[7], 0x00);

   /* RY slave 3 */
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 3)->bytes[0], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 3)->bytes[1], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 3)->bytes[2], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 3)->bytes[3], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 3)->bytes[4], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 3)->bytes[5], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 3)->bytes[6], 0x00);
   EXPECT_EQ (cl_iefb_request_get_ry (&request, 3)->bytes[7], 0x08);

   /* Too small buffer */
   EXPECT_EQ (
      cl_iefb_parse_cyclic_request (
         (uint8_t *)&request_payload_running,
         1,
         master_ip,
         remote_port,
         slave_id,
         &request),
      -1);

   /* Invalid header */
   EXPECT_EQ (
      cl_iefb_parse_cyclic_request (
         (uint8_t *)&invalid_header_payload,
         SIZE_REQUEST_3_SLAVES,
         master_ip,
         remote_port,
         slave_id,
         &request),
      -1);
}

TEST_F (IefbUnitTest, CciefbRequestGetRxArea)
{
   cl_rx_t rx_areas[4]                        = {};
   cls_cciefb_cyclic_response_info_t response = {};
   cl_cciefb_cyclic_resp_full_headers_t headers;

   clal_clear_memory (&headers, sizeof (headers));
   response.first_rx     = (cl_rx_t *)&rx_areas[0];
   response.full_headers = &headers;

   /* Valid pointers */
   EXPECT_EQ (cl_iefb_get_rx_area (&response, 0), (cl_rx_t *)&rx_areas[0]);
   EXPECT_EQ (cl_iefb_get_rx_area (&response, 1), (cl_rx_t *)&rx_areas[1]);
   EXPECT_EQ (cl_iefb_get_rx_area (&response, 2), (cl_rx_t *)&rx_areas[2]);
   EXPECT_EQ (cl_iefb_get_rx_area (&response, 3), (cl_rx_t *)&rx_areas[3]);
}

TEST_F (IefbUnitTest, CciefbRequestGetRwrArea)
{
   cl_rwr_t rwr_areas[4]                      = {};
   cls_cciefb_cyclic_response_info_t response = {};
   cl_cciefb_cyclic_resp_full_headers_t headers;

   clal_clear_memory (&headers, sizeof (headers));
   response.first_rwr    = (cl_rwr_t *)&rwr_areas[0];
   response.full_headers = &headers;

   /* Valid pointers */
   EXPECT_EQ (cl_iefb_get_rwr_area (&response, 0), (cl_rwr_t *)&rwr_areas[0]);
   EXPECT_EQ (cl_iefb_get_rwr_area (&response, 1), (cl_rwr_t *)&rwr_areas[1]);
   EXPECT_EQ (cl_iefb_get_rwr_area (&response, 2), (cl_rwr_t *)&rwr_areas[2]);
   EXPECT_EQ (cl_iefb_get_rwr_area (&response, 3), (cl_rwr_t *)&rwr_areas[3]);
}

TEST_F (IefbUnitTest, CciefbParseResponseHeader)
{
   cl_cciefb_resp_header_t * header = nullptr;

   uint8_t buffer[50] = {}; /* Larger than sizeof(cl_cciefb_resp_header_t) */
   uint32_t ix;
   for (ix = 0; ix < sizeof (buffer); ix++)
   {
      buffer[ix] = (uint8_t)ix;
   }

   /* Parsing of valid content */
   EXPECT_EQ (
      cl_iefb_parse_response_header ((uint8_t *)&buffer, sizeof (buffer), &header),
      0);
   EXPECT_EQ (CC_FROM_BE16 (header->reserved1), 0x0001); /* Big endian */
   EXPECT_EQ (header->reserved2, 0x02);
   EXPECT_EQ (header->reserved3, 0x03);
   EXPECT_EQ (CC_FROM_LE16 (header->reserved4), 0x0504);
   EXPECT_EQ (header->reserved5, 0x06);
   EXPECT_EQ (CC_FROM_LE16 (header->dl), 0x0807);
   EXPECT_EQ (CC_FROM_LE16 (header->reserved6), 0x0A09);

   /* Too short buffer.
      Buffer should be at least sizeof(cl_cciefb_resp_header_t) which is 11
      bytes according to BAP-C2010ENG-001-B */
   EXPECT_EQ (cl_iefb_parse_response_header ((uint8_t *)&buffer, 0, &header), -1);
   EXPECT_EQ (cl_iefb_parse_response_header ((uint8_t *)&buffer, 10, &header), -1);
   EXPECT_EQ (cl_iefb_parse_response_header ((uint8_t *)&buffer, 11, &header), 0);
}

TEST_F (IefbUnitTest, CciefbValidateResponseHeader)
{
   const uint16_t udp_len = 100; /* arbitrary value */

   /* Fixed values according to BAP-C2010ENG-001-B */
   const uint16_t dl              = udp_len - 9;
   cl_cciefb_resp_header_t header = {
      .reserved1 = CC_TO_BE16 (0xD000), /* Big endian */
      .reserved2 = 0x00,
      .reserved3 = 0xFF,
      .reserved4 = CC_TO_LE16 (0x03FF),
      .reserved5 = 0x00,
      .dl        = CC_TO_LE16 (dl),
      .reserved6 = CC_TO_LE16 (0x0000),
   };
   const cl_cciefb_resp_header_t default_header = header;

   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), 0);

   /* Length */
   header.dl = CC_TO_LE16 (0x0000);
   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), 0);

   /* Reserved1 */
   header.reserved1 = CC_TO_BE16 (0x0000);
   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), 0);

   /* Reserved2 */
   header.reserved2 = 0x12;
   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), 0);

   /* Reserved3 */
   header.reserved3 = 0x12;
   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), 0);

   /* Reserved4 */
   header.reserved4 = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), 0);

   /* Reserved5 */
   header.reserved5 = 0x12;
   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), 0);

   /* Reserved6 */
   header.reserved6 = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_response_header (&header, udp_len), 0);
}

TEST_F (IefbUnitTest, CciefbParseResponseFullCyclicHeaders)
{
   cl_cciefb_cyclic_resp_full_headers_t * header = nullptr;
   uint8_t buffer[100] = {}; /* cl_cciefb_cyclic_resp_full_headers_t */
   uint32_t ix;

   for (ix = 0; ix < sizeof (buffer); ix++)
   {
      buffer[ix] = (uint8_t)ix;
   }

   /* Parsing of valid content */
   EXPECT_EQ (
      cl_iefb_parse_resp_full_cyclic_headers (
         (uint8_t *)&buffer,
         sizeof (buffer),
         &header),
      0);

   /* Response header */
   EXPECT_EQ (CC_FROM_BE16 (header->resp_header.reserved1), 0x0001); /* BE */
   EXPECT_EQ (header->resp_header.reserved2, 0x02);
   EXPECT_EQ (header->resp_header.reserved3, 0x03);
   EXPECT_EQ (CC_FROM_LE16 (header->resp_header.reserved4), 0x0504);
   EXPECT_EQ (header->resp_header.reserved5, 0x06);
   EXPECT_EQ (CC_FROM_LE16 (header->resp_header.dl), 0x0807);
   EXPECT_EQ (CC_FROM_LE16 (header->resp_header.reserved6), 0x0A09);

   EXPECT_EQ (CC_FROM_LE16 (header->cyclic_header.protocol_ver), 0x0C0B);
   EXPECT_EQ (CC_FROM_LE16 (header->cyclic_header.end_code), 0x0E0D);
   EXPECT_EQ (CC_FROM_LE16 (header->cyclic_header.cyclic_info_offset_addr), 0x100F);
   for (ix = 0; ix < sizeof (header->cyclic_header.reserved1); ix++)
   {
      EXPECT_EQ (header->cyclic_header.reserved1[ix], ix + 0x11);
   }

   /* Slave station notification */
   EXPECT_EQ (
      CC_FROM_LE16 (header->slave_station_notification.vendor_code),
      0x201F);
   EXPECT_EQ (CC_FROM_LE16 (header->slave_station_notification.reserved1), 0x2221);
   EXPECT_EQ (
      CC_FROM_LE32 (header->slave_station_notification.model_code),
      0x26252423U);
   EXPECT_EQ (
      CC_FROM_LE16 (header->slave_station_notification.equipment_ver),
      0x2827);
   EXPECT_EQ (CC_FROM_LE16 (header->slave_station_notification.reserved2), 0x2A29);
   EXPECT_EQ (
      CC_FROM_LE16 (header->slave_station_notification.slave_local_unit_info),
      0x2C2B);
   EXPECT_EQ (
      CC_FROM_LE16 (header->slave_station_notification.slave_err_code),
      0x2E2D);
   EXPECT_EQ (
      CC_FROM_LE32 (header->slave_station_notification.local_management_info),
      0x3231302FU);

   /* Cyclic data header */
   EXPECT_EQ (CC_FROM_LE32 (header->cyclic_data_header.slave_id), 0x36353433U);
   EXPECT_EQ (header->cyclic_data_header.group_no, 0x37);
   EXPECT_EQ (header->cyclic_data_header.reserved2, 0x38);
   EXPECT_EQ (CC_FROM_LE16 (header->cyclic_data_header.frame_sequence_no), 0x3A39);

   /* Too short buffer.
   Buffer should be at least sizeof(cl_cciefb_cyclic_resp_full_headers_t)
   which is 59 bytes according to BAP-C2010ENG-001-B */
   EXPECT_EQ (
      cl_iefb_parse_resp_full_cyclic_headers ((uint8_t *)&buffer, 0, &header),
      -1);
   EXPECT_EQ (
      cl_iefb_parse_resp_full_cyclic_headers ((uint8_t *)&buffer, 58, &header),
      -1);
   EXPECT_EQ (
      cl_iefb_parse_resp_full_cyclic_headers ((uint8_t *)&buffer, 59, &header),
      0);
}

TEST_F (IefbUnitTest, CciefbValidateCyclicResponseHeader)
{

   /* Fixed values according to BAP-C2010ENG-001-B */
   cl_cciefb_cyclic_resp_header_t header = {
      .protocol_ver            = CC_TO_LE16 (1),
      .end_code                = CC_TO_LE16 (0x0000),
      .cyclic_info_offset_addr = CC_TO_LE16 (40),
      .reserved1               = {},
   };
   const cl_cciefb_cyclic_resp_header_t default_header = header;

   EXPECT_EQ (cl_iefb_validate_cyclic_resp_header (&header), 0);

   /* Protocol version */
   header.protocol_ver = CC_TO_LE16 (0);
   EXPECT_EQ (cl_iefb_validate_cyclic_resp_header (&header), -1);
   header.protocol_ver = CC_TO_LE16 (3);
   EXPECT_EQ (cl_iefb_validate_cyclic_resp_header (&header), -1);
   header.protocol_ver = CC_TO_LE16 (2);
   EXPECT_EQ (cl_iefb_validate_cyclic_resp_header (&header), 0);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_cyclic_resp_header (&header), 0);

   /* Cyclic offset */
   header.cyclic_info_offset_addr = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_iefb_validate_cyclic_resp_header (&header), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_cyclic_resp_header (&header), 0);

   /* Reserved1 */
   header.reserved1[0] = 0x12;
   EXPECT_EQ (cl_iefb_validate_cyclic_resp_header (&header), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_cyclic_resp_header (&header), 0);
}

TEST_F (IefbUnitTest, CciefbValidateSlaveStationNotification)
{
   /* Fixed values according to BAP-C2010ENG-001-B */
   cl_cciefb_slave_station_notification_t notification = {
      .vendor_code           = CC_TO_LE16 (0xAAAA),
      .reserved1             = CC_TO_LE16 (0x0000),
      .model_code            = CC_TO_LE32 (0xBBBBBBBB),
      .equipment_ver         = CC_TO_LE16 (0xCCCC),
      .reserved2             = CC_TO_LE16 (0x0000),
      .slave_local_unit_info = CC_TO_LE16 (0x0001),
      .slave_err_code        = CC_TO_LE16 (0xDDDD),
      .local_management_info = CC_TO_LE32 (0xEEEE)};
   const cl_cciefb_slave_station_notification_t default_notification =
      notification;

   EXPECT_EQ (cl_iefb_validate_slave_station_notification (&notification), 0);

   /* Reserved1 */
   notification.reserved1 = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_iefb_validate_slave_station_notification (&notification), -1);
   notification = default_notification;
   EXPECT_EQ (cl_iefb_validate_slave_station_notification (&notification), 0);

   /* Reserved2 */
   notification.reserved2 = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_iefb_validate_slave_station_notification (&notification), -1);
   notification = default_notification;
   EXPECT_EQ (cl_iefb_validate_slave_station_notification (&notification), 0);

   /* Slave local unit info */
   notification.slave_local_unit_info = CC_TO_LE16 (0x0002);
   EXPECT_EQ (cl_iefb_validate_slave_station_notification (&notification), -1);
   notification.slave_local_unit_info = CC_TO_LE16 (0x0003);
   EXPECT_EQ (cl_iefb_validate_slave_station_notification (&notification), -1);
   notification = default_notification;
   EXPECT_EQ (cl_iefb_validate_slave_station_notification (&notification), 0);
}

TEST_F (IefbUnitTest, CciefbValidateCyclicResponseDataHeader)
{
   const cl_ipaddr_t ip  = 1; /* 0.0.0.1 */
   const cl_ipaddr_t ip2 = 2; /* 0.0.0.2 */

   /* Fixed values according to BAP-C2010ENG-001-B */
   cl_cciefb_cyclic_resp_data_header_t header = {
      .slave_id          = CC_TO_LE32 (ip),
      .group_no          = 1,
      .reserved2         = 0x00,
      .frame_sequence_no = CC_TO_LE16 (1)};
   const cl_cciefb_cyclic_resp_data_header_t default_header = header;

   EXPECT_EQ (cl_iefb_validate_resp_cyclic_data_header (&header, ip), 0);

   /* Slave ID */
   header.slave_id = CC_TO_LE32 (CL_IPADDR_INVALID);
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_data_header (&header, ip), -1);
   header.slave_id = CC_TO_LE32 (ip2);
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_data_header (&header, ip), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_data_header (&header, ip), 0);
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_data_header (&header, ip2), -1);
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_data_header (&header, ip), 0);

   /* Group no */
   header.group_no = 0;
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_data_header (&header, ip), -1);
   header.group_no = 65;
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_data_header (&header, ip), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_data_header (&header, ip), 0);

   /* Reserved2 */
   header.reserved2 = 0x12;
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_data_header (&header, ip), -1);
   header = default_header;
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_data_header (&header, ip), 0);

   /* All values are allowed for frame_sequence_no */
}

TEST_F (IefbUnitTest, CciefbValidateCyclicRespFrameSize)
{
   /* Protocol version */
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_frame_size (0, 131, 122), -1);
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_frame_size (1, 131, 122), 0);
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_frame_size (2, 131, 122), 0);
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_frame_size (3, 131, 122), -1);

   /* Wrong recv_len */
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_frame_size (2, 130, 122), -1);
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_frame_size (2, 132, 122), -1);

   /* Wrong dl */
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_frame_size (2, 131, 121), -1);
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_frame_size (2, 131, 123), -1);

   /* Wrong recv_len and dl, but their difference is OK */
   EXPECT_EQ (cl_iefb_validate_resp_cyclic_frame_size (1, 132, 123), 0);
}

TEST_F (IefbUnitTest, CciefbValidateResponseFullCyclicHeaders)
{
   const cl_ipaddr_t ip   = 1;   /* 0.0.0.1 */
   const uint16_t udp_len = 131; /* For one occupied slave station */

   /* Fixed values according to BAP-C2010ENG-001-B */
   const uint16_t dl                            = udp_len - 9;
   cl_cciefb_cyclic_resp_full_headers_t headers = {
      {
         .reserved1 = CC_TO_BE16 (0xD000), /* Big endian */
         .reserved2 = 0x00,
         .reserved3 = 0xFF,
         .reserved4 = CC_TO_LE16 (0x03FF),
         .reserved5 = 0x00,
         .dl        = CC_TO_LE16 (dl),
         .reserved6 = CC_TO_LE16 (0x0000),
      },
      {
         .protocol_ver            = CC_TO_LE16 (1),
         .end_code                = CC_TO_LE16 (0x0000),
         .cyclic_info_offset_addr = CC_TO_LE16 (40),
         .reserved1               = {},
      },
      {
         .vendor_code           = CC_TO_LE16 (0xAAAA),
         .reserved1             = CC_TO_LE16 (0x0000),
         .model_code            = CC_TO_LE32 (0xBBBBBBBB),
         .equipment_ver         = CC_TO_LE16 (0xCCCC),
         .reserved2             = CC_TO_LE16 (0x0000),
         .slave_local_unit_info = CC_TO_LE16 (0x0001),
         .slave_err_code        = CC_TO_LE16 (0xDDDD),
         .local_management_info = CC_TO_LE32 (0xEEEE),
      },
      {.slave_id          = CC_TO_LE32 (ip),
       .group_no          = 1,
       .reserved2         = 0x00,
       .frame_sequence_no = CC_TO_LE16 (1)}};
   const cl_cciefb_cyclic_resp_full_headers_t default_headers = headers;

   EXPECT_EQ (cl_iefb_validate_resp_full_cyclic_headers (&headers, udp_len, ip), 0);

   /* The cl_cciefb_req_header_t is validated in another function */

   /* Example for cyclic response header */
   headers.cyclic_header.protocol_ver = CC_TO_LE16 (0);
   EXPECT_EQ (
      cl_iefb_validate_resp_full_cyclic_headers (&headers, udp_len, ip),
      -1);
   headers = default_headers;
   EXPECT_EQ (cl_iefb_validate_resp_full_cyclic_headers (&headers, udp_len, ip), 0);

   /* Example for slave station notification */
   headers.slave_station_notification.reserved1 = CC_TO_LE16 (0x0102);
   EXPECT_EQ (
      cl_iefb_validate_resp_full_cyclic_headers (&headers, udp_len, ip),
      -1);
   headers = default_headers;
   EXPECT_EQ (cl_iefb_validate_resp_full_cyclic_headers (&headers, udp_len, ip), 0);

   /* Example for cyclic data header */
   headers.cyclic_data_header.slave_id = CC_TO_LE32 (CL_IPADDR_INVALID);
   EXPECT_EQ (
      cl_iefb_validate_resp_full_cyclic_headers (&headers, udp_len, ip),
      -1);
   headers = default_headers;
   EXPECT_EQ (cl_iefb_validate_resp_full_cyclic_headers (&headers, udp_len, ip), 0);

   /* Example for wrong frame size */
   EXPECT_EQ (
      cl_iefb_validate_resp_full_cyclic_headers (&headers, udp_len + 1, ip),
      -1);
}

TEST_F (IefbUnitTest, CciefbParseResponseCyclicData)
{
   cl_rwr_t * first_rwr = nullptr;
   cl_rx_t * first_rx   = nullptr;

   uint8_t buffer[3000] = {}; /* Should fit largest response frame */
   uint32_t ix;

   for (ix = 0; ix < sizeof (buffer); ix++)
   {
      buffer[ix] = (uint8_t)ix;
   }

   /* Parsing of valid content */
   EXPECT_EQ (
      cl_iefb_parse_response_cyclic_data (
         (uint8_t *)&buffer,
         sizeof (buffer),
         1,
         &first_rwr,
         &first_rx),
      0);
   EXPECT_EQ (CC_FROM_LE16 (first_rwr->words[0]), 0x3C3BU);
   EXPECT_EQ (CC_FROM_LE16 (first_rwr->words[1]), 0x3E3DU);
   EXPECT_EQ (CC_FROM_LE16 (first_rwr->words[31]), 0x7A79U);
   EXPECT_EQ (first_rx->bytes[0], 0x7B);
   EXPECT_EQ (first_rx->bytes[1], 0x7C);
   EXPECT_EQ (first_rx->bytes[2], 0x7D);
   EXPECT_EQ (first_rx->bytes[3], 0x7E);
   EXPECT_EQ (first_rx->bytes[4], 0x7F);
   EXPECT_EQ (first_rx->bytes[5], 0x80);
   EXPECT_EQ (first_rx->bytes[6], 0x81);
   EXPECT_EQ (first_rx->bytes[7], 0x82);

   /* Too short buffer.
     Buffer should be at least 131 bytes to fit one occupied slave station.
     See BAP-C2010ENG-001-B. */
   EXPECT_EQ (
      cl_iefb_parse_response_cyclic_data (
         (uint8_t *)&buffer,
         130,
         1,
         &first_rwr,
         &first_rx),
      -1);
   EXPECT_EQ (
      cl_iefb_parse_response_cyclic_data (
         (uint8_t *)&buffer,
         131,
         1,
         &first_rwr,
         &first_rx),
      0);

   /* Invalid total occupied stations */
   EXPECT_EQ (
      cl_iefb_parse_response_cyclic_data (
         (uint8_t *)&buffer,
         sizeof (buffer),
         0,
         &first_rwr,
         &first_rx),
      -1);
   EXPECT_EQ (
      cl_iefb_parse_response_cyclic_data (
         (uint8_t *)&buffer,
         sizeof (buffer),
         17,
         &first_rwr,
         &first_rx),
      -1);
   EXPECT_EQ (
      cl_iefb_parse_response_cyclic_data (
         (uint8_t *)&buffer,
         sizeof (buffer),
         16,
         &first_rwr,
         &first_rx),
      0);
}

TEST_F (IefbUnitTest, CciefbParseCyclicResponseFrame)
{
   clm_cciefb_cyclic_response_info_t response                   = {};
   uint8_t invalid_header_payload[SIZE_RESPONSE_2_SLAVES]       = {};
   uint8_t invalid_num_stations_payload[SIZE_RESPONSE_2_SLAVES] = {};
   const uint32_t timestamp                                     = 0x62636465;
   const uint16_t remote_port                                   = 123;
   const cl_ipaddr_t remote_ip          = 0x01020306; /* IP 1.2.3.6 */
   const uint16_t group_no              = 1;
   const uint16_t protocol_ver          = 2;
   const uint16_t slave_err_code        = 0x3839;
   const uint16_t frame_sequence_no     = 0x0000;
   const uint16_t vendor_code           = 0x3456;
   const uint32_t model_code            = 0x789ABCDE;
   const uint16_t equipment_ver         = 0xF012;
   const uint32_t local_management_info = 0x23242526;

   /* Prepare response frame with invalid header */
   clal_memcpy (
      invalid_header_payload,
      sizeof (invalid_header_payload),
      response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   invalid_header_payload[11] = 7; /* Protocol version */

   /* Prepare response frame with invalid number of stations */
   clal_memcpy (
      invalid_num_stations_payload,
      sizeof (invalid_num_stations_payload),
      response_payload_di1,
      SIZE_RESPONSE_2_SLAVES);
   invalid_num_stations_payload[7] = 0xC3; /* DL 1 byte longer*/

   /* Parse valid frame */
   EXPECT_EQ (
      cl_iefb_parse_cyclic_response (
         (uint8_t *)&response_payload_di1,
         SIZE_RESPONSE_2_SLAVES,
         remote_ip,
         remote_port,
         timestamp,
         &response),
      0);

   EXPECT_EQ (response.remote_port, remote_port);
   EXPECT_EQ (response.remote_ip, remote_ip);
   EXPECT_EQ (response.reception_timestamp, timestamp);

   /* Response header */
   EXPECT_EQ (
      CC_FROM_LE16 (response.full_headers->resp_header.dl),
      SIZE_RESPONSE_2_SLAVES - 9);

   /* Cyclic header */
   EXPECT_EQ (
      CC_FROM_LE16 (response.full_headers->cyclic_header.protocol_ver),
      protocol_ver);
   EXPECT_EQ (
      CC_FROM_LE16 (response.full_headers->cyclic_header.end_code),
      CL_SLMP_ENDCODE_SUCCESS);

   /* Slave station notification */
   EXPECT_EQ (
      CC_FROM_LE16 (response.full_headers->slave_station_notification.vendor_code),
      vendor_code);
   EXPECT_EQ (
      CC_FROM_LE32 (response.full_headers->slave_station_notification.model_code),
      model_code);
   EXPECT_EQ (
      CC_FROM_LE16 (
         response.full_headers->slave_station_notification.equipment_ver),
      equipment_ver);
   EXPECT_EQ (
      CC_FROM_LE16 (
         response.full_headers->slave_station_notification.slave_local_unit_info),
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   EXPECT_EQ (
      CC_FROM_LE16 (
         response.full_headers->slave_station_notification.slave_err_code),
      slave_err_code);
   EXPECT_EQ (
      CC_FROM_LE32 (
         response.full_headers->slave_station_notification.local_management_info),
      local_management_info);
   /* Cyclic data header */
   EXPECT_EQ (
      CC_FROM_LE32 (response.full_headers->cyclic_data_header.slave_id),
      remote_ip);
   EXPECT_EQ (response.full_headers->cyclic_data_header.group_no, group_no);
   EXPECT_EQ (
      CC_FROM_LE16 (response.full_headers->cyclic_data_header.frame_sequence_no),
      frame_sequence_no);

   /* RWr occupied 1 */
   EXPECT_EQ (CC_FROM_LE16 (response.first_rwr->words[0]), 0x0100);
   EXPECT_EQ (CC_FROM_LE16 (response.first_rwr->words[1]), 0x0302);
   EXPECT_EQ (CC_FROM_LE16 (response.first_rwr->words[2]), 0x0504);
   EXPECT_EQ (CC_FROM_LE16 (response.first_rwr->words[31]), 0x3F3E);

   /* RWr occupied 2 */
   EXPECT_EQ (CC_FROM_LE16 ((response.first_rwr + 1)->words[0]), 0x4140);
   EXPECT_EQ (CC_FROM_LE16 ((response.first_rwr + 1)->words[1]), 0x4342);
   EXPECT_EQ (CC_FROM_LE16 ((response.first_rwr + 1)->words[2]), 0x4544);
   EXPECT_EQ (CC_FROM_LE16 ((response.first_rwr + 1)->words[31]), 0x7F7E);

   /* RX occupied 1 */
   EXPECT_EQ (response.first_rx->bytes[0], 0x80);
   EXPECT_EQ (response.first_rx->bytes[1], 0x81);
   EXPECT_EQ (response.first_rx->bytes[2], 0x82);
   EXPECT_EQ (response.first_rx->bytes[3], 0x83);
   EXPECT_EQ (response.first_rx->bytes[4], 0x84);
   EXPECT_EQ (response.first_rx->bytes[5], 0x85);
   EXPECT_EQ (response.first_rx->bytes[6], 0x86);
   EXPECT_EQ (response.first_rx->bytes[7], 0x87);

   /* RX occupied 2 */
   EXPECT_EQ ((response.first_rx + 1)->bytes[0], 0x88);
   EXPECT_EQ ((response.first_rx + 1)->bytes[1], 0x89);
   EXPECT_EQ ((response.first_rx + 1)->bytes[2], 0x8A);
   EXPECT_EQ ((response.first_rx + 1)->bytes[3], 0x8B);
   EXPECT_EQ ((response.first_rx + 1)->bytes[4], 0x8C);
   EXPECT_EQ ((response.first_rx + 1)->bytes[5], 0x8D);
   EXPECT_EQ ((response.first_rx + 1)->bytes[6], 0x8E);
   EXPECT_EQ ((response.first_rx + 1)->bytes[7], 0x8F);

   /* Too small buffer */
   EXPECT_EQ (
      cl_iefb_parse_cyclic_response (
         (uint8_t *)&response_payload_di1,
         1,
         remote_ip,
         remote_port,
         timestamp,
         &response),
      -1);

   /* Invalid header */
   EXPECT_EQ (
      cl_iefb_parse_cyclic_response (
         (uint8_t *)&invalid_header_payload,
         SIZE_RESPONSE_2_SLAVES,
         remote_ip,
         remote_port,
         timestamp,
         &response),
      -1);

   /* Invalid number of occupied */
   EXPECT_EQ (
      cl_iefb_parse_cyclic_response (
         (uint8_t *)&invalid_num_stations_payload,
         SIZE_RESPONSE_2_SLAVES + 1,
         remote_ip,
         remote_port,
         timestamp,
         &response),
      -1);
}

/************************* Sanity check *************************************/

TEST_F (IefbUnitTest, CciefbSanityCyclicRequest)
{
   uint8_t buffer[400]                                 = {0};
   clm_cciefb_cyclic_request_info_t outgoing_frameinfo = {};
   const uint16_t protocol_ver                         = 2;
   const cl_ipaddr_t master_ip_addr          = 0x01020304UL; /* 1.2.3.4 */
   const uint16_t remote_port                = 0x5354;
   const cl_ipaddr_t slave_ip_addr           = 0x01020306L; /* 1.2.3.6 */
   const cl_ipaddr_t other_slave_ip_addr     = 0x01020305L; /* 1.2.3.5 */
   const uint16_t group_no                   = 0x2F;
   const uint16_t parameter_no               = 0x9998;
   const uint16_t parallel_off_timeout_count = 0x7576;
   const uint16_t timeout_value              = 0x6968;
   const uint16_t group_occupied_stations    = 3;
   const uint16_t frame_sequence_no          = 0x2345;
   const uint64_t unix_timestamp_ms          = 0x0405060708090A0B;
   const uint16_t cyclic_transmission_state  = 0x7172;
   const uint16_t sdi = 1; /* slave_device_index (for slave station 2) */
   const uint16_t master_local_unit_info =
      CL_CCIEFB_MASTER_LOCAL_UNIT_INFO_RUNNING;
   cls_cciefb_cyclic_request_info_t result;

   clal_clear_memory (&result, sizeof (result));

   /* Prepare frame */
   cl_iefb_initialise_request_frame (
      (uint8_t *)&buffer,
      sizeof (buffer),
      protocol_ver,
      timeout_value,
      parallel_off_timeout_count,
      master_ip_addr,
      group_no,
      group_occupied_stations,
      parameter_no,
      &outgoing_frameinfo);

   cl_iefb_update_request_frame_headers (
      &outgoing_frameinfo,
      frame_sequence_no,
      unix_timestamp_ms,
      master_local_unit_info,
      cyclic_transmission_state);

   outgoing_frameinfo.first_slave_id[0] = CC_TO_LE32 (other_slave_ip_addr);
   outgoing_frameinfo.first_slave_id[1] = CC_TO_LE32 (slave_ip_addr);
   outgoing_frameinfo.first_slave_id[2] =
      CC_TO_LE32 (CL_CCIEFB_MULTISTATION_INDICATOR);
   outgoing_frameinfo.first_rww[sdi].words[0] = CC_TO_LE16 (0xA000);
   outgoing_frameinfo.first_rww[sdi].words[1] = CC_TO_LE16 (0xA001);
   outgoing_frameinfo.first_rww[sdi].words[2] = CC_TO_LE16 (0xA002);
   outgoing_frameinfo.first_ry[sdi].bytes[0]  = 0xB0;
   outgoing_frameinfo.first_ry[sdi].bytes[1]  = 0xB1;
   outgoing_frameinfo.first_ry[sdi].bytes[2]  = 0xB2;

   /* Parse frame */
   ASSERT_EQ (
      cl_iefb_parse_cyclic_request (
         (uint8_t *)&buffer,
         cl_calculate_cyclic_request_size (group_occupied_stations),
         master_ip_addr,
         remote_port,
         slave_ip_addr,
         &result),
      0);

   /* Validate */
   EXPECT_EQ (result.remote_ip, master_ip_addr);
   EXPECT_EQ (result.remote_port, remote_port);
   EXPECT_EQ (result.slave_ip_addr, slave_ip_addr);
   EXPECT_EQ (
      CC_FROM_LE16 (result.full_headers->cyclic_header.protocol_ver),
      protocol_ver);
   EXPECT_EQ (
      CC_FROM_LE16 (
         result.full_headers->cyclic_data_header.cyclic_transmission_state),
      cyclic_transmission_state);
   EXPECT_EQ (
      CC_FROM_LE16 (result.full_headers->cyclic_data_header.frame_sequence_no),
      frame_sequence_no);
   EXPECT_EQ (result.full_headers->cyclic_data_header.group_no, group_no);
   EXPECT_EQ (
      CC_FROM_LE32 (result.full_headers->cyclic_data_header.master_id),
      master_ip_addr);
   EXPECT_EQ (
      CC_FROM_LE16 (
         result.full_headers->cyclic_data_header.parallel_off_timeout_count),
      parallel_off_timeout_count);
   EXPECT_EQ (
      CC_FROM_LE16 (result.full_headers->cyclic_data_header.parameter_no),
      parameter_no);
   EXPECT_EQ (
      CC_FROM_LE16 (result.full_headers->cyclic_data_header
                       .slave_total_occupied_station_count),
      group_occupied_stations);
   EXPECT_EQ (
      CC_FROM_LE16 (result.full_headers->cyclic_data_header.timeout_value),
      timeout_value);
   EXPECT_EQ (
      CC_FROM_LE64 (result.full_headers->master_station_notification.clock_info),
      unix_timestamp_ms);
   EXPECT_EQ (
      CC_FROM_LE16 (
         result.full_headers->master_station_notification.master_local_unit_info),
      master_local_unit_info);

   EXPECT_EQ (CC_FROM_LE32 (result.first_slave_id[0]), other_slave_ip_addr);
   EXPECT_EQ (CC_FROM_LE32 (result.first_slave_id[1]), slave_ip_addr);
   EXPECT_EQ (
      CC_FROM_LE32 (result.first_slave_id[2]),
      CL_CCIEFB_MULTISTATION_INDICATOR);
   EXPECT_EQ (CC_FROM_LE16 (result.first_rww[sdi].words[0]), 0xA000);
   EXPECT_EQ (CC_FROM_LE16 (result.first_rww[sdi].words[1]), 0xA001);
   EXPECT_EQ (CC_FROM_LE16 (result.first_rww[sdi].words[2]), 0xA002);
   EXPECT_EQ (result.first_ry[sdi].bytes[0], 0xB0);
   EXPECT_EQ (result.first_ry[sdi].bytes[1], 0xB1);
   EXPECT_EQ (result.first_ry[sdi].bytes[2], 0xB2);
}

TEST_F (IefbUnitTest, CciefbSanityCyclicResponse)
{
   uint8_t buffer[400]                                  = {0};
   cls_cciefb_cyclic_response_info_t outgoing_frameinfo = {};
   const uint16_t vendor_code                           = 0x7172;
   const uint32_t model_code                            = 0x73747576;
   const uint16_t equipment_version                     = 0x7778;
   const uint16_t occupied_stations_for_slave           = 2;
   const uint16_t group_no                              = 0x2F;
   const uint16_t frame_sequence_no                     = 0x2345;
   const uint16_t slave_err_code                        = 0x789A;
   const uint16_t remote_port                           = 0x5354;
   const uint32_t local_management_info                 = 0x89ABCDEF;
   const uint32_t timestamp                             = 0x62636465;
   const cl_ipaddr_t slave_ip_addr = 0x010203046L; /* 1.2.3.6 */
   const cl_slmp_error_codes_t end_code =
      CL_SLMP_ENDCODE_PARAMETER_DOES_NOT_EXIST;
   const cl_slave_appl_operation_status_t slave_local_unit_info =
      CL_SLAVE_APPL_OPERATION_STATUS_OPERATING;
   clm_cciefb_cyclic_response_info_t result;

   clal_clear_memory (&result, sizeof (result));

   /* Prepare frame */
   cl_iefb_initialise_cyclic_response_frame (
      (uint8_t *)&buffer,
      sizeof (buffer),
      occupied_stations_for_slave,
      vendor_code,
      model_code,
      equipment_version,
      &outgoing_frameinfo);

   cl_iefb_update_cyclic_response_frame (
      &outgoing_frameinfo,
      slave_ip_addr,
      end_code,
      group_no,
      frame_sequence_no,
      slave_local_unit_info,
      slave_err_code,
      local_management_info);

   outgoing_frameinfo.first_rwr->words[0] = CC_TO_LE16 (0xA000);
   outgoing_frameinfo.first_rwr->words[1] = CC_TO_LE16 (0xA001);
   outgoing_frameinfo.first_rwr->words[2] = CC_TO_LE16 (0xA002);
   outgoing_frameinfo.first_rx->bytes[0]  = 0xB0;
   outgoing_frameinfo.first_rx->bytes[1]  = 0xB1;
   outgoing_frameinfo.first_rx->bytes[2]  = 0xB2;

   /* Parse frame */
   EXPECT_EQ (
      cl_iefb_parse_cyclic_response (
         (uint8_t *)&buffer,
         cl_calculate_cyclic_response_size (occupied_stations_for_slave),
         slave_ip_addr,
         remote_port,
         timestamp,
         &result),
      0);

   /* Validate */
   EXPECT_EQ (result.remote_ip, slave_ip_addr);
   EXPECT_EQ (result.remote_port, remote_port);
   EXPECT_EQ (result.number_of_occupied, occupied_stations_for_slave);
   EXPECT_EQ (result.reception_timestamp, timestamp);
   EXPECT_EQ (result.device_index, 0); /* Not modified by parser */
   EXPECT_EQ (result.full_headers->cyclic_header.end_code, end_code);
   EXPECT_EQ (
      CC_FROM_LE16 (result.full_headers->slave_station_notification.vendor_code),
      vendor_code);
   EXPECT_EQ (
      CC_FROM_LE32 (result.full_headers->slave_station_notification.model_code),
      model_code);
   EXPECT_EQ (
      CC_FROM_LE16 (result.full_headers->slave_station_notification.equipment_ver),
      equipment_version);
   EXPECT_EQ (
      CC_FROM_LE16 (
         result.full_headers->slave_station_notification.slave_err_code),
      slave_err_code);
   EXPECT_EQ (
      CC_FROM_LE16 (
         result.full_headers->slave_station_notification.slave_local_unit_info),
      slave_local_unit_info);
   EXPECT_EQ (result.full_headers->cyclic_data_header.group_no, group_no);
   EXPECT_EQ (
      CC_FROM_LE32 (result.full_headers->cyclic_data_header.slave_id),
      slave_ip_addr);
   EXPECT_EQ (
      CC_FROM_LE16 (result.full_headers->cyclic_data_header.frame_sequence_no),
      frame_sequence_no);
   EXPECT_EQ (CC_FROM_LE16 (result.first_rwr->words[0]), 0xA000);
   EXPECT_EQ (CC_FROM_LE16 (result.first_rwr->words[1]), 0xA001);
   EXPECT_EQ (CC_FROM_LE16 (result.first_rwr->words[2]), 0xA002);
   EXPECT_EQ (result.first_rx->bytes[0], 0xB0);
   EXPECT_EQ (result.first_rx->bytes[1], 0xB1);
   EXPECT_EQ (result.first_rx->bytes[2], 0xB2);
}
