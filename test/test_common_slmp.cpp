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
#include "common/cl_slmp.h"
#include "common/cl_types.h"

#include "mocks.h"
#include "utils_for_testing.h"
#include <gtest/gtest.h>

// Test fixture
class SlmpUnitTest : public UnitTest
{
};

/*********************** Numerical utilities *********************************/

TEST_F (SlmpUnitTest, SlmpCalculateNodeSearchDelay)
{
   cl_macaddr_t mac_addr = {0};

   mac_addr[0] = 0x00;
   mac_addr[1] = 0x00;
   mac_addr[2] = 0x00;
   mac_addr[3] = 0x00;
   mac_addr[4] = 0x00;
   mac_addr[5] = 0x00;
   EXPECT_EQ (cl_slmp_calculate_node_search_delay (&mac_addr), 0U);

   mac_addr[0] = 0xFF;
   mac_addr[1] = 0xFF;
   mac_addr[2] = 0xFF;
   mac_addr[3] = 0xFF;
   mac_addr[4] = 0x00;
   mac_addr[5] = 0x00;
   EXPECT_EQ (cl_slmp_calculate_node_search_delay (&mac_addr), 0U);

   mac_addr[4] = 0x02;
   mac_addr[5] = 0x00;
   EXPECT_EQ (cl_slmp_calculate_node_search_delay (&mac_addr), 1000U);

   mac_addr[4] = 0x01;
   mac_addr[5] = 0x00;
   EXPECT_EQ (cl_slmp_calculate_node_search_delay (&mac_addr), 2000U);

   mac_addr[4] = 0x00;
   mac_addr[5] = 0x80;
   EXPECT_EQ (cl_slmp_calculate_node_search_delay (&mac_addr), 4000U);

   mac_addr[4] = 0x00;
   mac_addr[5] = 0x40;
   EXPECT_EQ (cl_slmp_calculate_node_search_delay (&mac_addr), 8000U);

   mac_addr[4] = 0x00;
   mac_addr[5] = 0x20;
   EXPECT_EQ (cl_slmp_calculate_node_search_delay (&mac_addr), 16000U);

   mac_addr[4] = 0x00;
   mac_addr[5] = 0x02;
   EXPECT_EQ (cl_slmp_calculate_node_search_delay (&mac_addr), 256000U);

   mac_addr[4] = 0x00;
   mac_addr[5] = 0x01;
   EXPECT_EQ (cl_slmp_calculate_node_search_delay (&mac_addr), 512000U);

   mac_addr[4] = 0xFF;
   mac_addr[5] = 0xFF;
   EXPECT_EQ (cl_slmp_calculate_node_search_delay (&mac_addr), 1023000U);
}

/*********************** Parse frames **************************************/

TEST_F (SlmpUnitTest, SlmpParseRequestHeader)
{
   cl_slmp_req_header_t * header = nullptr;

   uint8_t buffer[50] = {0}; /* Larger than sizeof(cl_slmp_req_header_t) */
   uint32_t ix;
   for (ix = 0; ix < sizeof (buffer); ix++)
   {
      buffer[ix] = (uint8_t)ix;
   }

   /* Parsing of valid content */
   EXPECT_EQ (
      cl_slmp_parse_request_header ((uint8_t *)&buffer, sizeof (buffer), &header),
      0);

   EXPECT_EQ (CC_FROM_BE16 (header->sub1), 0x0001); /* Big endian */
   EXPECT_EQ (CC_FROM_LE16 (header->serial), 0x0302);
   EXPECT_EQ (CC_FROM_LE16 (header->sub2), 0x0504);
   EXPECT_EQ (header->network_number, 0x06);
   EXPECT_EQ (header->unit_number, 0x07);
   EXPECT_EQ (CC_FROM_LE16 (header->io_number), 0x0908);
   EXPECT_EQ (header->extension, 0x0A);
   EXPECT_EQ (CC_FROM_LE16 (header->length), 0x0C0B);
   EXPECT_EQ (CC_FROM_LE16 (header->timer), 0x0E0D);
   EXPECT_EQ (CC_FROM_LE16 (header->command), 0x100F);
   EXPECT_EQ (CC_FROM_LE16 (header->sub_command), 0x1211);

   /* Too short buffer.
      Buffer should be at least sizeof(cl_slmp_req_header_t) which is 19 */
   EXPECT_EQ (cl_slmp_parse_request_header ((uint8_t *)&buffer, 0, &header), -1);
   EXPECT_EQ (cl_slmp_parse_request_header ((uint8_t *)&buffer, 18, &header), -1);
   EXPECT_EQ (cl_slmp_parse_request_header ((uint8_t *)&buffer, 19, &header), 0);
}

TEST_F (SlmpUnitTest, SlmpParseResponseHeader)
{
   cl_slmp_resp_header_t * header = nullptr;

   TDATA_INIT_BUFFER (50); /* Larger than sizeof(cl_slmp_resp_header_t) */

   /* Parsing of valid content */
   EXPECT_EQ (
      cl_slmp_parse_response_header ((uint8_t *)&buffer, sizeof (buffer), &header),
      0);

   EXPECT_EQ (CC_FROM_BE16 (header->sub1), TDATA_BE16()); /* Big endian */
   EXPECT_EQ (CC_FROM_LE16 (header->serial), TDATA_LE16());
   EXPECT_EQ (CC_FROM_LE16 (header->sub2), TDATA_LE16());
   EXPECT_EQ (header->network_number, TDATA_8());
   EXPECT_EQ (header->unit_number, TDATA_8());
   EXPECT_EQ (CC_FROM_LE16 (header->io_number), TDATA_LE16());
   EXPECT_EQ (header->extension, TDATA_8());
   EXPECT_EQ (CC_FROM_LE16 (header->length), TDATA_LE16());
   EXPECT_EQ (CC_FROM_LE16 (header->endcode), TDATA_LE16());

   /* Too short buffer.
      Buffer should be at least sizeof(cl_slmp_resp_header_t) which is 15 */
   EXPECT_EQ (cl_slmp_parse_response_header ((uint8_t *)&buffer, 0, &header), -1);
   EXPECT_EQ (cl_slmp_parse_response_header ((uint8_t *)&buffer, 14, &header), -1);
   EXPECT_EQ (cl_slmp_parse_response_header ((uint8_t *)&buffer, 15, &header), 0);
}

TEST_F (SlmpUnitTest, SlmpParseNodeSearchRequest)
{
   cl_slmp_node_search_request_t * request = nullptr;
   uint8_t buffer[100] = {0}; /* cl_slmp_node_search_request_t */
   uint32_t ix;

   for (ix = 0; ix < sizeof (buffer); ix++)
   {
      buffer[ix] = (uint8_t)ix;
   }

   /* Parsing of valid content */
   EXPECT_EQ (
      cl_slmp_parse_node_search_request (
         (uint8_t *)&buffer,
         SIZE_REQUEST_NODE_SEARCH,
         &request),
      0);

   /* Request header */
   EXPECT_EQ (CC_FROM_BE16 (request->header.sub1), 0x0001); /* BE */
   EXPECT_EQ (CC_FROM_LE16 (request->header.serial), 0x0302);
   EXPECT_EQ (CC_FROM_LE16 (request->header.sub2), 0x0504);
   EXPECT_EQ (request->header.network_number, 0x06);
   EXPECT_EQ (request->header.unit_number, 0x07);
   EXPECT_EQ (CC_FROM_LE16 (request->header.io_number), 0x0908);
   EXPECT_EQ (request->header.extension, 0x0A);
   EXPECT_EQ (CC_FROM_LE16 (request->header.length), 0x0C0B);
   EXPECT_EQ (CC_FROM_LE16 (request->header.timer), 0x0E0D);
   EXPECT_EQ (CC_FROM_LE16 (request->header.command), 0x100F);
   EXPECT_EQ (CC_FROM_LE16 (request->header.sub_command), 0x1211);

   EXPECT_EQ (request->master_mac_addr[0], 0x13); /* Little-endian */
   EXPECT_EQ (request->master_mac_addr[1], 0x14);
   EXPECT_EQ (request->master_mac_addr[2], 0x15);
   EXPECT_EQ (request->master_mac_addr[3], 0x16);
   EXPECT_EQ (request->master_mac_addr[4], 0x17);
   EXPECT_EQ (request->master_mac_addr[5], 0x18);
   EXPECT_EQ (request->master_ip_addr_size, 0x19);
   EXPECT_EQ (CC_FROM_LE32 (request->master_ip_addr), 0x1D1C1B1AUL);

   /* Too short buffer */
   EXPECT_EQ (
      cl_slmp_parse_node_search_request ((uint8_t *)&buffer, 0, &request),
      -1);
   EXPECT_EQ (
      cl_slmp_parse_node_search_request (
         (uint8_t *)&buffer,
         SIZE_REQUEST_NODE_SEARCH - 1,
         &request),
      -1);
   EXPECT_EQ (
      cl_slmp_parse_node_search_request (
         (uint8_t *)&buffer,
         SIZE_REQUEST_NODE_SEARCH,
         &request),
      0);
}

