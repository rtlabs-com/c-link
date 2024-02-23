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
 * @brief Parsing, validating and building frames for CCIEFB cyclic data
 *
 * Intended to be useful for both master and slave.
 * The functions in this file should not have knowledge on master or slave
 * states.
 *
 * It is preferred that any logging is done when calling these functions
 * (not in this file, if avoidable).
 *
 * No mocking should be necessary for testing these functions.
 *
 */

#include "common/cl_iefb.h"

#include "common/cl_types.h"

#include <inttypes.h>
#include <string.h>

void cl_iefb_initialise_request_frame (
   uint8_t * buffer,
   uint32_t buf_size,
   uint16_t protocol_ver,
   uint16_t timeout_value,
   uint16_t parallel_off_timeout_count,
   cl_ipaddr_t master_id,
   uint16_t group_no,
   uint16_t occupied_stations,
   uint16_t parameter_no,
   clm_cciefb_cyclic_request_info_t * frame_info)
{
   uint32_t pos = 0;
   cl_cciefb_req_header_t * req_header;
   cl_cciefb_cyclic_req_header_t * cyclic_header;
   cl_cciefb_master_station_notification_t * notification;
   cl_cciefb_cyclic_req_data_header_t * cyclic_data_header;

   frame_info->udp_payload_len =
      cl_calculate_cyclic_request_size (occupied_stations);
   CC_ASSERT (buf_size >= frame_info->udp_payload_len);

   /* Store reference to buffer */
   clal_clear_memory (buffer, buf_size);
   frame_info->buffer       = buffer;
   frame_info->buf_size     = buf_size;
   frame_info->full_headers = (cl_cciefb_cyclic_req_full_headers_t *)buffer;

   /* Request header
      Note that reserved1 is coded in big endian.
      See section 9.1.2 in BAP-C2010-ENG-004-A */
   req_header            = &frame_info->full_headers->req_header;
   req_header->reserved1 = CC_TO_BE16 (CL_CCIEFB_REQ_HEADER_RESERVED1);
   req_header->reserved2 = CL_CCIEFB_REQ_HEADER_RESERVED2;
   req_header->reserved3 = CL_CCIEFB_REQ_HEADER_RESERVED3;
   req_header->reserved4 = CC_TO_LE16 (CL_CCIEFB_REQ_HEADER_RESERVED4);
   req_header->reserved5 = CL_CCIEFB_REQ_HEADER_RESERVED5;
   req_header->dl =
      CC_TO_LE16 (frame_info->udp_payload_len - CL_CCIEFB_REQ_HEADER_DL_OFFSET);
   req_header->reserved6   = CC_TO_LE16 (CL_CCIEFB_REQ_HEADER_RESERVED6);
   req_header->command     = CC_TO_LE16 (CL_SLMP_COMMAND_CCIEFB_CYCLIC);
   req_header->sub_command = CC_TO_LE16 (CL_SLMP_SUBCOMMAND_CCIEFB_CYCLIC);

   /* Cyclic request header */
   cyclic_header               = &frame_info->full_headers->cyclic_header;
   cyclic_header->protocol_ver = CC_TO_LE16 (protocol_ver);
   cyclic_header->reserved1 = CC_TO_LE16 (CL_CCIEFB_CYCLIC_REQ_HEADER_RESERVED1);
   cyclic_header->cyclic_info_offset_addr =
      CC_TO_LE16 (CL_CCIEFB_CYCLIC_REQ_CYCLIC_OFFSET);
   clal_clear_memory (
      &cyclic_header->reserved2,
      sizeof (cyclic_header->reserved2));

   /* Master station notification */
   notification = &frame_info->full_headers->master_station_notification;
   notification->master_local_unit_info = 0; /* Update before sending */
   notification->reserved =
      CC_TO_LE16 (CL_CCIEFB_MASTER_STATION_NOTIFICATION_RESERVED);
   notification->clock_info = 0; /* Update before sending */

   /* Cyclic data request header */
   cyclic_data_header = &frame_info->full_headers->cyclic_data_header;
   cyclic_data_header->master_id = CC_TO_LE32 (master_id);
   cyclic_data_header->group_no  = group_no & UINT8_MAX;
   cyclic_data_header->reserved3 = CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_RESERVED3;
   cyclic_data_header->frame_sequence_no = 0; /* Update before sending */
   cyclic_data_header->timeout_value     = CC_TO_LE16 (timeout_value);
   cyclic_data_header->parallel_off_timeout_count =
      CC_TO_LE16 (parallel_off_timeout_count);
   cyclic_data_header->parameter_no = CC_TO_LE16 (parameter_no);
   cyclic_data_header->slave_total_occupied_station_count =
      CC_TO_LE16 (occupied_stations);
   cyclic_data_header->cyclic_transmission_state = 0; /* Update before sending
                                                       */
   cyclic_data_header->reserved4 =
      CC_TO_LE16 (CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_RESERVED4);

   /* Calculate positions for SlaveID, RWw and RY */
   pos                        = sizeof (cl_cciefb_cyclic_req_full_headers_t);
   frame_info->first_slave_id = (uint32_t *)(&buffer[pos]);
   pos += occupied_stations * sizeof (uint32_t);

   frame_info->first_rww = (cl_rww_t *)(&buffer[pos]);
   pos += occupied_stations * sizeof (cl_rww_t);

   frame_info->first_ry = (cl_ry_t *)(&buffer[pos]);
   pos += occupied_stations * sizeof (cl_ry_t);

   CC_ASSERT (pos <= buf_size);
}

