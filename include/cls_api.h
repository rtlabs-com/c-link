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

#ifndef CLS_API_H
#define CLS_API_H

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

/** c-link slave stack handle */
typedef struct cls cls_t;

/** Slave states. See also the internal cl_literals_get_slave_state() */
typedef enum cls_slave_state
{
   /** Initial state */
   CLS_SLAVE_STATE_SLAVE_DOWN,

   /** Waiting for master to connect */
   CLS_SLAVE_STATE_MASTER_NONE,

   /** Connected to master */
   CLS_SLAVE_STATE_MASTER_CONTROL,

   /** The slave has disabled the cyclic communication */
   CLS_SLAVE_STATE_SLAVE_DISABLED,

   /** Slave in progress of disabling cyclic communication */
   CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE,

   /** Dummy state */
   CLS_SLAVE_STATE_LAST
} cls_slave_state_t;

/** Master connection details, stored in slave */
typedef struct cls_master_connection
{
   /** Master IP address */
   cl_ipaddr_t master_id;

   /** Unix timestamp with milliseconds */
   uint64_t clock_info;

   /** True when clock_info is valid */
   bool clock_info_valid;

   /** Protocol version used by master */
   uint16_t protocol_ver;

   /** Group number. Allowed 1..64 */
   uint16_t group_no;

   /** Parameter number from master */
   uint16_t parameter_no;

   /** Timeout value in milliseconds */
   uint16_t timeout_value;

   /** Number of timeouts before disconnect   */
   uint16_t parallel_off_timeout_count;

   /** Slave station number. Allowed 1..16 */
   uint16_t slave_station_no;

   /** Total number of occupied stations in this group */
   uint16_t total_occupied_station_count;
} cls_master_connection_t;

/** Error messages reported in the callback \a cls_error_ind_t()
 *  Literals are implemented in the internal function
 *  cl_literals_get_slave_error_message() */
typedef enum cls_error_message
{
   CLS_ERROR_SLAVE_STATION_DUPLICATION,
   CLS_ERROR_MASTER_STATION_DUPLICATION,
   CLS_ERROR_WRONG_NUMBER_OCCUPIED
} cls_error_message_t;

/**
 * Indication to the application that a state transition has occurred within the
 * CC-Link slave stack.
 *
 * It is optional to implement this callback.
 *
 * @param cls              The slave stack instance
 * @param arg              User-defined data (not used by c-link)
 * @param state            The new state
 */
typedef void (*cls_state_ind_t) (cls_t * cls, void * arg, cls_slave_state_t state);

/**
 * Indication to the application that the master application state has changed.
 *
 * It is optional to implement this callback.
 *
 * @param cls                       The slave stack instance
 * @param arg                       User-defined data (not used by c-link)
 * @param connected_to_master       True if there is cyclic data to and from
 *                                  the PLC.
 * @param connected_and_running     True if the master application is running
 *                                  on the PLC, otherwise false.
 *                                  False also when PLC is disconnected.
 * @param stopped_by_user           True if the master application is stopped
 *                                  by the user. Only supported for protocol
 *                                  version 2 or higher, otherwise false.
 *                                  False also when PLC is disconnected.
 * @param protocol_ver              Protocol version of the request from the
 *                                  PLC. Value is 0 when PLC is disconnected.
 * @param master_application_status This is the MasterLocalUnitInfo from the
 *                                  PLC. Value is 0 when PLC is disconnected.
 *                                  Most of the booleans above is parsed from
 *                                  this value.
 */
typedef void (*cls_master_state_ind_t) (
   cls_t * cls,
   void * arg,
   bool connected_to_master,
   bool connected_and_running,
   bool stopped_by_user,
   uint16_t protocol_ver,
   uint16_t master_application_status);

