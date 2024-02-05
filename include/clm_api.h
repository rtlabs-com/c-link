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

#ifndef CLM_API_H
#define CLM_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cl_common.h"
#include "cl_export.h"
#include "cl_options.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/************************ Structs and callbacks ******************************/

/** c-link master stack handle */
typedef struct clm clm_t;

/** Simplified master states for reporting to the user application.
 *  See also cl_literals_get_master_state() */
typedef enum clm_master_state
{
   /** Initial state */
   CLM_MASTER_STATE_DOWN,

   /** State where the master not is running, nor in arbitration.
    * Used for example at startup and at failed arbitration. */
   CLM_MASTER_STATE_STANDBY,

   /** Master is making sure there are no other masters on the same subnet */
   CLM_MASTER_STATE_ARBITRATION,

   /** Master is running */
   CLM_MASTER_STATE_RUNNING
} clm_master_state_t;

/** Result of the set IP operation.
    See also cl_literals_get_master_set_ip_result() */
typedef enum clm_master_setip_status
{
   CLM_MASTER_SET_IP_STATUS_SUCCESS,
   CLM_MASTER_SET_IP_STATUS_ERROR,
   CLM_MASTER_SET_IP_STATUS_TIMEOUT
} clm_master_setip_status_t;

/** State for group of slave devices.
    See also cl_literals_get_group_state() */
typedef enum clm_group_state
{
   /** Initial state, not yet started */
   CLM_GROUP_STATE_MASTER_DOWN,

   /** In 'standby mode' before arbitration */
   CLM_GROUP_STATE_MASTER_LISTEN,

   /** Arbitration (listening for other masters) */
   CLM_GROUP_STATE_MASTER_ARBITRATION,

   /** Link scan is completed */
   CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP,

   /** Performing link scan, waiting for completion */
   CLM_GROUP_STATE_MASTER_LINK_SCAN,

   /** Dummy state */
   CLM_GROUP_STATE_LAST
} clm_group_state_t;

/** State for slave device representation in master.
    See also cl_literals_get_device_state() */
typedef enum clm_device_state
{
   /** Initial state, not yet started */
   CLM_DEVICE_STATE_MASTER_DOWN,

   /** Device is disconnected */
   CLM_DEVICE_STATE_LISTEN,

   /** Waiting for reconnection */
   CLM_DEVICE_STATE_WAIT_TD,

   /** Cyclic transmission stopped */
   CLM_DEVICE_STATE_CYCLIC_SUSPEND,

   /** Response has been received */
   CLM_DEVICE_STATE_CYCLIC_SENT,

   /** Waiting for response */
   CLM_DEVICE_STATE_CYCLIC_SENDING,

   /** Dummy state */
   CLM_DEVICE_STATE_LAST
} clm_device_state_t;

/** Error messages reported in the callback \a clm_error_ind_t()
 *  Literals are implemented in the internal function
 *  cl_literals_get_master_error_message() */
typedef enum clm_error_message
{
   CLM_ERROR_ARBITRATION_FAILED,
   CLM_ERROR_SLAVE_DUPLICATION,
   CLM_ERROR_SLAVE_REPORTS_WRONG_NUMBER_OCCUPIED,
   CLM_ERROR_SLAVE_REPORTS_MASTER_DUPLICATION
} clm_error_message_t;

/**
 * Indication to the application that a state transition has occurred within the
 * CC-Link master stack.
 *
 * It is optional to implement this callback.
 *
 * @param clm              The master stack instance
 * @param arg              User-defined data (not used by c-link)
 * @param state            The new state
 */
typedef void (*clm_state_ind_t) (clm_t * clm, void * arg, clm_master_state_t state);

/**
 * Indication to the application that a slave device has connected.
 *
 * It is optional to implement this callback.
 *
 * @param clm                    The master stack instance
 * @param arg                    User-defined data (not used by c-link)
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param slave_id               IP address of the slave device
 */
typedef void (*clm_connect_ind_t) (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   cl_ipaddr_t slave_id);

/**
 * Indication to the application that a slave device has disconnected.
 *
 * This callback is not triggered for those errors that causes the master
 * to completely stop, see \a clm_error_ind_t().
 *
 * It is optional to implement this callback.
 *
 * @param clm                    The master stack instance
 * @param arg                    User-defined data (not used by c-link)
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param slave_id               IP address of the slave device
 */
