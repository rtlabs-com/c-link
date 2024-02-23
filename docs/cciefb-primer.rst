CC-Link IE Field Basic primer
=============================

CC-link is a protocol for communication between a Programmable Logic Controller
(PLC) and remote input/output devices. It is used for example by Mitsubishi PLCs.
See https://en.wikipedia.org/wiki/CC-Link_Open_Automation_Networks

CC-Link is governed by the CC-Link Partner Association (CLPA),
see https://www.cc-link.org/


CC-Link protocol variants
-------------------------

+---------------------------------+------------------------------------------------------------------------------+
| Protocol variant                | Description                                                                  |
+=================================+==============================================================================+
| SLMP                            | Seamless Message Protocol (used by all protocols below)                      |
+---------------------------------+------------------------------------------------------------------------------+
| CC-Link                         | Uses RS-485, up to 10 Mbit/s                                                 |
+---------------------------------+------------------------------------------------------------------------------+
| CC-Link/LT                      | Uses RS-485, smaller number of I/O                                           |
+---------------------------------+------------------------------------------------------------------------------+
| CC-Link Safety                  | Uses RS-485 (see also "CC-Link IE Safety")                                   |
+---------------------------------+------------------------------------------------------------------------------+
| CC-Link IE                      | See "CC-Link IE Control" and "CC-Link IE Field"                              |
+---------------------------------+------------------------------------------------------------------------------+
| CC-Link IE Control              | 1 Gbit/s PLC-to-PLC communication. Special hardware required, fiber optics.  |
+---------------------------------+------------------------------------------------------------------------------+
| CC-Link IE Field                | 1 Gbit/s PLC to field devices. Special hardware required. Copper cables.     |
+---------------------------------+------------------------------------------------------------------------------+
| CC-Link IE Field Basic (CCIEFB) | 100 Mbit/s. No specialized hardware required. 64 stations per network.       |
+---------------------------------+------------------------------------------------------------------------------+
| CC-Link IE TSN                  | 100 Mbit/s or 1 Gbit/s. Uses Time-sensitive networking.                      |
+---------------------------------+------------------------------------------------------------------------------+
| CC-Link IE Safety               | Runs on top of "IE Control", "IE Field" or "IE TSN". For SIL level 3.        |
+---------------------------------+------------------------------------------------------------------------------+

CCIEFB
------
CCIEFB is one of the CC-Link protocol variants, on which SLMP may be implemented.
The "binary" variant of SLMP is used.

CCIEFB slave devices typically have one or two Ethernet connectors. Slaves
with one connector are used in a star topology, with standard
Ethernet switches.
Devices with two connectors can also be used in a line topology.
A transmission speed of 100 Mbit/s (optionally 1 Gbit/s) is used, and the
maximum segment length is 100 m.

One master is used, and there can be at most 64 slave stations.
There can only be one master within an Ethernet subnet,
so multiple subnets must be used if deploying multiple masters.
The recommended subnet size (subnet mask) is class C (``255.255.255.0``) according to BAP-C2010ENG-001.
The recommended factory default IP addresses are in the ``192.168.3.x`` range,
according to BAP-C3003ENG-001-B.

Data is cyclically sent between the master and the slave. This is called link scan.
The master is sending the cyclic data using a directed broadcast within its subnet,
and the slaves are responding with unicast messages. All messages use little endian encoding.

Slaves can be assigned to groups. The master station talks to each group
sequentially. Each slave belongs to one group only. Each group can have up to 16 slaves (out of the max 64 slaves),
and the groups are numbered 1 to 64.

The master is an SLMP client and the slave is an SLMP server.

CCIEFB uses IPv4 (not IPv6).


CCIEFB cyclic data
^^^^^^^^^^^^^^^^^^
The cyclic data is transported in UDP frames.

* Request: Master (PLC) -> Slave
* Response: Slave -> Master (PLC)

SLMP messages are sent via CC-Link IE Field Basic (CCIEFB).

There are two types of cyclic data; bits and words. These are called points. The size of the word is 16 bits.