/**
 * Indication that there is an error in the slave.
 *
 * Interpretation of arguments:
 * - CLS_ERROR_MASTER_STATION_DUPLICATION: IP address of other master.
 *    You must remove the other master.
 * - CLS_ERROR_WRONG_NUMBER_OCCUPIED: Number of occupied in request.
 *    You must adjust the configuration in the master.
 * - CLS_ERROR_SLAVE_STATION_DUPLICATION: (no args)
 *    You must remove the offending slave.
 *    (This slave might just be restarted, wait for master time out)
 *
 * It is optional to implement this callback.
 *
 * @param cls                    The slave stack instance
 * @param arg                    User-defined data (not used by c-link)
 * @param error_message          Enum of error messages
 * @param ip_addr                IP address
 * @param argument_2             Numerical argument (if any)
 */
typedef void (*cls_error_ind_t) (
   cls_t * cls,
   void * arg,
   cls_error_message_t error_message,
   cl_ipaddr_t ip_addr,
   uint16_t argument_2);

/**
 * Indication to the application that a master has connected, or changed
 * parameters.
 *
 * It is optional to implement this callback.
 *
 * @param cls                          The slave stack instance
 * @param arg                          User-defined data (not used by c-link)
 * @param master_ip_addr               IP address of the master
 * @param group_no                     Our group number
 * @param slave_station_no             Our slave station number
 */
typedef void (*cls_connect_ind_t) (
   cls_t * cls,
   void * arg,
   cl_ipaddr_t master_ip_addr,
   uint16_t group_no,
   uint16_t slave_station_no);

/**
 * Indication to the application that a master has disconnected.
 *
 * It is optional to implement this callback.
 *
 * @param cls                          The slave stack instance
 * @param arg                          User-defined data (not used by c-link)
 */
typedef void (*cls_disconnect_ind_t) (cls_t * cls, void * arg);

/**
 * Indication to the application that the master is doing a node search.
 *
 * The stack will respond to the node search, so this callback is only for
 * information to the application.
 *
 * It is optional to implement this callback.
 *
 * @param cls                 The slave stack instance
 * @param arg                 User-defined data (not used by c-link)
 * @param master_mac_addr     Master MAC address.
 * @param master_ip_addr      Master IP address.
 */
typedef void (*cls_node_search_ind_t) (
   cls_t * cls,
   void * arg,
   cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr);

/**
 * Indication to the application that the master would like us to set
 * the IP-address and netmask.
 *
 * The stack will handle the setting of the IP address and the netmask,
 * according to the value of \a ip_setting_allowed, so this is only for
 * information to the application.
 *
 * Note that depending on your current routing table settings in the operating
 * system, the new netmask might make it impossible to send a response
 * back to the master.
 *
 * It is optional to implement this callback.
 *
 * @param cls                 The slave stack instance
 * @param arg                 User-defined data (not used by c-link)
 * @param master_mac_addr     Master MAC address.
 * @param master_ip_addr      Master IP address.
 * @param new_ip_addr         IP address we should change to.
 * @param new_netmask         Netmask we should change to.
 * @param ip_setting_allowed  True if our configuration allows the master to
 *                            set our IP address.
 * @param did_set_ip          True if we are allowed to set IP address and it
 *                            was successful,
 */
typedef void (*cls_set_ip_ind_t) (
   cls_t * cls,
   void * arg,
   cl_macaddr_t * master_mac_addr,
   cl_ipaddr_t master_ip_addr,
   cl_ipaddr_t new_ip_addr,
   cl_ipaddr_t new_netmask,
   bool ip_setting_allowed,
   bool did_set_ip);

