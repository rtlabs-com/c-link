Hardware for testing
=====================
When you develop application software for fieldbuses, you typically need access
to relevant hardware for testing. When developing a CCIEFB slave application,
you need a master (for example a PLC).
Similarly you need some CCIEFB slave when working on a master application.

A search tool for CC-Link enabled products is available on the CLPA web page:
https://www.cc-link.org/sch/C010 Select "CC-Link IE Field Basic"


PLCs
----
MELSEC PLC series:

+------------+--------------------+-----------+---------------------------------------------+
| PLC series | Type               | Tool      | Comments                                    |
+============+====================+===========+=============================================+
| iQ-R       | Modular            | GX Works3 | High performance                            |
+------------+--------------------+-----------+---------------------------------------------+
| iQ-F       | Compact            | GX Works3 | (limited) FX5U, FX5UC, FX5UJ series         |
+------------+--------------------+-----------+---------------------------------------------+
| Q          | Modular            | GX Works2 | QnU series                                  |
+------------+--------------------+-----------+---------------------------------------------+
| L          | Modular, rack-free | GX Works2 | (limited)                                   |
+------------+--------------------+-----------+---------------------------------------------+
| F          |                    |           | FX3 series. No CCIEFB.                      |
+------------+--------------------+-----------+---------------------------------------------+
| QS/WS      |                    |           | Safety-applications. No CCIEFB?             |
+------------+--------------------+-----------+---------------------------------------------+

For a modular PLC you typically need to combine it from a base plane, a CPU
module, a communication module and a power supply. These items are combined
into a single housing for a compact PLC.

The different parts and accessories generally have a product code starting
with the letter of the PLC series.

iQ-F series
^^^^^^^^^^^
Variants of the iQ-F series:

* FX5U  Flagship model 64-128k steps
* FX5UC For expansion modules 64-128k steps
* FX5UJ Cost performance. 48k steps. Supply voltage 100 to 240 V

No support for:

 * Group number setting.
 * Multidrop communication or connection via multiple SLMP networks.
 * COM instruction.

The FX5UJ does not support 1E SLMP frames.

Example of relevant models:

* ``FX5U-32MR/ES``
* ``FX5UJ-24MR/ES``

The number after the dash in the model name indicates the total number of
inputs and outputs. Last part indicates:

* ``MR/ES`` 230 V supply, Relay outputs
* ``MT/ES`` 230 V supply, Transistor outputs (sink type)
* ``MT/ESS`` 230 V supply, Transistor outputs (source type)
* ``MR/DS`` 24 V supply, Relay outputs
* ``MT/DS`` 24 V supply, Transistor outputs (sink type)
* ``MT/DSS`` 24 V supply, Transistor outputs (source type)

There is the plug-in extension module FX5-ENET that supports CCIEFB.

PLCs in the iQ-F series use the engineering tool GX Works3.


iQ-R series
^^^^^^^^^^^
R12CCPU-V


Q series
^^^^^^^^
The CPU naming is ``QnU``, where ``n`` indicates the maximum number of program
steps.

CPUs with CC-Link IE Field Basic support:

* ``Q03UDVCPU`` 30 ksteps
* ``Q04UDVCPU`` 40 ksteps
* ``Q06UDVCPU`` 60 ksteps
* ``Q13UDVCPU`` 130 ksteps
* ``Q16UDVCPU`` 160 ksteps

``UDV`` is high-speed universal CPU, ``UD(E)H`` is universal model CPU
(no CCIEFB).

Base unit:

* ``Q33B`` Art no 136369 (3 slots for I/O modules)
* (``Q32SB`` Art no 147273 2 slots for I/O modules, requires slim power supply)

``S`` is slim type without bus connection to HMI, requires slim power supply.
``D`` is high speed type for multiple CPUs. ``R`` is for redundant power supplies.

Power supply:

* ``Q61P`` 5V output. Supply 230V.
* (``Q61SP`` 5V output, Supply 230V, for slim base unit)
* (``Q62P`` 5V and 24 V output. Supply 230V)

PLCs in the Q series use the engineering tool GX Works2.


Wiring a FX5U PLC
-----------------

Connecting mains voltage to the PLC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Connect the 230 V supply voltage to the ``L`` and ``N`` terminals. Make sure
to read the instruction manual for details.


Wiring a button to the PLC inputs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To connect a button to the input ``X0`` on an AC-powered PLC in the FX5U series,
use the built-in 24 V power supply. Connect the button between the ``24V``
terminal and the ``X0`` terminal. Also connect a wire between the ``0V`` and
the ``S/S`` terminals. There is a status LED for each digital input (``X``
terminal) on the front of the PLC, showing a green light when the input is
active.

