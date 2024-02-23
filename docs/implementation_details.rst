Implementation details
======================
This software stack is written in C, version C99.

C++ (version C++20 or later) is required for running the unit tests.


Test case statistics
--------------------

.. jinja:: teststatistics_context

    There are {{ number_of_testcases }} test cases implemented, using the Google Test framework.
    They do {{ number_of_expects }} value checks  via ``EXPECT_`` (for example ``EXPECT_EQUAL``)
    and {{ number_of_asserts }} via ``ASSERT_`` (for example ``ASSERT_EQUAL``).


State machine implementation for slave
--------------------------------------
This graph shows the state machine for the slave. Note that the figure
is generated from the state machine source code.

Right-click the image and select "Open image in new tab" for a larger version
of the image.

.. graphviz:: _generated/slave_state_machine.dot


State machine implementation for master
---------------------------------------
There are two types of state machines in the master, one type for controlling
groups of slave devices and one type for controlling individual slave devices.
See the figures below.

Right-click an image and select "Open image in new tab" for a larger version
of it.

.. graphviz:: _generated/group_state_machine.dot

.. graphviz:: _generated/device_state_machine.dot



Specification details for different commands
--------------------------------------------

Node search
^^^^^^^^^^^

============================= ============== =================================================================
Document                      Section        Description
============================= ============== =================================================================
SLMP Overview                 6.22.2         Connected Device Search (NodeSearch)
SLMP Overview                 7.1.1          Automatic Detection Function
SLMP Services                 8.2.2.3.11.1   NodeSearch
SLMP Protocol                 6.7.2.2.2      ReqNodeSearch
SLMP Protocol                 6.9.2.2.2      ResNodeSearch
SLMP Protocol                 7.1.9.49       NodeConnect NodeSearch
SLMP Protocol                 7.1.10.22      NodeConnect NodeSearch
SLMP Protocol                 8.3.10.5.1     ReqNodeSearch
SLMP Protocol                 8.3.11.3.1     ResNodeSearch
SLMP reference manual         3.12           Node Search
CCIEFB Overview               5.7            Necessity of Command Implementation
CCIEFB Development Guideline   3              Confirmation of protocol specification
CCIEFB Development Guideline   5.2.3          Automatic detection and IP address setting of connected devices
============================= ============== =================================================================


Set IP
^^^^^^

============================= ============== =================================================================
Document                      Section        Description
============================= ============== =================================================================
SLMP Overview                 6.5.6          rdErrMT-PDU, wrErrMT-PDU (error frame)
SLMP Overview                 6.9            Correspondence between Functions of SLMP and PDU to Use
SLMP Overview                 6.22.3         Setting IP Address of Device to Be Connected (IPAddressSet)
SLMP Overview                 7.1.2          Communication Setting Function
SLMP Services                 8.2.2.3.11.2   IPAddressSet
SLMP Protocol                 6.7.2.2.3      ReqIPAddressSet
SLMP Protocol                 6.9.2.2.3      ResIPAddressSet
SLMP Protocol                 7.1.9.50       NodeConnect IPAddressSet
SLMP Protocol                 7.1.10.23      NodeConnect IPAddressSet
SLMP Protocol                 8.3.10.5.2     ReqIPAddressSet
SLMP reference manual         3.12           IP Address Set
CCIEFB Overview               5.7            Necessity of Command Implementation
CCIEFB Development Guideline   3              Confirmation of protocol specification
CCIEFB Development Guideline   5.2.3          Automatic detection and IP address setting of connected devices
============================= ============== =================================================================


Parameter setting
^^^^^^^^^^^^^^^^^

============================= ============== ==================================
Document                      Section        Description
============================= ============== ==================================
SLMP Overview                 6.23           Setting parameters
SLMP Overview                 7.2            Parameter Reading/Writing
CCIEFB Development Guideline   3.1.6          Parameter setting function
============================= ============== ==================================


DeviceInfoCompare
^^^^^^^^^^^^^^^^^

============================= ============== ==================================
Document                      Section        Description
============================= ============== ==================================
SLMP Services                 8.2.2.3.12.1   DeviceInfoCompare
SLMP Protocol                 6.7.2.3.2      ReqDeviceInfoCompare
SLMP Protocol                 6.9.2.3.2      ResDeviceInfoCompare
SLMP Protocol                 7.1.9.51       ParameterSetting DeviceInfoCompare
SLMP Protocol                 7.1.10.24      ParameterSetting DeviceInfoCompare
SLMP Protocol                 8.3.10.3       command
SLMP Protocol                 8.3.10.5.3     ReqDeviceInfoCompare
SLMP Protocol                 8.3.11.3.2     ResDeviceInfoCompare
============================= ============== ==================================


Stopping cyclic communication from slave
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

============================= ============== ===================================================
Document                      Section        Description
============================= ============== ===================================================
CCIEFB Overview               5.4.3 (11)     Sequence of cyclic transmission
CCIEFB Development Guideline  3.1.4          Disconnection request function from slave stations
============================= ============== ===================================================


Description of parameters
-------------------------

Within each slave device, we use the term "area number" to indicate which of the
occupied slave stations to transfer data to or from.

