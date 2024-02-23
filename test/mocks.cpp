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

#include "mocks.h"

#include "common/cl_file.h"
#include "common/cl_util.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

cl_mock_data_t mock_data;

void mock_clear (void)
{
   clal_clear_memory (&mock_data, sizeof (mock_data));

   /* See also values given in utils_for_testing.h */

   mock_data.interfaces[0].ifname     = "ethmock0";
   mock_data.interfaces[0].ifindex    = 4;
   mock_data.interfaces[0].ip_address = 0x01020306; /* IP 1.2.3.6 for slave */
   mock_data.interfaces[0].mac_address[0] = 0x51;   /* MAC address for slave */
   mock_data.interfaces[0].mac_address[1] = 0x52;
   mock_data.interfaces[0].mac_address[2] = 0x53;
   mock_data.interfaces[0].mac_address[3] = 0x54;
   mock_data.interfaces[0].mac_address[4] = 0x55;
   mock_data.interfaces[0].mac_address[5] = 0x56;
   mock_data.interfaces[0].netmask    = 0xFFFF0000; /* 255.255.0.0 for slave */
   mock_data.interfaces[1].ifname     = "ethmock1";
   mock_data.interfaces[1].ifindex    = 7;
   mock_data.interfaces[1].ip_address = 0x01020307; /* IP 1.2.3.7 */
   mock_data.interfaces[1].mac_address[0] = 0x71;
   mock_data.interfaces[1].mac_address[1] = 0x72;
   mock_data.interfaces[1].mac_address[2] = 0x73;
   mock_data.interfaces[1].mac_address[3] = 0x74;
   mock_data.interfaces[1].mac_address[4] = 0x75;
   mock_data.interfaces[1].mac_address[5] = 0x76;
   mock_data.interfaces[1].netmask        = 0xFFFFFF00; /* 255.255.255.0 */

   mock_data.unix_timestamp_ms = 1640000000000; /* December 2021 */
}

void mock_clear_master (void)
{
#ifdef _WINDOWS
   const char file_fullpath[] = "my_directory\\clm_data_param_no.bin";
#else
   const char file_fullpath[] = "my_directory/clm_data_param_no.bin";
#endif
   const uint32_t magic                = CC_FROM_BE32 (0x434C4E4B); /* "CLNK" */
   const uint32_t version              = CC_FROM_BE32 (0x00000001);
   const uint16_t initial_parameter_no = 500; /* See also utils_for_testing.h */

   clal_clear_memory (&mock_data, sizeof (mock_data));

   /* See also values given in utils_for_testing.h */
   mock_data.interfaces[0].ifname     = "ethmock0";
   mock_data.interfaces[0].ifindex    = 5;
   mock_data.interfaces[0].ip_address = 0x01020304; /* IP 1.2.3.4 for master */
   mock_data.interfaces[0].netmask    = 0xFFFF0000; /* 255.255.0.0 for master */
   mock_data.interfaces[0].mac_address[0] = 0x21;   /* MAC address for master */
   mock_data.interfaces[0].mac_address[1] = 0x22;
   mock_data.interfaces[0].mac_address[2] = 0x23;
   mock_data.interfaces[0].mac_address[3] = 0x24;
   mock_data.interfaces[0].mac_address[4] = 0x25;
   mock_data.interfaces[0].mac_address[5] = 0x26;
   mock_data.interfaces[1].ifname         = "ethmock1";
   mock_data.interfaces[1].ifindex        = 9;
   mock_data.interfaces[1].ip_address     = 0x01080304; /* IP 1.8.3.4 */
   mock_data.interfaces[1].netmask        = 0xFFFFFF00; /* 255.255.255.0 */
   mock_data.interfaces[1].mac_address[0] = 0x64;
   mock_data.interfaces[1].mac_address[1] = 0x65;
   mock_data.interfaces[1].mac_address[2] = 0x66;
   mock_data.interfaces[1].mac_address[3] = 0x67;
   mock_data.interfaces[1].mac_address[4] = 0x68;
   mock_data.interfaces[1].mac_address[5] = 0x69;

   mock_data.unix_timestamp_ms = 1640000000000; /* December 2021 */
   mock_data.timestamp_us      = 123456;

   /* Set up file with parameter number */
   mock_data.storage_files[0].in_use = true;
   clal_copy_string (
      mock_data.storage_files[0].file_fullpath,
      file_fullpath,
      sizeof (mock_data.storage_files[0].file_fullpath));
   mock_data.storage_files[0].file_size = 10;
   clal_memcpy (
      mock_data.storage_files[0].file_content,
      sizeof (mock_data.storage_files[0].file_content),
      &magic,
      sizeof (magic));
   clal_memcpy (
      &(mock_data.storage_files[0].file_content[sizeof (magic)]),
      sizeof (mock_data.storage_files[0].file_content) - sizeof (magic),
      &version,
      sizeof (version));
   clal_memcpy (
      &(mock_data.storage_files[0].file_content[sizeof (magic) + sizeof (version)]),
      sizeof (mock_data.storage_files[0].file_content) - sizeof (magic) -
         sizeof (version),
      &initial_parameter_no,
      sizeof (initial_parameter_no));
}

void * mock_calloc (size_t nitems, size_t size)
{
   if (mock_data.will_fail_calloc)
   {
      return nullptr;
   }
   return calloc (nitems, size);
}

uint32_t mock_os_get_current_time_us (void)
{
   return mock_data.timestamp_us;
}

int mock_clal_init (void)
{
   return mock_data.clal_init_returnvalue;
}

int mock_clal_exit (void)
{
   return mock_data.clal_exit_returnvalue;
}

uint64_t mock_clal_get_unix_timestamp_ms (void)
{
   return mock_data.unix_timestamp_ms;
}

void mock_transfer_udp_fakedata (
   cl_mock_udp_port_t * recieve_port,
   cl_mock_udp_port_t * send_port)
{
   if (!send_port->in_use || !recieve_port->in_use)
   {
      return;
   }

   if (send_port->output_data_size == 0)
   {
      return;
   }

   recieve_port->input_data_size    = send_port->output_data_size;
   recieve_port->remote_source_port = send_port->port_number;
   if ((send_port->ip_addr & 0x000000FF) == 0x000000FF)
   {
      recieve_port->remote_source_ip = send_port->local_ip_addr;
   }
   else
   {
      recieve_port->remote_source_ip = send_port->ip_addr;
   }
   clal_memcpy (
      recieve_port->udp_input_buffer,
      sizeof (recieve_port->udp_input_buffer),
      send_port->udp_output_buffer,
      send_port->output_data_size);
   send_port->output_data_size = 0;
}