TEST_F (SlmpUnitTest, SlmpParseNodeSearchResponse)
{
   cl_slmp_node_search_resp_t * resp = nullptr;

   TDATA_INIT_BUFFER (100);

   /* Parsing of valid content */
   EXPECT_EQ (
      cl_slmp_parse_node_search_response (
         (uint8_t *)&buffer,
         SIZE_RESPONSE_NODE_SEARCH,
         &resp),
      0);

   /* Request header */
   EXPECT_EQ (CC_FROM_BE16 (resp->header.sub1), TDATA_BE16()); /* BE */
   EXPECT_EQ (CC_FROM_LE16 (resp->header.serial), TDATA_LE16());
   EXPECT_EQ (CC_FROM_LE16 (resp->header.sub2), TDATA_LE16());
   EXPECT_EQ (resp->header.network_number, TDATA_8());
   EXPECT_EQ (resp->header.unit_number, TDATA_8());
   EXPECT_EQ (CC_FROM_LE16 (resp->header.io_number), TDATA_LE16());
   EXPECT_EQ (resp->header.extension, TDATA_8());
   EXPECT_EQ (CC_FROM_LE16 (resp->header.length), TDATA_LE16());
   EXPECT_EQ (CC_FROM_LE16 (resp->header.endcode), TDATA_LE16());

   /* Node search response */
   EXPECT_EQ (resp->master_mac_addr[0], TDATA_8()); /* Little-endian */
   EXPECT_EQ (resp->master_mac_addr[1], TDATA_8());
   EXPECT_EQ (resp->master_mac_addr[2], TDATA_8());
   EXPECT_EQ (resp->master_mac_addr[3], TDATA_8());
   EXPECT_EQ (resp->master_mac_addr[4], TDATA_8());
   EXPECT_EQ (resp->master_mac_addr[5], TDATA_8());
   EXPECT_EQ (resp->master_ip_addr_size, TDATA_8());
   EXPECT_EQ (CC_FROM_LE32 (resp->master_ip_addr), TDATA_LE32());

   EXPECT_EQ (resp->slave_mac_addr[0], TDATA_8()); /* Little-endian */
   EXPECT_EQ (resp->slave_mac_addr[1], TDATA_8());
   EXPECT_EQ (resp->slave_mac_addr[2], TDATA_8());
   EXPECT_EQ (resp->slave_mac_addr[3], TDATA_8());
   EXPECT_EQ (resp->slave_mac_addr[4], TDATA_8());
   EXPECT_EQ (resp->slave_mac_addr[5], TDATA_8());

   EXPECT_EQ (resp->slave_ip_addr_size, TDATA_8());
   EXPECT_EQ (CC_FROM_LE32 (resp->slave_ip_addr), TDATA_LE32());
   EXPECT_EQ (resp->slave_netmask, TDATA_LE32());
   EXPECT_EQ (resp->slave_default_gateway, TDATA_LE32());

   EXPECT_EQ (resp->slave_hostname_size, TDATA_8());
   EXPECT_EQ (resp->vendor_code, TDATA_LE16());
   EXPECT_EQ (resp->model_code, TDATA_LE32());
   EXPECT_EQ (resp->equipment_ver, TDATA_LE16());
   EXPECT_EQ (resp->target_ip_addr_size, TDATA_8());
   EXPECT_EQ (resp->target_ip_addr, TDATA_LE32());
   EXPECT_EQ (resp->target_port, TDATA_LE16());
   EXPECT_EQ (resp->slave_status, TDATA_LE16());
   EXPECT_EQ (resp->slave_port, TDATA_LE16());
   EXPECT_EQ (resp->slave_protocol_settings, TDATA_8());

   /* Too short buffer */
   EXPECT_EQ (
      cl_slmp_parse_node_search_response ((uint8_t *)&buffer, 0, &resp),
      -1);
   EXPECT_EQ (
      cl_slmp_parse_node_search_response (
         (uint8_t *)&buffer,
         SIZE_RESPONSE_NODE_SEARCH - 1,
         &resp),
      -1);
   EXPECT_EQ (
      cl_slmp_parse_node_search_response (
         (uint8_t *)&buffer,
         SIZE_RESPONSE_NODE_SEARCH,
         &resp),
      0);
}

TEST_F (SlmpUnitTest, SlmpParseSetIpRequest)
{
   cl_slmp_set_ipaddr_request_t * request = nullptr;
   uint8_t buffer[100] = {0}; /* cl_slmp_set_ipaddr_request_t */
   uint32_t ix;

   for (ix = 0; ix < sizeof (buffer); ix++)
   {
      buffer[ix] = (uint8_t)ix;
   }

   /* Parsing of valid content */
   EXPECT_EQ (
      cl_slmp_parse_set_ip_request (
         (uint8_t *)&buffer,
         SIZE_REQUEST_SET_IP,
         &request),
      0);

   /* Request header */
   EXPECT_EQ (CC_FROM_BE16 (request->header.sub1), 0x0001); /* BE */
   EXPECT_EQ (CC_FROM_LE16 (request->header.serial), 0x0302);
   EXPECT_EQ (CC_FROM_LE16 (request->header.sub2), 0x0504);
   EXPECT_EQ (request->header.network_number, 0x06);
   EXPECT_EQ (request->header.unit_number, 0x07);
   EXPECT_EQ (CC_FROM_LE16 (request->header.io_number), 0x0908);
   EXPECT_EQ (request->header.extension, 0x0A);
   EXPECT_EQ (CC_FROM_LE16 (request->header.length), 0x0C0B);
   EXPECT_EQ (CC_FROM_LE16 (request->header.timer), 0x0E0D);
   EXPECT_EQ (CC_FROM_LE16 (request->header.command), 0x100F);
   EXPECT_EQ (CC_FROM_LE16 (request->header.sub_command), 0x1211);

   EXPECT_EQ (request->master_mac_addr[0], 0x13);
   EXPECT_EQ (request->master_mac_addr[1], 0x14);
   EXPECT_EQ (request->master_mac_addr[2], 0x15);
   EXPECT_EQ (request->master_mac_addr[3], 0x16);
   EXPECT_EQ (request->master_mac_addr[4], 0x17);
   EXPECT_EQ (request->master_mac_addr[5], 0x18);
   EXPECT_EQ (request->master_ip_addr_size, 0x19);
   EXPECT_EQ (CC_FROM_LE32 (request->master_ip_addr), 0x1D1C1B1AUL);
   EXPECT_EQ (request->slave_mac_addr[0], 0x1E);
   EXPECT_EQ (request->slave_mac_addr[1], 0x1F);
   EXPECT_EQ (request->slave_mac_addr[2], 0x20);
   EXPECT_EQ (request->slave_mac_addr[3], 0x21);
   EXPECT_EQ (request->slave_mac_addr[4], 0x22);
   EXPECT_EQ (request->slave_mac_addr[5], 0x23);
   EXPECT_EQ (request->slave_ip_addr_size, 0x24);
   EXPECT_EQ (CC_FROM_LE32 (request->slave_new_ip_addr), 0x28272625UL);
   EXPECT_EQ (CC_FROM_LE32 (request->slave_new_netmask), 0x2C2B2A29UL);
   EXPECT_EQ (CC_FROM_LE32 (request->slave_default_gateway), 0x302F2E2DUL);
   EXPECT_EQ (request->slave_hostname_size, 0x31);
   EXPECT_EQ (request->target_ip_addr_size, 0x32);
   EXPECT_EQ (CC_FROM_LE32 (request->target_ip_addr), 0x36353433UL);
   EXPECT_EQ (CC_FROM_LE16 (request->target_port), 0x3837UL);
   EXPECT_EQ (request->slave_protocol_settings, 0x39);

   /* Too short buffer */
   EXPECT_EQ (cl_slmp_parse_set_ip_request ((uint8_t *)&buffer, 0, &request), -1);
   EXPECT_EQ (
      cl_slmp_parse_set_ip_request (
         (uint8_t *)&buffer,
         SIZE_REQUEST_SET_IP - 1,
         &request),
      -1);
   EXPECT_EQ (
      cl_slmp_parse_set_ip_request (
         (uint8_t *)&buffer,
         SIZE_REQUEST_SET_IP,
         &request),
      0);
}