typedef void (*clm_disconnect_ind_t) (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   cl_ipaddr_t slave_id);

/**
 * Indication to the application that the link scan is complete for a group.
 *
 * The incoming data is available, and it is time for the application to update
 * the outgoing data.
 *
 * It is optional to implement this callback.
 *
 * @param clm              The master stack instance
 * @param arg              User-defined data (not used by c-link)
 * @param group_index      Group index (Note that the group number is
 *                         group_index + 1)
 * @param success          True if all slave devices have responded, or false
 *                         otherwise.
 */
typedef void (*clm_linkscan_complete_ind_t) (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   bool success);

/**
 * Indication to the application that we received an alarm frame from a
 * slave device.
 *
 * It is triggered for end code 'error' (0xCFF0) and 'slave wants to disconnect'
 * (0xCFFF), and only when the end code is changed.
 *
 * This callback is not triggered for the alarm frames that instead
 * trigger the \a clm_error_ind_t() callback.
 *
 * It is optional to implement this callback.
 *
 * @param clm                    The master stack instance
 * @param arg                    User-defined data (not used by c-link)
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param end_code               End code, is != 0 for an alarm frame.
 * @param slave_err_code         Custom value from slave device.
 * @param local_management_info  Another custom value from slave device.
 */
typedef void (*clm_alarm_ind_t) (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t end_code,
   uint16_t slave_err_code,
   uint32_t local_management_info);

/**
 * Indication that there is an error state in the master.
 *
 * Interpretation of arguments:
 * - CLM_ERROR_ARBITRATION_FAILED: IP address of other master.
 *     You must change your network setup, and restart.
 * - CLM_ERROR_SLAVE_DUPLICATION: IP address of slaves.
 *     You must remove the offending slave.
 * - CLM_ERROR_SLAVE_REPORTS_WRONG_NUMBER_OCCUPIED:
 *    IP address of slave.
 *    You must replace the slave with one that corresponds to your
 *    configuration.
 * - CLM_ERROR_SLAVE_REPORTS_MASTER_DUPLICATION: IP address of slave.
 *     You must change your network setup, and restart.
 *
 * It is optional to implement this callback.
 *
 * See also \a clm_alarm_ind_t()
 *
 * @param clm                    The master stack instance
 * @param arg                    User-defined data (not used by c-link)
 * @param error_message          Enum of error messages
 * @param ip_addr                IP address
 * @param argument_2             Numerical argument (if any)
 */
typedef void (*clm_error_ind_t) (
   clm_t * clm,
   void * arg,
   clm_error_message_t error_message,
   cl_ipaddr_t ip_addr,
   uint16_t argument_2);

/**
 * Indication to the application that we received a frame with changed slave
 * info from a slave device (a frame with changed slave_err_code or
 * changed local_management_info).
 *
 * It is optional to implement this callback.
 *
 * @param clm                    The master stack instance
 * @param arg                    User-defined data (not used by c-link)
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param end_code               End code, is != 0 for an alarm frame.
 * @param slave_err_code         Custom value from slave device.
 * @param local_management_info  Another custom value from slave device.
 */
typedef void (*clm_changed_slave_info_ind_t) (
   clm_t * clm,
   void * arg,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t end_code,
   uint16_t slave_err_code,
   uint32_t local_management_info);

/** For reporting back the master status details to users */
typedef struct clm_master_status_details
{
   /** Simplified master states for reporting to the user application */
   clm_master_state_t master_state;

   /** Parameter setting version number */
   uint16_t parameter_no;

   /** Serial number of current node search request, or -1 if no request is
       pending.*/
   int node_search_serial;

   /** Serial number of current set IP request, or -1 if no request is pending.
    */
   int set_ip_request_serial;

} clm_master_status_details_t;

/** For reporting back the group status details to users */
typedef struct clm_group_status_details
{
   /** Group index, allowed 0..63. Group number 1 has group_index 0 */
   uint16_t group_index;

   /** Number of occupied slave stations in this group according to config.
    *  Allowed 1..16 */
   uint16_t total_occupied;

   /** Frame sequence counter */
   uint16_t frame_sequence_no;

   /** One bit per slave */
   uint16_t cyclic_transmission_state;

   /** Timestamp in microseconds when the current link scan started for this
    * group */
   uint32_t timestamp_link_scan_start;

   /** Group state */
   clm_group_state_t group_state;

} clm_group_status_details_t;