void cl_iefb_update_request_frame_headers (
   clm_cciefb_cyclic_request_info_t * frame_info,
   uint16_t frame_sequence_no,
   uint64_t clock_info,
   uint16_t master_local_unit_info,
   uint16_t cyclic_transmission_state)
{
   cl_cciefb_master_station_notification_t * notification;
   cl_cciefb_cyclic_req_data_header_t * cyclic_data_header;

   /* Master station notification */
   notification = &frame_info->full_headers->master_station_notification;
   notification->master_local_unit_info = CC_TO_LE16 (master_local_unit_info);
   notification->clock_info             = CC_TO_LE64 (clock_info);

   /* Cyclic data request header */
   cyclic_data_header = &frame_info->full_headers->cyclic_data_header;
   cyclic_data_header->frame_sequence_no = CC_TO_LE16 (frame_sequence_no);
   cyclic_data_header->cyclic_transmission_state =
      CC_TO_LE16 (cyclic_transmission_state);
}

int cl_iefb_parse_request_header (
   uint8_t * buffer,
   size_t recv_len,
   cl_cciefb_req_header_t ** header)
{
   if (sizeof (cl_cciefb_req_header_t) > recv_len)
   {
      return -1;
   }

   *header = (cl_cciefb_req_header_t *)buffer;

   return 0;
}

int cl_iefb_validate_request_header (
   const cl_cciefb_req_header_t * header,
   size_t recv_len)
{
   /* Validate length */
   if (CC_FROM_LE16 (header->dl) + CL_CCIEFB_REQ_HEADER_DL_OFFSET != recv_len)
   {
      return -1;
   }

   /* Validate reserved fields
      See section 5.2.4 a) in BAP-C2010ENG-001-B
      Note that reserved1 is coded in big endian.
      See section 9.1.2 in BAP-C2010-ENG-004-A */
   if (
      CC_FROM_BE16 (header->reserved1) != CL_CCIEFB_REQ_HEADER_RESERVED1 ||
      header->reserved2 != CL_CCIEFB_REQ_HEADER_RESERVED2 ||
      header->reserved3 != CL_CCIEFB_REQ_HEADER_RESERVED3 ||
      CC_FROM_LE16 (header->reserved4) != CL_CCIEFB_REQ_HEADER_RESERVED4 ||
      header->reserved5 != CL_CCIEFB_REQ_HEADER_RESERVED5 ||
      CC_FROM_LE16 (header->reserved6) != CL_CCIEFB_REQ_HEADER_RESERVED6)
   {
      return -1;
   }

   return 0;
}

int cl_iefb_validate_cyclic_request_header (
   const cl_cciefb_cyclic_req_header_t * cyclic_header)
{
   uint16_t protocol_ver = CC_FROM_LE16 (cyclic_header->protocol_ver);
   size_t i;

   /* Validate protocol version */
   if (protocol_ver < CL_CCIEFB_MIN_SUPPORTED_PROTOCOL_VER || protocol_ver > CL_CCIEFB_MAX_SUPPORTED_PROTOCOL_VER)
   {
      return -1;
   }

   /* Validate cyclic offset */
   if (CC_FROM_LE16 (cyclic_header->cyclic_info_offset_addr) != CL_CCIEFB_CYCLIC_REQ_CYCLIC_OFFSET)
   {
      return -1;
   }

   /* Validate reserved values */
   if (CC_FROM_LE16 (cyclic_header->reserved1) != CL_CCIEFB_CYCLIC_REQ_HEADER_RESERVED1)
   {
      return -1;
   }
   for (i = 0; i < sizeof (cyclic_header->reserved2); i++)
   {
      if (cyclic_header->reserved2[i] != CL_CCIEFB_CYCLIC_REQ_HEADER_RESERVED2)
      {
         return -1;
      }
   }

   return 0;
}

int cl_iefb_validate_master_station_notification (
   const cl_cciefb_master_station_notification_t * master_station_notification,
   uint16_t protocol_ver)
{
   /* Verify master_local_unit_info bits */
   switch (protocol_ver)
   {
   case 1:
      if (
         (CC_FROM_LE16 (master_station_notification->master_local_unit_info) &
          CL_CCIEFB_MASTER_STATION_NOTIFICATION_MASK_BITS_VER1) > 0)
      {
         return -1;
      }
      break;
   case 2:
      if (
         (CC_FROM_LE16 (master_station_notification->master_local_unit_info) &
          CL_CCIEFB_MASTER_STATION_NOTIFICATION_MASK_BITS_VER2) > 0)
      {
         return -1;
      }
      break;
   default:
      return -1;
      break;
   }

   /* Verify reserved value */
   if (CC_FROM_LE16 (master_station_notification->reserved) != CL_CCIEFB_MASTER_STATION_NOTIFICATION_RESERVED)
   {
      return -1;
   }

   return 0;
}