TEST_F (SlmpUnitTest, SlmpParseSetIpResponse)
{
   cl_slmp_set_ipaddr_resp_t * resp = nullptr;

   TDATA_INIT_BUFFER (100);

   /* Parsing of valid content */
   EXPECT_EQ (
      cl_slmp_parse_set_ip_response ((uint8_t *)&buffer, SIZE_RESPONSE_SET_IP, &resp),
      0);

   /* Request header */
   EXPECT_EQ (CC_FROM_BE16 (resp->header.sub1), TDATA_BE16()); /* BE */
   EXPECT_EQ (CC_FROM_LE16 (resp->header.serial), TDATA_LE16());
   EXPECT_EQ (CC_FROM_LE16 (resp->header.sub2), TDATA_LE16());
   EXPECT_EQ (resp->header.network_number, TDATA_8());
   EXPECT_EQ (resp->header.unit_number, TDATA_8());
   EXPECT_EQ (CC_FROM_LE16 (resp->header.io_number), TDATA_LE16());
   EXPECT_EQ (resp->header.extension, TDATA_8());
   EXPECT_EQ (CC_FROM_LE16 (resp->header.length), TDATA_LE16());
   EXPECT_EQ (CC_FROM_LE16 (resp->header.endcode), TDATA_LE16());

   /* Set IP response */
   EXPECT_EQ (resp->master_mac_addr[0], TDATA_8()); /* Little-endian */
   EXPECT_EQ (resp->master_mac_addr[1], TDATA_8());
   EXPECT_EQ (resp->master_mac_addr[2], TDATA_8());
   EXPECT_EQ (resp->master_mac_addr[3], TDATA_8());
   EXPECT_EQ (resp->master_mac_addr[4], TDATA_8());
   EXPECT_EQ (resp->master_mac_addr[5], TDATA_8());

   /* Too short buffer  */
   EXPECT_EQ (cl_slmp_parse_set_ip_response ((uint8_t *)&buffer, 0, &resp), -1);
   EXPECT_EQ (
      cl_slmp_parse_set_ip_response (
         (uint8_t *)&buffer,
         SIZE_RESPONSE_SET_IP - 1,
         &resp),
      -1);
   EXPECT_EQ (
      cl_slmp_parse_set_ip_response ((uint8_t *)&buffer, SIZE_RESPONSE_SET_IP, &resp),
      0);
}

TEST_F (SlmpUnitTest, SlmpParseErrorResponse)
{
   cl_slmp_error_resp_t * resp = nullptr;

   TDATA_INIT_BUFFER (100);

   /* Parsing of valid content */
   EXPECT_EQ (
      cl_slmp_parse_error_response ((uint8_t *)&buffer, SIZE_RESPONSE_ERROR, &resp),
      0);

   /* Request header */
   EXPECT_EQ (CC_FROM_BE16 (resp->header.sub1), TDATA_BE16()); /* BE */
   EXPECT_EQ (CC_FROM_LE16 (resp->header.serial), TDATA_LE16());
   EXPECT_EQ (CC_FROM_LE16 (resp->header.sub2), TDATA_LE16());
   EXPECT_EQ (resp->header.network_number, TDATA_8());
   EXPECT_EQ (resp->header.unit_number, TDATA_8());
   EXPECT_EQ (CC_FROM_LE16 (resp->header.io_number), TDATA_LE16());
   EXPECT_EQ (resp->header.extension, TDATA_8());
   EXPECT_EQ (CC_FROM_LE16 (resp->header.length), TDATA_LE16());
   EXPECT_EQ (CC_FROM_LE16 (resp->header.endcode), TDATA_LE16());

   /* Error response */
   EXPECT_EQ (resp->error_network_number, TDATA_8());
   EXPECT_EQ (resp->error_unit_number, TDATA_8());
   EXPECT_EQ (resp->error_io_number, TDATA_LE16());
   EXPECT_EQ (resp->error_extension, TDATA_8());
   EXPECT_EQ (resp->command, TDATA_LE16());
   EXPECT_EQ (resp->sub_command, TDATA_LE16());

   /* Too short buffer. */
   EXPECT_EQ (cl_slmp_parse_error_response ((uint8_t *)&buffer, 0, &resp), -1);
   EXPECT_EQ (
      cl_slmp_parse_error_response (
         (uint8_t *)&buffer,
         SIZE_RESPONSE_ERROR - 1,
         &resp),
      -1);
   EXPECT_EQ (
      cl_slmp_parse_error_response ((uint8_t *)&buffer, SIZE_RESPONSE_ERROR, &resp),
      0);
}

/************************** Validate frames *******************************/

TEST_F (SlmpUnitTest, SlmpValidateRequestHeader)
{
   const uint16_t udp_len      = 100; /* arbitrary value */
   const uint16_t length       = udp_len - 13;
   cl_slmp_req_header_t header = {
      .sub1           = CC_TO_BE16 (0x5400), /* Big endian */
      .serial         = CC_TO_LE16 (0x0123),
      .sub2           = CC_TO_LE16 (0x0000),
      .network_number = 0x00,
      .unit_number    = 0xFF,
      .io_number      = CC_TO_LE16 (0x03FF),
      .extension      = 0x00,
      .length         = CC_TO_LE16 (length),
      .timer          = CC_TO_LE16 (0x0000),
      .command        = CC_TO_LE16 (0x3456),
      .sub_command    = CC_TO_LE16 (0x789A),
   };
   const cl_slmp_req_header_t default_header = header;
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), 0);

   /* Length */
   header.length = CC_TO_LE16 (0x0000);
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), 0);

   /* Sub1. Note: Big endian */
   header.sub1 = CC_TO_BE16 (0x0000);
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), 0);

   /* Sub2 */
   header.sub2 = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), 0);

   /* Network number */
   header.network_number = 0x12;
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), 0);

   /* Unit number */
   header.unit_number = 0x12;
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), 0);

   /* IO number */
   header.io_number = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), 0);

   /* Extension */
   header.extension = 0x12;
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), 0);

   /* Timer */
   header.timer = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_request_header (&header, udp_len), 0);
}