void mock_set_udp_fakedata (
   cl_mock_udp_port_t * mock_port,
   cl_ipaddr_t remote_source_ip,
   uint16_t remote_source_port,
   uint8_t * data,
   size_t size)
{
   size_t resulting_size;
   if (mock_port->is_open)
   {
      resulting_size = MIN (mock_port->buf_size, size);
      clal_memcpy (
         mock_port->udp_input_buffer,
         sizeof (mock_port->udp_input_buffer),
         data,
         resulting_size);
      mock_port->remote_source_ip   = remote_source_ip;
      mock_port->remote_source_port = remote_source_port;
      mock_port->input_data_size    = resulting_size;
   }
}

void mock_set_udp_fakedata_with_local_ipaddr (
   cl_mock_udp_port_t * mock_port,
   cl_ipaddr_t local_ip_addr,
   int local_ifindex,
   cl_ipaddr_t remote_source_ip,
   uint16_t remote_source_port,
   uint8_t * data,
   size_t size)
{
   if (mock_port->is_open)
   {
      mock_port->local_ip_addr = local_ip_addr;
      mock_port->local_ifindex = (uint16_t)local_ifindex;
   }

   mock_set_udp_fakedata (
      mock_port,
      remote_source_ip,
      remote_source_port,
      data,
      size);
}

/**
 * Validate fixed parts of SLMP request header
 *
 * @param buffer        Buffer
 * @param message_size  Message size
 * @return true if valid, false if invalid
 */
static bool mock_is_fake_slmp_request_header_valid (
   uint8_t * buffer,
   size_t message_size)
{
   cl_slmp_req_header_t * header;

   if (message_size < sizeof (cl_slmp_req_header_t))
   {
      return false;
   }

   header = (cl_slmp_req_header_t *)buffer;

   if (CC_FROM_BE16 (header->sub1) != 0x5400)
   {
      return false;
   }
   if (CC_FROM_LE16 (header->sub2) != 0x0000)
   {
      return false;
   }
   if (CC_FROM_LE16 (header->io_number) != 0x03FF)
   {
      return false;
   }
   if (header->network_number != 0x00)
   {
      return false;
   }
   if (header->unit_number != 0xFF)
   {
      return false;
   }
   if (header->extension != 0x00)
   {
      return false;
   }
   if (CC_FROM_LE16 (header->timer) != 0x0000)
   {
      return false;
   }

   return true;
}

/**
 * Validate fixed parts of SLMP response header
 *
 * @param buffer        Buffer
 * @param message_size  Message size
 * @return true if valid, false if invalid
 */
static bool mock_is_fake_slmp_response_header_valid (
   uint8_t * buffer,
   size_t message_size)
{
   cl_slmp_resp_header_t * header;

   if (message_size < sizeof (cl_slmp_resp_header_t))
   {
      return false;
   }

   header = (cl_slmp_resp_header_t *)buffer;

   if (CC_FROM_BE16 (header->sub1) != 0xD400)
   {
      return false;
   }
   if (CC_FROM_LE16 (header->sub2) != 0x0000)
   {
      return false;
   }
   if (CC_FROM_LE16 (header->io_number) != 0x03FF)
   {
      return false;
   }
   if (header->network_number != 0x00)
   {
      return false;
   }
   if (header->unit_number != 0xFF)
   {
      return false;
   }
   if (header->extension != 0x00)
   {
      return false;
   }

   return true;
}

int mock_analyze_cyclic_request (
   cl_mock_udp_port_t * udp_port,
   cl_mock_cyclic_request_result_t * result)
{
   cl_cciefb_cyclic_req_full_headers_t * full_headers;

   if (udp_port->output_data_size < sizeof (cl_cciefb_cyclic_req_full_headers_t))
   {
      return -1;
   }

   /* Check fixed part of the message */
   full_headers =
      (cl_cciefb_cyclic_req_full_headers_t *)udp_port->udp_output_buffer;
   if (CC_FROM_BE16 (full_headers->req_header.reserved1) != 0x5000)
   {
      return -1;
   }
   if (full_headers->req_header.reserved2 != 0x00)
   {
      return -1;
   }
   if (full_headers->req_header.reserved3 != 0xFF)
   {
      return -1;
   }
   if (CC_FROM_LE16 (full_headers->req_header.reserved4) != 0x03FF)
   {
      return -1;
   }
   if (full_headers->req_header.reserved5 != 0x00)
   {
      return -1;
   }
   if (CC_FROM_LE16 (full_headers->cyclic_header.reserved1) != 0x0000)
   {
      return -1;
   }
   if (
      full_headers->cyclic_header.reserved2[0] != 0 ||
      full_headers->cyclic_header.reserved2[1] != 0 ||
      full_headers->cyclic_header.reserved2[13] != 0)
   {
      return -1;
   }
   if (CC_FROM_LE16 (full_headers->master_station_notification.reserved) != 0x0000)
   {
      return -1;
   }
   if (full_headers->cyclic_data_header.reserved3 != 0x00)
   {
      return -1;
   }
   if (CC_FROM_LE16 (full_headers->cyclic_data_header.reserved4) != 0x0000)
   {
      return -1;
   }

   /* Extract variable parts from the buffer.
      Use byte numbers from specification (to find problems in our typedefs)
      Take little-endian into account */
   result->dl =
      udp_port->udp_output_buffer[7] + 0x100 * udp_port->udp_output_buffer[8];
   result->command =
      udp_port->udp_output_buffer[11] + 0x100 * udp_port->udp_output_buffer[12];
   result->sub_command =
      udp_port->udp_output_buffer[13] + 0x100 * udp_port->udp_output_buffer[14];
   result->master_protocol_ver =
      udp_port->udp_output_buffer[15] + 0x100 * udp_port->udp_output_buffer[16];
   result->master_local_unit_info =
      udp_port->udp_output_buffer[35] + 0x100 * udp_port->udp_output_buffer[36];
   result->clock_info =
      (uint64_t)udp_port->udp_output_buffer[39] +
      0x100 * (uint64_t)udp_port->udp_output_buffer[40] +
      0x10000 * (uint64_t)udp_port->udp_output_buffer[41] +
      0x1000000 * (uint64_t)udp_port->udp_output_buffer[42] +
      0x100000000 * (uint64_t)udp_port->udp_output_buffer[43] +
      0x10000000000 * (uint64_t)udp_port->udp_output_buffer[44] +
      0x1000000000000 * (uint64_t)udp_port->udp_output_buffer[45] +
      0x100000000000000 * (uint64_t)udp_port->udp_output_buffer[46];
   result->master_ip_addr = udp_port->udp_output_buffer[47] +
                            0x100 * udp_port->udp_output_buffer[48] +
                            0x10000 * udp_port->udp_output_buffer[49] +
                            0x1000000 * udp_port->udp_output_buffer[50];
   result->group_no = udp_port->udp_output_buffer[51];
   result->frame_sequence_no =
      udp_port->udp_output_buffer[53] + 0x100 * udp_port->udp_output_buffer[54];
   result->timeout_value =
      udp_port->udp_output_buffer[55] + 0x100 * udp_port->udp_output_buffer[56];
   result->parallel_off_timeout_count =
      udp_port->udp_output_buffer[57] + 0x100 * udp_port->udp_output_buffer[58];
   result->parameter_no =
      udp_port->udp_output_buffer[59] + 0x100 * udp_port->udp_output_buffer[60];
   result->slave_total_occupied_station_count =
      udp_port->udp_output_buffer[61] + 0x100 * udp_port->udp_output_buffer[62];
   result->cyclic_transmission_state =
      udp_port->udp_output_buffer[63] + 0x100 * udp_port->udp_output_buffer[64];

   return 0;
}

