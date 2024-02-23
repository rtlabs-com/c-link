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

#ifndef MOCKS_H
#define MOCKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <cstdint>

#include "cls_api.h"
#include "common/cl_types.h"

void mock_test_value_init (uint8_t * counter, uint8_t * buffer, size_t len);
uint8_t mock_test_value_8 (uint8_t * counter);
uint16_t mock_test_value_16 (uint8_t * counter, bool be);
uint32_t mock_test_value_32 (uint8_t * counter, bool be);

#define TDATA_INIT_BUFFER(len)                                                 \
   uint8_t buffer[len] = {0};                                                  \
   uint8_t c;                                                                  \
   mock_test_value_init (&c, buffer, sizeof (buffer));

#define TDATA_8()    mock_test_value_8 (&c)
#define TDATA_LE16() mock_test_value_16 (&c, false)
#define TDATA_LE32() mock_test_value_32 (&c, false)
#define TDATA_BE16() mock_test_value_16 (&c, true)
#define TDATA_BE32() mock_test_value_32 (&c, true)

typedef struct cl_mock_udp_port
{
   bool in_use;
   int handle_number;
   uint16_t port_number;
   uint32_t ip_addr;
   uint16_t ifindex;
   bool is_open;

   /** Used if ip_addr is a broadcast address */
   uint32_t local_ip_addr;

   /** Used if ip_addr is a broadcast address */
   uint16_t local_ifindex;

   cl_ipaddr_t remote_source_ip;
   uint16_t remote_source_port;
   cl_ipaddr_t remote_destination_ip;
   uint16_t remote_destination_port;

   bool will_fail_open;
   bool will_fail_send;
   bool will_fail_recv;

   /* Send fewer bytes. Only affects the result if you try to send more
      than this number of bytes or if this is less than buffer size */
   bool use_modified_send_size_returnvalue;
   ssize_t modified_send_size_returnvalue;

   uint8_t udp_input_buffer[CL_BUFFER_LEN];
   uint8_t udp_output_buffer[CL_BUFFER_LEN];

   /* Normally sizeof(udp_input_buffer) but can be set to smaller
      values during test */
   size_t buf_size;

   size_t input_data_size;
   size_t output_data_size;
   size_t total_sent_bytes;
   size_t total_recv_bytes;
   uint16_t number_of_calls_open;
   uint16_t number_of_calls_recv;
   uint16_t number_of_calls_send;
   uint16_t number_of_calls_close;
} cl_mock_udp_port_t;

/** Counters and stored values from master callbacks */
typedef struct cl_mock_master_callback_counters
{
   /** Master callback for state change */
   struct
   {
      uint16_t calls;
      clm_master_state_t state;
   } master_cb_state;

   /** Master callback for connect */
   struct
   {
      uint16_t calls;
      uint16_t group_index;
      uint16_t slave_device_index;
      cl_ipaddr_t slave_id;
   } master_cb_connect;

   /** Master callback for disconnect */
   struct
   {
      uint16_t calls;
      uint16_t group_index;
      uint16_t slave_device_index;
      cl_ipaddr_t slave_id;
   } master_cb_disconnect;

   /** Master callback for linkscan complete */
   struct
   {
      uint16_t calls;
      uint16_t group_index;
      bool success;
   } master_cb_linkscan;

   /** Master callback for alarm frame */
   struct
   {
      uint16_t calls;
      uint16_t group_index;
      uint16_t slave_device_index;
      uint16_t end_code;
      uint16_t slave_err_code;
      uint32_t local_management_info;
   } master_cb_alarm;

   /** Master callback for error message */
   struct
   {
      uint16_t calls;
      clm_error_message_t error_message;
      cl_ipaddr_t ip_addr;
      uint16_t argument_2;
   } master_cb_error;

   /** Master callback for changed slave info frame */
   struct
   {
      uint16_t calls;
      uint16_t group_index;
      uint16_t slave_device_index;
      uint16_t end_code;
      uint16_t slave_err_code;
      uint32_t local_management_info;
   } master_cb_changed_slave_info;

   /** Master callback for node search result */
   struct
   {
      uint16_t calls;
      clm_node_search_db_t db;
   } master_cb_node_search;

   /** Master callback for set IP result */
   struct
   {
      uint16_t calls;
      clm_master_setip_status_t status;
   } master_cb_set_ip;

} cl_mock_master_callback_counters_t;

/** A file in a simulated file system */
typedef struct cl_mock_master_file_info
{
   bool in_use;

   /** NULL terminated string */
   char file_fullpath[40];
   size_t file_size;
   uint8_t file_content[20];
} cl_mock_master_file_info_t;