======================= =========================== ========== =========================================
Item                    Type                        Min        Max
======================= =========================== ========== =========================================
Group                   uint16_t (uint8_t in frame) 1          64 or CLM_MAX_GROUPS
Group index             uint16_t                    0          63 or (CLM_MAX_GROUPS-1)
Number of groups        uint16_t                    1          64 or CLM_MAX_GROUPS
Slave station number    uint16_t                    1          16 or CLM_MAX_OCCUPIED_STATIONS_PER_GROUP
Slave device number     uint16_t                    1          16 or CLM_MAX_OCCUPIED_STATIONS_PER_GROUP
Slave device index      uint16_t                    0          15 or (CLM_MAX_OCCUPIED_STATIONS_PER_GROUP-1)
Number of slave devices uint16_t                    1          16 or CLM_MAX_OCCUPIED_STATIONS_PER_GROUP
Total occupied          uint16_t in frame           1          16 or CLM_MAX_OCCUPIED_STATIONS_PER_GROUP
Frame sequence number   uint16_t in frame           0          0xFFFF
Parameter number        uint16_t in frame           0          0xFFFF
Protocol version        uint16_t in frame           1          2
Timeout value           uint16_t in frame           1 (0)      0xFFFF
Timeout count           uint16_t in frame           1 (0)      0xFFFF
UDP payload size        uint16_t in frame           0
End code                uint16_t in frame           0          0xFFFF
RY                      uint16_t                    0          64*num_occupied_stations - 1
RX                      uint16_t                    0          64*num_occupied_stations - 1
RWw                     uint16_t                    0          32*num_occupied_stations - 1
RWr                     uint16_t                    0          32*num_occupied_stations - 1
======================= =========================== ========== =========================================


UDP socket usage on different operating systems
-----------------------------------------------
Given an IP address of for example ``192.168.0.50`` and a netmask of ``255.255.255.0``, the
directed broadcast address is ``192.168.0.255``.

The local broadcast address is ``255.255.255.255`` (confusingly also known as global broadcast).
From :rfc:`919`: "The address 255.255.255.255 denotes a broadcast on a local hardware
network, which must not be forwarded." Also :rfc:`922` is relevant to broadcasts.

For IEFB (cyclic data):

* A master must not hear other masters, or it will stop.
* A slave must not hear more than one master, or it will send an alarm.
* If there is a master and a slave on the same machine, they must be on different subnets
  otherwise the slave will hear both the internal and the external master.
* If there are more than one master on a machine, they must be on different subnets
  otherwise they will hear each other.
* Slaves on the same subnet must have different IP addresses.
* Thus, all masters and slaves on a machine must have different IP addresses.
* If a slave listens to all network interfaces on a machine, there can not be a master
  on the same machine because the slave would hear both the internal and the external master.
* Requests are sent to directed broadcast address (for example ``x.x.x.255``)
* Responses are sent to the IP address of the master.
* The request sent from the master contains the IP address of the master, so the
  master IP address must be known before the master sends the first request frame.

For SLMP (node search etc):

* Requests and responses are sent to the local broadcast address (``255.255.255.255``) so
  everyone will hear them regardless of their network settings.
* Slaves are only interested in incoming SLMP requests.
* Masters are only interested in incoming SLMP responses.

UDP socket behavior on Linux:

=============================== ============ ========================== =====================
Receive frames with destination Bind to IP   Bind to directed broadcast Bind to IP_ADDR_ANY
=============================== ============ ========================== =====================
IP                              OK           No reception               OK
Directed broadcast              No reception OK                         OK
Local broadcast                 No reception No reception               OK
Send to directed broadcast      OK           OK                         OK
Send to local broadcast         OK           Not possible               Not possible
=============================== ============ ========================== =====================

On Windows will directed broadcast frames be received after binding to an IP address (it
is not possible to bind to the directed broadcast address)

On RT-Kernel (using LwIP) it is possible to send to local broadcast on a socket bound to IP_ADDR_ANY.
Binding multiple sockets to the same port number makes the reception unpredictable.

Valid for Linux:

================== ==================================== ============== =============================================
Socket             Bind to address                      Bind to port   Notes
================== ==================================== ============== =============================================
Master IEFB Listen Directed broadcast (or IP_ADDR_ANY)  IEFB           Receive directed broadcast from other masters
Master IEFB Listen IP (IP_ADDR_ANY not of much use)     IEFB           Receive unicast from slave
Master IEFB Send   Whatever                             IEFB           Send to directed broadcast
Master SLMP Listen IP_ADDR_ANY                          SLMP           Receive local broadcast on all interfaces
Master SLMP Send   IP                                   SLMP           Send local broadcast on own interface
Slave IEFB Listen  Directed broadcast or IP_ADDR_ANY    IEFB           Receive directed broadcast
Slave IEFB Send    Whatever                             IEFB           Send to IP address of the master
Slave SLMP Listen  IP_ADDR_ANY                          SLMP           Receive local broadcast on all interfaces
Slave SLMP Send    IP                                   SLMP           Send local broadcast on own interface
================== ==================================== ============== =============================================

For Windows, the slave IEFB Listen socket must be bound to the IP address or to IP_ADDR_ANY.

The conclusion is that for normal IEFB communication a single socket for sending and receiving is sufficient, but for
SLMP there must be a separate socket for sending and receiving. This applies to both the master and the slave.
However, for the master to detect other masters via IEFB, a separate socket is required on Linux (not on Windows
or RT-kernel). For RT-Kernel, the master should use a single socket for both sending and receiving SLMP messages.

If a master uses IP_ADDR_ANY when listening for SLMP responses it will hear messages also from the subnets
that a slave or other masters on the same machine are using. Thus it is necessary to filter incoming responses
with regards to subnet or interface index.

Implementation limitations:

* Only a single slave per machine (no filtering of incoming SLMP requests with regards to interface index).
  The single slave might occupy several slave stations.
* Only a single IP address and a single subnet per network interface.


File path separators on different operating systems
---------------------------------------------------
File paths on Windows use the backslash ``\`` instead
of forward slash ``/`` as path separator.

When compiling for Windows the ``CL_USE_BACKSLASH_PATH_SEPARATOR`` define will
automatically be set to ``true`` (if not already set to something else).
You can override that define (to ``true`` or ``false``) during compilation.
