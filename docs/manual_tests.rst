Manual Tests
============


Manual testing of slave stack
-----------------------------

Use a commercial PLC.

For each of the supported operating systems:

* Verify cyclic data (flashing LED) and slave disconnect request
* Verify node search (using the PLC engineering tool)
* Verify Set IP address (using the PLC engineering tool)
* Verify the SLMP "directed broadcast" config setting
* Verify CL_IPADDR_ANY and specific IP address in the configuration


Manual testing of master stack
------------------------------

For each of the supported operating systems:

* Verify cyclic data (flashing LED) and other functionality in
  the sample app, by using a commercial IO-device.
* Verify detection of a competing master during arbitration,
  by using a commercial PLC.
* Verify the SLMP "directed broadcast" config setting