TEST_F (SlmpUnitTest, SlmpValidateResponseHeader)
{
   const uint16_t udp_len       = 100; /* arbitrary value */
   const uint16_t length        = udp_len - 13;
   cl_slmp_resp_header_t header = {
      .sub1           = CC_TO_BE16 (0xD400), /* Big endian */
      .serial         = CC_TO_LE16 (0x0123),
      .sub2           = CC_TO_LE16 (0x0000),
      .network_number = 0x00,
      .unit_number    = 0xFF,
      .io_number      = CC_TO_LE16 (0x03FF),
      .extension      = 0x00,
      .length         = CC_TO_LE16 (length),
      .endcode        = CC_TO_LE16 (0xEF12)};
   const cl_slmp_resp_header_t default_header = header;
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), 0);

   /* Length */
   header.length = CC_TO_LE16 (0x0000);
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), 0);

   /* Sub1. Note: Big endian */
   header.sub1 = CC_TO_BE16 (0x0000);
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), 0);

   /* Sub2 */
   header.sub2 = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), 0);

   /* Network number */
   header.network_number = 0x12;
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), 0);

   /* Unit number */
   header.unit_number = 0x12;
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), 0);

   /* IO number */
   header.io_number = CC_TO_LE16 (0x1234);
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), 0);

   /* Extension */
   header.extension = 0x12;
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), -1);
   header = default_header;
   EXPECT_EQ (cl_slmp_validate_response_header (&header, udp_len), 0);
}

TEST_F (SlmpUnitTest, SlmpValidateNodeSearchRequest)
{
   const uint16_t length                 = SIZE_REQUEST_NODE_SEARCH - 13;
   cl_slmp_node_search_request_t request = {
      .header =
         {
            .sub1           = CC_TO_BE16 (0x5400), /* Big endian */
            .serial         = CC_TO_LE16 (0x0123),
            .sub2           = CC_TO_LE16 (0x0000),
            .network_number = 0x45,
            .unit_number    = 0x67,
            .io_number      = CC_TO_LE16 (0x89AB),
            .extension      = 0xCD,
            .length         = CC_TO_LE16 (length),
            .timer          = CC_TO_LE16 (0xEF12),
            .command        = CC_TO_LE16 (0x3456),
            .sub_command    = CC_TO_LE16 (0x789A),
         },
      .master_mac_addr     = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26},
      .master_ip_addr_size = 4,
      .master_ip_addr      = CC_TO_LE32 (0x01020304)};
   const cl_slmp_node_search_request_t default_request = request;
   EXPECT_EQ (cl_slmp_validate_node_search_request (&request), 0);

   /* IP address size */
   request.master_ip_addr_size = 0;
   EXPECT_EQ (cl_slmp_validate_node_search_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_node_search_request (&request), 0);

   /* Invalid master IP */
   request.master_ip_addr = CC_TO_LE32 (0x00000000);
   EXPECT_EQ (cl_slmp_validate_node_search_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_node_search_request (&request), 0);

   /* Length */
   request.header.length = 0xFF;
   EXPECT_EQ (cl_slmp_validate_node_search_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_node_search_request (&request), 0);
}

/**
 *  Validate SLMP Node search response
 *
 * @req REQ_CL_SLMP_04
 *
 */
TEST_F (SlmpUnitTest, SlmpValidateNodeSearchResponse)
{
   const uint16_t length           = SIZE_RESPONSE_NODE_SEARCH - 13;
   cl_slmp_node_search_resp_t resp = {
      .header =
         {
            .sub1           = CC_TO_BE16 (0x5400), /* Big endian */
            .serial         = CC_TO_LE16 (0x0123),
            .sub2           = CC_TO_LE16 (0x0000),
            .network_number = 0x45,
            .unit_number    = 0x67,
            .io_number      = CC_TO_LE16 (0x89AB),
            .extension      = 0xCD,
            .length         = CC_TO_LE16 (length),
            .endcode        = CC_TO_LE16 (CL_SLMP_ENDCODE_SUCCESS),
         },
      .master_mac_addr     = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26},
      .master_ip_addr_size = 4,
      .master_ip_addr      = CC_TO_LE32 (0x01020304),
      .slave_mac_addr      = {0x21, 0x22, 0x23, 0x24, 0x25, 0x27},
      .slave_ip_addr_size  = 4,
      .slave_ip_addr       = CC_TO_LE32 (0x01020304),
      .slave_netmask       = CC_TO_LE32 (0xFFFFFF00),
      .slave_default_gateway =
         CC_TO_LE32 (CL_SLMP_NODE_SEARCH_RESP_SLAVE_DEFAULT_GATEWAY),
      .slave_hostname_size = CL_SLMP_NODE_SEARCH_RESP_SLAVE_HOSTNAME_SIZE,
      .vendor_code         = CC_TO_LE16 (0x4321),
      .model_code          = CC_TO_LE32 (0x87654321),
      .equipment_ver       = CC_TO_LE16 (0x0001),
      .target_ip_addr_size = CL_ADDRSIZE_IPV4,
      .target_ip_addr = CC_TO_LE32 (CL_SLMP_NODE_SEARCH_RESP_TARGET_IP_ADDR),
      .target_port    = CC_TO_LE16 (CL_SLMP_NODE_SEARCH_RESP_TARGET_PORT),
      .slave_status = CC_TO_LE16 (CL_SLMP_NODE_SEARCH_RESP_SERVER_STATUS_NORMAL),
      .slave_port              = CC_TO_LE16 (CL_SLMP_PORT),
      .slave_protocol_settings = CL_SLMP_PROTOCOL_IDENTIFIER_UDP,
   };
   const cl_slmp_node_search_resp_t default_resp = resp;

   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), 0);

   /* IP address size */
   resp.slave_ip_addr_size = 0;
   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), -1);
   resp = default_resp;
   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), 0);

   resp.master_ip_addr_size = 0;
   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), -1);
   resp = default_resp;
   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), 0);

   resp.target_ip_addr_size = 0;
   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), -1);
   resp = default_resp;
   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), 0);

   /* Invalid IP */
   resp.slave_ip_addr = CC_TO_LE32 (0x00000000);
   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), -1);
   resp = default_resp;
   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), 0);

   resp.master_ip_addr = CC_TO_LE32 (0x00000000);
   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), -1);
   resp = default_resp;
   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), 0);

   /* Netmask */
   resp.slave_netmask = CC_TO_LE32 (0xFF00FF00);
   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), -1);
   resp = default_resp;
   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), 0);

   /* Length */
   resp.header.length = 0xFF;
   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), -1);
   resp = default_resp;
   EXPECT_EQ (cl_slmp_validate_node_search_response (&resp), 0);

   /* Fixed values should not be checked, see REQ_CL_SLMP_04 */
}

TEST_F (SlmpUnitTest, SlmpValidateSetIpRequest)
{
   const uint16_t length                = SIZE_REQUEST_SET_IP - 13;
   cl_slmp_set_ipaddr_request_t request = {
      .header =
         {
            .sub1           = CC_TO_BE16 (0x5400), /* Big endian */
            .serial         = CC_TO_LE16 (0x0123),
            .sub2           = CC_TO_LE16 (0x0000),
            .network_number = 0x45,
            .unit_number    = 0x67,
            .io_number      = CC_TO_LE16 (0x89AB),
            .extension      = 0xCD,
            .length         = CC_TO_LE16 (length),
            .timer          = CC_TO_LE16 (0xEF12),
            .command        = CC_TO_LE16 (0x3456),
            .sub_command    = CC_TO_LE16 (0x789A),
         },
      .master_mac_addr         = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26},
      .master_ip_addr_size     = 4,
      .master_ip_addr          = CC_TO_LE32 (0x31323334),
      .slave_mac_addr          = {0x56, 0x55, 0x54, 0x53, 0x52, 0x51},
      .slave_ip_addr_size      = 4,
      .slave_new_ip_addr       = CC_TO_LE32 (0x81828384),
      .slave_new_netmask       = CC_TO_LE32 (0xFFFFFF00),
      .slave_default_gateway   = CC_TO_LE32 (UINT32_MAX),
      .slave_hostname_size     = 0x00,
      .target_ip_addr_size     = 4,
      .target_ip_addr          = CC_TO_LE32 (UINT32_MAX),
      .target_port             = CC_TO_LE16 (UINT16_MAX),
      .slave_protocol_settings = 0x01};
   const cl_slmp_set_ipaddr_request_t default_request = request;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), 0);

   /* IP address size */
   request.master_ip_addr_size = 0;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), 0);

   request.slave_ip_addr_size = 0;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), 0);

   request.target_ip_addr_size = 0;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), 0);

   /* Protocol settings */
   request.slave_protocol_settings = 0x02;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), 0);

   /* Hostname size */
   request.slave_hostname_size = 2;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), 0);

   /* Invalid IP */
   request.master_ip_addr = CC_TO_LE32 (0x00000000);
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), 0);

   request.slave_new_ip_addr = CC_TO_LE32 (0x00000000);
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), 0);

   /* Invalid netmask */
   request.slave_new_netmask = CC_TO_LE32 (0xF0F0F0F0);
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), 0);

   /* Length */
   request.header.length = 0xFF;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), 0);

   /* Fixed values */
   request.slave_default_gateway = CC_TO_LE32 (0x02030405);
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), 0);

   request.target_ip_addr = CC_TO_LE32 (0x02030405);
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), 0);

   request.target_port = CC_TO_LE16 (0x0123);
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), -1);
   request = default_request;
   EXPECT_EQ (cl_slmp_validate_set_ip_request (&request), 0);
}

