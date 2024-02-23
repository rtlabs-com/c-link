Compliance testing of CCIEFB slave devices
==========================================
Also known as certification testing or conformance testing.

All CC-Link products need to be certified.

The main document is "CC-Link IE Field Network Basic Slave Station
Conformance Test Specifications" (BAP-C0401ENG-045-B).

The test tool for CCIEFB slaves is a master running on a Windows PC. This
tool can also be used for measuring the performance of a slave.


Network setup for slave conformance testing
-------------------------------------------

+-----------------------+-----------------+
| Hardware              | IP address      |
+=======================+=================+
| Master                | 192.168.3.39    |
+-----------------------+-----------------+
| Additional master     | 192.168.3.40    |
+-----------------------+-----------------+
| Slave under test      | 192.168.3.1     |
+-----------------------+-----------------+


Downloading and installing the conformance test tool
-----------------------------------------------------

#. Log in at https://www.cc-link.org/mnt/regularDownload

#. Select :file:`cclink_ief_basic_testtools_en.zip`.

#. Unzip the folder containing the tool.

#. To test a CC-Link IE Field Basic slave device, use the tool "CC-Link IE Field
   conformance test tool master".

#. Start the program by clicking the file
   :file:`Conf_TestTool_ToMaster` (no installation is required).

#. Make sure your PC netmask is ``255.255.255.0``.


Using the test tool
-------------------
#. In the main Window, select :guilabel:`Step1. Input of client information`. Fill in
   the relevant fields.
   If you just want to use the tool as a simulated master, instead
   click :guilabel:`Debug mode` to skip Step1.

#. Select :guilabel:`Step2. Test executed`.

#. In the :guilabel:`Cyclic operation` window, select correct Ethernet interface
   by using the top dropdown selector.

#. In the :guilabel:`Setting for each slave station`, enter the IP address of
   your slave in the top line (slave station 1). Also enter the number of occupied
   stations.

#. Click :guilabel:`Cyclic Start`. Make sure the
   :guilabel:`Own station unit information` setting is ``0001`` (master running).
   When there are responses from the slave, the :guilabel:`Own station data link status`
   will turn blue. Also the :guilabel:`Link status` indicator for the corresponding slave
   station will turn blue.


Using the slave sample app with the conformance test tool
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#. Press and hold button1 ("data button") of the sample app in order to make the
   field :guilabel:`RX00-0F` in the test tool display ``0001``.

   The button press counter is displayed in the :guilabel:`RWrOO` field.

#. To enable the output data LED (which is Y10 = 16 dec), set the field
   :guilabel:`RY10-1F` to ``0001``. The field turns red.

#. Press :guilabel:`Update`, and confirm by pressing :guilabel:`All No.`.

   The sample app will display "fourtytwo" when the field :guilabel:`RWw00`
   is changed to ``002A``.

Doing a node search and setting IP
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
It is possible to do a node search or a remote set IP operation using the
conformance test tool.

#. In the :guilabel:`Cyclic operation` window press the
   :guilabel:`Detect connected device` to perform a node search.

#. To set the IP address of your device press the
   :guilabel:`IP address settings of connected devices`.

#. In the pop-up window, enter the MAC address of your device and the new IP address.
   A confirmation notification will show the result of the operation.


Other tool settings
^^^^^^^^^^^^^^^^^^^
Deselecting the :guilabel:`Start status` will set the IP address for the
corresponding slave to ``0.0.0.0`` in the outgoing frame from the
master station. This causes the slave to refuse any communication.
You need to click on another row in the table for this to take effect.

To tell the slaves whether the master station application is running or not,
use the field :guilabel:`Own station unit information`.
To tell that the master application is
running use the value ``0001``, otherwise ``0000``. The value can be changed
while the cyclic data is running. If you run protocol version 2, it is also
possible to use ``0002`` which indicates "master stopped by user".

It is possible to simulate slave duplication errors by enabling the
:guilabel:`Forced ON of transmission status` check box.
A slave receiving this frame as the first frame will interpret it as that
the master is already communicating with some other slave (having the
same slave address).