int cl_iefb_validate_req_cyclic_data_header (
   const cl_cciefb_cyclic_req_data_header_t * cyclic_data_header,
   cl_ipaddr_t remote_ip)
{
   uint16_t group_no     = cyclic_data_header->group_no; /* uint8_t in frame */
   cl_ipaddr_t master_id = CC_FROM_LE32 (cyclic_data_header->master_id);
   uint16_t slave_total_occupied =
      CC_FROM_LE16 (cyclic_data_header->slave_total_occupied_station_count);

   /* Verify group number */
   if (group_no < CL_CCIEFB_MIN_GROUP_NO || group_no > CL_CCIEFB_MAX_GROUP_NO)
   {
      return -1;
   }

   /* Verify total number of occupied slave stations */
   if (
      slave_total_occupied < CL_CCIEFB_MIN_OCCUPIED_STATIONS_PER_GROUP ||
      slave_total_occupied > CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP)
   {
      return -1;
   }

   /* Verify reserved fields */
   if (
      cyclic_data_header->reserved3 !=
         CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_RESERVED3 ||
      CC_FROM_LE16 (cyclic_data_header->reserved4) !=
         CL_CCIEFB_CYCLIC_REQ_DATA_HEADER_RESERVED4)
   {
      return -1;
   }

   /* Verify master ID */
   if (master_id == CL_IPADDR_INVALID || master_id != remote_ip)
   {
      return -1;
   }

   return 0;
}

int cl_iefb_parse_request_cyclic_data (
   uint8_t * buffer,
   size_t recv_len,
   uint16_t slave_total_occupied_station_count,
   uint32_t ** first_slave_id,
   cl_rww_t ** first_rww,
   cl_ry_t ** first_ry)
{
   size_t pos = sizeof (cl_cciefb_cyclic_req_full_headers_t);

   /* Check slave_total_occupied_station_count validity */
   if (
      slave_total_occupied_station_count <
         CL_CCIEFB_MIN_OCCUPIED_STATIONS_PER_GROUP ||
      slave_total_occupied_station_count >
         CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP)
   {
      return -1;
   }

   *first_slave_id = (uint32_t *)(&buffer[pos]);
   pos += slave_total_occupied_station_count * sizeof (cl_ipaddr_t);

   *first_rww = (cl_rww_t *)(&buffer[pos]);
   pos += slave_total_occupied_station_count * sizeof (cl_rww_t);

   *first_ry = (cl_ry_t *)(&buffer[pos]);
   pos += slave_total_occupied_station_count * sizeof (cl_ry_t);

   if (pos > recv_len)
   {
      return -1;
   }

   return 0;
}

bool cl_is_slave_endcode_valid (cl_slmp_error_codes_t end_code)
{
   if (
      end_code == CL_SLMP_ENDCODE_SUCCESS ||
      end_code == CL_SLMP_ENDCODE_CCIEFB_MASTER_DUPLICATION ||
      end_code == CL_SLMP_ENDCODE_CCIEFB_WRONG_NUMBER_OCCUPIED_STATIONS ||
      end_code == CL_SLMP_ENDCODE_CCIEFB_SLAVE_ERROR ||
      end_code == CL_SLMP_ENDCODE_CCIEFB_SLAVE_REQUESTS_DISCONNECT)
   {
      return true;
   }

   return false;
}

size_t cl_calculate_cyclic_request_size (uint16_t slave_total_occupied_station_count)
{
   /* Fixed part 67 bytes, and 76 bytes for each occupied station. */
   return sizeof (cl_cciefb_req_header_t) +
          sizeof (cl_cciefb_cyclic_req_header_t) +
          sizeof (cl_cciefb_master_station_notification_t) +
          sizeof (cl_cciefb_cyclic_req_data_header_t) +
          slave_total_occupied_station_count *
             (sizeof (cl_ipaddr_t) + sizeof (cl_rww_t) + sizeof (cl_ry_t));
}

size_t cl_calculate_cyclic_response_size (uint16_t occupied_stations)
{
   /* Fixed part 59 bytes, and 72 bytes for each occupied station. */
   return sizeof (cl_cciefb_resp_header_t) +
          sizeof (cl_cciefb_cyclic_resp_header_t) +
          sizeof (cl_cciefb_slave_station_notification_t) +
          sizeof (cl_cciefb_cyclic_resp_data_header_t) +
          occupied_stations * (sizeof (cl_rwr_t) + sizeof (cl_rx_t));
}

uint16_t cl_calculate_number_of_occupied_stations (size_t response_udp_payload_size)
{
   uint16_t occupied_stations = 0;
   size_t remainder           = 0;

   /* Fixed part 59 bytes, and 72 bytes for each occupied station.
      See cl_calculate_cyclic_response_size() for calculation of values. */
   occupied_stations = ((response_udp_payload_size - 59) / 72) & UINT16_MAX;
   remainder         = response_udp_payload_size -
               cl_calculate_cyclic_response_size (occupied_stations);
   if (remainder != 0)
   {
      return 0; /* error */
   }

   return occupied_stations;
}