typedef struct cl_mock_network_interface
{
   int ifindex;
   const char * ifname;
   uint8_t mac_address[6];
   cl_ipaddr_t ip_address;
   cl_ipaddr_t netmask;
} cl_mock_network_interface_t;

typedef struct cl_mock_data
{
   cl_mock_udp_port_t udp_ports[10];

   /* Interface settings */
   cl_mock_network_interface_t interfaces[2];

   bool will_fail_set_ip_addr;
   bool will_fail_read_netmask;
   bool will_fail_read_mac_addr;
   bool will_fail_read_ifname;
   bool will_fail_read_ifindex;
   uint16_t number_of_calls_set_ip_address_netmask;

   /* Operating system values */

   bool will_fail_calloc;
   int clal_init_returnvalue;
   int clal_exit_returnvalue;
   uint64_t unix_timestamp_ms;
   uint32_t timestamp_us;

   /** Slave callback for state change */
   struct
   {
      uint16_t calls;
      cls_slave_state_t state;
   } slave_cb_state;

   /** Slave callback for error message  */
   struct
   {
      uint16_t calls;
      cls_error_message_t error_message;
      cl_ipaddr_t ip_addr;
      uint16_t argument_2;
   } slave_cb_error;

   /** Slave callback for connect */
   struct
   {
      uint16_t calls;
      cl_ipaddr_t master_ip_addr;
      uint16_t group_no;
      uint16_t slave_station_no;
   } slave_cb_connect;

   /** Slave callback for disconnect */
   struct
   {
      uint16_t calls;
   } slave_cb_disconnect;

   /** Slave callback for master running */
   struct
   {
      uint16_t calls;
      uint16_t protocol_ver; /* Master protocol version */
      uint16_t master_application_status;
      bool connected_and_running;
      bool connected_to_master;
      bool stopped_by_user;
   } slave_cb_master_running;

   /** Slave callback for node search */
   struct
   {
      uint16_t calls;
      cl_macaddr_t master_mac_addr;
      cl_ipaddr_t master_ip_addr;
   } slave_cb_nodesearch;

   /** Slave callback for set IP */
   struct
   {
      uint16_t calls;
      cl_macaddr_t master_mac_addr;
      cl_ipaddr_t master_ip_addr;
      cl_ipaddr_t new_ip_addr;
      cl_ipaddr_t new_netmask;
      bool ip_setting_allowed;
      bool did_set_ip;
   } slave_cb_set_ip;

   cl_mock_master_callback_counters_t master_cb_counters[2];

   /** Filesystem for storing data */
   cl_mock_master_file_info_t storage_files[2];

} cl_mock_data_t;

typedef struct cl_mock_cyclic_request_result
{
   uint16_t dl;
   uint16_t command;
   uint16_t sub_command;
   uint16_t master_protocol_ver;
   uint16_t master_local_unit_info;
   uint64_t clock_info; /** Unix timestamp with milliseconds */
   uint32_t master_ip_addr;
   uint8_t group_no;
   uint16_t frame_sequence_no;
   uint16_t timeout_value;
   uint16_t parallel_off_timeout_count;
   uint16_t parameter_no;
   uint16_t slave_total_occupied_station_count;
   uint16_t cyclic_transmission_state;
} cl_mock_cyclic_request_result_t;

typedef struct cl_mock_cyclic_response_result
{
   uint16_t dl;
   uint16_t slave_protocol_ver;
   uint16_t end_code;
   uint16_t vendor_code;
   uint32_t model_code;
   uint16_t equiment_version;
   uint16_t slave_local_unit_info;
   uint16_t slave_err_code;
   uint32_t local_management_info;
   cl_ipaddr_t slave_id;
   uint16_t group_no;
   uint16_t frame_sequence_no;
} cl_mock_cyclic_response_result_t;

typedef struct cl_mock_error_response_result
{
   uint16_t serial;
   uint16_t length;
   uint16_t end_code;
   uint16_t command;
   uint16_t sub_command;
} cl_mock_error_response_result_t;

typedef struct cl_mock_node_search_request_result
{
   uint16_t serial;
   uint16_t length;
   uint16_t command;
   uint16_t sub_command;
   cl_macaddr_t master_mac_addr; /** Correct byte order */
   uint32_t master_ip_addr;
} cl_mock_node_search_request_result_t;

typedef struct cl_mock_node_search_response_result
{
   uint16_t serial;
   uint16_t length;
   uint16_t end_code;
   cl_macaddr_t master_mac_addr; /** Correct byte order */
   uint32_t master_ip_addr;
   cl_macaddr_t slave_mac_addr; /** Correct byte order */
   cl_ipaddr_t slave_ip_addr;
   cl_ipaddr_t slave_netmask;
   uint16_t vendor_code;
   uint32_t model_code;
   uint16_t equipment_ver;
   uint16_t slave_status;
} cl_mock_node_search_response_result_t;