TEST_F (SlmpUnitTest, SlmpValidateSetIpResponse)
{
   const uint16_t length          = SIZE_RESPONSE_SET_IP - 13;
   const cl_macaddr_t my_mac_addr = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26};
   cl_slmp_set_ipaddr_resp_t resp = {
      .header =
         {
            .sub1           = CC_TO_BE16 (0x5400), /* Big endian */
            .serial         = CC_TO_LE16 (0x0123),
            .sub2           = CC_TO_LE16 (0x0000),
            .network_number = 0x45,
            .unit_number    = 0x67,
            .io_number      = CC_TO_LE16 (0x89AB),
            .extension      = 0xCD,
            .length         = CC_TO_LE16 (length),
            .endcode        = CC_TO_LE16 (CL_SLMP_ENDCODE_SUCCESS),
         },
      .master_mac_addr = {0x26, 0x25, 0x24, 0x23, 0x22, 0x21},
   };
   const cl_slmp_set_ipaddr_resp_t default_resp = resp;

   EXPECT_EQ (cl_slmp_validate_set_ip_response (&resp, &my_mac_addr), 0);

   /* MAC address */
   resp.master_mac_addr[0] = 0xAA;
   EXPECT_EQ (cl_slmp_validate_set_ip_response (&resp, &my_mac_addr), -1);
   resp = default_resp;
   EXPECT_EQ (cl_slmp_validate_set_ip_response (&resp, &my_mac_addr), 0);

   /* Length */
   resp.header.length = 0xFF;
   EXPECT_EQ (cl_slmp_validate_set_ip_response (&resp, &my_mac_addr), -1);
   resp = default_resp;
   EXPECT_EQ (cl_slmp_validate_set_ip_response (&resp, &my_mac_addr), 0);
}

TEST_F (SlmpUnitTest, SlmpValidateErrorResponse)
{
   const uint16_t length         = SIZE_RESPONSE_ERROR - 13;
   cl_slmp_error_resp_t response = {
      .header =
         {
            .sub1           = CC_TO_BE16 (0x5400), /* Big endian */
            .serial         = CC_TO_LE16 (0x0123),
            .sub2           = CC_TO_LE16 (0x0000),
            .network_number = 0x45,
            .unit_number    = 0x67,
            .io_number      = CC_TO_LE16 (0x89AB),
            .extension      = 0xCD,
            .length         = CC_TO_LE16 (length),
            .endcode        = CC_TO_LE16 (0xEF12),
         },
      .error_network_number = 0x00,
      .error_unit_number    = 0xFF,
      .error_io_number      = CC_TO_LE16 (0x03FF),
      .error_extension      = 0x00,
      .command              = CC_TO_LE16 (0x3456),
      .sub_command          = CC_TO_LE16 (0x789A)};
   const cl_slmp_error_resp_t default_response = response;
   EXPECT_EQ (cl_slmp_validate_error_response (&response), 0);

   /* Error network number */
   response.error_network_number = 0x01;
   EXPECT_EQ (cl_slmp_validate_error_response (&response), -1);
   response = default_response;
   EXPECT_EQ (cl_slmp_validate_error_response (&response), 0);

   /* Error unit number */
   response.error_unit_number = 0x00;
   EXPECT_EQ (cl_slmp_validate_error_response (&response), -1);
   response = default_response;
   EXPECT_EQ (cl_slmp_validate_error_response (&response), 0);

   /* Error IO number */
   response.error_io_number = CC_TO_LE16 (0x0333);
   EXPECT_EQ (cl_slmp_validate_error_response (&response), -1);
   response = default_response;
   EXPECT_EQ (cl_slmp_validate_error_response (&response), 0);

   /* Error extension */
   response.error_extension = 0x01;
   EXPECT_EQ (cl_slmp_validate_error_response (&response), -1);
   response = default_response;
   EXPECT_EQ (cl_slmp_validate_error_response (&response), 0);

   /* Length */
   response.header.length = 0xFF;
   EXPECT_EQ (cl_slmp_validate_error_response (&response), -1);
   response = default_response;
   EXPECT_EQ (cl_slmp_validate_error_response (&response), 0);
}
/************************* Create frames *********************************/

TEST_F (SlmpUnitTest, SlmpPrepareRequestHeader)
{
   cl_slmp_req_header_t header;
   const uint16_t serial_number = 0x1234;
   const uint16_t command       = 0x1234;
   const uint16_t sub_command   = 0x5678;
   const size_t udp_payload_len = 0x9ABC;

   cl_slmp_prepare_request_header (
      serial_number,
      command,
      sub_command,
      udp_payload_len,
      &header);

   EXPECT_EQ (header.sub1, CC_TO_BE16 (0x5400)); /* Big endian */
   EXPECT_EQ (header.serial, CC_TO_LE16 (serial_number));
   EXPECT_EQ (header.sub2, CC_TO_LE16 (0x0000));
   EXPECT_EQ (header.network_number, 0x00);
   EXPECT_EQ (header.unit_number, 0xFF);
   EXPECT_EQ (header.io_number, CC_TO_LE16 (0x03FF));
   EXPECT_EQ (header.extension, 0x00);
   EXPECT_EQ (header.length, CC_TO_LE16 (udp_payload_len - 13));
   EXPECT_EQ (header.timer, CC_TO_LE16 (0000));
   EXPECT_EQ (header.command, CC_TO_LE16 (command));
   EXPECT_EQ (header.sub_command, CC_TO_LE16 (sub_command));
}

TEST_F (SlmpUnitTest, SlmpPrepareResponseHeader)
{
   cl_slmp_resp_header_t header;
   const uint16_t serial_number = 0x1234;
   const uint16_t end_code      = 0x5678;
   const size_t udp_payload_len = 0x9ABC;

   cl_slmp_prepare_response_header (
      serial_number,
      end_code,
      udp_payload_len,
      &header);

   EXPECT_EQ (header.sub1, CC_TO_BE16 (0xD400)); /* Big endian */
   EXPECT_EQ (header.serial, CC_TO_LE16 (serial_number));
   EXPECT_EQ (header.sub2, CC_TO_LE16 (0x0000));
   EXPECT_EQ (header.network_number, 0x00);
   EXPECT_EQ (header.unit_number, 0xFF);
   EXPECT_EQ (header.io_number, CC_TO_LE16 (0x03FF));
   EXPECT_EQ (header.extension, 0x00);
   EXPECT_EQ (header.length, CC_TO_LE16 (udp_payload_len - 13));
   EXPECT_EQ (header.endcode, CC_TO_LE16 (end_code));
}

/**
 * Validate NodeSearch request frame
 *
 * @req REQ_CL_SLMP_02
 *
 */