int mock_analyze_cyclic_request_slaveid (
   cl_mock_udp_port_t * udp_port,
   uint16_t slave_station_no,
   cl_ipaddr_t * ip_addr)
{
   if (slave_station_no < 1 || slave_station_no > 16)
   {
      return -1;
   }

   const uint16_t startpos =
      (uint16_t)sizeof (cl_cciefb_cyclic_req_full_headers_t) +
      ((slave_station_no - 1U) * 4U);
   const uint16_t minsize = startpos + 4U;

   if (udp_port->output_data_size < minsize)
   {
      return -1;
   }

   /* Take little-endian into account */
   *ip_addr = udp_port->udp_output_buffer[startpos] +
              0x100 * udp_port->udp_output_buffer[startpos + 1] +
              0x10000 * udp_port->udp_output_buffer[startpos + 2] +
              0x1000000 * udp_port->udp_output_buffer[startpos + 3];

   return 0;
}

cl_rww_t * mock_analyze_cyclic_request_rww (
   cl_mock_udp_port_t * udp_port,
   uint16_t total_number_of_slavestations,
   uint16_t slave_station_no)
{

   if (slave_station_no == 0 || slave_station_no > total_number_of_slavestations)
   {
      return nullptr;
   }

   const uint16_t startpos =
      (uint16_t)sizeof (cl_cciefb_cyclic_req_full_headers_t) +
      (total_number_of_slavestations * 4U) +
      ((slave_station_no - 1U) * (uint16_t)sizeof (cl_rww_t));
   const uint16_t minsize = startpos + sizeof (cl_rww_t);

   if (udp_port->output_data_size < minsize)
   {
      return nullptr;
   }

   return (cl_rww_t *)&udp_port->udp_output_buffer[startpos];
}

int mock_analyze_fake_cyclic_response (
   cl_mock_udp_port_t * udp_port,
   cl_mock_cyclic_response_result_t * result)
{
   cl_cciefb_cyclic_resp_full_headers_t * full_headers;

   if (udp_port->output_data_size < sizeof (cl_cciefb_cyclic_resp_full_headers_t))
   {
      return -1;
   }

   /* Check fixed part of the message */
   full_headers =
      (cl_cciefb_cyclic_resp_full_headers_t *)udp_port->udp_output_buffer;
   if (CC_FROM_BE16 (full_headers->resp_header.reserved1) != 0xD000)
   {
      return -1;
   }
   if (full_headers->resp_header.reserved2 != 0x00)
   {
      return -1;
   }
   if (full_headers->resp_header.reserved3 != 0xFF)
   {
      return -1;
   }
   if (CC_FROM_LE16 (full_headers->resp_header.reserved4) != 0x03FF)
   {
      return -1;
   }
   if (full_headers->resp_header.reserved5 != 0x00)
   {
      return -1;
   }
   if (CC_FROM_LE16 (full_headers->resp_header.reserved6) != 0x0000)
   {
      return -1;
   }
   if (
      full_headers->cyclic_header.reserved1[0] != 0 ||
      full_headers->cyclic_header.reserved1[1] != 0 ||
      full_headers->cyclic_header.reserved1[13] != 0)
   {
      return -1;
   }
   if (CC_FROM_LE16 (full_headers->slave_station_notification.reserved1) != 0x0000)
   {
      return -1;
   }

   if (CC_FROM_LE16 (full_headers->slave_station_notification.reserved2) != 0x0000)
   {
      return -1;
   }
   if (full_headers->cyclic_data_header.reserved2 != 0x00)
   {
      return -1;
   }

   /* Extract variable parts from the buffer.
   Use byte numbers from specification (to find problems in our typedefs)
   Take little-endian into account */
   result->dl =
      udp_port->udp_output_buffer[7] + 0x100 * udp_port->udp_output_buffer[8];
   result->slave_protocol_ver =
      udp_port->udp_output_buffer[11] + 0x100 * udp_port->udp_output_buffer[12];
   result->end_code =
      udp_port->udp_output_buffer[13] + 0x100 * udp_port->udp_output_buffer[14];
   result->vendor_code =
      udp_port->udp_output_buffer[31] + 0x100 * udp_port->udp_output_buffer[32];
   result->model_code = udp_port->udp_output_buffer[35] +
                        0x100 * udp_port->udp_output_buffer[36] +
                        0x10000 * udp_port->udp_output_buffer[37] +
                        0x1000000 * udp_port->udp_output_buffer[38];
   result->equiment_version =
      udp_port->udp_output_buffer[39] + 0x100 * udp_port->udp_output_buffer[40];
   result->slave_local_unit_info =
      udp_port->udp_output_buffer[43] + 0x100 * udp_port->udp_output_buffer[44];
   result->slave_err_code =
      udp_port->udp_output_buffer[45] + 0x100 * udp_port->udp_output_buffer[46];
   result->local_management_info = udp_port->udp_output_buffer[47] +
                                   0x100 * udp_port->udp_output_buffer[48] +
                                   0x10000 * udp_port->udp_output_buffer[49] +
                                   0x1000000 * udp_port->udp_output_buffer[50];
   result->slave_id = udp_port->udp_output_buffer[51] +
                      0x100 * udp_port->udp_output_buffer[52] +
                      0x10000 * udp_port->udp_output_buffer[53] +
                      0x1000000 * udp_port->udp_output_buffer[54];
   result->group_no = udp_port->udp_output_buffer[55]; /* uint8_t in frame */
   result->frame_sequence_no =
      udp_port->udp_output_buffer[57] + 0x100 * udp_port->udp_output_buffer[58];

   return 0;
}