typedef struct cl_mock_set_ip_request_result
{
   uint16_t serial;
   uint16_t length;
   uint16_t command;
   uint16_t sub_command;
   cl_macaddr_t master_mac_addr; /** Correct byte order */
   uint32_t master_ip_addr;
   cl_macaddr_t slave_mac_addr; /** Correct byte order */
   cl_ipaddr_t slave_ip_addr;
   cl_ipaddr_t slave_netmask;
} cl_mock_set_ip_request_result_t;

typedef struct cl_mock_set_ip_response_result
{
   uint16_t serial;
   uint16_t length;
   uint16_t end_code;
   cl_macaddr_t master_mac_addr; /** Correct byte order */
} cl_mock_set_ip_response_result_t;

/****************** Mock existing functions *************/

int mock_clal_init (void);
int mock_clal_exit (void);
uint64_t mock_clal_get_unix_timestamp_ms (void);

int mock_clal_get_ifindex (uint32_t ip_address, int * ifindex);
int mock_clal_get_ifname (int ifindex, char * ifname);
int mock_clal_get_mac_address (int ifindex, uint8_t (*mac_address)[6]);
int mock_clal_get_netmask (int ifindex, uint32_t * netmask);
int mock_clal_set_ip_address_netmask (
   int ifindex,
   uint32_t ip_address,
   uint32_t netmask);

int mock_clal_udp_open (uint32_t ip, uint16_t port);
ssize_t mock_clal_udp_sendto (
   int handle,
   uint32_t ip,
   uint16_t port,
   const void * data,
   size_t size);
ssize_t mock_clal_udp_recvfrom (
   int handle,
   uint32_t * ip,
   uint16_t * port,
   void * data,
   size_t size);
ssize_t mock_clal_udp_recvfrom_with_ifindex (
   int handle,
   uint32_t * remote_ip,
   uint16_t * remote_port,
   uint32_t * local_ip,
   int * ifindex,
   void * data,
   size_t size);
void mock_clal_udp_close (int handle);

int mock_clal_save_file (
   const char * fullpath,
   const void * object_1,
   size_t size_1,
   const void * object_2,
   size_t size_2);
void mock_clal_clear_file (const char * fullpath);
int mock_clal_load_file (
   const char * fullpath,
   void * object_1,
   size_t size_1,
   void * object_2,
   size_t size_2);

void * mock_calloc (size_t nitems, size_t size);
uint32_t mock_os_get_current_time_us (void);

/**************** Helper functions ********************/

/**
 * Show mocked network interfaces
 */
void cl_mock_show_mocked_interfaces (void);

/**
 * Show mocked UDP ports
 */
void cl_mock_show_mocked_udp_ports (void);

/**
 * Show mocked files
 */
void cl_mock_show_mocked_files (void);

/**
 * Clear the mock data, prepare for slave simulation
 */
void mock_clear (void);

/**
 * Clear the mock data, prepare for master simulation
 */
void mock_clear_master (void);

/**
 * Transfer mocked UDP data between simulated ports
 *
 * If there is any data in the send port, it will be transferred to
 * the receive port.
 *
 * @param recieve_port   Mocked UDP port
 * @param send_port      Mocked UDP port
 */
void mock_transfer_udp_fakedata (
   cl_mock_udp_port_t * recieve_port,
   cl_mock_udp_port_t * send_port);

/**
 * Set data in a mocked UDP port
 *
 * @param mock_port           Mocked UDP port (including a buffer)
 * @param remote_source_ip    Remote IP
 * @param remote_source_port  Remote port
 * @param data                Data
 * @param size                Size
 */
void mock_set_udp_fakedata (
   cl_mock_udp_port_t * mock_port,
   cl_ipaddr_t remote_source_ip,
   uint16_t remote_source_port,
   uint8_t * data,
   size_t size);

/**
 * Set data in a mocked UDP port, with local IP address
 *
 * @param mock_port           Mocked UDP port (including a buffer)
 * @param local_ip_addr       Local IP address
 * @param local_ifindex       Local ifindex
 * @param remote_source_ip    Remote IP
 * @param remote_source_port  Remote port
 * @param data                Data
 * @param size                Size
 */
void mock_set_udp_fakedata_with_local_ipaddr (
   cl_mock_udp_port_t * mock_port,
   cl_ipaddr_t local_ip_addr,
   int local_ifindex,
   cl_ipaddr_t remote_source_ip,
   uint16_t remote_source_port,
   uint8_t * data,
   size_t size);

/*************** Analyse data on mocked UDP ports **********/