TEST_F (SlmpUnitTest, SlmpPrepareNodeSearchRequestFrame)
{
   uint8_t buffer[400]                = {0};
   const cl_macaddr_t master_mac_addr = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};

   cl_slmp_prepare_node_search_request_frame (
      (uint8_t *)&buffer,
      sizeof (buffer),
      &master_mac_addr,
      0x01020304UL, /* Master IP address 1.2.3.4 */
      0x9ABC        /* Request serial */
   );

   EXPECT_EQ (buffer[0], 0x54); /* Fixed value in response */
   EXPECT_EQ (buffer[1], 0x00);
   EXPECT_EQ (buffer[2], 0xBC); /* Serial number */
   EXPECT_EQ (buffer[3], 0x9A);
   EXPECT_EQ (buffer[4], 0x00); /* Fixed value */
   EXPECT_EQ (buffer[5], 0x00);
   EXPECT_EQ (buffer[6], 0x00); /* Network number */
   EXPECT_EQ (buffer[7], 0xFF); /* Unit number */
   EXPECT_EQ (buffer[8], 0xFF); /* IO number */
   EXPECT_EQ (buffer[9], 0x03);
   EXPECT_EQ (buffer[10], 0x00);    /* Extension */
   EXPECT_EQ (buffer[11], 30 - 13); /* Length */
   EXPECT_EQ (buffer[12], 0x00);
   EXPECT_EQ (buffer[13], 0x00); /* Timer */
   EXPECT_EQ (buffer[14], 0x00);
   EXPECT_EQ (buffer[15], 0x30); /* Command */
   EXPECT_EQ (buffer[16], 0x0E);
   EXPECT_EQ (buffer[17], 0x00); /* Sub-command */
   EXPECT_EQ (buffer[18], 0x00);
   EXPECT_EQ (buffer[19], 0x16); /* Master MAC address */
   EXPECT_EQ (buffer[20], 0x15);
   EXPECT_EQ (buffer[21], 0x14);
   EXPECT_EQ (buffer[22], 0x13);
   EXPECT_EQ (buffer[23], 0x12);
   EXPECT_EQ (buffer[24], 0x11);
   EXPECT_EQ (buffer[25], 4);    /* Master IP address size */
   EXPECT_EQ (buffer[26], 0x04); /* Master IP address */
   EXPECT_EQ (buffer[27], 0x03);
   EXPECT_EQ (buffer[28], 0x02);
   EXPECT_EQ (buffer[29], 0x01);
}

/**
 * Validate Node Search response frame
 *
 * @req REQ_CL_SLMP_05
 * @req REQ_CL_SLMP_06
 * @req REQ_CL_SLMP_07
 * @req REQ_CL_SLMP_08
 * @req REQ_CL_SLMP_09
 * @req REQ_CL_SLMP_10
 * @req REQ_CL_SLMP_11
 *
 */
TEST_F (SlmpUnitTest, SlmpPrepareNodeSearchResponseFrame)
{
   uint8_t buffer[400]                = {0};
   const cl_macaddr_t master_mac_addr = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};
   const cl_macaddr_t slave_mac_addr  = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26};

   cl_slmp_prepare_node_search_response_frame (
      (uint8_t *)&buffer,
      sizeof (buffer),
      &master_mac_addr,
      0x01020304UL, /* Master IP address 1.2.3.4 */
      &slave_mac_addr,
      0x01020306UL, /* Slave IP address 1.2.3.6 */
      0xFFFFFF00UL, /* Slave netmask 255.255.255.0 */
      0x5678U,      /* Slmp slave status */
      0x7172,       /* Vendor code */
      0x73747576,   /* Model code */
      0x7778,       /* Equipment version */
      0x9ABC        /* Request serial */
   );

   EXPECT_EQ (buffer[0], 0xD4); /* Fixed value in response */
   EXPECT_EQ (buffer[1], 0x00);
   EXPECT_EQ (buffer[2], 0xBC); /* Serial number */
   EXPECT_EQ (buffer[3], 0x9A);
   EXPECT_EQ (buffer[4], 0x00); /* Fixed value */
   EXPECT_EQ (buffer[5], 0x00);
   EXPECT_EQ (buffer[6], 0x00); /* Network number */
   EXPECT_EQ (buffer[7], 0xFF); /* Unit number */
   EXPECT_EQ (buffer[8], 0xFF); /* IO number */
   EXPECT_EQ (buffer[9], 0x03);
   EXPECT_EQ (buffer[10], 0x00);    /* Extension */
   EXPECT_EQ (buffer[11], 66 - 13); /* Length */
   EXPECT_EQ (buffer[12], 0x00);
   EXPECT_EQ (buffer[13], 0x00); /* End code = Success */
   EXPECT_EQ (buffer[14], 0x00);
   EXPECT_EQ (buffer[15], 0x16); /* Master MAC address */
   EXPECT_EQ (buffer[16], 0x15);
   EXPECT_EQ (buffer[17], 0x14);
   EXPECT_EQ (buffer[18], 0x13);
   EXPECT_EQ (buffer[19], 0x12);
   EXPECT_EQ (buffer[20], 0x11);
   EXPECT_EQ (buffer[21], 4);    /* Master IP address size */
   EXPECT_EQ (buffer[22], 0x04); /* Master IP address */
   EXPECT_EQ (buffer[23], 0x03);
   EXPECT_EQ (buffer[24], 0x02);
   EXPECT_EQ (buffer[25], 0x01);
   EXPECT_EQ (buffer[26], 0x26); /* Slave MAC address */
   EXPECT_EQ (buffer[27], 0x25);
   EXPECT_EQ (buffer[28], 0x24);
   EXPECT_EQ (buffer[29], 0x23);
   EXPECT_EQ (buffer[30], 0x22);
   EXPECT_EQ (buffer[31], 0x21);
   EXPECT_EQ (buffer[32], 4);    /* Slave IP address size */
   EXPECT_EQ (buffer[33], 0x06); /* Slave IP address */
   EXPECT_EQ (buffer[34], 0x03);
   EXPECT_EQ (buffer[35], 0x02);
   EXPECT_EQ (buffer[36], 0x01);
   EXPECT_EQ (buffer[37], 0x00); /* Slave netmask */
   EXPECT_EQ (buffer[38], 0xFF);
   EXPECT_EQ (buffer[39], 0xFF);
   EXPECT_EQ (buffer[40], 0xFF);
   EXPECT_EQ (buffer[41], 0xFF); /* Slave default gateway */
   EXPECT_EQ (buffer[42], 0xFF);
   EXPECT_EQ (buffer[43], 0xFF);
   EXPECT_EQ (buffer[44], 0xFF);
   EXPECT_EQ (buffer[45], 0);    /* Slave hostname size */
   EXPECT_EQ (buffer[46], 0x72); /* Vendor code */
   EXPECT_EQ (buffer[47], 0x71);
   EXPECT_EQ (buffer[48], 0x76); /* Model code */
   EXPECT_EQ (buffer[49], 0x75);
   EXPECT_EQ (buffer[50], 0x74);
   EXPECT_EQ (buffer[51], 0x73);
   EXPECT_EQ (buffer[52], 0x78); /* Equipment version */
   EXPECT_EQ (buffer[53], 0x77);
   EXPECT_EQ (buffer[54], 4);    /* Target IP address size */
   EXPECT_EQ (buffer[55], 0xFF); /* Target IP address  */
   EXPECT_EQ (buffer[56], 0xFF);
   EXPECT_EQ (buffer[57], 0xFF);
   EXPECT_EQ (buffer[58], 0xFF);
   EXPECT_EQ (buffer[59], 0xFF); /* Target port */
   EXPECT_EQ (buffer[60], 0xFF);
   EXPECT_EQ (buffer[61], 0x78); /* SLMP slave status */
   EXPECT_EQ (buffer[62], 0x56);
   EXPECT_EQ (buffer[63], 0x0B); /* Slave port */
   EXPECT_EQ (buffer[64], 0xF0);
   EXPECT_EQ (buffer[65], 0x01); /* Slave protocol */
}

/**
 * Validate Set IP request frame
 *
 * @req REQ_CL_SLMP_03
 *
 */
