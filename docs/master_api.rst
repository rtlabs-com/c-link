Master stack API reference
==========================
This is a brief overview of the public API of the c-link CC-Link IE Field Basic
master stack.

The API uses these prefixes:

* ``cl_`` Common functionality for CC-Link
* ``cls_`` Functionality for CC-Link slave
* ``clm_`` Functionality for CC-Link master


Master Initialisation, periodic update and shutdown
---------------------------------------------------
For :c:func:`cl_version` see the slave stack API description.

.. doxygenfunction:: clm_init
.. doxygenfunction:: clm_handle_periodic
.. doxygenfunction:: clm_set_master_application_status
.. doxygenfunction:: clm_get_master_application_status
.. doxygenfunction:: clm_set_slave_communication_status
.. doxygenfunction:: clm_clear_statistics
.. doxygenfunction:: clm_exit


Master SLMP commands
--------------------
.. doxygenfunction:: clm_perform_node_search
.. doxygenfunction:: clm_get_node_search_result
.. doxygenfunction:: clm_set_slave_ipaddr


Master: Access to memory areas
------------------------------
.. doxygenfunction:: clm_get_first_rx_area
.. doxygenfunction:: clm_get_first_ry_area
.. doxygenfunction:: clm_get_first_rwr_area
.. doxygenfunction:: clm_get_first_rww_area
.. doxygenfunction:: clm_get_first_device_rx_area
.. doxygenfunction:: clm_get_first_device_ry_area
.. doxygenfunction:: clm_get_first_device_rwr_area
.. doxygenfunction:: clm_get_first_device_rww_area


Master: Data convenience functions
----------------------------------
.. doxygenfunction:: clm_get_rx_bit
.. doxygenfunction:: clm_set_ry_bit
.. doxygenfunction:: clm_get_ry_bit
.. doxygenfunction:: clm_get_rwr_value
.. doxygenfunction:: clm_set_rww_value
.. doxygenfunction:: clm_get_rww_value


Master: Functions for testing
-----------------------------
.. doxygenfunction:: clm_get_master_status
.. doxygenfunction:: clm_get_group_status
.. doxygenfunction:: clm_get_device_connection_details
.. doxygenfunction:: clm_force_cyclic_transmission_bit


Master callbacks
----------------
.. doxygentypedef:: clm_state_ind_t
.. doxygentypedef:: clm_connect_ind_t
.. doxygentypedef:: clm_disconnect_ind_t
.. doxygentypedef:: clm_changed_slave_info_ind_t
.. doxygentypedef:: clm_linkscan_complete_ind_t
.. doxygentypedef:: clm_alarm_ind_t
.. doxygentypedef:: clm_error_ind_t
.. doxygentypedef:: clm_node_search_cfm_t
.. doxygentypedef:: clm_set_ip_cfm_t


Master: Structs
----------------
.. doxygenstruct:: clm_cfg_t
   :members:
   :undoc-members:

.. doxygenstruct:: clm_slave_hierarchy_t
   :members:
   :undoc-members:

.. doxygenstruct:: clm_group_setting_t
   :members:
   :undoc-members:

.. doxygenstruct:: clm_slave_device_setting_t
   :members:
   :undoc-members:

.. doxygenstruct:: clm_node_search_response_entry_t
   :members:
   :undoc-members:

.. doxygenstruct:: clm_node_search_db_t
   :members:
   :undoc-members:

.. doxygenstruct:: clm_slave_device_data_t
   :members:
   :undoc-members:

.. doxygenstruct:: clm_slave_device_time_statistics_t
   :members:
   :undoc-members:

.. doxygenstruct:: clm_slave_device_statistics_t
   :members:
   :undoc-members:

.. doxygenstruct:: clm_master_status_details_t
   :members:
   :undoc-members:

.. doxygenstruct:: clm_group_status_details_t
   :members:
   :undoc-members:

.. doxygenstruct:: clm_device_framevalues_t
   :members:
   :undoc-members:


Master: Enums
-------------
.. doxygenenum:: clm_master_state_t
.. doxygenenum:: clm_device_state_t
.. doxygenenum:: clm_master_setip_status_t
.. doxygenenum:: clm_error_message_t


Master: Defines
---------------
See the slave stack API documentation for:

* :c:type:`CL_WORDSIGNALS_PER_AREA`
* :c:type:`CL_BITSIGNALS_PER_AREA`
* :c:type:`CL_BYTES_PER_BITAREA`

Compile time settings for master
--------------------------------
* CLM_MAX_GROUPS
* CLM_MAX_OCCUPIED_STATIONS_PER_GROUP
* CLM_MAX_NODE_SEARCH_DEVICES