Exporting the result of the tests in CSV format
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In the main window press :guilabel:`Step3. Output of test result`. The resulting
file will be saved in the same directory as the test tool binary is located in.


Using a Mitsubishi IO device with the conformance test tool
-----------------------------------------------------------
Setting the input terminal X0 to +24 V results in the field :guilabel:`RX00-0F`
to display ``0001`` in the test tool.

To enable output terminal Y10 (which is 16 dec), set the field
:guilabel:`RY10-1F` to ``0001``. The field turns red. Press :guilabel:`Update`,
and confirm by pressing :guilabel:`All No.`.

Moving the physical switches on the IO device while running will change the field
:guilabel:`error code` in the section :guilabel:`Slave station input data`.


Test cases for slave conformance test
-------------------------------------

1.1 (1) Known values
^^^^^^^^^^^^^^^^^^^^

* Enter known values of manufacturer, model and version. This will later
  be used for comparison when reading out data.

1.1 (2) Slave info in CCIEFB response
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use a simulated master
* Start cyclic communication where the slave under test is slave station 1.
* Verify that there is cyclic communication, by looking at the
  :guilabel:`Link status` indicator.
* Use button :guilabel:`Check data` in the popup window of the particular test
  case to copy the received vendor code, model code etc into the test results.
* REQ_CLS_CONFORMANCE_01

1.2 (1) Cyclic data for max occupied slave stations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use a simulated master
* Set the number of occupied stations in the slave under test to maximum allowed value.
* Use a corresponding setting in the simulated master test tool. Start cyclic communication.
* Verify that RWw and RY values can be sent from the test tool to the device.
* Verify that RWr and RX values can be sent from the device to the test tool.
* REQ_CLS_CONFORMANCE_02

1.2 (2) Cyclic data for min occupied slave stations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use a simulated master
* Set the number of occupied stations in the slave under test to minimum allowed value.
* Use a corresponding setting in the simulated master test tool. Start cyclic communication.
* Verify that RWw and RY values can be sent from the test tool to the device.
* Verify that RWr and RX values can be sent from the device to the test tool.
* REQ_CLS_CONFORMANCE_03

1.3 (1) Start and stop cyclic communication
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use a simulated master
* Run cyclic communication with maximum number of occupied stations.
* Use the check box :guilabel:`Start status` to start and stop the particular slave.
  Note that you need to click on another row in the table for the changes
  to take effect.
* Verify in slave and in the simulated master tool that the slave is disconnected,
  and subsequently reconnected.
* REQ_CLS_CONFORMANCE_04

1.3 (2) Slave stops cyclic communication
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use a simulated master
* Run cyclic communication with maximum number of occupied stations.
* In the slave under test, indicate to the master to disconnect.
* Verify the disconnection in the simulated master.
* In the slave under test, indicate to the master to reconnect.
* Verify the reconnection in the simulated master.
* REQ_CLS_CONFORMANCE_05

1.3 (3) Slave reacts to master operation status
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use a simulated master
* Run cyclic communication with maximum number of occupied stations,
  using protocol version 1 in the simulated master tool.
* In the simulated master tool, set :guilabel:`Own station unit information`
  to ``0000`` to indicate that the master application is stopped.
* Verify that the slave under test reacts according to its documentation.
* Re-enable the master application by setting the value to ``0000``. Verify
  the slave behavior.
* Repeat for protocol version 2. With this protocol version it is also possible
  to use the value ``0002``, which is "application stopped by user".
* REQ_CLS_CONFORMANCE_06

1.4 (1) Master duplication alarm from slave
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Use two simulated masters.
* Start cyclic communication between a master and the slave under test.
* Verify in the master tool that the communication is running, and click
  the :guilabel:`Clear Number of Detected`.
* Use the additional simulated master tool, with the same setting for slave
  IP address and number of occupied slave stations as in the first master tool.
  Also enable the :guilabel:`No master station arbitration` setting.
  Click :guilabel:`Cyclic start`.
