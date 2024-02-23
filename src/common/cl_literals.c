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
 * @brief Descriptive names for enums
 */

#include "common/cl_literals.h"

#include "common/cl_types.h"

const char * cl_literals_get_master_state (clm_master_state_t state)
{
   switch (state)
   {
   case CLM_MASTER_STATE_DOWN:
      return "STATE_DOWN";
   case CLM_MASTER_STATE_STANDBY:
      return "STATE_STANDBY";
   case CLM_MASTER_STATE_ARBITRATION:
      return "STATE_ARBITRATION";
   case CLM_MASTER_STATE_RUNNING:
      return "STATE_RUNNING";
   default:
      return "unknown state";
   }
}

const char * cl_literals_get_group_event (clm_group_event_t event)
{
   switch (event)
   {
   case CLM_GROUP_EVENT_NONE:
      return "EVENT_NONE";
   case CLM_GROUP_EVENT_STARTUP:
      return "EVENT_STARTUP";
   case CLM_GROUP_EVENT_PARAMETER_CHANGE:
      return "EVENT_PARAMETER_CHANGE";
   case CLM_GROUP_EVENT_NEW_CONFIG:
      return "EVENT_NEW_CONFIG";
   case CLM_GROUP_EVENT_ARBITRATION_DONE:
      return "EVENT_ARBITRATION_DONE";
   case CLM_GROUP_EVENT_REQ_FROM_OTHER:
      return "EVENT_REQ_FROM_OTHER";
   case CLM_GROUP_EVENT_MASTERDUPL_ALARM:
      return "EVENT_MASTERDUPL_ALARM";
   case CLM_GROUP_EVENT_LINKSCAN_START:
      return "EVENT_LINKSCAN_START";
   case CLM_GROUP_EVENT_LINKSCAN_TIMEOUT:
      return "EVENT_LINKSCAN_TIMEOUT";
   case CLM_GROUP_EVENT_LINKSCAN_COMPLETE:
      return "EVENT_LINKSCAN_COMPLETE";
   case CLM_GROUP_EVENT_LAST:
      return "EVENT_LAST (dummy state)";
   default:
      return "unknown event";
   }
}

const char * cl_literals_get_group_state (clm_group_state_t state)
{
   switch (state)
   {
   case CLM_GROUP_STATE_MASTER_DOWN:
      return "STATE_MASTER_DOWN";
   case CLM_GROUP_STATE_MASTER_LISTEN:
      return "STATE_MASTER_LISTEN";
   case CLM_GROUP_STATE_MASTER_ARBITRATION:
      return "STATE_MASTER_ARBITRATION";
   case CLM_GROUP_STATE_MASTER_LINK_SCAN_COMP:
      return "STATE_MASTER_LINK_SCAN_COMP";
   case CLM_GROUP_STATE_MASTER_LINK_SCAN:
      return "STATE_MASTER_LINK_SCAN";
   case CLM_GROUP_STATE_LAST:
      return "STATE_LAST (dummy state)";
   default:
      return "unknown state";
   }
}

const char * cl_literals_get_device_state (clm_device_state_t state)
{
   switch (state)
   {
   case CLM_DEVICE_STATE_MASTER_DOWN:
      return "STATE_MASTER_DOWN";
   case CLM_DEVICE_STATE_LISTEN:
      return "STATE_LISTEN";
   case CLM_DEVICE_STATE_WAIT_TD:
      return "STATE_WAIT_TD";
   case CLM_DEVICE_STATE_CYCLIC_SUSPEND:
      return "STATE_CYCLIC_SUSPEND";
   case CLM_DEVICE_STATE_CYCLIC_SENT:
      return "STATE_CYCLIC_SENT";
   case CLM_DEVICE_STATE_CYCLIC_SENDING:
      return "STATE_CYCLIC_SENDING";
   case CLM_DEVICE_STATE_LAST:
      return "STATE_LAST (dummy state)";
   default:
      return "unknown state";
   }
}

const char * cl_literals_get_device_event (clm_device_event_t event)
{
   switch (event)
   {
   case CLM_DEVICE_EVENT_NONE:
      return "EVENT_NONE (dummy state)";
   case CLM_DEVICE_EVENT_GROUP_STARTUP:
      return "EVENT_GROUP_STARTUP";
   case CLM_DEVICE_EVENT_GROUP_STANDBY:
      return "EVENT_GROUP_STANDBY";
   case CLM_DEVICE_EVENT_GROUP_TIMEOUT:
      return "EVENT_GROUP_TIMEOUT";
   case CLM_DEVICE_EVENT_GROUP_ALL_RESPONDED:
      return "EVENT_GROUP_ALL_RESPONDED";
   case CLM_DEVICE_EVENT_RECEIVE_OK:
      return "EVENT_RECEIVE_OK";
   case CLM_DEVICE_EVENT_RECEIVE_ERROR:
      return "EVENT_RECEIVE_ERROR";
   case CLM_DEVICE_EVENT_SCAN_START_DEVICE_START:
      return "EVENT_SCAN_START_DEVICE_START";
   case CLM_DEVICE_EVENT_SCAN_START_DEVICE_STOP:
      return "EVENT_SCAN_START_DEVICE_STOP";
   case CLM_DEVICE_EVENT_TIMEOUTCOUNTER_FULL:
      return "EVENT_TIMEOUTCOUNTER_FULL";
   case CLM_DEVICE_EVENT_TIMEOUTCOUNTER_NOT_FULL:
      return "EVENT_TIMEOUTCOUNTER_NOT_FULL";
   case CLM_DEVICE_EVENT_SLAVE_DUPLICATION:
      return "EVENT_SLAVE_DUPLICATION";
   case CLM_DEVICE_EVENT_LAST:
      return "EVENT_LAST (dummy state)";
   default:
      return "unknown event";
   }
}