int cl_iefb_validate_req_cyclic_frame_size (
   uint16_t protocol_ver,
   size_t recv_len,
   uint16_t dl,
   uint16_t slave_total_occupied_station_count,
   uint16_t cyclic_info_offset_addr)
{
   /* The cl_cciefb_master_station_notification_t size might be dependent on
      protocol version. (But is same for v1 and v2) */
   if (protocol_ver < CL_CCIEFB_MIN_SUPPORTED_PROTOCOL_VER || protocol_ver > CL_CCIEFB_MAX_SUPPORTED_PROTOCOL_VER)
   {
      return -1;
   }

   /* Check that the length value from the header corresponds to total size */
   if (recv_len != dl + CL_CCIEFB_REQ_HEADER_DL_OFFSET)
   {
      return -1;
   }

   /* Check that the size is consistent with the number of occupied stations */
   if (recv_len != cl_calculate_cyclic_request_size (slave_total_occupied_station_count))
   {
      return -1;
   }

   /* Check offset position for cyclic data in frame */
   if (cyclic_info_offset_addr != CL_CCIEFB_CYCLIC_REQ_CYCLIC_OFFSET)
   {
      return -1;
   }

   return 0;
}

const cl_rww_t * cl_iefb_request_get_rww (
   const cls_cciefb_cyclic_request_info_t * request,
   uint16_t abs_slave_station)
{
   uint16_t slave_total_occupied_station_count = CC_FROM_LE16 (
      request->full_headers->cyclic_data_header.slave_total_occupied_station_count);

   /* Check validity of abs_slave_station */
   if (
      (abs_slave_station < CL_CCIEFB_MIN_OCCUPIED_STATIONS_PER_GROUP) ||
      (abs_slave_station > CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP) ||
      (abs_slave_station > slave_total_occupied_station_count))
   {
      return NULL;
   }

   if (request->first_rww == NULL)
   {
      return NULL;
   }

   return request->first_rww + (abs_slave_station - 1);
}

const cl_ry_t * cl_iefb_request_get_ry (
   const cls_cciefb_cyclic_request_info_t * request,
   uint16_t abs_slave_station)
{
   uint16_t slave_total_occupied_station_count = CC_FROM_LE16 (
      request->full_headers->cyclic_data_header.slave_total_occupied_station_count);

   /* Check validity of abs_slave_station */
   if (
      abs_slave_station < CL_CCIEFB_MIN_OCCUPIED_STATIONS_PER_GROUP ||
      abs_slave_station > CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP ||
      abs_slave_station > slave_total_occupied_station_count)
   {
      return NULL;
   }

   if (request->first_ry == NULL)
   {
      return NULL;
   }

   return request->first_ry + (abs_slave_station - 1);
}

int cl_iefb_request_get_slave_id (
   const uint32_t * first_slave_id,
   uint16_t abs_slave_station,
   uint16_t total_occupied,
   cl_ipaddr_t * slave_id)
{
   /* Validate the error-checking argument itself */
   if (
      total_occupied < CL_CCIEFB_MIN_OCCUPIED_STATIONS_PER_GROUP ||
      total_occupied > CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP)
   {
      return -1;
   }

   /* Check validity of abs_slave_station */
   if (
      abs_slave_station < CL_CCIEFB_MIN_OCCUPIED_STATIONS_PER_GROUP ||
      abs_slave_station > CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP ||
      abs_slave_station > total_occupied)
   {
      return -1;
   }

   *slave_id = CC_FROM_LE32 (*(first_slave_id + (abs_slave_station - 1)));

   return 0;
}

int cl_iefb_parse_req_full_cyclic_headers (
   uint8_t * buffer,
   size_t recv_len,
   cl_cciefb_cyclic_req_full_headers_t ** full_headers)
{
   if (recv_len < sizeof (cl_cciefb_cyclic_req_full_headers_t))
   {
      return -1;
   }

   *full_headers = (cl_cciefb_cyclic_req_full_headers_t *)buffer;

   return 0;
}

int cl_iefb_validate_req_full_cyclic_headers (
   const cl_cciefb_cyclic_req_full_headers_t * full_headers,
   size_t recv_len,
   cl_ipaddr_t remote_ip)
{
   /* The cl_cciefb_req_header_t has been validated earlier */

   if (cl_iefb_validate_cyclic_request_header (&full_headers->cyclic_header) != 0)
   {
      return -1;
   }

   if (
      cl_iefb_validate_master_station_notification (
         &full_headers->master_station_notification,
         CC_FROM_LE16 (full_headers->cyclic_header.protocol_ver)) != 0)
   {
      return -1;
   }

   if (cl_iefb_validate_req_cyclic_data_header (&full_headers->cyclic_data_header, remote_ip) != 0)
   {
      return -1;
   }

   if (
      cl_iefb_validate_req_cyclic_frame_size (
         CC_FROM_LE16 (full_headers->cyclic_header.protocol_ver),
         recv_len,
         CC_FROM_LE16 (full_headers->req_header.dl),
         CC_FROM_LE16 (
            full_headers->cyclic_data_header.slave_total_occupied_station_count),
         CC_FROM_LE16 (full_headers->cyclic_header.cyclic_info_offset_addr)) != 0)
   {
      return -1;
   }

   return 0;
}