cl_rwr_t * mock_analyze_cyclic_response_rwr (
   cl_mock_udp_port_t * udp_port,
   uint16_t areanumber)
{
   const uint16_t startpos =
      (uint16_t)sizeof (cl_cciefb_cyclic_resp_full_headers_t) +
      areanumber * (uint16_t)sizeof (cl_rwr_t);
   const uint16_t minsize = startpos + sizeof (cl_rwr_t);

   if (udp_port->output_data_size < minsize)
   {
      return nullptr;
   }

   return (cl_rwr_t *)&udp_port->udp_output_buffer[startpos];
}

cl_rx_t * mock_analyze_cyclic_response_rx (
   cl_mock_udp_port_t * udp_port,
   uint16_t number_of_occupied_stations,
   uint16_t areanumber)
{
   if (number_of_occupied_stations == 0)
   {
      return nullptr;
   }

   if (areanumber >= number_of_occupied_stations)
   {
      return nullptr;
   }

   const uint16_t startpos =
      (uint16_t)sizeof (cl_cciefb_cyclic_resp_full_headers_t) +
      number_of_occupied_stations * (uint16_t)sizeof (cl_rwr_t) +
      areanumber * (uint16_t)sizeof (cl_rx_t);
   const uint16_t minsize = startpos + sizeof (cl_rx_t);

   if (udp_port->output_data_size < minsize)
   {
      return nullptr;
   }

   return (cl_rx_t *)&udp_port->udp_output_buffer[startpos];
}

int mock_analyze_fake_node_search_request (
   cl_mock_udp_port_t * udp_port,
   cl_mock_node_search_request_result_t * result)
{
   cl_slmp_node_search_request_t * parsed;

   if (udp_port->output_data_size < sizeof (cl_slmp_node_search_request_t))
   {
      return -1;
   }
   if (!mock_is_fake_slmp_request_header_valid (
          udp_port->udp_output_buffer,
          udp_port->output_data_size))
   {
      return -1;
   }

   /* Check fixed part of the message */
   parsed = (cl_slmp_node_search_request_t *)udp_port->udp_output_buffer;
   if (parsed->master_ip_addr_size != 4)
   {
      return -1;
   }

   /* Extract variable parts from the buffer.
      Use byte numbers from specification (to find problems in our typedefs)
      Take little-endian into account */
   result->serial =
      udp_port->udp_output_buffer[2] + 0x100 * udp_port->udp_output_buffer[3];
   result->length =
      udp_port->udp_output_buffer[11] + 0x100 * udp_port->udp_output_buffer[12];
   result->command =
      udp_port->udp_output_buffer[15] + 0x100 * udp_port->udp_output_buffer[16];
   result->sub_command =
      udp_port->udp_output_buffer[17] + 0x100 * udp_port->udp_output_buffer[18];
   cl_util_copy_mac_reverse (
      &result->master_mac_addr,
      (cl_macaddr_t *)&udp_port->udp_output_buffer[19]);
   result->master_ip_addr = udp_port->udp_output_buffer[26] +
                            0x100 * udp_port->udp_output_buffer[27] +
                            0x10000 * udp_port->udp_output_buffer[28] +
                            0x1000000 * udp_port->udp_output_buffer[29];

   return 0;
}

int mock_analyze_fake_node_search_response (
   cl_mock_udp_port_t * udp_port,
   cl_mock_node_search_response_result_t * result)
{
   cl_slmp_node_search_resp_t * parsed;

   if (udp_port->output_data_size < sizeof (cl_slmp_node_search_resp_t))
   {
      return -1;
   }
   if (!mock_is_fake_slmp_response_header_valid (
          udp_port->udp_output_buffer,
          udp_port->output_data_size))
   {
      return -1;
   }

   /* Check fixed part of the message */
   parsed = (cl_slmp_node_search_resp_t *)udp_port->udp_output_buffer;
   if (parsed->master_ip_addr_size != 4)
   {
      return -1;
   }
   if (parsed->slave_ip_addr_size != 4)
   {
      return -1;
   }
   if (parsed->slave_hostname_size != 0)
   {
      return -1;
   }
   if (parsed->target_ip_addr_size != 4)
   {
      return -1;
   }
   if (CC_FROM_LE16 (parsed->target_port) != 0xFFFF)
   {
      return -1;
   }
   if (CC_FROM_LE32 (parsed->slave_default_gateway) != 0xFFFFFFFF)
   {
      return -1;
   }
   if (CC_FROM_LE32 (parsed->target_ip_addr) != 0xFFFFFFFF)
   {
      return -1;
   }
   if (CC_FROM_LE16 (parsed->slave_port) != 61451)
   {
      return -1;
   }
   if (parsed->slave_protocol_settings != 1)
   {
      return -1;
   }

   /* Extract variable parts from the buffer.
      Use byte numbers from specification (to find problems in our typedefs)
      Take little-endian into account */
   result->serial =
      udp_port->udp_output_buffer[2] + 0x100 * udp_port->udp_output_buffer[3];
   result->length =
      udp_port->udp_output_buffer[11] + 0x100 * udp_port->udp_output_buffer[12];
   result->end_code =
      udp_port->udp_output_buffer[13] + 0x100 * udp_port->udp_output_buffer[14];
   cl_util_copy_mac_reverse (
      &result->master_mac_addr,
      (cl_macaddr_t *)&udp_port->udp_output_buffer[15]);
   result->master_ip_addr = udp_port->udp_output_buffer[22] +
                            0x100 * udp_port->udp_output_buffer[23] +
                            0x10000 * udp_port->udp_output_buffer[24] +
                            0x1000000 * udp_port->udp_output_buffer[25];
   cl_util_copy_mac_reverse (
      &result->slave_mac_addr,
      (cl_macaddr_t *)&udp_port->udp_output_buffer[26]);
   result->slave_ip_addr = udp_port->udp_output_buffer[33] +
                           0x100 * udp_port->udp_output_buffer[34] +
                           0x10000 * udp_port->udp_output_buffer[35] +
                           0x1000000 * udp_port->udp_output_buffer[36];
   result->slave_netmask = udp_port->udp_output_buffer[37] +
                           0x100 * udp_port->udp_output_buffer[38] +
                           0x10000 * udp_port->udp_output_buffer[39] +
                           0x1000000 * udp_port->udp_output_buffer[40];
   result->vendor_code =
      udp_port->udp_output_buffer[46] + 0x100 * udp_port->udp_output_buffer[47];
   result->model_code = udp_port->udp_output_buffer[48] +
                        0x100 * udp_port->udp_output_buffer[49] +
                        0x10000 * udp_port->udp_output_buffer[50] +
                        0x1000000 * udp_port->udp_output_buffer[51];
   result->equipment_ver =
      udp_port->udp_output_buffer[52] + 0x100 * udp_port->udp_output_buffer[53];
   result->slave_status =
      udp_port->udp_output_buffer[61] + 0x100 * udp_port->udp_output_buffer[62];

   return 0;
}