TEST_F (SlmpUnitTest, SlmpPrepeareSetIpRequestFrame)
{
   uint8_t buffer[400]                = {0};
   const cl_macaddr_t master_mac_addr = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};
   const cl_macaddr_t slave_mac_addr  = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26};

   cl_slmp_prepare_set_ip_request_frame (
      (uint8_t *)&buffer,
      sizeof (buffer),
      &master_mac_addr,
      0x01020304UL, /* Master IP address 1.2.3.4 */
      0x9ABC,       /* Request serial */
      &slave_mac_addr,
      0x01020306UL, /* New slave IP address 1.2.3.6 */
      0xFFFFFF00UL  /* New slave netmask 255.255.255.0 */
   );

   EXPECT_EQ (buffer[0], 0x54); /* Fixed value in response */
   EXPECT_EQ (buffer[1], 0x00);
   EXPECT_EQ (buffer[2], 0xBC); /* Serial number */
   EXPECT_EQ (buffer[3], 0x9A);
   EXPECT_EQ (buffer[4], 0x00); /* Fixed value */
   EXPECT_EQ (buffer[5], 0x00);
   EXPECT_EQ (buffer[6], 0x00); /* Network number */
   EXPECT_EQ (buffer[7], 0xFF); /* Unit number */
   EXPECT_EQ (buffer[8], 0xFF); /* IO number */
   EXPECT_EQ (buffer[9], 0x03);
   EXPECT_EQ (buffer[10], 0x00);    /* Extension */
   EXPECT_EQ (buffer[11], 58 - 13); /* Length */
   EXPECT_EQ (buffer[12], 0x00);
   EXPECT_EQ (buffer[13], 0x00); /* Timer */
   EXPECT_EQ (buffer[14], 0x00);
   EXPECT_EQ (buffer[15], 0x31); /* Command */
   EXPECT_EQ (buffer[16], 0x0E);
   EXPECT_EQ (buffer[17], 0x00); /* Sub-command */
   EXPECT_EQ (buffer[18], 0x00);
   EXPECT_EQ (buffer[19], 0x16); /* Master MAC address */
   EXPECT_EQ (buffer[20], 0x15);
   EXPECT_EQ (buffer[21], 0x14);
   EXPECT_EQ (buffer[22], 0x13);
   EXPECT_EQ (buffer[23], 0x12);
   EXPECT_EQ (buffer[24], 0x11);
   EXPECT_EQ (buffer[25], 4);    /* Master IP address size */
   EXPECT_EQ (buffer[26], 0x04); /* Master IP address */
   EXPECT_EQ (buffer[27], 0x03);
   EXPECT_EQ (buffer[28], 0x02);
   EXPECT_EQ (buffer[29], 0x01);
   EXPECT_EQ (buffer[30], 0x26); /* Slave MAC address */
   EXPECT_EQ (buffer[31], 0x25);
   EXPECT_EQ (buffer[32], 0x24);
   EXPECT_EQ (buffer[33], 0x23);
   EXPECT_EQ (buffer[34], 0x22);
   EXPECT_EQ (buffer[35], 0x21);
   EXPECT_EQ (buffer[36], 4);    /* Slave IP address size */
   EXPECT_EQ (buffer[37], 0x06); /* Slave IP address */
   EXPECT_EQ (buffer[38], 0x03);
   EXPECT_EQ (buffer[39], 0x02);
   EXPECT_EQ (buffer[40], 0x01);
   EXPECT_EQ (buffer[41], 0x00); /* Slave netmask */
   EXPECT_EQ (buffer[42], 0xFF);
   EXPECT_EQ (buffer[43], 0xFF);
   EXPECT_EQ (buffer[44], 0xFF);
   EXPECT_EQ (buffer[45], 0xFF); /* Slave default gateway */
   EXPECT_EQ (buffer[46], 0xFF);
   EXPECT_EQ (buffer[47], 0xFF);
   EXPECT_EQ (buffer[48], 0xFF);
   EXPECT_EQ (buffer[49], 0);    /* Slave hostname size */
   EXPECT_EQ (buffer[50], 4);    /* Target IP address size */
   EXPECT_EQ (buffer[51], 0xFF); /* Target IP address  */
   EXPECT_EQ (buffer[52], 0xFF);
   EXPECT_EQ (buffer[53], 0xFF);
   EXPECT_EQ (buffer[54], 0xFF);
   EXPECT_EQ (buffer[55], 0xFF); /* Target port */
   EXPECT_EQ (buffer[56], 0xFF);
   EXPECT_EQ (buffer[57], 0x01); /* Slave protocol */
}

TEST_F (SlmpUnitTest, SlmpPrepeareSetIpResponseFrame)
{
   uint8_t buffer[400]                = {0};
   const cl_macaddr_t master_mac_addr = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};

   cl_slmp_prepare_set_ip_response_frame (
      (uint8_t *)&buffer,
      sizeof (buffer),
      &master_mac_addr,
      0x9ABC /* Request serial */
   );

   EXPECT_EQ (buffer[0], 0xD4); /* Fixed value in response */
   EXPECT_EQ (buffer[1], 0x00);
   EXPECT_EQ (buffer[2], 0xBC); /* Serial number */
   EXPECT_EQ (buffer[3], 0x9A);
   EXPECT_EQ (buffer[4], 0x00); /* Fixed value */
   EXPECT_EQ (buffer[5], 0x00);
   EXPECT_EQ (buffer[6], 0x00); /* Network number */
   EXPECT_EQ (buffer[7], 0xFF); /* Unit number */
   EXPECT_EQ (buffer[8], 0xFF); /* IO number */
   EXPECT_EQ (buffer[9], 0x03);
   EXPECT_EQ (buffer[10], 0x00);    /* Extension */
   EXPECT_EQ (buffer[11], 21 - 13); /* Length */
   EXPECT_EQ (buffer[12], 0x00);
   EXPECT_EQ (buffer[13], 0x00); /* End code = Success */
   EXPECT_EQ (buffer[14], 0x00);
   EXPECT_EQ (buffer[15], 0x16); /* Master MAC address */
   EXPECT_EQ (buffer[16], 0x15);
   EXPECT_EQ (buffer[17], 0x14);
   EXPECT_EQ (buffer[18], 0x13);
   EXPECT_EQ (buffer[19], 0x12);
   EXPECT_EQ (buffer[20], 0x11);
}

TEST_F (SlmpUnitTest, SlmpPrepeareErrorResponseFrame)
{
   uint8_t buffer[400] = {0};

   cl_slmp_prepare_error_response_frame (
      (uint8_t *)&buffer,
      sizeof (buffer),
      0x3132, /* Command */
      0x4142, /* Subcommand */
      0x5152, /* End code */
      0x6162  /* Request serial */
   );

   EXPECT_EQ (buffer[0], 0xD4); /* Fixed value in response */
   EXPECT_EQ (buffer[1], 0x00);
   EXPECT_EQ (buffer[2], 0x62); /* Serial number */
   EXPECT_EQ (buffer[3], 0x61);
   EXPECT_EQ (buffer[4], 0x00); /* Fixed value */
   EXPECT_EQ (buffer[5], 0x00);
   EXPECT_EQ (buffer[6], 0x00); /* Network number */
   EXPECT_EQ (buffer[7], 0xFF); /* Unit number */
   EXPECT_EQ (buffer[8], 0xFF); /* IO number */
   EXPECT_EQ (buffer[9], 0x03);
   EXPECT_EQ (buffer[10], 0x00);    /* Extension */
   EXPECT_EQ (buffer[11], 24 - 13); /* Length */
   EXPECT_EQ (buffer[12], 0x00);
   EXPECT_EQ (buffer[13], 0x52); /* End code */
   EXPECT_EQ (buffer[14], 0x51);
   EXPECT_EQ (buffer[15], 0x00); /* Network number */
   EXPECT_EQ (buffer[16], 0xFF); /* Unit number */
   EXPECT_EQ (buffer[17], 0xFF); /* IO number */
   EXPECT_EQ (buffer[18], 0x03);
   EXPECT_EQ (buffer[19], 0x00); /* Extension */
   EXPECT_EQ (buffer[20], 0x32); /* Command */
   EXPECT_EQ (buffer[21], 0x31);
   EXPECT_EQ (buffer[22], 0x42); /* Subcommand */
   EXPECT_EQ (buffer[23], 0x41);
}

/************************* Sanity check *************************************/