/** Configuration for the c-link slave stack */
typedef struct cls_cfg
{
   /** Vendor code handed out by the CLPA organisation */
   uint16_t vendor_code;

   /** Model code defined by vendor */
   uint32_t model_code;

   /** Equipment version, also known as device version or machine version.
    *  Defined by vendor. If unsure, use 0x001 */
   uint16_t equipment_ver;

   /** Number of occupied slave stations.
      Allowed values 1 to \a CLS_MAX_OCCUPIED_STATIONS */
   uint16_t num_occupied_stations;

   /** True if the master should be allowed to set the slave IP address */
   bool ip_setting_allowed;

   /** User data passed to callbacks, not used by stack */
   void * cb_arg;

   /** Callback for slave stack state change, or \a NULL if not implemented */
   cls_state_ind_t state_cb;

   /** Callback for errors, or \a NULL if not implemented */
   cls_error_ind_t error_cb;

   /** Callback for master connect, or \a NULL if not implemented */
   cls_connect_ind_t connect_cb;

   /** Callback for master disconnect, or \a NULL if not implemented */
   cls_disconnect_ind_t disconnect_cb;

   /** Callback for changes in master application running state, or \a NULL if
    * not implemented */
   cls_master_state_ind_t master_running_cb;

   /** Callback for when master does a node search, or \a NULL if
    * not implemented */
   cls_node_search_ind_t node_search_cb;

   /** Callback for when master wants to change our IP address. The stack
    * will do the changing of the IP address (if \a ip_setting_allowed is true),
    * so this is for informational purpose. Set to NULL if you don't want to
    * use it.  */
   cls_set_ip_ind_t set_ip_cb;

   /** Which IP address the IEFB socket should bind to.
    *  Use CL_IPADDR_ANY to listen on all interfaces, or
    *  the IP address of the interface.
    *  On Linux use the broadcast address of the interface (x.x.x.255),
    *  while on Windows use the IP address of the interface instead.
    */
   cl_ipaddr_t iefb_ip_addr;

   /** Send SLMP messages as directed broadcast (x.x.x.255) instead
    *  of local broadcast (255.255.255.255).
    *  If unsure, set it to false. */
   bool use_slmp_directed_broadcast;

} cls_cfg_t;

/********************** General functions ***********************************/

/**
 * Initialise c-link stack
 *
 * Allocates memory for the stack instance handle
 *
 * @param cfg              c-link stack configuration
 * @return                 c-link stack instance handle, or \a NULL on failure.
 */
CL_EXPORT cls_t * cls_init (const cls_cfg_t * cfg);

/**
 * Initialise c-link stack, without memory allocation for the stack handle
 *
 * @param cls              c-link slave stack instance handle to be initialised
 * @param cfg              c-link slave stack configuration
 * @return 0 on success, or -1 on failure.
 */
CL_EXPORT int cls_init_only (cls_t * cls, const cls_cfg_t * cfg);

/**
 * Execute all periodic functions within the c-link stack
 *
 * @param cls              c-link slave stack instance handle
 */
CL_EXPORT void cls_handle_periodic (cls_t * cls);

/**
 * Exit c-link stack.
 *
 * You need to manually free cls afterwards.
 *
 * @param cls              c-link slave stack instance handle
 * @return 0 on success, or -1 on failure.
 */
CL_EXPORT int cls_exit (cls_t * cls);

/**
 * Tell the PLC to stop the cyclic communication.
 *
 * Use the \a is_error flag to indicate that we would like to stop due to an
 * error in software/hardware.
 *
 * @param cls              c-link slave stack instance handle
 * @param is_error         true if the stop is due to an error
 */
CL_EXPORT void cls_stop_cyclic_data (cls_t * cls, bool is_error);

/**
 * Restart cyclic communication.
 *
 * @param cls              c-link slave stack instance handle
 */
CL_EXPORT void cls_restart_cyclic_data (cls_t * cls);

/**
 * Get the latest available master timestamp.
 *
 * Valid only if the master is connected, and if the master has clock info.
 *
 * @param cls                c-link slave stack instance handle
 * @param master_timestamp   Resulting timestamp in Unix time with milliseconds
 * @return 0 of the timestamp is valid, -1 if it is invalid.
 */
CL_EXPORT int cls_get_master_timestamp (cls_t * cls, uint64_t * master_timestamp);

/**
 * Set the slave application status, for sending to the PLC.
 *
 * This is information whether the slave application is running or is stopped.
 * When the stack is started, the slave application status is automatically set
 * to running. Any change you make to this value survives disconnect and
 * reconnect.
 *
 * This is named "slaveLocalUnitInfo Application operation status" in the
 * specification.
 *
 * @param cls              c-link slave stack instance handle
 * @param slave_application_status  Slave application status
 *
 * @req REQ_CL_PROTOCOL_48
 * @req REQ_CL_PROTOCOL_49
 */