Each digital output (``Y`` terminal) has a green status LED, which is on when
the output is active. The output LEDs are functional without connecting
anything to the screw terminals.


Resetting a FX5U PLC
--------------------
Hold the small reset switch for a few seconds, until the ERR LED starts flashing.

It is necessary to reset the PLC for example after a failed arbitration (the
PLC hears another PLC during startup).


Engineering tools
-----------------
This is software you use on your laptop, to program the PLC.

* MELSOFT GX Works3  For iQ-F and iQ-R PLCs (Maybe all series?)
* MELSOFT GX Works2  For FX, L, and Q series PLCs.

``-E`` in the part number indicates English version of the software.

* ``GX Works3 V01-2L0C-E``   Pack of 2 licenses
* ``GX Works3 V01-5L0C-E``   Pack of 5 licenses


Frequency inverters
-------------------
These Mitsubishi frequency inverters can be controlled by CCIEFB:

* FR-E700-NE
* FR-A800-E
* FR-F800-E


NZ2MF IO modules
----------------
For development and evaluation purposes it is handy to have access to a
conformant CCIEFB slave device. An example is the NZ2MF digital input and
output module.

See https://www.mitsubishielectric.com/fa/products/cnt/plcnet/pmerit/cclink_ie/basic/lineup/block_type.html

Part number description
^^^^^^^^^^^^^^^^^^^^^^^
NZ2MF

* ``B2`` Screw terminal block
* ``B1`` Screw terminal block
* ``2S1`` Spring clamp terminal block

Then ``-n`` is the total number of inputs and outputs (16 or 32).

* ``A`` AC input Module (120 V AC)
* ``D`` DC input Module (positive or negative common)
* ``R`` Relay output Module
* ``T`` Transistor output module (sink type). Typically used in Asia.
* ``TE1`` Transistor output module (source type) Typically used in Europe.
* ``DT`` DC input (positive common) and transistor output module (sink type). Typically used in Asia.
* ``DTE1`` DC input (negative common) and transistor output module (source type). Typically used in Europe.

Examples:

* ``NZ2MF2S1-32DT``    Spring clamp terminal  Positive common inputs
* ``NZ2MF2S1-32DTE1``  Spring clamp terminal  Negative common inputs
* ``NZ2MFB1-32DT``     Screw terminal        Positive common inputs
* ``NZ2MFB1-32DTE1``   Screw terminal        Negative common inputs

Terminal naming:

* Inputs ``X`` (for example ``X0`` to ``XF``)
* Outputs ``Y`` (for example ``Y10`` to ``Y1F``)


Downloading CSP+ files
^^^^^^^^^^^^^^^^^^^^^^
The CSP+ file for each version of the IO module can be downloaded from
the Mitsubishi Electric web page (login required): https://www.mitsubishielectric.com


Details of NZ2MFB1-32DTE1
-------------------------

============== ============
Vendor code    0x0000
Product ID     0x0000300E
Version        0x0001
============== ============

To connect a button to one of the digital inputs, connect it between the +24 V
supply and the corresponding input terminal. Connect the 0V terminal of your
power supply to the COM- terminal.

The green DLINK LED in on when there is cyclic communication.


Cyclic data for the IO module
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Hexadecimal numbering is used on the front of the IO-module.
Note that the inputs (X) start from 0, while the outputs (Y) start from 16 (dec).

* X0-XF (hex) = X0-X15 (dec)
* Y10-Y1F (hex) = Y16-Y31 (dec)

When X0 is high, the RX data bytes sent to the PLC are ``01 00 00 00 00 00 00 00``.

The RY data bytes from the PLC to set Y10 (hex) are ``00 00 01 00 00 00 00 00``.

PLC program ``Y120`` controls the output ``Y10`` (hex), when RY is mapped to Y
with a start of 100.


Remote setting of IP address
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In order to set the IP address remotely, all IP address physical switches on
the unit must be in the "off" position.

If you try to remotely change the IP address while any of the front side
switches for setting the IP address is in the "on" position, the IO-module will
respond with an error frame.


Triggering a slave error
^^^^^^^^^^^^^^^^^^^^^^^^
Trigger an error by changing position of function switch 1 while the
slave is running. This causes the red ERR LED to flash, and the slave
error code is set to 0x0202 in the response frame. However the unit will
continue to work normally.


LocalManagementInfo from the slave
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The field "Own station management information" contains information on for example
the response time settings. If you set the response time to 70 ms by setting
the corresponding front side switches 1, 2 and 3 to "on" position, the bits
0, 1 and 2 in the upper half of the "Own station management information" field
are enabled. The upper half will be 0x0007, and the total value will be
for example 0x00070002. It is unclear what the lower half of the field represents.