TEST_F (SlmpUnitTest, SlmpSanityNodeSearchRequest)
{
   uint8_t buffer[400]                = {0};
   const cl_macaddr_t master_mac_addr = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};
   const cl_ipaddr_t master_ip_addr   = 0x01020304UL; /* 1.2.3.4 */
   const uint16_t request_serial      = 0x9ABC;
   cl_slmp_node_search_request_t * result = nullptr;

   /* Prepare frame */
   cl_slmp_prepare_node_search_request_frame (
      (uint8_t *)&buffer,
      sizeof (buffer),
      &master_mac_addr,
      master_ip_addr,
      request_serial);

   /* Parse frame */
   EXPECT_EQ (
      cl_slmp_parse_node_search_request (
         (uint8_t *)&buffer,
         SIZE_REQUEST_NODE_SEARCH,
         &result),
      0);

   /* Validate */
   EXPECT_EQ (cl_slmp_validate_node_search_request (result), 0);
   EXPECT_EQ (CC_FROM_LE16 (result->header.serial), request_serial);
   EXPECT_EQ (CC_FROM_LE32 (result->master_ip_addr), master_ip_addr);
   EXPECT_TRUE (ReversedMacMatch (&result->master_mac_addr, &master_mac_addr));
}

TEST_F (SlmpUnitTest, SlmpSanityNodeSearchResponse)
{
   uint8_t buffer[400]                 = {0};
   const cl_macaddr_t master_mac_addr  = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};
   const cl_ipaddr_t master_ip_addr    = 0x01020304UL; /* 1.2.3.4 */
   const cl_macaddr_t slave_mac_addr   = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26};
   const cl_ipaddr_t slave_ip_addr     = 0x010203046L; /* 1.2.3.6 */
   const cl_ipaddr_t slave_netmask     = 0xFFFFFF00UL; /* 255.255.255.0 */
   const uint16_t slave_status         = 0x5678;
   const uint16_t vendor_code          = 0x7172;
   const uint32_t model_code           = 0x73747576;
   const uint16_t equipment_version    = 0x7778;
   const uint16_t request_serial       = 0x9ABC;
   cl_slmp_node_search_resp_t * result = nullptr;

   /* Prepare frame */
   cl_slmp_prepare_node_search_response_frame (
      (uint8_t *)&buffer,
      sizeof (buffer),
      &master_mac_addr,
      master_ip_addr,
      &slave_mac_addr,
      slave_ip_addr,
      slave_netmask,
      slave_status,
      vendor_code,
      model_code,
      equipment_version,
      request_serial);

   /* Parse frame */
   EXPECT_EQ (
      cl_slmp_parse_node_search_response (
         (uint8_t *)&buffer,
         SIZE_RESPONSE_NODE_SEARCH,
         &result),
      0);

   /* Validate */
   EXPECT_EQ (cl_slmp_validate_node_search_response (result), 0);
   EXPECT_TRUE (ReversedMacMatch (&result->master_mac_addr, &master_mac_addr));
   EXPECT_EQ (CC_FROM_LE32 (result->master_ip_addr), master_ip_addr);
   EXPECT_TRUE (ReversedMacMatch (&result->slave_mac_addr, &slave_mac_addr));
   EXPECT_EQ (CC_FROM_LE32 (result->slave_ip_addr), slave_ip_addr);
   EXPECT_EQ (CC_FROM_LE32 (result->slave_netmask), slave_netmask);
   EXPECT_EQ (CC_FROM_LE16 (result->slave_status), slave_status);
   EXPECT_EQ (CC_FROM_LE16 (result->vendor_code), vendor_code);
   EXPECT_EQ (CC_FROM_LE32 (result->model_code), model_code);
   EXPECT_EQ (CC_FROM_LE16 (result->equipment_ver), equipment_version);
   EXPECT_EQ (CC_FROM_LE16 (result->header.serial), request_serial);
}

TEST_F (SlmpUnitTest, SlmpSanitySetIpRequest)
{
   uint8_t buffer[400]                   = {0};
   const cl_macaddr_t master_mac_addr    = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};
   const cl_ipaddr_t master_ip_addr      = 0x01020304UL; /* 1.2.3.4 */
   const uint16_t request_serial         = 0x9ABC;
   const cl_macaddr_t slave_mac_addr     = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26};
   const cl_ipaddr_t slave_ip_addr       = 0x010203046L; /* 1.2.3.6 */
   const cl_ipaddr_t slave_netmask       = 0xFFFFFF00UL; /* 255.255.255.0 */
   cl_slmp_set_ipaddr_request_t * result = nullptr;

   /* Prepare frame */
   cl_slmp_prepare_set_ip_request_frame (
      (uint8_t *)&buffer,
      sizeof (buffer),
      &master_mac_addr,
      master_ip_addr,
      request_serial,
      &slave_mac_addr,
      slave_ip_addr,
      slave_netmask);

   /* Parse frame */
   EXPECT_EQ (
      cl_slmp_parse_set_ip_request ((uint8_t *)&buffer, SIZE_REQUEST_SET_IP, &result),
      0);

   /* Validate */
   EXPECT_EQ (cl_slmp_validate_set_ip_request (result), 0);
   EXPECT_TRUE (ReversedMacMatch (&result->master_mac_addr, &master_mac_addr));
   EXPECT_EQ (CC_FROM_LE32 (result->master_ip_addr), master_ip_addr);
   EXPECT_EQ (CC_FROM_LE16 (result->header.serial), request_serial);
   EXPECT_TRUE (ReversedMacMatch (&result->slave_mac_addr, &slave_mac_addr));
   EXPECT_EQ (CC_FROM_LE32 (result->slave_new_ip_addr), slave_ip_addr);
   EXPECT_EQ (CC_FROM_LE32 (result->slave_new_netmask), slave_netmask);
}

TEST_F (SlmpUnitTest, SlmpSanitySetIpResponse)
{
   uint8_t buffer[400]                = {0};
   const cl_macaddr_t master_mac_addr = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};
   const uint16_t request_serial      = 0x9ABC;
   cl_slmp_set_ipaddr_resp_t * result = nullptr;

   /* Prepare frame */
   cl_slmp_prepare_set_ip_response_frame (
      (uint8_t *)&buffer,
      sizeof (buffer),
      &master_mac_addr,
      request_serial);

   /* Parse frame */
   EXPECT_EQ (
      cl_slmp_parse_set_ip_response (
         (uint8_t *)&buffer,
         SIZE_RESPONSE_SET_IP,
         &result),
      0);

   /* Validate */
   EXPECT_EQ (cl_slmp_validate_set_ip_response (result, &master_mac_addr), 0);
   EXPECT_TRUE (ReversedMacMatch (&result->master_mac_addr, &master_mac_addr));
   EXPECT_EQ (CC_FROM_LE16 (result->header.serial), request_serial);
}

TEST_F (SlmpUnitTest, SlmpSanityErrorResponse)
{
   uint8_t buffer[400]           = {0};
   const uint16_t command        = 0x3132;
   const uint16_t sub_command    = 0x4142;
   const uint16_t end_code       = 0x5152;
   const uint16_t request_serial = 0x6162;
   cl_slmp_error_resp_t * result = nullptr;

   /* Prepare frame */
   cl_slmp_prepare_error_response_frame (
      (uint8_t *)&buffer,
      sizeof (buffer),
      command,
      sub_command,
      end_code,
      request_serial);

   /* Parse frame */
   EXPECT_EQ (
      cl_slmp_parse_error_response ((uint8_t *)&buffer, SIZE_RESPONSE_ERROR, &result),
      0);

   /* Validate */
   EXPECT_EQ (cl_slmp_validate_error_response (result), 0);
   EXPECT_EQ (CC_FROM_LE16 (result->command), command);
   EXPECT_EQ (CC_FROM_LE16 (result->sub_command), sub_command);
   EXPECT_EQ (CC_FROM_LE16 (result->header.serial), request_serial);
   EXPECT_EQ (CC_FROM_LE16 (result->header.endcode), end_code);
}