int cl_iefb_parse_cyclic_request (
   uint8_t * buffer,
   size_t recv_len,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port,
   cl_ipaddr_t local_ip,
   cls_cciefb_cyclic_request_info_t * request)
{
   int result_parse_cyclic = 0;

   request->remote_port   = remote_port;
   request->remote_ip     = remote_ip;
   request->slave_ip_addr = local_ip;

   if (cl_iefb_parse_req_full_cyclic_headers (buffer, recv_len, &request->full_headers) != 0)
   {
      return -1;
   }

   if (cl_iefb_validate_req_full_cyclic_headers (request->full_headers, recv_len, remote_ip) != 0)
   {
      return -1;
   }

   result_parse_cyclic = cl_iefb_parse_request_cyclic_data (
      buffer,
      recv_len,
      CC_FROM_LE16 (request->full_headers->cyclic_data_header
                       .slave_total_occupied_station_count),
      &request->first_slave_id,
      &request->first_rww,
      &request->first_ry);

   /* Already checked in cl_iefb_validate_req_full_cyclic_headers() */
   CC_ASSERT (result_parse_cyclic == 0);

   return 0;
}

void cl_iefb_initialise_cyclic_response_frame (
   uint8_t * buffer,
   uint32_t buf_size,
   uint16_t occupied_stations,
   uint16_t vendor_code,
   uint32_t model_code,
   uint16_t equipment_ver,
   cls_cciefb_cyclic_response_info_t * frame_info)
{
   size_t pos = 0;
   cl_cciefb_resp_header_t * resp_header;
   cl_cciefb_cyclic_resp_header_t * cyclic_header;
   cl_cciefb_slave_station_notification_t * notification;
   cl_cciefb_cyclic_resp_data_header_t * cyclic_data_header;

   frame_info->udp_payload_len =
      cl_calculate_cyclic_response_size (occupied_stations);
   CC_ASSERT (buf_size >= frame_info->udp_payload_len);

   /* Store reference to buffer */
   clal_clear_memory (buffer, buf_size);
   frame_info->buffer       = buffer;
   frame_info->buf_size     = buf_size;
   frame_info->full_headers = (cl_cciefb_cyclic_resp_full_headers_t *)buffer;

   /* Response header
      Note that reserved1 is coded in big endian.
      See section 9.1.2 in BAP-C2010-ENG-004-A */
   resp_header            = &frame_info->full_headers->resp_header;
   resp_header->reserved1 = CC_TO_BE16 (CL_CCIEFB_RESP_HEADER_RESERVED1);
   resp_header->reserved2 = CL_CCIEFB_RESP_HEADER_RESERVED2;
   resp_header->reserved3 = CL_CCIEFB_RESP_HEADER_RESERVED3;
   resp_header->reserved4 = CC_TO_LE16 (CL_CCIEFB_RESP_HEADER_RESERVED4);
   resp_header->reserved5 = CL_CCIEFB_RESP_HEADER_RESERVED5;
   resp_header->dl        = CC_TO_LE16 (
      (frame_info->udp_payload_len & UINT16_MAX) -
      CL_CCIEFB_RESP_HEADER_DL_OFFSET);
   resp_header->reserved6 = CC_TO_LE16 (CL_CCIEFB_RESP_HEADER_RESERVED6);

   /* Cyclic response header */
   cyclic_header = &frame_info->full_headers->cyclic_header;
   cyclic_header->protocol_ver =
      CC_TO_LE16 (CL_CCIEFB_MAX_SUPPORTED_PROTOCOL_VER);
   cyclic_header->end_code = CC_TO_LE16 (CL_SLMP_ENDCODE_SUCCESS);
   cyclic_header->cyclic_info_offset_addr =
      CC_TO_LE16 (CL_CCIEFB_CYCLIC_RESP_CYCLIC_OFFSET);
   clal_clear_memory (
      &cyclic_header->reserved1,
      sizeof (cyclic_header->reserved1));

   /* Slave station notification */
   notification = &frame_info->full_headers->slave_station_notification;
   notification->vendor_code = CC_TO_LE16 (vendor_code);
   notification->reserved1 =
      CC_TO_LE16 (CL_CCIEFB_SLAVE_STATION_NOTIFICATION_RESERVED1);
   notification->model_code    = CC_TO_LE32 (model_code);
   notification->equipment_ver = CC_TO_LE16 (equipment_ver);
   notification->reserved2 =
      CC_TO_LE16 (CL_CCIEFB_SLAVE_STATION_NOTIFICATION_RESERVED2);
   notification->slave_local_unit_info =
      CC_TO_LE16 (CL_SLAVE_APPL_OPERATION_STATUS_OPERATING);
   notification->slave_err_code =
      CC_TO_LE16 (CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_SLAVE_ERR_CODE);
   notification->local_management_info =
      CC_TO_LE32 (CL_CCIEFB_SLAVE_STATION_NOTIFICATION_DEFAULT_LOCAL_MANAG_INFO);

   /* Cyclic data response header
      Slave ID is already cleared to 0.0.0.0 */
   cyclic_data_header           = &frame_info->full_headers->cyclic_data_header;
   cyclic_data_header->group_no = 0;
   cyclic_data_header->reserved2 = CL_CCIEFB_CYCLIC_RESP_DATA_HEADER_RESERVED2;
   cyclic_data_header->frame_sequence_no = CC_TO_LE16 (0);

   /* Calculate positions for RWr and RX */
   pos                   = sizeof (cl_cciefb_cyclic_resp_full_headers_t);
   frame_info->first_rwr = (cl_rwr_t *)(&buffer[pos]);
   pos += occupied_stations * sizeof (cl_rwr_t);

   frame_info->first_rx = (cl_rx_t *)(&buffer[pos]);
   pos += occupied_stations * sizeof (cl_rx_t);

   CC_ASSERT (pos <= buf_size);
}