/** Information from slave response frame headers, stored in master.
 *  Valid for latest received frame from this slave.
 *  Converted endianness. */
typedef struct clm_device_framevalues
{
   /** True if the values in this struct are valid */
   bool has_been_received;

   /** Number of occupied stations by the slave device. Allowed 1..16  */
   uint16_t num_occupied_stations;

   /* Protocol version */
   uint16_t protocol_ver;

   /** End code. Values defined in cl_slmp_error_codes_t */
   uint16_t end_code;

   /** Vendor code */
   uint16_t vendor_code;

   /** Model code */
   uint32_t model_code;

   /** Equipment version */
   uint16_t equipment_ver;

   /** Slave local unit info.
    * Values defined in cl_slave_appl_operation_status_t */
   uint16_t slave_local_unit_info;

   /** Slave local management info */
   uint32_t local_management_info;

   /** Slave error code */
   uint16_t slave_err_code;

   /** Slave IP address */
   cl_ipaddr_t slave_id;

   /** Group number. Sent as uint8_t in the frame. Allowed 1..64 */
   uint16_t group_no;

   /** Frame sequence number */
   uint16_t frame_sequence_no;

   /** Time since request, in microseconds */
   uint32_t response_time;
} clm_device_framevalues_t;

/** Response time statistics */
typedef struct clm_slave_device_time_statistics
{
   /** Number of stored response time values. Limited by the
    * max_statistics_samples configuration setting. */
   uint32_t number_of_samples;

   /** Sum of response times, microseconds */
   uint32_t sum;

   /** Min response time, microseconds */
   uint32_t min;

   /** Average response time, microseconds */
   uint32_t average;

   /** Max response time, microseconds */
   uint32_t max;
} clm_slave_device_time_statistics_t;

/** Slave device statistics */
typedef struct clm_slave_device_statistics
{
   /** Number of connects */
   uint32_t number_of_connects;

   /** Number of connects, including timeouts */
   uint32_t number_of_disconnects;

   /** Number of timeouts */
   uint32_t number_of_timeouts;

   /** With slave IP address */
   uint32_t number_of_sent_frames;

   /** Including alarm frames etc */
   uint32_t number_of_incoming_frames;

   /** Endcode != 0 */
   uint32_t number_of_incoming_alarm_frames;

   /** Frames that can be partially parsed, but have wrong sequence number,
       wrong number of occupied stations etc */
   uint32_t number_of_incoming_invalid_frames;

   /** Slave device response time statistics */
   clm_slave_device_time_statistics_t measured_time;
} clm_slave_device_statistics_t;

/** Runtime data for one slave device (stored in master) */
typedef struct clm_slave_device_data
{
   /** Within a group. Starts at 0. */
   uint16_t device_index;

   /** Also known as StartCyclicFlag or StartDirection.
       Use \a clm_set_slave_communication_status() to modify.*/
   bool enabled;

   /** Transmission bit */
   bool transmission_bit;

   /** For slave testing only.
       Use \a clm_force_cyclic_transmission_bit() to modify. */
   bool force_transmission_bit;

   /** Also known as ContinuousTimeoutCount */
   uint16_t timeout_count;

   /** Within a group. Starts at 1. */
   uint16_t slave_station_no;

   /** Device state */
   clm_device_state_t device_state;

   /** Kept after disconnect */
   clm_device_framevalues_t latest_frame;

   /** Kept after disconnect */
   clm_slave_device_statistics_t statistics;
} clm_slave_device_data_t;

/** Node search response entry. Note that the protocol version not is available
 * in the response. */
typedef struct clm_node_search_response_entry
{
   /** Slave IP address */
   cl_ipaddr_t slave_id;

   /** Slave netmask */
   cl_ipaddr_t slave_netmask;

   /** Slave MAC address */
   cl_macaddr_t slave_mac_addr;

   /** Vendor code */
   uint16_t vendor_code;

   /** Model code */
   uint32_t model_code;

   /** Equipment version */
   uint16_t equipment_ver;
} clm_node_search_response_entry_t;