const char * cl_literals_get_master_error_message (clm_error_message_t message)
{
   switch (message)
   {
   case CLM_ERROR_ARBITRATION_FAILED:
      return "ARBITRATION_FAILED";
   case CLM_ERROR_SLAVE_DUPLICATION:
      return "SLAVE_DUPLICATION";
   case CLM_ERROR_SLAVE_REPORTS_WRONG_NUMBER_OCCUPIED:
      return "SLAVE_REPORTS_WRONG_NUMBER_OCCUPIED";
   case CLM_ERROR_SLAVE_REPORTS_MASTER_DUPLICATION:
      return "SLAVE_REPORTS_MASTER_DUPLICATION";
   default:
      return "unknown error";
   }
}

const char * cl_literals_get_master_set_ip_result (clm_master_setip_status_t message)
{
   switch (message)
   {
   case CLM_MASTER_SET_IP_STATUS_SUCCESS:
      return "STATUS_SUCCESS";
   case CLM_MASTER_SET_IP_STATUS_ERROR:
      return "STATUS_ERROR";
   case CLM_MASTER_SET_IP_STATUS_TIMEOUT:
      return "STATUS_TIMEOUT";
   default:
      return "unknown result";
   }
}

const char * cl_literals_get_slave_error_message (cls_error_message_t message)
{
   switch (message)
   {
   case CLS_ERROR_SLAVE_STATION_DUPLICATION:
      return "SLAVE_STATION_DUPLICATION";
   case CLS_ERROR_MASTER_STATION_DUPLICATION:
      return "MASTER_STATION_DUPLICATION";
   case CLS_ERROR_WRONG_NUMBER_OCCUPIED:
      return "WRONG_NUMBER_OCCUPIED";
   default:
      return "unknown error";
   }
}

const char * cl_literals_get_slave_state (cls_slave_state_t state)
{
   switch (state)
   {
   case CLS_SLAVE_STATE_SLAVE_DOWN:
      return "STATE_SLAVE_DOWN";
   case CLS_SLAVE_STATE_MASTER_NONE:
      return "STATE_MASTER_NONE";
   case CLS_SLAVE_STATE_MASTER_CONTROL:
      return "STATE_MASTER_CONTROL";
   case CLS_SLAVE_STATE_SLAVE_DISABLED:
      return "STATE_SLAVE_DISABLED";
   case CLS_SLAVE_STATE_WAIT_DISABLING_SLAVE:
      return "STATE_WAIT_DISABLING_SLAVE";
   case CLS_SLAVE_STATE_LAST:
      return "STATE_LAST (dummy state)";
   default:
      return "unknown state";
   }
}

const char * cl_literals_get_slave_event (cls_slave_event_t event)
{
   switch (event)
   {
   case CLS_SLAVE_EVENT_NONE:
      return "EVENT_NONE (dummy state)";
   case CLS_SLAVE_EVENT_STARTUP:
      return "EVENT_STARTUP";
   case CLS_SLAVE_EVENT_CYCLIC_NEW_MASTER:
      return "EVENT_CYCLIC_NEW_MASTER";
   case CLS_SLAVE_EVENT_CYCLIC_CORRECT_MASTER:
      return "EVENT_CYCLIC_CORRECT_MASTER";
   case CLS_SLAVE_EVENT_CYCLIC_WRONG_MASTER:
      return "EVENT_CYCLIC_WRONG_MASTER";
   case CLS_SLAVE_EVENT_CYCLIC_WRONG_STATIONCOUNT:
      return "EVENT_CYCLIC_WRONG_STATIONCOUNT";
   case CLS_SLAVE_EVENT_CYCLIC_INCOMING_WHEN_DISABLED:
      return "EVENT_CYCLIC_INCOMING_WHEN_DISABLED";
   case CLS_SLAVE_EVENT_TIMEOUT_MASTER:
      return "EVENT_TIMEOUT_MASTER";
   case CLS_SLAVE_EVENT_REENABLE_SLAVE:
      return "EVENT_REENABLE_SLAVE";
   case CLS_SLAVE_EVENT_DISABLE_SLAVE:
      return "EVENT_DISABLE_SLAVE";
   case CLS_SLAVE_EVENT_DISABLE_SLAVE_WAIT_ENDED:
      return "EVENT_DISABLE_SLAVE_WAIT_ENDED";
   case CLS_SLAVE_EVENT_IP_UPDATED:
      return "EVENT_IP_UPDATED";
   case CLS_SLAVE_EVENT_LAST:
      return "EVENT_LAST (dummy state)";
   default:
      return "unknown event";
   }
}