/**
 * Analyze a CCIEFB cyclic request
 *
 * Reads the headers, but not the slave IDs or the cyclic data
 *
 * @param udp_port         Mocked UDP port (including a buffer)
 * @param result           Resulting parsed frame
 * @return 0 on success, -1 on error
 */
int mock_analyze_cyclic_request (
   cl_mock_udp_port_t * udp_port,
   cl_mock_cyclic_request_result_t * result);

/**
 * Find slave ID in a CCIEFB cyclic request
 *
 * @param udp_port         Mocked UDP port (including a buffer)
 * @param slave_station_no Slave station number. Starts at 1.
 * @param ip_addr          Resulting slave ID (IP address)
 * @return 0 on success, -1 on error
 */
int mock_analyze_cyclic_request_slaveid (
   cl_mock_udp_port_t * udp_port,
   uint16_t slave_station_no,
   cl_ipaddr_t * ip_addr);

/**
 * Find cyclic RWw data in a CCIEFB cyclic request
 *
 * @param udp_port                        Mocked UDP port (including a buffer)
 * @param total_number_of_slavestations   Total number of occupied stations
 * @param slave_station_no                Slave station number. Starts at 1.
 * @return pointer to area on success, NULL on error
 */
cl_rww_t * mock_analyze_cyclic_request_rww (
   cl_mock_udp_port_t * udp_port,
   uint16_t total_number_of_slavestations,
   uint16_t slave_station_no);

/**
 * Analyze a CCIEFB cyclic response
 *
 * @param udp_port         Mocked UDP port (including a buffer)
 * @param result           Resulting parsed frame
 * @return 0 on success, -1 on error
 */
int mock_analyze_fake_cyclic_response (
   cl_mock_udp_port_t * udp_port,
   cl_mock_cyclic_response_result_t * result);

/**
 * Find cyclic RWr data area in a CCIEFB cyclic response
 *
 * @param udp_port      Mocked UDP port (including a buffer)
 * @param areanumber    Memory area number in slave. Starts at 0.
 * @return pointer to area on success, NULL on error
 */
cl_rwr_t * mock_analyze_cyclic_response_rwr (
   cl_mock_udp_port_t * udp_port,
   uint16_t areanumber);

/**
 * Find cyclic RX data area in a CCIEFB cyclic response
 *
 * @param udp_port                     Mocked UDP port (including a buffer)
 * @param number_of_occupied_stations  Num occupied stations for this slave
 * @param areanumber                   Memory area number in slave. Starts at 0.
 * @return pointer to area on success, NULL on error
 */
cl_rx_t * mock_analyze_cyclic_response_rx (
   cl_mock_udp_port_t * udp_port,
   uint16_t number_of_occupied_stations,
   uint16_t areanumber);

/**
 * Analyze a node search request
 *
 * @param udp_port         Mocked UDP port (including a buffer)
 * @param result           Resulting parsed frame
 * @return 0 on success, -1 on error
 */
int mock_analyze_fake_node_search_request (
   cl_mock_udp_port_t * udp_port,
   cl_mock_node_search_request_result_t * result);

/**
 * Analyze a node search response
 *
 * @param udp_port         Mocked UDP port (including a buffer)
 * @param result           Resulting parsed frame
 * @return 0 on success, -1 on error
 */
int mock_analyze_fake_node_search_response (
   cl_mock_udp_port_t * udp_port,
   cl_mock_node_search_response_result_t * result);

/**
 * Analyse a set IP request
 *
 * @param udp_port         Mocked UDP port (including a buffer)
 * @param result           Resulting parsed frame
 * @return 0 on success, -1 on error
 */
int mock_analyze_fake_set_ip_request (
   cl_mock_udp_port_t * udp_port,
   cl_mock_set_ip_request_result_t * result);

/**
 * Analyze a set IP response
 *
 * @param udp_port         Mocked UDP port (including a buffer)
 * @param result           Resulting parsed frame
 * @return 0 on success, -1 on error
 */
int mock_analyze_fake_set_ip_response (
   cl_mock_udp_port_t * udp_port,
   cl_mock_set_ip_response_result_t * result);

/**
 * Analyze an error response frame
 *
 * @param udp_port         Mocked UDP port (including a buffer)
 * @param result           Resulting parsed frame
 * @return 0 on success, -1 on error
 */
int mock_analyze_fake_error_response (
   cl_mock_udp_port_t * udp_port,
   cl_mock_error_response_result_t * result);

extern cl_mock_data_t mock_data;

/***************** Made available for unit testing ******************/

cl_mock_network_interface_t * mock_helper_find_interface (int ifindex);

cl_mock_udp_port_t * mock_find_simulated_udp_port (int handle);

#ifdef __cplusplus
}
#endif

#endif /* MOCKS_H */