/** Node search database */
typedef struct clm_node_search_db
{
   /** Number of slaves discovered, may be higher than stored entries
       if exceeding CLM_MAX_NODE_SEARCH_DEVICES */
   uint8_t count;

   /** Number of slave nodes stored in db */
   uint8_t stored;

   /** Node search response entries */
   clm_node_search_response_entry_t entries[CLM_MAX_NODE_SEARCH_DEVICES];
} clm_node_search_db_t;

/**
 * Indication to the application that node search completed
 *
 * The stack will parse the node search response, so this callback is only
 * for information to the application.
 *
 * It is optional to implement this callback.
 *
 * @param clm              The stack instance
 * @param arg              User-defined data (not used by c-link)
 * @param db               Node search database
 */
typedef void (
   *clm_node_search_cfm_t) (clm_t * clm, void * arg, clm_node_search_db_t * db);

/**
 * Indication to the application that set IP address completed
 *
 * It is optional to implement this callback.
 *
 * @param clm              The stack instance
 * @param arg              User-defined data (not used by c-link)
 * @param status           Result of the set IP address operation
 */
typedef void (
   *clm_set_ip_cfm_t) (clm_t * clm, void * arg, clm_master_setip_status_t status);

/** IP address and number of occupied stations for a slave device */
typedef struct clm_slave_device_setting
{
   /** Slave IP address */
   cl_ipaddr_t slave_id;

   /** Number of occupied stations, 1..CLM_MAX_OCCUPIED_STATIONS_PER_GROUP */
   uint16_t num_occupied_stations;

   /** Reserve the station, so it will not take part in cyclic communication */
   bool reserved_slave_device;
} clm_slave_device_setting_t;

/** Settings for a group of slaves */
typedef struct clm_group_setting
{
   /** Link scan timeout in milliseconds. If unsure, use 500. */
   uint16_t timeout_value;

   /** Number of missed link scan timeout values before disconnect.
       If unsure, use 3. */
   uint16_t parallel_off_timeout_count;

   /** Use constant link scan time. If unsure, use false.
    * Also known as ConstantLinkScanFlag in the specification */
   bool use_constant_link_scan_time;

   /** Number of slave_devices in the array for this group.
       Allowed values 1..CLM_MAX_OCCUPIED_STATIONS_PER_GROUP. */
   uint16_t num_slave_devices;

   /** Settings for the individual slave devices */
   clm_slave_device_setting_t slave_devices[CLM_MAX_OCCUPIED_STATIONS_PER_GROUP];
} clm_group_setting_t;

/** Slave settings for all groups */
typedef struct clm_slave_hierarchy
{
   /** Number of groups to use. Allowed 1..CLM_MAX_GROUPS. If unsure, use 1. */
   uint16_t number_of_groups;

   /** Settings for the individual groups.
       First group number (at group index 0) is 1, next is 2 etc */
   clm_group_setting_t groups[CLM_MAX_GROUPS];
} clm_slave_hierarchy_t;