int mock_analyze_fake_set_ip_request (
   cl_mock_udp_port_t * udp_port,
   cl_mock_set_ip_request_result_t * result)
{
   cl_slmp_set_ipaddr_request_t * parsed;

   if (udp_port->output_data_size < sizeof (cl_slmp_set_ipaddr_request_t))
   {
      return -1;
   }
   if (!mock_is_fake_slmp_request_header_valid (
          udp_port->udp_output_buffer,
          udp_port->output_data_size))
   {
      return -1;
   }

   /* Check fixed part of the message */
   parsed = (cl_slmp_set_ipaddr_request_t *)udp_port->udp_output_buffer;
   if (parsed->master_ip_addr_size != 4)
   {
      return -1;
   }
   if (parsed->slave_ip_addr_size != 4)
   {
      return -1;
   }
   if (parsed->slave_hostname_size != 0)
   {
      return -1;
   }
   if (parsed->target_ip_addr_size != 4)
   {
      return -1;
   }
   if (CC_FROM_LE32 (parsed->slave_default_gateway) != 0xFFFFFFFF)
   {
      return -1;
   }
   if (CC_FROM_LE32 (parsed->target_ip_addr) != 0xFFFFFFFF)
   {
      return -1;
   }
   if (CC_FROM_LE16 (parsed->target_port) != 0xFFFF)
   {
      return -1;
   }
   if (parsed->slave_protocol_settings != 1)
   {
      return -1;
   }

   /* Extract variable parts from the buffer.
      Use byte numbers from specification (to find problems in our typedefs)
      Take little-endian into account */
   result->serial =
      udp_port->udp_output_buffer[2] + 0x100 * udp_port->udp_output_buffer[3];
   result->length =
      udp_port->udp_output_buffer[11] + 0x100 * udp_port->udp_output_buffer[12];
   result->command =
      udp_port->udp_output_buffer[15] + 0x100 * udp_port->udp_output_buffer[16];
   result->sub_command =
      udp_port->udp_output_buffer[17] + 0x100 * udp_port->udp_output_buffer[18];
   cl_util_copy_mac_reverse (
      &result->master_mac_addr,
      (cl_macaddr_t *)&udp_port->udp_output_buffer[19]);
   result->master_ip_addr = udp_port->udp_output_buffer[26] +
                            0x100 * udp_port->udp_output_buffer[27] +
                            0x10000 * udp_port->udp_output_buffer[28] +
                            0x1000000 * udp_port->udp_output_buffer[29];
   cl_util_copy_mac_reverse (
      &result->slave_mac_addr,
      (cl_macaddr_t *)&udp_port->udp_output_buffer[30]);
   result->slave_ip_addr = udp_port->udp_output_buffer[37] +
                           0x100 * udp_port->udp_output_buffer[38] +
                           0x10000 * udp_port->udp_output_buffer[39] +
                           0x1000000 * udp_port->udp_output_buffer[40];
   result->slave_netmask = udp_port->udp_output_buffer[41] +
                           0x100 * udp_port->udp_output_buffer[42] +
                           0x10000 * udp_port->udp_output_buffer[43] +
                           0x1000000 * udp_port->udp_output_buffer[44];

   return 0;
}

int mock_analyze_fake_set_ip_response (
   cl_mock_udp_port_t * udp_port,
   cl_mock_set_ip_response_result_t * result)
{
   if (udp_port->output_data_size < sizeof (cl_slmp_set_ipaddr_resp_t))
   {
      return -1;
   }
   if (!mock_is_fake_slmp_response_header_valid (
          udp_port->udp_output_buffer,
          udp_port->output_data_size))
   {
      return -1;
   }

   /* Extract variable parts from the buffer.
      Use byte numbers from specification (to find problems in our typedefs)
      Take little-endian into account */
   result->serial =
      udp_port->udp_output_buffer[2] + 0x100 * udp_port->udp_output_buffer[3];
   result->length =
      udp_port->udp_output_buffer[11] + 0x100 * udp_port->udp_output_buffer[12];
   result->end_code =
      udp_port->udp_output_buffer[13] + 0x100 * udp_port->udp_output_buffer[14];
   cl_util_copy_mac_reverse (
      &result->master_mac_addr,
      (cl_macaddr_t *)&udp_port->udp_output_buffer[15]);

   return 0;
}

int mock_analyze_fake_error_response (
   cl_mock_udp_port_t * udp_port,
   cl_mock_error_response_result_t * result)
{
   cl_slmp_error_resp_t * parsed;

   if (udp_port->output_data_size < sizeof (cl_slmp_error_resp_t))
   {
      return -1;
   }
   if (!mock_is_fake_slmp_response_header_valid (
          udp_port->udp_output_buffer,
          udp_port->output_data_size))
   {
      return -1;
   }

   /* Check fixed part of the message */
   parsed = (cl_slmp_error_resp_t *)udp_port->udp_output_buffer;
   if (CC_FROM_LE16 (parsed->error_io_number) != 0x03FF)
   {
      return -1;
   }
   if (parsed->error_network_number != 0x00)
   {
      return -1;
   }
   if (parsed->error_unit_number != 0xFF)
   {
      return -1;
   }
   if (parsed->error_extension != 0x00)
   {
      return -1;
   }

   /* Extract variable parts from the buffer.
      Use byte numbers from specification (to find problems in our typedefs)
      Take little-endian into account */
   result->serial =
      udp_port->udp_output_buffer[2] + 0x100 * udp_port->udp_output_buffer[3];
   result->length =
      udp_port->udp_output_buffer[11] + 0x100 * udp_port->udp_output_buffer[12];
   result->end_code =
      udp_port->udp_output_buffer[13] + 0x100 * udp_port->udp_output_buffer[14];
   result->command =
      udp_port->udp_output_buffer[20] + 0x100 * udp_port->udp_output_buffer[21];
   result->sub_command =
      udp_port->udp_output_buffer[22] + 0x100 * udp_port->udp_output_buffer[23];

   return 0;
}