void cl_iefb_update_cyclic_response_frame (
   cls_cciefb_cyclic_response_info_t * frame_info,
   cl_ipaddr_t slave_id,
   cl_slmp_error_codes_t end_code,
   uint16_t group_no,
   uint16_t frame_sequence_no,
   cl_slave_appl_operation_status_t slave_local_unit_info,
   uint16_t slave_err_code,
   uint32_t local_management_info)
{
   cl_cciefb_slave_station_notification_t * notification;
   cl_cciefb_cyclic_resp_data_header_t * cyclic_data_header;

   /* Cyclic response header */
   frame_info->full_headers->cyclic_header.end_code = CC_TO_LE16 (end_code);

   /* Slave station notification */
   notification = &frame_info->full_headers->slave_station_notification;
   notification->slave_local_unit_info = CC_TO_LE16 (slave_local_unit_info);
   notification->slave_err_code        = CC_TO_LE16 (slave_err_code);
   notification->local_management_info = CC_TO_LE32 (local_management_info);

   /* Cyclic data response header */
   cyclic_data_header           = &frame_info->full_headers->cyclic_data_header;
   cyclic_data_header->slave_id = CC_TO_LE32 (slave_id);
   cyclic_data_header->group_no = group_no & UINT8_MAX;
   cyclic_data_header->frame_sequence_no = CC_TO_LE16 (frame_sequence_no);
}

int cl_iefb_analyze_slave_ids (
   cl_ipaddr_t my_slave_id,
   uint16_t total_occupied,
   uint32_t * request_first_slave_id,
   bool * found_my_slave_id,
   uint16_t * my_slave_station_no,
   uint16_t * implied_occupation_count)
{
   cl_ipaddr_t req_slave_id   = CL_IPADDR_INVALID;
   bool look_for_multistation = false;
   uint16_t abs_slave_station = 0;

   if (my_slave_id == CL_IPADDR_INVALID)
   {
      goto error;
   }

   if (total_occupied < CL_CCIEFB_MIN_OCCUPIED_STATIONS_PER_GROUP)
   {
      goto error;
   }

   *found_my_slave_id        = false;
   *my_slave_station_no      = 0;
   *implied_occupation_count = 0;

   for (abs_slave_station = 1; abs_slave_station <= total_occupied;
        abs_slave_station++)
   {
      if (
         cl_iefb_request_get_slave_id (
            request_first_slave_id,
            abs_slave_station,
            total_occupied,
            &req_slave_id) != 0)
      {
         /* The slave id should always be valid if valid parameters are
          * given. Report error. */
         goto error;
      }

      if (req_slave_id == my_slave_id)
      {
         if (*found_my_slave_id == false)
         {
            /* First occurrence of my IP address */
            *my_slave_station_no      = abs_slave_station;
            *implied_occupation_count = 1;
            look_for_multistation     = true;
         }
         else
         {
            /* Repeated occurrence of my IP address, should not happen. */
            goto error;
         }
         *found_my_slave_id = true;
      }
      else if (look_for_multistation)
      {
         if (req_slave_id == CL_CCIEFB_MULTISTATION_INDICATOR)
         {
            (*implied_occupation_count)++;
         }
         else
         {
            look_for_multistation = false;
         }
      }
   }

   return 0;

error:
   *found_my_slave_id        = false;
   *my_slave_station_no      = 0;
   *implied_occupation_count = 0;

   return -1;
}

void cl_iefb_set_cyclic_transmission_state (
   uint16_t * cyclic_transmission_state,
   uint16_t slave_station_no,
   bool state)
{
   uint16_t mask = 0;

   if (slave_station_no == 0 || slave_station_no > CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP)
   {
      return;
   }

   mask = BIT (slave_station_no - 1U) & UINT16_MAX;

   if (state)
   {
      *cyclic_transmission_state |= mask;
   }
   else
   {
      *cyclic_transmission_state &= ~mask;
   }
}

bool cl_iefb_extract_my_transmission_state (
   uint16_t cyclic_transmission_state,
   uint16_t my_slave_station_no)
{
   uint16_t mask = 0;

   if (
      (my_slave_station_no < CL_CCIEFB_MIN_OCCUPIED_STATIONS_PER_GROUP) ||
      (my_slave_station_no > CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP))
   {
      return false;
   }

   mask = BIT (my_slave_station_no - 1U) & UINT16_MAX;

   return (cyclic_transmission_state & mask) > 0;
}

uint64_t cl_calculate_total_timeout_us (
   uint16_t timeout_value,
   uint16_t parallel_off_timeout_count)
{
   uint32_t timeout = (timeout_value < CL_CCIEFB_MIN_TIMEOUT)
                         ? CL_CCIEFB_DEFAULT_TIMEOUT
                         : timeout_value;
   uint32_t count   = (parallel_off_timeout_count < CL_CCIEFB_MIN_TIMEOUT_COUNT)
                         ? CL_CCIEFB_DEFAULT_TIMEOUT_COUNT
                         : parallel_off_timeout_count;

   return (uint64_t)(timeout * count) * CL_TIMER_MICROSECONDS_PER_MILLISECOND;
}