/** Configuration for the c-link master stack */
typedef struct clm_cfg
{
   /** User data passed to callbacks, or \a NULL if not implemented.
    *  Not used by stack */
   void * cb_arg;

   /** Callback for master stack state change, or \a NULL if not implemented */
   clm_state_ind_t state_cb;

   /** Callback for slave device connect, or \a NULL if not implemented */
   clm_connect_ind_t connect_cb;

   /** Callback for slave device disconnect, or \a NULL if not implemented */
   clm_disconnect_ind_t disconnect_cb;

   /** Callback for link scan complete, or \a NULL if not implemented */
   clm_linkscan_complete_ind_t linkscan_cb;

   /** Callback for incoming frames with endcode != 0, indicating an error.
    * Use \a NULL if not implemented */
   clm_alarm_ind_t alarm_cb;

   /** Callback for detected errors in master.
    * Use \a NULL if not implemented */
   clm_error_ind_t error_cb;

   /** Callback for incoming frames with with changed slave info.
    * Use \a NULL if not implemented */
   clm_changed_slave_info_ind_t changed_slave_info_cb;

   /** Callback for when node search is completed or \a NULL if
    * not implemented */
   clm_node_search_cfm_t node_search_cfm_cb;

   /** Callback for when set IP request is completed or \a NULL if
    * not implemented */
   clm_set_ip_cfm_t set_ip_cfm_cb;

   /** Slave IP addresses etc for each group */
   clm_slave_hierarchy_t hier;

   /** CCIEFB protocol version. Allowed 1 or 2. If unsure, use 2. */
   uint16_t protocol_ver;

   /** Master arbitration time, in milliseconds. Must be 2500 to be standard
       compliant (REQ_CLM_ARBITR_03). For slave device testing, it can be
       convenient to change this value. Allowed 0 to 65535 ms. */
   uint16_t arbitration_time;

   /** Setting of the time after the \a clm_perform_node_search() call until the
    * callback \a clm_node_search_cfm_t() is triggered, in milliseconds. Must be
    * at least 1500 to be standard compliant (TODO add requirement).
    * If unsure, use 2000. For slave device testing, it can be convenient
    * to change this value. Allowed 0 to 65535 ms. */
   uint16_t callback_time_node_search;

   /** Setting of the time after the \a clm_set_slave_ipaddr() call until the
    * callback \a clm_set_ip_cfm_t() is triggered if there is no response. Value
    * in milliseconds. If unsure, use 500. For slave device testing, it can be
    * convenient to change this value. Allowed 0 to 65535 ms. */
   uint16_t callback_time_set_ip;

   /** Max number of samples to calculate min, max and average response time.
    *  If unsure, use 0. */
   uint16_t max_statistics_samples;

   /** Use a separate socket listening for other masters.
    *  This is necessary on Linux but should not be used on Windows or
    *  on operating system based on LwIP (for example RT-Kernel). */
   bool use_separate_arbitration_socket;

   /** Send SLMP messages as directed broadcast (x.x.x.255) instead
    *  of local broadcast (255.255.255.255).
    *  If unsure, set it to false. */
   bool use_slmp_directed_broadcast;

   /** Send SLMP messages on the same socket as SLMP reception.
    *  This is necessary for some operating systems using LwIP
    *  (for example RT-Kernel), but not on Windows or Linux. */
   bool use_single_slmp_socket;

   /** Master IP address. Also known as MyMasterID in the specification. */
   cl_ipaddr_t master_id;

   /** Storage between runs. Terminated string with absolute path.
       Use empty string for current directory. */
   char file_directory[CL_MAX_DIRECTORYPATH_SIZE];

} clm_cfg_t;

/********************** General functions ***********************************/

/**
 * Initialise c-link master stack
 *
 * Allocates memory for the stack instance handle
 *
 * The network interface must be up, or this function will fail.
 * Your application needs to handle that, for example by prompting the user.
 *
 * The IP address and netmask of the network interface are expected to stay the
 * same during the entire execution.
 *
 * @param cfg              c-link stack configuration. Contents will be copied.
 * @return                 c-link stack instance handle, or \a NULL on failure.
 *
 * @req REQ_CLM_ARBITR_03
 *
 */
CL_EXPORT clm_t * clm_init (const clm_cfg_t * cfg);

/**
 * Initialise c-link master stack, without memory allocation for the handle
 *
 * Intended for test automation only.
 *
 * @param clm              c-link master stack instance handle to be initialised
 * @param cfg              c-link stack configuration. Contents will be copied.
 * @return 0 on success, or -1 on failure.
 */
int clm_init_only (clm_t * clm, const clm_cfg_t * cfg);

/**
 * Execute all periodic functions within the c-link master stack
 *
 * @param clm              c-link master stack instance handle
 */
CL_EXPORT void clm_handle_periodic (clm_t * clm);

/**
 * Set the master application status "Own station unit information"
 *
 * Informs whether the master application is running or not. Note that the
 * cyclic messages still will be sent even if the master application is stopped.
 *
 * This value is sent to the slaves.
 *
 * @param clm              c-link master stack instance handle
 * @param running          True if the master application is running, false if
 *                         stopped.
 * @param stopped_by_user  True if stopped by user, false if stopped by error.
 *                         Will only have an effect when `running` is false and
 *                         for protocol version 2 or later.
 *
 * @req REQ_CLM_COMMUNIC_01
 */
CL_EXPORT void clm_set_master_application_status (
   clm_t * clm,
   bool running,
   bool stopped_by_user);

