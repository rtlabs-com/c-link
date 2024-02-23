Compliance testing of a CCIEFB master
=====================================
Also known as certification testing or conformance testing.

All CC-Link products need to be certified.

The main document is “CC-Link IE Field Network Basic Master Station Conformance Test Specifications” (BAP-C0401ENG-044).

In order to test a master, you typically run a test tool (on a Windows PC) which is simulating a slave.

Some test cases uses the master under test together with three Windows PCs,
each running the (simulated slave) test tool.

Other test cases use a simulated master on one of the Windows PCs,
in order to study how the master under test reacts to a competing (simulated) master.


Network setup for master conformance testing
--------------------------------------------

+-----------------------+-----------------+-------------------------+
| Hardware              | IP address      | Occupied slave stations |
+=======================+=================+=========================+
| Master under test     | 192.168.3.100   |                         |
+-----------------------+-----------------+-------------------------+
| Competing master      | 192.168.3.50    |                         |
+-----------------------+-----------------+-------------------------+
| Slave                 | 192.168.3.1     | 1                       |
+-----------------------+-----------------+-------------------------+
| Additional slave      | 192.168.3.2     | 1                       |
+-----------------------+-----------------+-------------------------+
| Additional slave      | 192.168.3.3     | 2                       |
+-----------------------+-----------------+-------------------------+


Downloading and installing the simulated slave test tool
--------------------------------------------------------

#. Log in at https://www.cc-link.org/mnt/regularDownload

#. Select :file:`cclink_ief_basic_testtools_en.zip`.

#. Unzip the folder containing the tool.

#. To test a CC-Link IE Field Basic master device, use the tool "CC-Link IE Field
   conformance test tool slave".

#. Start the program by clicking the file
   :file:`Conf_TestTool_ToSlave` (no installation is required).

#. Make sure your PC netmask is ``255.255.255.0``.


Using the simulated slave test tool
-----------------------------------
#. In the main Window, select :guilabel:`Step1. Input of client information`. Fill in
   the relevant fields.
   If you just want to use the tool as a simulated slave, instead
   click :guilabel:`Debug mode` to skip Step1.

#. Select :guilabel:`Step2. Test executed`.

#. In the :guilabel:`Cyclic operation` window, select correct Ethernet interface
   (with correct IP address) by using the :guilabel:`Slave station IP address` dropdown selector.

#. In the :guilabel:`Number of occupies station`, enter the number of slave stations
   that the slave tool should occupy.

#. Press :guilabel:`Cyclic Start`.

Note that the simulated slave test tool must run on a rather powerful Windows PC.
Otherwise its CPU load will reach 100%, and there will be a significant delay in
the response frames sent by the test tool. The master under test will drop incoming
responses as the sequence numbers do not match the numbers in the requests.

The simulated slave conformance test tool responds to SLMP node search requests from the master.


Using the master sample app with the simulated slave test tool
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#. To send the simulated button input (RX00) to the sample app master, enter ``0001`` in
   the ``RX000-00F`` text box in the test tool. Enter ``0000`` to release it.
   Click :guilabel:`Update` and :guilabel:`Update All Stations`.

#. The simulated LED output from the sample app master is sent in the RY16 bit.
   Read its value in the ``RY010-01F`` text box.
   When the simulated LED is on, the value is shown as ``0001``.

#. A value sent to register RWr00 in the master will be doubled by the master sample app,
   and sent back to the slave via register RWw00.
   Try this by entering ``0002`` in the ``RWr000`` text box and verify that
   the value for ``RWw000`` is shown as ``0004``.


Other settings for the simulated slave test tool
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When pressing the :guilabel:`Cyclic Stop` button, the tool will send a response
with endcode `0xCFFF`. This should cause the master to disconnect the slave tool.

To stop sending responses from the simulated slave test tool, use the checkbox
:guilabel:`Stop sending responses`.

When the :guilabel:`Operation mode` is set to :guilabel:`Slave station duplication`,
each response frame from the tool is sent twice.

Use the settings in :guilabel:`Slave station notification information settings`
to adjust the vendor code, model code, device version and management information
to be used in the response.

Note that if the master indicates that the slave test tool is for example slave
station number 3, then you need to use the tab :guilabel:`No.3` to set
the :guilabel:`Slave station output data`.
Thus if you intend to set the register RWr0, you should enter the value in the text
box :guilabel:`RWr040`. The same relation holds for RWw, RX and RY.