+---------+------------------------------------+------------------+--------------------------+--------------------+
| Device  | Description                        | Direction        | Points (inputs/outputs)  | Size               |
+=========+====================================+==================+==========================+====================+
| ``RY``  | Remote IO Request bits             | Outputs from PLC | 64 per slave (bit size)  | 8 bytes per slave  |
+---------+------------------------------------+                  +--------------------------+--------------------+
| ``RWw`` | Remote IO Register words (16 bits) |                  | 32 per slave (word size) | 64 bytes per slave |
+---------+------------------------------------+------------------+--------------------------+--------------------+
| ``RX``  | Remote IO Request bits             | Inputs to PLC    | 64 per slave (bit size)  | 8 bytes per slave  |
+---------+------------------------------------+                  +--------------------------+--------------------+
| ``RWr`` | Remote IO Register words (16 bits) |                  | 32 per slave (word size) | 64 bytes per slave |
+---------+------------------------------------+------------------+--------------------------+--------------------+

The RY bits are numbered RY0, RY1, RY2 etc. The RX bits and the RWw and RWr registers are numbered similarly.

By occupying more than one station, each slave device can handle a multiple of
these points. Each slave device has its own IP address.

The slave station number is 1 to 16 (within one group).

Each time the PLC modifies the grouping of slaves etc, the parameterNo value
is updated. The parameterNo value is the same in outgoing frames for all groups.


Configuration using CSP+ files
------------------------------
CC-Link uses CSP+ files (``.cspp`` file ending) for configuration. The file
is in a XML-based format. See the :ref:`Working with CSP+ files` section for more details.


UDP port numbers
----------------
CC-Link IE Field Basic uses UDP for all communication. These port numbers are
used:

* 45237 ``0xb0b5`` SLMP Default port number for server (Used at Communication Setting Get)
* 61450 ``0xf00a`` CCIEFB Cyclic data transmission
* 61451 ``0xf00b`` CCIEFB Slave port for automatic device detection via SLMP
* 61550 ``0xf06e`` SLMP communication (not CCIEFB)


Info values sent from slave to master
-------------------------------------

- slaveLocalUnitInfo - Slave is running or stopped
- endCode - Standardized error codes. Normal state is 0x0000.
- slaveErrCode - User defined error codes, should be documented in the CSP+ file. Normal state is 0x0000.
- localManagementInfo - User defined data, for example hardware filtering time setting. Should be described in the documentation.


endCode values
--------------

* ``0x0000`` No error
* ``0xCFE0`` Master station duplication
* ``0xCFE1`` Wrong number of occupied stations
* ``0xCFF0`` Error in slave station (for example high communication load)
* ``0xCFFF`` Disconnection notification from slave station


Commands
--------

* ``0x0E70`` CCIEFB
* ``0x0E30`` Node search (for CCIEFB only)
* ``0x0E31`` Set IP address (for CCIEFB only)


Layout of CCIEFB messages
-------------------------

Request message:

* Request header (fixed size)
* List of IP addresses for slave stations
* Cyclic RWW data to slave stations
* Cyclic RY data to slave stations

Response message:

* Response header (fixed size)
* Cyclic RWr data from occupied stations
* Cyclic RX data from occupied stations


Standards
---------

=========================================== ===================================
Name                                        Number (may have a revision
                                            letter as suffix)
=========================================== ===================================
SLMP overview                               BAP-C2006ENG-001
SLMP services                               BAP-C2006ENG-002
SLMP protocol                               BAP-C2006ENG-003
SLMP reference manual                       BAP-C3002-001
SLMP conformance test for client            BAP-C0401ENG-039
SLMP conformance test for server            BAP-C0401ENG-040
SLMP conformance test tool guide            BAP-C3011ENG-001
CCIEFB overview                             BAP-C2010ENG-001
CCIEFB data link layer                      BAP-C2010ENG-002
CCIEFB service                              BAP-C2010ENG-003
CCIEFB protocol                             BAP-C2010ENG-004
CCIEFB communication profile                BAP-C2010ENG-005
CCIEFB implementation rules                 BAP-C2010ENG-006
CCIEFB master conformance test              BAP-C0401ENG-044
CCIEFB slave conformance test               BAP-C0401ENG-045
CCIEFB Development Guideline                BAP-C3003ENG-001
CSP+ specification v2.2                     BAP-C2008ENG-001
CSP+ specification for machine              BAP-C2008ENG-002 (parts 1-5)
CSP+ creation guidelines, CCIEFB version    BAP-C3001ENG-001
CSP+ creation guidelines, CC-Link version   BAP-C3001ENG-002
CSP+ creation guidelines, IE Field version  BAP-C3001ENG-003
=========================================== ===================================

.. tip:: Start by reading the "CCIEFB Development Guideline" and the "CCIEFB overview".

The specifications are available at
https://www.cc-link.org/en/downloads/ (registration required).

.. note:: CLPA membership is required to access most of the specifications.