/**
 * Read back the master application status.
 *
 * Finds out which value will be sent to the slaves.
 * Bit 0: 1=running, 0=stopped
 * Bit 1: 1=stopped by user, 0=stopped by error
 *
 * The information in bit 1 is valid only for protocol version 2 and later.
 *
 * @param clm              c-link master stack instance handle
 * @return Master local unit info
 */
CL_EXPORT uint16_t clm_get_master_application_status (clm_t * clm);

/**
 * Set the slave communication status.
 *
 * Disables or enables the cyclic data communication to a slave device.
 *
 * On startup, this is affected by the value given in the configuration.
 * A setting \a reserved_slave_device=true corresponds to \a enabled=false.
 *
 * Use \a clm_get_device_connection_details() to read current value.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param enabled                True if the cyclic data communication is
 *                               enabled, false otherwise
 * @return 0 on success, or -1 on illegal argument values.
 */
CL_EXPORT int clm_set_slave_communication_status (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   bool enabled);

/**
 * Force the cyclic transmission bit for a specific slave device.
 *
 * This is intending only for slave testing, and will trigger non-conformant
 * behavior.
 *
 * Will set the corresponding cyclic transmission bit in outgoing frame,
 * regardless of the state machine value in the master.
 *
 * Use \a clm_get_device_connection_details() to read current value.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param force                  True if the corresponding bit should be forced,
 *                               or false to stop forcing the bit value.
 * @return 0 on success, or -1 on illegal argument values.
 */
CL_EXPORT int clm_force_cyclic_transmission_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   bool force);

/**
 * Clear statistics for communication with slave devices
 *
 * This clears all statistics.
 *
 * @param clm                    c-link master stack instance handle
 */
CL_EXPORT void clm_clear_statistics (clm_t * clm);

/**
 * Read out master internal details.
 *
 * This function is intended for debugging and for automated tests.
 *
 * Copies information to the resulting struct.
 *
 * @param clm                    c-link master stack instance handle
 * @param details                Resulting details
 * @return 0 on success, or -1 on error.
 */
CL_EXPORT int clm_get_master_status (
   const clm_t * clm,
   clm_master_status_details_t * details);

/**
 * Read out details about the group of slave devices.
 *
 * This function is intended for debugging and for automated tests.
 *
 * Copies the information to the resulting struct.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param details                Resulting details
 * @return 0 on success, or -1 on error.
 */
CL_EXPORT int clm_get_group_status (
   const clm_t * clm,
   uint16_t group_index,
   clm_group_status_details_t * details);

/**
 * Get a pointer to connection details for a slave device (stored in master).
 *
 * This function is intended for debugging and for automated tests.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @return Pointer to slave device connection details, or NULL on error.
 *         No copying is done.
 *
 * @req REQ_CLM_DIAGNOSIS_01
 *
 */
CL_EXPORT const clm_slave_device_data_t * clm_get_device_connection_details (
   const clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index);

/**
 * Exit c-link master stack.
 *
 * You need to manually free clm afterwards.
 *
 * @param clm              c-link master stack instance handle
 * @return 0 on success, or -1 on failure.
 */
CL_EXPORT int clm_exit (clm_t * clm);

/**
 * Perform a node search on the network.
 *
 * Cleans the list of found nodes before it sends the request.
 *
 * Triggers the callback node_search_complete approximately 2 seconds
 * after this function is called.
 *
 * The result will also be available via \a clm_get_node_search_result().
 *
 * No new node search will be triggered if there already is one in progress.
 *
 * @param clm              c-link master stack instance handle
 * @return 0 on success, or -1 on failure to send the request. Note that the
 * result of the node search itself is reported in the callback.
 */
CL_EXPORT int clm_perform_node_search (clm_t * clm);

/**
 * Get a pointer to the node search result database.
 *
 * @param clm              c-link master stack instance handle
 * @return                 Pointer to node search result database
 */
CL_EXPORT const clm_node_search_db_t * clm_get_node_search_result (clm_t * clm);

/**
 * Set IP address for a slave device on the network.
 *
 * Triggers the callback set_ip_cfm_cb to inform about the result.
 *
 * @param clm                c-link master stack instance handle
 * @param slave_mac_addr     Slave MAC address
 * @param slave_new_ip_addr  New IP address for the slave device
 * @param slave_new_netmask  New netmask for the slave device
 * @return 0 on success, or -1 on failure to send the request or on invalid
 * arguments. Note that the result of the set IP operation (in the slave
 * device) is reported in the callback.
 */