Exporting the result of the tests
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
See the description for the simulated master test tool.


Using the master sample app on Linux for conformance testing
------------------------------------------------------------

To run the master with three slaves in one group (with IP addresses
according to the test specification)::

   cl_sample_master -d button.txt -m 192.168.3.100 -s 192.168.3.1 -a


Using a Mitsubishi PLC with the conformance test tool
-----------------------------------------------------------
Program the PLC with the master PLC application developed for demonstrating the slave sample app.

#. To send the simulated button input (RX00) to the PLC, enter ``0001`` in
   the ``RX000-00F`` text box in the test tool. Enter ``0000`` to release it.

#. The LED output from the PLC is sent in the RY16 bit.
   Read its value in the ``RY010-01F`` text box.
   When the LED is on, the value is shown as ``0001``.

#. A value sent to register RWr00 in the PLC will be doubled by the PLC sample app,
   and sent back to the slave via register RWw00.
   Try this by entering ``0002`` in the ``RWr000`` text box and verify that
   the value for ``RWw000`` is shown as ``0004``.


Test cases for master conformance test
--------------------------------------

1.1 (1) Master application status
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use 3 slaves in one group (one slave needs to be simulated)
* Start the cyclic communication.
* Set the master station application status to "Running" (1) and subsequently to "Stopped" (0).
* Verify that the values are sent to the slaves, by studying the value in the
  :guilabel:`Own station unit information` text box in the simulated slave.
* If the master supports protocol version 2 then the status value "Stopped by user" (2)
  should also be used.
* REQ_CLM_CONFORMANCE_01

1.2 (1) Cyclic data
^^^^^^^^^^^^^^^^^^^

* Use 3 slaves in one group.
* Start the cyclic communication.
* Make sure that the master station reads RX values and writes RY values. For the
  sample app, see instructions above.
* Similarly, make sure that the master station reads RWr values and writes RWw values.
* Verify data reception and sending for all slaves.
* REQ_CLM_CONFORMANCE_02

1.2 (2) Max number of occupied slaves
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use one simulated slave.
* Start the master with a configuration having a slave occupying the max number of slave stations.
* In the simulated slave test tool, set :guilabel:`Number of occupied stations` to
  the max value supported by your master.
* Make sure that the master station reads RX values and writes RY values. For the
  sample app, see instructions above.
* Similarly, make sure that the master station reads RWr values and writes RWw values.
* REQ_CLM_CONFORMANCE_03

1.3 (1) Stop communication to individual slave
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use 3 slaves in one group
* Start the cyclic communication.
* The master stops the cyclic communication to one slave.
* Verify that the slave is disconnected.
* Start the cyclic communication to the slave.
* Verify that the slave is connected again.
* REQ_CLM_CONFORMANCE_04

1.3 (2) Stop communication to group
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use 3 slaves in two groups.
* Start the cyclic communication.
* The master stops the cyclic communication to first group.
* Verify that the slaves in first group are disconnected.
* The master starts the cyclic communication to first group, and stops it to the second group.
* Verify that the slaves in first group are connected, and the slave in the
  second group is disconnected.
* REQ_CLM_CONFORMANCE_05

1.3 (3) Lost slave
^^^^^^^^^^^^^^^^^^

* Use 3 slaves in one group.
* Start the cyclic communication.
* In one of the slaves, disable sending responses. If using a simulated slave
  test tool, use the :guilabel:`Stop sending responses` check box.
* Verify that the master detects that a slave is lost, and that the communication
  continues in the rest of the group.
* Use Wireshark to verify the timeout time.
* REQ_CLM_CONFORMANCE_06

1.3 (4) Repeat requests when no slaves are connected
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use one simulated slave.
* Start cyclic communication between the master and the simulated slave.
* In the simulated slave test tool, enable :guilabel:`Stop sending responses`,
  and then in the :guilabel:`Request receiving interval` section,
  click :guilabel:`Clear Measured Data`.
* Verify that the master disconnects the slave.
* Let the measurement run until 1000 data points have been recorded.
* Verify that the average value corresponds to the timeout setting in your master.
* REQ_CLM_CONFORMANCE_07

1.3 (5) Reconnect slave
^^^^^^^^^^^^^^^^^^^^^^^