cl_rx_t * cl_iefb_get_rx_area (
   cls_cciefb_cyclic_response_info_t * frame_info,
   uint16_t area_number)
{
   return frame_info->first_rx + area_number;
}

cl_rwr_t * cl_iefb_get_rwr_area (
   cls_cciefb_cyclic_response_info_t * frame_info,
   uint16_t area_number)
{
   return frame_info->first_rwr + area_number;
}

uint16_t cl_iefb_bit_calculate_areanumber (
   uint16_t number,
   uint16_t * byte_in_area,
   uint8_t * bitmask)
{
   /* Note: Avoid division and modulo operator */

   /* Bitmask */
   uint16_t bitnumber = number & 0x07U;
   *bitmask           = BIT (bitnumber) & UINT8_MAX;

   /* Find which byte in the area */
   *byte_in_area = (number >> 3U) & 0x07U;

   /* Area number, divide by 64 */
   return number >> 6U;
}

uint16_t cl_iefb_register_calculate_areanumber (
   uint16_t number,
   uint16_t * register_in_area)
{
   /* Note: Avoid division and modulo operator */

   /* Find which register in the area */
   *register_in_area = number & 0x1FU;

   /* Area number, divide by 32 */
   return number >> 5U;
}

int cl_iefb_parse_response_header (
   uint8_t * buffer,
   size_t recv_len,
   cl_cciefb_resp_header_t ** header)
{
   if (sizeof (cl_cciefb_resp_header_t) > recv_len)
   {
      return -1;
   }

   *header = (cl_cciefb_resp_header_t *)buffer;

   return 0;
}

int cl_iefb_validate_response_header (
   const cl_cciefb_resp_header_t * header,
   size_t recv_len)
{
   /* Validate length */
   if (CC_FROM_LE16 (header->dl) + CL_CCIEFB_RESP_HEADER_DL_OFFSET != recv_len)
   {
      return -1;
   }

   /* Validate reserved fields
      Note that reserved1 is coded in big endian.
      See section 9.1.2 in BAP-C2010-ENG-004-A */
   if (
      CC_FROM_BE16 (header->reserved1) != CL_CCIEFB_RESP_HEADER_RESERVED1 ||
      header->reserved2 != CL_CCIEFB_RESP_HEADER_RESERVED2 ||
      header->reserved3 != CL_CCIEFB_RESP_HEADER_RESERVED3 ||
      CC_FROM_LE16 (header->reserved4) != CL_CCIEFB_RESP_HEADER_RESERVED4 ||
      header->reserved5 != CL_CCIEFB_RESP_HEADER_RESERVED5 ||
      CC_FROM_LE16 (header->reserved6) != CL_CCIEFB_REQ_HEADER_RESERVED6)
   {
      return -1;
   }

   return 0;
}

int cl_iefb_parse_resp_full_cyclic_headers (
   uint8_t * buffer,
   size_t recv_len,
   cl_cciefb_cyclic_resp_full_headers_t ** full_headers)
{
   if (recv_len < sizeof (cl_cciefb_cyclic_resp_full_headers_t))
   {
      return -1;
   }

   *full_headers = (cl_cciefb_cyclic_resp_full_headers_t *)buffer;

   return 0;
}

int cl_iefb_validate_cyclic_resp_header (
   const cl_cciefb_cyclic_resp_header_t * cyclic_header)
{
   uint16_t protocol_ver = CC_FROM_LE16 (cyclic_header->protocol_ver);
   size_t i;

   /* Validate protocol version */
   if (protocol_ver < CL_CCIEFB_MIN_SUPPORTED_PROTOCOL_VER || protocol_ver > CL_CCIEFB_MAX_SUPPORTED_PROTOCOL_VER)
   {
      return -1;
   }

   /* Validate cyclic offset */
   if (CC_FROM_LE16 (cyclic_header->cyclic_info_offset_addr) != CL_CCIEFB_CYCLIC_RESP_CYCLIC_OFFSET)
   {
      return -1;
   }

   /* Validate reserved values */
   for (i = 0; i < sizeof (cyclic_header->reserved1); i++)
   {
      if (cyclic_header->reserved1[i] != CL_CCIEFB_CYCLIC_RESP_HEADER_RESERVED1)
      {
         return -1;
      }
   }

   return 0;
}

int cl_iefb_validate_slave_station_notification (
   const cl_cciefb_slave_station_notification_t * slave_station_notification)
{
   /* Verify slave_local_unit_info bits */
   if (
      (CC_FROM_LE16 (slave_station_notification->slave_local_unit_info) &
       CL_CCIEFB_SLAVE_STATION_NOTIFICATION_MASK_BITS) > 0)
   {
      return -1;
   }

   /* Verify reserved value */
   if (
      CC_FROM_LE16 (slave_station_notification->reserved1) !=
         CL_CCIEFB_SLAVE_STATION_NOTIFICATION_RESERVED1 ||
      CC_FROM_LE16 (slave_station_notification->reserved2) !=
         CL_CCIEFB_SLAVE_STATION_NOTIFICATION_RESERVED2)
   {
      return -1;
   }
   return 0;
}