* Verify that a pop-up window :guilabel:`Master station duplication error` appears
  in the additional master tool.
* Use the :guilabel:`Detected disconnections` counter in the first master tool to
  verify that the slave under test has not lost communication with the master.
* REQ_CLS_CONFORMANCE_07

1.4 (2) Detect master disconnection
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use a master
* Verify in the slave that there is cyclic communication.
* Disconnect the master. In the simulated master this is done by clicking the
  :guilabel:`Cyclic Stop` button.
* Verify that the slave detects the master disconnect.
* REQ_CLS_CONFORMANCE_08

1.4 (3) Slave duplication detected by slave
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use a simulated master
* Enable the :guilabel:`Forced ON of transmission status` check box on the
  corresponding row in the :guilabel:`Setting for each slave station` table.
  This will simulate slave duplication errors. Click :guilabel:`Cyclic start`.
* Start the slave under test.
* Verify that the slave detects the slave duplication.
* Verify that the link status does not turn on in the simulated master test tool.
* REQ_CLS_CONFORMANCE_09

1.4 (4) Wrong number of occupied stations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use a simulated master
* In the simulated master tool use a number of occupied slave stations that is
  not supported by the current configuration of the slave under test.
* Start the cyclic communication.
* Verify that an error message is sent to the test tool.
  It is seen in the :guilabel:`Link Status` text box.
* REQ_CLS_CONFORMANCE_10

1.5 (1) Measure slave response time
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use a simulated master
* Start cyclic communication.
* Click the :guilabel:`Clear Measured Data` button in the master test tool.
* The response time of the slave is now measured. Wait until 1000 data points
  have been recorded.
* Use the button :guilabel:`Check data` in the popup window of the particular
  test case to copy the min, max and average response time values into the test results.
* REQ_CLS_CONFORMANCE_11

1.6 (1) Node search
^^^^^^^^^^^^^^^^^^^

* Use a simulated master
* Perform a node search by clicking the :guilabel:`Detect Connected Device` button
  in the simulated master tool.
* Use the button :guilabel:`Check data` in the popup window of the particular
  test case to copy the data received from the slave.
* Verify that the received values are correct.
* REQ_CLS_CONFORMANCE_12

1.6 (2) Set IP address
^^^^^^^^^^^^^^^^^^^^^^

* Use a simulated master
* Change the IP address of the slave by clicking the
  :guilabel:`IP address settings of connected devices` button in the simulated
  master tool. Manually enter the MAC address of the slave, the new IP address and
  the new netmask.
* Verify that a pop-up window "Normally processed" appears in the test tool.
* Verify that the IP address of the slave under test has changed.
* Use the button :guilabel:`Check data` in the popup window of the particular
  test case to copy the data received from the slave.
* Verify that the received values are correct.
* REQ_CLS_CONFORMANCE_13

2.1 CSP+ file format
^^^^^^^^^^^^^^^^^^^^

* Use the "CSP+ profile creation support tool" on a Windows PC.
* Load the CSP+ file, and use the menu File -> Check to verify the format
  of the CSP+ file.
* REQ_CLS_CONFORMANCE_14

2.2 CSP+ file contents
^^^^^^^^^^^^^^^^^^^^^^

* Use the "CSP+ profile creation support tool" on a Windows PC.
* Manually check that the fields are correct. See the test specification
  for a list of fields to be checked.
* REQ_CLS_CONFORMANCE_15

3.1 Aging test for 12 hours
^^^^^^^^^^^^^^^^^^^^^^^^^^^

* Use a simulated master
* Run cyclic communication with maximum number of occupied stations.
  Use a :guilabel:`Response waiting time` of 500 ms and
  :guilabel:`Number of consecutive timeouts` to 3.
* Click the :guilabel:`Clear Number of Detected`.
* Let the cyclic communication run for 12 hours.
* Verify that the :guilabel:`Detected disconnections` counter still is 0.
* REQ_CLS_CONFORMANCE_16