* Use one slave.
* In the simulated slave test tool, enable :guilabel:`Stop sending responses`. In the master,
  verify that it disconnects the slave. Then start sending responses again, and verify
  that the master reconnects the slave.
* Use Wireshark to confirm that the master disconnects the slave after correct number of
  missing responses, and that the master increases the time between the requests until it
  considers the slave disconnected. The master should reconnect the slave after the first
  re-appearing response.
* REQ_CLM_CONFORMANCE_08

1.3 (6) A slave wants to disconnect
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use one simulated slave.
* In the simulated slave test tool, click :guilabel:`Cyclic Stop` to send the end code 0xCFFF
  to the master. In the master, verify that it disconnects the slave because the slave asks for it.
  Then click :guilabel:`Cyclic Start`, and verify that the master reconnects the slave.
* Use Wireshark to confirm that the master disconnects the slave immediately when receiving
  the endcode. The master should reconnect the slave after the first re-appearing response.
* REQ_CLM_CONFORMANCE_09

1.3 (7) Other master does not interrupt communication
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use one slave and one additional simulated master.
* Start cyclic communication to the slave.
* Run a simulated master test tool on a Windows PC, which uses the same subnet but a different
  IP address than your master.
  Enable the setting :guilabel:`No master station arbitration` in the simulated master tool,
  and use a non-existing IP address in its setting for slave station.
  Press :guilabel:`Cyclic Start` in the simulated master.
* Verify that the communication between your master and the slave not is interrupted.
* REQ_CLM_CONFORMANCE_10

1.3 (8) Constant link scan
^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use one simulated slave.
* Adjust the settings in the master so that it will use constant link scan.
* Run cyclic communication with the simulated slave test tool. In the
  :guilabel:`Request receiving interval` section, press :guilabel:`Clear Measured Data`.
  Wait until :guilabel:`Number of measured data` reaches ``1000``, and verify that
  the value corresponds to the constant link scan time setting of the master.
* REQ_CLM_CONFORMANCE_11

1.4 (1) Detect other master during arbitration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use one slave and one additional simulated master.
* Run a simulated master test tool on a Windows PC, which uses the same subnet but a different
  IP address than your master. Use the correct IP address setting for the slave slave station.
  Press :guilabel:`Cyclic Start` in the simulated master.
  Verify that there is cyclic communication between the simulated master and the slave.
* Start your master.
* Verify that your master reports failed arbitration.
* Use Wireshark to verify that your slave does not send any frames.
* REQ_CLM_CONFORMANCE_12

1.4 (2) Master duplication alarm from slave
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use one simulated slave.
* Start the master with normal link scan (not constant link scan).
* In the (simulated slave) test tool, use the operation mode :guilabel:`Master station duplication`
  and click :guilabel:`Cyclic Start`.
* Verify that your master reports master duplication alarm from a slave.
* Use Wireshark to verify that the master does not send any more requests.
* NOTE: If using the suggested setup from the specification document, the master
  under test will not be able to send any requests at all, as it will stop already
  in arbitration.
* REQ_CLM_CONFORMANCE_13

1.4 (3) Slave duplication reported by master
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use one simulated slave.
* Start the master with normal link scan (not constant link scan).
* In the (simulated slave) test tool, use the operation mode :guilabel:`Slave station ID duplication`.
* Verify that the master reports slave station duplication.
* Verify that the master stops the cyclic transmission to the corresponding
  slave (by clearing the relevant bit in the outgoing frame).
* REQ_CLM_CONFORMANCE_14

1.5 (1) Measure master request time
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use one simulated slave.
* Set the master to use normal link scan (not constant link scan). Run cyclic communication.
* In the (simulated slave) test tool :guilabel:`Standard request time` section,
  press :guilabel:`Clear Measured Data`. Wait until :guilabel:`Number of measured data`
  reaches ``1000``.
* In the popup window of the particular test case, click the :guilabel:`Check data` button
  to insert the min, max and average values from the measurement. Enter the model of the
  Ethernet switch used during the measurement.
* REQ_CLM_CONFORMANCE_15

2 (1) Aging test for 12 hours without errors
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use one simulated slave.
* Run cyclic communication with normal link scan (not constant link scan).
* In the (simulated slave) test tool press :guilabel:`Clear Number of Disconnections`.
* The test should be running for at least 12 hours.
* Verify that the :guilabel:`Number of disconnections` counter in the test tool still is ``0``.
* REQ_CLM_CONFORMANCE_16