CL_EXPORT void cls_set_slave_application_status (
   cls_t * cls,
   cl_slave_appl_operation_status_t slave_application_status);

/**
 * Read back the slave application status
 *
 * @param cls              c-link slave stack instance handle
 * @return the slave application status
 */
CL_EXPORT cl_slave_appl_operation_status_t
cls_get_slave_application_status (cls_t * cls);

/**
 * Set the slave local management info
 *
 * This value is sent to the PLC in each cyclic frame, and its contents
 * is user defined. A program on the PLC can read this value.
 * This is named localManagementInfo in the specification, and
 * "Detailed module information" in some equipment manuals.
 *
 * When the stack is started, this is automatically set to 0. Any change you
 * make to this value survives disconnect and reconnect.
 *
 * For example, a remote IO-module can have its hardware input filtering
 * setting adjustable by mechanical switches on the hardware. The current
 * filter setting could then be sent in the local management info value.
 * Make sure to describe in the user documentation what you use this value for.
 *
 * @param cls                     c-link slave stack instance handle
 * @param local_management_info   Local management info
 */
CL_EXPORT void cls_set_local_management_info (
   cls_t * cls,
   uint32_t local_management_info);

/**
 * Read back the slave local management info
 *
 * This is a user-defined value that will be sent to the PLC.
 *
 * @param cls              c-link slave stack instance handle
 * @return the local management info
 */
CL_EXPORT uint32_t cls_get_local_management_info (cls_t * cls);

/**
 * Set the slave error code
 *
 * This value is sent to the PLC in each cyclic frame, and its contents
 * is user defined. A program on the PLC can read this value.
 * The name in the standard is \a slaveErrCode.
 *
 * Setting this value will not cause a disconnect from the master.
 *
 * When the stack is started, this is automatically set to 0. Any change you
 * make to this value survives disconnect and reconnect.
 *
 * The corresponding values should be documented in the CSP+ file.
 *
 * @param cls              c-link slave stack instance handle
 * @param slave_err_code          Slave error code
 */
CL_EXPORT void cls_set_slave_error_code (cls_t * cls, uint16_t slave_err_code);

/**
 * Read back the slave error code
 *
 * This is a user-defined value that will be sent to the PLC.
 *
 * @param cls              c-link slave stack instance handle
 * @return the slave error code
 */
CL_EXPORT uint16_t cls_get_slave_error_code (cls_t * cls);

/**
 * Get a pointer to the master connection details (stored in slave).
 *
 * This function is intended for debugging and for automated tests.
 *
 * @param cls        c-link slave stack instance handle
 * @return           Pointer to master connection details, or NULL on error
 */
CL_EXPORT const cls_master_connection_t * cls_get_master_connection_details (
   cls_t * cls);

/********************* Memory areas *****************************************/

/**
 * Get a pointer to the first RX memory area (bits to PLC).
 *
 * Note that the slave has \a num_occupied_stations of these memory areas in an
 * array (arranged linearly).
 *
 * The data is stored in the outgoing frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * Note that the RY, RX, RWw and RWr memory areas are not guaranteed to be
 * adjacent.
 *
 * @param cls              c-link slave stack instance handle
 * @return                 Pointer to first memory area, or NULL on error
 */
CL_EXPORT cl_rx_t * cls_get_first_rx_area (cls_t * cls);

/**
 * Get a pointer to the first RY memory area (bits from PLC).
 *
 * Note that the slave has \a num_occupied_stations of these memory areas in an
 * array (arranged linearly).
 *
 * The data is copied directly from incoming frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * Note that the RY, RX, RWw and RWr memory areas are not guaranteed to be
 * adjacent.
 *
 * @param cls              c-link slave stack instance handle
 * @return                 Pointer to first memory area, or NULL on error
 */
CL_EXPORT const cl_ry_t * cls_get_first_ry_area (cls_t * cls);