int cl_iefb_validate_resp_cyclic_data_header (
   const cl_cciefb_cyclic_resp_data_header_t * cyclic_data_header,
   cl_ipaddr_t remote_ip)
{
   uint16_t group_no    = cyclic_data_header->group_no; /* uint8_t in frame */
   cl_ipaddr_t slave_id = CC_FROM_LE32 (cyclic_data_header->slave_id);

   /* Verify group number */
   if (group_no < CL_CCIEFB_MIN_GROUP_NO || group_no > CL_CCIEFB_MAX_GROUP_NO)
   {
      return -1;
   }

   /* Verify reserved fields */
   if (cyclic_data_header->reserved2 != CL_CCIEFB_CYCLIC_RESP_DATA_HEADER_RESERVED2)
   {
      return -1;
   }

   /* Verify slave ID */
   if (slave_id == CL_IPADDR_INVALID || slave_id != remote_ip)
   {
      return -1;
   }

   return 0;
}

int cl_iefb_validate_resp_cyclic_frame_size (
   uint16_t protocol_ver,
   size_t recv_len,
   uint16_t dl)
{
   /* This function is valid for v1 and v2 */
   if (protocol_ver < CL_CCIEFB_MIN_SUPPORTED_PROTOCOL_VER || protocol_ver > CL_CCIEFB_MAX_SUPPORTED_PROTOCOL_VER)
   {
      return -1;
   }

   /* Check that the length value from the header corresponds to total size */
   if (recv_len != dl + CL_CCIEFB_REQ_HEADER_DL_OFFSET)
   {
      return -1;
   }

   return 0;
}

int cl_iefb_validate_resp_full_cyclic_headers (
   const cl_cciefb_cyclic_resp_full_headers_t * full_headers,
   size_t recv_len,
   cl_ipaddr_t remote_ip)
{
   /* The cl_cciefb_resp_header_t has been validated earlier */

   if (cl_iefb_validate_cyclic_resp_header (&full_headers->cyclic_header) != 0)
   {
      return -1;
   }

   if (
      cl_iefb_validate_slave_station_notification (
         &full_headers->slave_station_notification) != 0)
   {
      return -1;
   }

   if (
      cl_iefb_validate_resp_cyclic_data_header (
         &full_headers->cyclic_data_header,
         remote_ip) != 0)
   {
      return -1;
   }

   if (
      cl_iefb_validate_resp_cyclic_frame_size (
         CC_FROM_LE16 (full_headers->cyclic_header.protocol_ver),
         recv_len,
         CC_FROM_LE16 (full_headers->resp_header.dl)) != 0)
   {
      return -1;
   }

   return 0;
}

int cl_iefb_parse_response_cyclic_data (
   uint8_t * buffer,
   size_t recv_len,
   uint16_t number_of_occupied_stations,
   cl_rwr_t ** first_rwr,
   cl_rx_t ** first_rx)
{
   uint32_t pos = sizeof (cl_cciefb_cyclic_resp_full_headers_t);

   /* Check slave_total_occupied_station_count validity */
   if (
      number_of_occupied_stations < CL_CCIEFB_MIN_OCCUPIED_STATIONS_PER_GROUP ||
      number_of_occupied_stations > CL_CCIEFB_MAX_OCCUPIED_STATIONS_PER_GROUP)
   {
      return -1;
   }

   *first_rwr = (cl_rwr_t *)(&buffer[pos]);
   pos += number_of_occupied_stations * sizeof (cl_rwr_t);

   *first_rx = (cl_rx_t *)(&buffer[pos]);
   pos += number_of_occupied_stations * sizeof (cl_rx_t);

   if (pos > recv_len)
   {
      return -1;
   }

   return 0;
}

int cl_iefb_parse_cyclic_response (
   uint8_t * buffer,
   size_t recv_len,
   cl_ipaddr_t remote_ip,
   uint16_t remote_port,
   uint32_t now,
   clm_cciefb_cyclic_response_info_t * response)
{
   int result_parse_cyclic              = 0;
   uint16_t number_of_occupied_stations = 0;

   response->remote_port         = remote_port;
   response->remote_ip           = remote_ip;
   response->reception_timestamp = now;

   if (cl_iefb_parse_resp_full_cyclic_headers (buffer, recv_len, &response->full_headers) != 0)
   {
      return -1;
   }

   if (
      cl_iefb_validate_resp_full_cyclic_headers (
         response->full_headers,
         recv_len,
         remote_ip) != 0)
   {
      return -1;
   }

   /* Validate that the size is correct with regards to the number of
    * occupied stations */
   number_of_occupied_stations =
      cl_calculate_number_of_occupied_stations (recv_len);
   if (number_of_occupied_stations == 0)
   {
      return -1;
   }
   response->number_of_occupied = number_of_occupied_stations;

   result_parse_cyclic = cl_iefb_parse_response_cyclic_data (
      buffer,
      recv_len,
      number_of_occupied_stations,
      &response->first_rwr,
      &response->first_rx);

   /* Already checked in cl_iefb_validate_req_full_cyclic_headers() */
   CC_ASSERT (result_parse_cyclic == 0);

   return 0;
}