CL_EXPORT int clm_set_slave_ipaddr (
   clm_t * clm,
   const cl_macaddr_t * slave_mac_addr,
   cl_ipaddr_t slave_new_ip_addr,
   cl_ipaddr_t slave_new_netmask);

/********************* Memory areas *****************************************/

/**
 * Get a pointer to the first RX memory area (bits from slaves), for a group.
 *
 * The number of memory areas in the group is equal to the total number of
 * occupied slave stations in the group. The memory areas are arranged linearly.
 *
 * The data is copied directly from incoming frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * Note that the RY, RX, RWw and RWr memory areas are not guaranteed to be
 * adjacent.
 *
 * @param clm              c-link master stack instance handle
 * @param group_index      Group index (starts from 0).
 *                         Note that group number 1 has group_index 0.
 * @param total_occupied   Resulting number of total occupied stations for
 *                         this group. 0 on error.
 * @return                 Pointer to first memory area, or NULL on error
 */
CL_EXPORT const cl_rx_t * clm_get_first_rx_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied);

/**
 * Get a pointer to the first RY memory area (bits to slaves), for a group.
 *
 * The number of memory areas in the group is equal to the total number of
 * occupied slave stations in the group. The memory areas are arranged linearly.
 *
 * The data is copied directly to outgoing frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * Note that the RY, RX, RWw and RWr memory areas are not guaranteed to be
 * adjacent.
 *
 * @param clm              c-link master stack instance handle
 * @param group_index      Group index (starts from 0).
 *                         Note that group number 1 has group_index 0.
 * @param total_occupied   Resulting number of total occupied stations for
 *                         this group. 0 on error.
 * @return                 Pointer to first memory area, or NULL on error
 */
CL_EXPORT cl_ry_t * clm_get_first_ry_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied);

/**
 * Get a pointer to the first RWr memory area (uint16_t values from slaves),
 * for a group.
 *
 * The number of memory areas in the group is equal to the total number of
 * occupied slave stations in the group. The memory areas are arranged linearly.
 *
 * The data is copied directly from incoming frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * Note that the RY, RX, RWw and RWr memory areas are not guaranteed to be
 * adjacent.
 *
 * @param clm              c-link master stack instance handle
 * @param group_index      Group index (starts from 0).
 *                         Note that group number 1 has group_index 0.
 * @param total_occupied   Resulting number of total occupied stations for
 *                         this group. 0 on error.
 * @return                 Pointer to first memory area, or NULL on error
 */
CL_EXPORT const cl_rwr_t * clm_get_first_rwr_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied);

/**
 * Get a pointer to the first RWw memory area (uint16_t values to slaves),
 * for a group.
 *
 * The number of memory areas in the group is equal to the total number of
 * occupied slave stations in the group. The memory areas are arranged linearly.
 *
 * The data is copied directly to outgoing frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * Note that the RY, RX, RWw and RWr memory areas are not guaranteed to be
 * adjacent.
 *
 * @param clm              c-link master stack instance handle
 * @param group_index      Group index (starts from 0).
 *                         Note that group number 1 has group_index 0.
 * @param total_occupied   Resulting number of total occupied stations for
 *                         this group. 0 on error.
 * @return                 Pointer to first memory area, or NULL on error
 */
CL_EXPORT cl_rww_t * clm_get_first_rww_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t * total_occupied);

/**
 * Get a pointer to the first RX memory area (bits from slaves), for a device.
 *
 * The number of memory areas for the device is equal to the number of
 * occupied slave stations for the device. The memory areas are arranged
 * linearly.
 *
 * The data is copied directly from incoming frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * Note that the RY, RX, RWw and RWr memory areas are not guaranteed to be
 * adjacent.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param num_occupied_stations  Resulting number of occupied stations for
 *                               this device. 0 on error.
 * @return                       Pointer to first memory area, or NULL on error
 */
CL_EXPORT const cl_rx_t * clm_get_first_device_rx_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations);