/**
 * Get a pointer to the first RWr memory area (uint16_t values to PLC).
 *
 * Note that the slave has \a num_occupied_stations of these memory areas in an
 * array (arranged linearly).
 *
 * The data is stored in the outgoing frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * Note that the RY, RX, RWw and RWr memory areas are not guaranteed to be
 * adjacent.
 *
 * @param cls              c-link slave stack instance handle
 * @return                 Pointer to first memory area, or NULL on error
 */
CL_EXPORT cl_rwr_t * cls_get_first_rwr_area (cls_t * cls);

/**
 * Get a pointer to the first RWw memory area (uint16_t values from PLC).
 *
 * Note that the slave has \a num_occupied_stations of these memory areas in an
 * array (arranged linearly).
 *
 * The data is copied directly from incoming frame (which is little endian).
 * You need to handle endianness conversion.
 *
 * Note that the RY, RX, RWw and RWr memory areas are not guaranteed to be
 * adjacent.
 *
 * @param cls              c-link slave stack instance handle
 * @return                 Pointer to first memory area, or NULL on error
 */
CL_EXPORT const cl_rww_t * cls_get_first_rww_area (cls_t * cls);

/******************** Data convenience functions *****************************/

/**
 * Set an individual bit (RX) for sending to the PLC.
 *
 * Note that the slave has \a 64*num_occupied_stations of these bits.
 * Will assert for invalid number.
 *
 * @param cls              c-link slave stack instance handle
 * @param number           Bit number. Use 0 for RX0.
 * @param value            Value to send to the PLC
 */
CL_EXPORT void cls_set_rx_bit (cls_t * cls, uint16_t number, bool value);

/**
 * Read back an individual bit (RX), for sending to the PLC.
 *
 * Note that the slave has \a 64*num_occupied_stations of these bits.
 * Will assert for invalid number.
 *
 * @param cls              c-link slave stack instance handle
 * @param number           Bit number. Use 0 for RX0.
 * @return the bit value
 */
CL_EXPORT bool cls_get_rx_bit (cls_t * cls, uint16_t number);

/**
 * Read an individual bit (RY) from to the PLC.
 *
 * Note that the slave has \a 64*num_occupied_stations of these bits.
 * Will assert for invalid number.
 *
 * @param cls              c-link slave stack instance handle
 * @param number           Bit number. Use 0 for RY0.
 * @return the bit value
 */
CL_EXPORT bool cls_get_ry_bit (cls_t * cls, uint16_t number);

/**
 * Set a 16-bit register value (RWr) for sending to the PLC.
 *
 * The endianness is handled automatically.
 *
 * Note that the slave has \a 32*num_occupied_stations of these registers.
 * Will assert for invalid number.
 *
 * @param cls              c-link slave stack instance handle
 * @param number           Register number. Use 0 for RWr0.
 * @param value            Value to send to the PLC
 */
CL_EXPORT void cls_set_rwr_value (cls_t * cls, uint16_t number, uint16_t value);

/**
 * Read back a 16-bit register value (RWr), for sending to the PLC.
 *
 * The endianness is handled automatically.
 *
 * Note that the slave has \a 32*num_occupied_stations of these registers.
 * Will assert for invalid values.
 *
 * @param cls              c-link slave stack instance handle
 * @param number           Register number. Use 0 for RWr0.
 * @return the register value
 */
CL_EXPORT uint16_t cls_get_rwr_value (cls_t * cls, uint16_t number);

/**
 * Read a 16-bit register value (RWw) from the PLC.
 *
 * The endianness is handled automatically.
 *
 * Note that the slave has \a 32*num_occupied_stations of these registers.
 * Will assert for invalid number.
 *
 * @param cls              c-link slave stack instance handle
 * @param number           Register number. Use 0 for RWw0.
 * @return the register value
 */
CL_EXPORT uint16_t cls_get_rww_value (cls_t * cls, uint16_t number);

#ifdef __cplusplus
}
#endif

#endif /* CLS_API_H */