void cl_mock_show_mocked_interfaces (void)
{
   cl_mock_network_interface_t * mock_interf;
   char ip_string[CL_INET_ADDRSTR_SIZE]      = {0}; /** Terminated string */
   char netmask_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
   size_t i_interface                        = 0;

   for (i_interface = 0; i_interface < NELEMENTS (mock_data.interfaces);
        i_interface++)
   {
      mock_interf = &mock_data.interfaces[i_interface];
      printf (
         "Mock interface ifindex %u \"%s\" (array index %u)\n",
         mock_interf->ifindex,
         mock_interf->ifname,
         (unsigned)i_interface);
      printf (
         "  MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
         mock_interf->mac_address[0],
         mock_interf->mac_address[1],
         mock_interf->mac_address[2],
         mock_interf->mac_address[3],
         mock_interf->mac_address[4],
         mock_interf->mac_address[5]);
      cl_util_ip_to_string (mock_interf->ip_address, ip_string);
      printf ("  IP address: %s\n", ip_string);
      cl_util_ip_to_string (mock_interf->netmask, netmask_string);
      printf ("  Netmask: %s\n", netmask_string);
   }
}

void cl_mock_show_mocked_udp_ports (void)
{
   cl_mock_udp_port_t * udp_port;
   size_t i_port                        = 0;
   char ip_string[CL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */

   for (i_port = 0; i_port < NELEMENTS (mock_data.udp_ports); i_port++)
   {
      udp_port = &mock_data.udp_ports[i_port];
      if (!udp_port->in_use)
      {
         printf ("Handle %u not in use\n", (unsigned)(i_port + 1));
         continue;
      }
      cl_util_ip_to_string (udp_port->ip_addr, ip_string);
      printf (
         "Handle %u  IP %s  UDP port %u %s  ifindex %u\n",
         udp_port->handle_number,
         ip_string,
         udp_port->port_number,
         udp_port->is_open ? "Open" : "Closed",
         udp_port->ifindex);
      printf (
         "  Buffer size: %u bytes  Input data: %u bytes  Output "
         "data: %u bytes\n",
         (unsigned)udp_port->buf_size,
         (unsigned)udp_port->input_data_size,
         (unsigned)udp_port->output_data_size);
      printf (
         "  Total sent: %u bytes  received: %u bytes\n",
         (unsigned)udp_port->total_sent_bytes,
         (unsigned)udp_port->total_recv_bytes);
      printf (
         "  Calls open: %u  close: %u  send: %u  receive: %u\n",
         udp_port->number_of_calls_open,
         udp_port->number_of_calls_close,
         udp_port->number_of_calls_send,
         udp_port->number_of_calls_recv);
      if (udp_port->will_fail_open)
      {
         printf ("  Will fail to open\n");
      }
      if (udp_port->will_fail_send)
      {
         printf ("  Will fail to send\n");
      }
      if (udp_port->will_fail_recv)
      {
         printf ("  Will fail to receive\n");
      }
      cl_util_ip_to_string (udp_port->remote_destination_ip, ip_string);
      printf (
         "  Remote destination: %s port %u\n",
         ip_string,
         udp_port->remote_destination_port);
      cl_util_ip_to_string (udp_port->remote_source_ip, ip_string);
      printf (
         "  Remote source: %s port %u\n",
         ip_string,
         udp_port->remote_source_port);
      if (udp_port->local_ip_addr != CL_IPADDR_ANY || udp_port->local_ifindex != 0)
      {
         cl_util_ip_to_string (udp_port->local_ip_addr, ip_string);
         printf (
            "  Local IP address: %s ifindex %d (used if main IP "
            "address is broadcast)\n",
            ip_string,
            udp_port->local_ifindex);
      }
   }
}

/**
 * Find a simulated network interface
 *
 * @param ifindex   Interface index
 * @return Pointer to simulated interfacw, or nullptr if not found.
 */
cl_mock_network_interface_t * mock_helper_find_interface (int ifindex)
{
   unsigned int i;

   for (i = 0; i < NELEMENTS (mock_data.interfaces); i++)
   {
      if (mock_data.interfaces[i].ifindex == ifindex)
      {
         return &mock_data.interfaces[i];
      }
   }

   return nullptr;
}

int mock_clal_get_ifindex (uint32_t ip_address, int * ifindex)
{
   unsigned int i;

   if (mock_data.will_fail_read_ifindex)
   {
      return -1;
   }

   for (i = 0; i < NELEMENTS (mock_data.interfaces); i++)
   {
      if (mock_data.interfaces[i].ip_address == ip_address)
      {
         *ifindex = mock_data.interfaces[i].ifindex;
         return 0;
      }
   }

   return -1;
};

int mock_clal_get_mac_address (int ifindex, uint8_t (*mac_address)[6])
{
   cl_mock_network_interface_t * mock_interf =
      mock_helper_find_interface (ifindex);
   if (mock_interf == nullptr)
   {
      return -1;
   }

   if (mock_data.will_fail_read_mac_addr)
   {
      return -1;
   }

   cl_util_copy_mac (mac_address, (cl_macaddr_t *)mock_interf->mac_address);

   return 0;
}

int mock_clal_get_ifname (int ifindex, char * ifname)
{
   cl_mock_network_interface_t * mock_interf =
      mock_helper_find_interface (ifindex);
   if (mock_interf == nullptr)
   {
      return -1;
   }

   if (mock_data.will_fail_read_ifname)
   {
      return -1;
   }

   (void)clal_snprintf (ifname, CLAL_IFNAME_SIZE, "%s", mock_interf->ifname);

   return 0;
};

int mock_clal_get_netmask (int ifindex, uint32_t * netmask)
{
   cl_mock_network_interface_t * mock_interf =
      mock_helper_find_interface (ifindex);
   if (mock_interf == nullptr)
   {
      return -1;
   }

   if (mock_data.will_fail_read_netmask)
   {
      return -1;
   }

   *netmask = mock_interf->netmask;

   return 0;
}

int mock_clal_set_ip_address_netmask (int ifindex, uint32_t ip_addr, uint32_t netmask)
{
   cl_mock_network_interface_t * mock_interf =
      mock_helper_find_interface (ifindex);
   if (mock_interf == nullptr)
   {
      return -1;
   }

   mock_data.number_of_calls_set_ip_address_netmask++;

   if (mock_data.will_fail_set_ip_addr)
   {
      return -1;
   }

   mock_interf->ip_address = ip_addr;
   mock_interf->netmask    = netmask;

   return 0;
}

int mock_clal_udp_open (uint32_t ip_addr, uint16_t port_number)
{
   bool is_broadcast            = (ip_addr & 0x000000FF) == 0x000000FF;
   bool found_interface         = false;
   int ifindex                  = 0;
   bool found_reusable_port     = false;
   ssize_t use_port_array_index = -1;
   cl_mock_udp_port_t * udp_port;
   cl_mock_network_interface_t * interf;
   size_t i_interface = 0;
   size_t i_port      = 0;

   /* Make sure the port_number not is already occupied.
    * Reuse closed port if possible */
   if (!is_broadcast && ip_addr != CL_IPADDR_ANY)
   {
      for (i_port = 0; i_port < NELEMENTS (mock_data.udp_ports); i_port++)
      {
         udp_port = &mock_data.udp_ports[i_port];
         if (udp_port->in_use && udp_port->ip_addr == ip_addr && udp_port->port_number == port_number)
         {
            if (udp_port->is_open)
            {
               /* Port is already in use */
               return -1;
            }

            /* Port has been used before, but is now closed */
            found_reusable_port  = true;
            use_port_array_index = (ssize_t)i_port;
            break;
         }
      }
   }

   /* Find network interface */
   if (ip_addr != CL_IPADDR_ANY)
   {
      for (i_interface = 0; i_interface < NELEMENTS (mock_data.interfaces);
           i_interface++)
      {
         interf = &mock_data.interfaces[i_interface];
         if (is_broadcast)
         {
            if (
               ip_addr == cl_util_calc_broadcast_address (
                             interf->netmask,
                             interf->ip_address))
            {
               found_interface = true;
               ifindex         = interf->ifindex;
               break;
            }
         }
         else
         {
            if (ip_addr == interf->ip_address)
            {
               found_interface = true;
               ifindex         = interf->ifindex;
               break;
            }
         }
      }

      if (!found_interface)
      {
         /* IP address not found on interfaces */
         return -1;
      }
   }

   if (!found_reusable_port)
   {
      /* Allocate a new handle for a UDP port */
      for (i_port = 0; i_port < NELEMENTS (mock_data.udp_ports); i_port++)
      {
         udp_port = &mock_data.udp_ports[i_port];
         if (udp_port->in_use)
         {
            continue;
         }

         use_port_array_index = (ssize_t)i_port;
         break;
      }
   }

   if (use_port_array_index < 0)
   {
      return -1;
   }

   /* Now we have found correct IP address and port number */
   udp_port         = &mock_data.udp_ports[use_port_array_index];
   udp_port->in_use = true;
   udp_port->number_of_calls_open++;

   if (udp_port->will_fail_open)
   {
      return -1;
   }

   CC_ASSERT (!udp_port->is_open);
   udp_port->is_open       = true;
   udp_port->handle_number = (int)(use_port_array_index + 1);
   udp_port->ip_addr       = ip_addr;
   udp_port->port_number   = port_number;
   udp_port->ifindex       = (uint16_t)ifindex;
   udp_port->buf_size      = sizeof (udp_port->udp_input_buffer);

   udp_port->remote_source_ip        = CL_IPADDR_INVALID;
   udp_port->remote_source_port      = 0;
   udp_port->remote_destination_ip   = CL_IPADDR_INVALID;
   udp_port->remote_destination_port = 0;
   udp_port->input_data_size         = 0;
   udp_port->output_data_size        = 0;

   /* NOTE: Do not reset these as we might have set them from test code:
    *       - local_ip_addr
    *       - local_ifindex */

   return udp_port->handle_number;
}

cl_mock_udp_port_t * mock_find_simulated_udp_port (int handle)
{
   cl_mock_udp_port_t * udp_port;

   if (handle < 1)
   {
      return nullptr;
   }

   if (handle > (int)NELEMENTS (mock_data.udp_ports))
   {
      return nullptr;
   }

   udp_port = &mock_data.udp_ports[handle - 1];

   return (udp_port->in_use) ? udp_port : nullptr;
}

ssize_t mock_clal_udp_sendto (
   int handle,
   uint32_t ip,
   uint16_t port_number,
   const void * data,
   size_t size)
{
   ssize_t send_size;

   cl_mock_udp_port_t * udp_port = mock_find_simulated_udp_port (handle);
   if (udp_port == nullptr)
   {
      return -1;
   }

   udp_port->number_of_calls_send++;
   if (udp_port->is_open && !udp_port->will_fail_send && udp_port->buf_size >= size)
   {
      send_size = MIN (udp_port->buf_size, size);

      /* Fake that the port accepts fewer bytes right now */
      if (udp_port->use_modified_send_size_returnvalue)
      {
         send_size = MIN (send_size, udp_port->modified_send_size_returnvalue);
      }

      clal_memcpy (
         udp_port->udp_output_buffer,
         sizeof (udp_port->udp_output_buffer),
         data,
         send_size);
      udp_port->output_data_size = send_size;
      udp_port->total_sent_bytes += send_size;
      udp_port->remote_destination_ip   = ip;
      udp_port->remote_destination_port = port_number;

      return send_size;
   }

   return -1;
}

ssize_t mock_clal_udp_recvfrom (
   int handle,
   uint32_t * remote_ip,
   uint16_t * remote_port,
   void * data,
   size_t size)
{
   uint32_t local_ip;
   int ifindex;

   return mock_clal_udp_recvfrom_with_ifindex (
      handle,
      remote_ip,
      remote_port,
      &local_ip,
      &ifindex,
      data,
      size);
}

ssize_t mock_clal_udp_recvfrom_with_ifindex (
   int handle,
   uint32_t * remote_ip,
   uint16_t * remote_port,
   uint32_t * local_ip,
   int * ifindex,
   void * data,
   size_t size)
{
   ssize_t receive_size;

   cl_mock_udp_port_t * udp_port = mock_find_simulated_udp_port (handle);
   if (udp_port == nullptr)
   {
      return -1;
   }

   udp_port->number_of_calls_recv++;
   if (udp_port->is_open && !udp_port->will_fail_recv)
   {
      receive_size = MIN (udp_port->input_data_size, size);
      if (receive_size > 0)
      {
         if (udp_port->ip_addr == CL_IPADDR_ANY)
         {
            *local_ip = udp_port->local_ip_addr;
            *ifindex  = udp_port->local_ifindex;
         }
         else if ((udp_port->ip_addr & 0x000000FF) == 0x000000FF)
         {
            /* Listening to a broadcast address */
            *local_ip = udp_port->local_ip_addr;
            *ifindex  = udp_port->ifindex;
         }
         else
         {
            *local_ip = udp_port->ip_addr;
            *ifindex  = udp_port->ifindex;
         }

         clal_memcpy (data, size, &udp_port->udp_input_buffer, receive_size);
         udp_port->total_recv_bytes += receive_size;
         *remote_port = udp_port->remote_source_port;
         *remote_ip   = udp_port->remote_source_ip;
      }
      udp_port->input_data_size = 0; /* Support only one read */

      return receive_size;
   }

   return -1;
}

void mock_clal_udp_close (int handle)
{

   cl_mock_udp_port_t * udp_port = mock_find_simulated_udp_port (handle);
   if (udp_port == nullptr)
   {
      return;
   }

   /* NOTE: Do not reset these as we might use them for verification:
    *       - handle_number
    *       - ip_addr
    *       - port_number
    *       - ifindex
    *       - remote_destination_ip
    *       - remote_destination_port
    *       - local_ip_addr
    *       - local_ifindex
    *       - output_data_size */

   udp_port->number_of_calls_close++;
   udp_port->is_open            = false;
   udp_port->remote_source_ip   = CL_IPADDR_INVALID;
   udp_port->remote_source_port = 0;
   udp_port->input_data_size    = 0;
}

void mock_test_value_init (uint8_t * counter, uint8_t * buffer, size_t len)
{
   size_t ix;
   *counter = 0;

   for (ix = 0; ix < len; ix++)
   {
      buffer[ix] = (uint8_t)ix;
   }
}

uint8_t mock_test_value_8 (uint8_t * counter)
{
   return (*counter)++;
}

uint16_t mock_test_value_16 (uint8_t * counter, bool be)
{
   uint16_t value = 0;

   if (be)
   {
      value |= ((*counter)++ << 8);
      value |= (*counter)++;
   }
   else
   {
      value |= (*counter)++;
      value |= ((*counter)++ << 8);
   }
   return value;
}

uint32_t mock_test_value_32 (uint8_t * counter, bool be)
{
   uint32_t value = 0;

   if (be)
   {
      value |= ((*counter)++ << 24);
      value |= ((*counter)++ << 16);
      value |= ((*counter)++ << 8);
      value |= (*counter)++;
   }
   else
   {
      value |= (*counter)++;
      value |= ((*counter)++ << 8);
      value |= ((*counter)++ << 16);
      value |= ((*counter)++ << 24);
   }
   return value;
}

void cl_mock_show_mocked_files (void)
{
   size_t i;
   size_t j;
   cl_mock_master_file_info_t * found_file = nullptr;

   for (i = 0; i < NELEMENTS (mock_data.storage_files); i++)
   {
      if (!mock_data.storage_files[i].in_use)
      {
         printf ("File index %u not in use\n", (unsigned)i);
         continue;
      }

      found_file = &mock_data.storage_files[i];
      printf (
         "File index %u. Size %u bytes. \n",
         (unsigned)i,
         (unsigned)found_file->file_size);
      printf ("  Path:     %s\n", found_file->file_fullpath);
      printf ("  Contents: ");
      for (j = 0; j < found_file->file_size; j++)
      {
         printf ("0x%02X ", mock_data.storage_files[i].file_content[j]);
      }
      printf ("\n");
   }
}

/**
 * Find a simulated file
 *
 * @param fullpath   Full path to simulated file
 * @return Pointer to simulated file, or nullptr if not found.
 */
static cl_mock_master_file_info_t * mock_helper_find_file (const char * fullpath)
{
   unsigned int i;
   cl_mock_master_file_info_t * found_file = nullptr;

   for (i = 0; i < NELEMENTS (mock_data.storage_files); i++)
   {
      if (
         mock_data.storage_files[i].in_use &&
         strcmp (fullpath, mock_data.storage_files[i].file_fullpath) == 0)
      {
         found_file = &mock_data.storage_files[i];
         break;
      }
   }

   return found_file;
}

int mock_clal_save_file (
   const char * fullpath,
   const void * object_1,
   size_t size_1,
   const void * object_2,
   size_t size_2)
{
   unsigned int i;
   cl_mock_master_file_info_t * current_file = nullptr;

   if (size_1 + size_2 > sizeof (current_file->file_content))
   {
      return -1;
   }
   if (strlen (fullpath) >= sizeof (current_file->file_fullpath))
   {
      return -1;
   }

   /* Look for existing file */
   current_file = mock_helper_find_file (fullpath);

   /* Create new file if necessary */
   if (current_file == nullptr)
   {
      for (i = 0; i < NELEMENTS (mock_data.storage_files); i++)
      {
         if (!mock_data.storage_files[i].in_use)
         {
            current_file = &mock_data.storage_files[i];

            current_file->in_use = true;
            if (
               clal_copy_string (
                  current_file->file_fullpath,
                  fullpath,
                  sizeof (current_file->file_fullpath)) != 0)
            {
               return -1;
            }
            break;
         }
      }
   }
   if (current_file == nullptr)
   {
      return -1;
   }

   current_file->file_size = size_1 + size_2;
   if (size_1 > 0)
   {
      clal_memcpy (
         current_file->file_content,
         sizeof (current_file->file_content),
         object_1,
         size_1);
   }
   if (size_2 > 0)
   {
      clal_memcpy (
         &current_file->file_content[size_1],
         sizeof (current_file->file_content) - size_1,
         object_2,
         size_2);
   }

   return 0;
}

void mock_clal_clear_file (const char * fullpath)
{
   cl_mock_master_file_info_t * found_file = mock_helper_find_file (fullpath);

   if (found_file == nullptr)
   {
      return;
   }

   clal_clear_memory (found_file, sizeof (*found_file));
}

int mock_clal_load_file (
   const char * fullpath,
   void * object_1,
   size_t size_1,
   void * object_2,
   size_t size_2)
{
   cl_mock_master_file_info_t * found_file = mock_helper_find_file (fullpath);

   if (found_file == nullptr)
   {
      return -1;
   }

   if (size_1 + size_2 > found_file->file_size)
   {
      return -1;
   }

   if (size_1 > 0)
   {
      clal_memcpy (object_1, size_1, found_file->file_content, size_1);
   }
   if (size_2 > 0)
   {
      clal_memcpy (object_2, size_2, &found_file->file_content[size_1], size_2);
   }

   return 0;
}
