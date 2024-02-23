Slave stack API reference
=========================
This is a brief overview of the public API of the c-link CC-Link IE Field Basic
slave stack.

The API uses these prefixes:

* ``cl_`` Common functionality for CC-Link
* ``cls_`` Functionality for CC-Link slave
* ``clm_`` Functionality for CC-Link master


Initialisation, periodic update and shutdown
--------------------------------------------
.. doxygenfunction:: cl_version
.. doxygenfunction:: cls_init
.. doxygenfunction:: cls_exit
.. doxygenfunction:: cls_handle_periodic
.. doxygenfunction:: cls_stop_cyclic_data
.. doxygenfunction:: cls_restart_cyclic_data
.. doxygenfunction:: cls_get_master_timestamp
.. doxygenfunction:: cls_get_master_connection_details


Values describing the slave status
----------------------------------
.. doxygenfunction:: cls_set_slave_application_status
.. doxygenfunction:: cls_get_slave_application_status
.. doxygenfunction:: cls_set_local_management_info
.. doxygenfunction:: cls_get_local_management_info
.. doxygenfunction:: cls_set_slave_error_code
.. doxygenfunction:: cls_get_slave_error_code


Slave: Access to memory areas
-----------------------------
.. doxygenfunction:: cls_get_first_rx_area
.. doxygenfunction:: cls_get_first_ry_area
.. doxygenfunction:: cls_get_first_rwr_area
.. doxygenfunction:: cls_get_first_rww_area


Slave: Data convenience functions
---------------------------------
.. doxygenfunction:: cls_set_rx_bit
.. doxygenfunction:: cls_get_rx_bit
.. doxygenfunction:: cls_get_ry_bit
.. doxygenfunction:: cls_set_rwr_value
.. doxygenfunction:: cls_get_rwr_value
.. doxygenfunction:: cls_get_rww_value


Slave: Callbacks
----------------
.. doxygentypedef:: cls_state_ind_t
.. doxygentypedef:: cls_master_state_ind_t
.. doxygentypedef:: cls_error_ind_t
.. doxygentypedef:: cls_node_search_ind_t
.. doxygentypedef:: cls_set_ip_ind_t
.. doxygentypedef:: cls_connect_ind_t
.. doxygentypedef:: cls_disconnect_ind_t


Slave: Structs
--------------
.. doxygenstruct:: cls_cfg_t
   :members:
   :undoc-members:

.. doxygenstruct:: cl_rx_t
   :members:
   :undoc-members:

.. doxygenstruct:: cl_ry_t
   :members:
   :undoc-members:

.. doxygenstruct:: cl_rwr_t
   :members:
   :undoc-members:

.. doxygenstruct:: cl_rww_t
   :members:
   :undoc-members:

.. doxygenstruct:: cls_master_connection_t
   :members:
   :undoc-members:


Slave: Enums
------------
.. doxygenenum:: cls_slave_state_t
.. doxygenenum:: cl_slave_appl_operation_status_t
.. doxygenenum:: cls_error_message_t


Slave: Typedefs
---------------
.. doxygentypedef:: cl_ipaddr_t
.. doxygentypedef:: cl_macaddr_t


Slave: Defines
--------------
.. doxygendefine:: CL_WORDSIGNALS_PER_AREA
.. doxygendefine:: CL_BITSIGNALS_PER_AREA
.. doxygendefine:: CL_BYTES_PER_BITAREA


Compile time settings for slave
-------------------------------
* CLS_MAX_OCCUPIED_STATION