/**
 * Get a pointer to the first RY memory area (bits to slaves), for a device.
 *
 * The number of memory areas for the device is equal to the number of
 * occupied slave stations for the device. The memory areas are arranged
 * linearly.
 *
 * The data is copied directly to outgoing frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * Note that the RY, RX, RWw and RWr memory areas are not guaranteed to be
 * adjacent.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param num_occupied_stations  Resulting number of occupied stations for
 *                               this device. 0 on error.
 * @return                       Pointer to first memory area, or NULL on error
 */
CL_EXPORT cl_ry_t * clm_get_first_device_ry_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations);

/**
 * Get a pointer to the first RWr memory area (uint16_t values from slaves),
 * for a device.
 *
 * The number of memory areas for the device is equal to the number of
 * occupied slave stations for the device. The memory areas are arranged
 * linearly.
 *
 * The data is copied directly from incoming frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * Note that the RY, RX, RWw and RWr memory areas are not guaranteed to be
 * adjacent.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param num_occupied_stations  Resulting number of occupied stations for
 *                               this device. 0 on error.
 * @return                       Pointer to first memory area, or NULL on error
 */
CL_EXPORT const cl_rwr_t * clm_get_first_device_rwr_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations);

/**
 * Get a pointer to the first RWw memory area (uint16_t values to slaves),
 * for a device.
 *
 * The number of memory areas for the device is equal to the number of
 * occupied slave stations for the device. The memory areas are arranged
 * linearly.
 *
 * The data is copied directly to outgoing frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * Note that the RY, RX, RWw and RWr memory areas are not guaranteed to be
 * adjacent.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param num_occupied_stations  Resulting number of occupied stations for
 *                               this device. 0 on error.
 * @return                       Pointer to first memory area, or NULL on error
 */
CL_EXPORT cl_rww_t * clm_get_first_device_rww_area (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t * num_occupied_stations);

/******************** Data convenience functions *****************************/

/**
 * Read an individual bit (RX) from a slave.
 *
 * Note that the slave has \a 64*num_occupied_stations of these bits.
 * Will assert for invalid arguments.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param number                 Bit number. Use 0 for RX0.
 * @return the bit value
 */
CL_EXPORT bool clm_get_rx_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number);

/**
 * Set an individual bit (RY) for sending to a slave.
 *
 * Note that the slave has \a 64*num_occupied_stations of these bits.
 * Will assert for invalid arguments.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param number                 Bit number. Use 0 for RY0.
 * @param value                  Value to send to the slave
 */
CL_EXPORT void clm_set_ry_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number,
   bool value);

/**
 * Read back an individual bit (RY, for sending to a slave).
 *
 * Note that the slave has \a 64*num_occupied_stations of these bits.
 * Will assert for invalid arguments.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param number                 Bit number. Use 0 for RY0.
 * @return the bit value
 */
CL_EXPORT bool clm_get_ry_bit (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number);

/**
 * Read a 16-bit register value (RWr) from a slave.
 *
 * The endianness is handled automatically.
 *
 * Note that the slave has \a 32*num_occupied_stations of these registers.
 * Will assert for invalid arguments.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param number                 Register number. Use 0 for RWr0.
 * @return the register value
 */
CL_EXPORT uint16_t clm_get_rwr_value (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number);

/**
 * Set a 16-bit register value (RWw) for sending to a slave.
 *
 * The endianness is handled automatically.
 *
 * Note that the slave has \a 32*num_occupied_stations of these registers.
 * Will assert for invalid arguments.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param number                 Register number. Use 0 for RWw0.
 * @param value                  Value to send to the PLC
 */
CL_EXPORT void clm_set_rww_value (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number,
   uint16_t value);

/**
 * Read back a 16-bit register value (RWw, for sending to a slave).
 *
 * The endianness is handled automatically.
 *
 * Note that the slave has \a 32*num_occupied_stations of these registers.
 * Will assert for invalid arguments.
 *
 * @param clm                    c-link master stack instance handle
 * @param group_index            Group index (starts from 0).
 *                               Note that group number 1 has group_index 0.
 * @param slave_device_index     Device index in group (starts from 0).
 * @param number                 Register number. Use 0 for RWw0.
 * @return the register value
 */
CL_EXPORT uint16_t clm_get_rww_value (
   clm_t * clm,
   uint16_t group_index,
   uint16_t slave_device_index,
   uint16_t number);

#ifdef __cplusplus
}
#endif

#endif /* CLM_API_H */
