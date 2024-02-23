.. _using-gxworks:

Using GX Works3
===============
GX Works is the tool for programming Mitsubishi PLCs. Use GX Works3 to program
PLCs in the iQ-F and iQ-R series. Other Mitsubishi PLC series use GX Works2.

See the :ref:`Hardware for testing` section for more information on the hardware.


Installation
------------
The software is delivered on a USB memory stick.

#. Right-click on the :file:`autorun` file and select :guilabel:`Run as administrator`.

#. Select :guilabel:`GX Works3` on the menu screen.

#. Enter your name and the product key.

#. Select installation locations.

#. Reboot the computer when asked.


Creating a project
------------------

#. Start GX Works3.

#. Use the menu :menuselection:`Project --> New`.

#. Select the PLC series (for example ``FX5CPU``) and the
   PLC type (for example ``FX5U``).

#. Select programming language ("Ladder" or "ST").

#. Click :guilabel:`OK`.

#. In the left-side menu of the main window, click :menuselection:`Project --> "Module configuration"`.

#. On the CPU block, right-click and select :menuselection:`"Change CPU Model Name"`. Use the
   exact variant name of your PLC.

#. Click :guilabel:`OK`.

#. In the right-side panel, set the PLC IP address to ``192.168.3.250``.

#. In the left side menu of the main window, click :menuselection:`Parameter --> FX5UCPU --> "CPU
   Parameter"`.

#. Use "Operation related settings" to enable "Remote reset".

#. Click :guilabel:`Check` and :guilabel:`Apply`.


Entering a PLC program to test the digital inputs and outputs on the PLC
------------------------------------------------------------------------
Use ladder logic as the programming language. We will control the ``Y0``
output by the ``X0`` input.

#. In the left side menu, use :menuselection:`Program --> Scan --> MAIN --> ProgPou --> ProgramBody`.

#. On the top ladder diagram line, insert an input by clicking the icon "Open
   contact". Enter ``X0`` in the pop-up box.

#. On the same line insert an output by clicking the icon "Coil". Enter ``Y0``
   in the pop-up box.

#. Compile the program by using the menu Convert.

#. To modify the program later on, make sure that you are in "Write mode". Click
   the small :guilabel:`Write mode` icon.


Connecting your laptop to the PLC
---------------------------------
#. Make sure your PLC is powered on, and connected to your laptop via an Ethernet
   cable.

#. Use the menu :menuselection:`Online --> "Current Connection Destination"`.

#. Select :guilabel:`Direct Coupled Setting` and :guilabel:`Ethernet`.

#. Choose the correct Ethernet adapter.

#. Click the :guilabel:`Communication Test` button. Click :guilabel:`OK`.

#. To download the program to the PLC, use the menu :menuselection:`Online --> "Write to PLC"`.

#. Click the :guilabel:`Write` tab, the :guilabel:`Select All` button, and then
   the :guilabel:`Execute` button.

#. Once the programming is done, move the run-switch on the front of the PLC first
   to :guilabel:`Reset` and then to :guilabel:`Run`.

#. Verify that the :guilabel:`P.RUN` LED is on and the :guilabel:`ERR` LED is off.

#. Make sure that the program behaves as intended.


Enabling monitoring of data in the laptop program
-------------------------------------------------

#. Use the menu :menuselection:`Online --> Monitor --> "Monitor mode"`.

#. Then :menuselection:`Online --> Monitor --> "Start Monitoring (All Windows)."`

#. (Use the menu Online > Monitor > "Device/Buffer Memory Batch Monitor".
   Enter device name ``x100``. Similarly use another monitor for ``y100``.)?


Entering a structured text program
----------------------------------
The input ``RX0``=``X100`` will control whether the output ``RY16``=``Y120``
is flashing or not.

#. In the left side menu, use :menuselection:`Program --> Scan --> MAIN --> ProgPou --> "Local label"`.

#. Enter these labels:

   +--------------------+--------------------------+
   | Label Name         | Data Type                |
   +====================+==========================+
   | oscillator_cycles  | Word [Unsigned] [16 bit] |
   +--------------------+--------------------------+
   | oscillator_state   | Bit                      |
   +--------------------+--------------------------+
   | flashing           | Bit                      |
   +--------------------+--------------------------+
   | input_button       | Bit                      |
   +--------------------+--------------------------+
   | output_LED         | Bit                      |
   +--------------------+--------------------------+
   | previous_button    | Bit                      |
   +--------------------+--------------------------+
   | integer_from_slave | Word [Unsigned] [16 bit] |
   +--------------------+--------------------------+
   | integer_to_slave   | Word [Unsigned] [16 bit] |
   +--------------------+--------------------------+

#. In the left side menu, use :menuselection:`Program --> Scan --> MAIN --> ProgPou --> ProgramBody`.

#. Enter this program::

      // Implement flashing on/off
      oscillator_cycles := oscillator_cycles + 1;
      IF oscillator_cycles > 3000 THEN
         oscillator_cycles := 0;
         oscillator_state := NOT oscillator_state;
      END_IF;

      // Read inputs
      input_button := X100;
      integer_from_slave := W0;

      // Implement logic
      IF input_button = TRUE THEN
         IF previous_button = FALSE THEN
            flashing := NOT flashing;
         END_IF;
         output_LED := TRUE;
      ELSIF flashing = TRUE THEN
         output_LED := oscillator_state;
      ELSE
         output_LED := FALSE;
      END_IF;
      previous_button := input_button;
      integer_to_slave := 2 * integer_from_slave;

      // Set outputs
      Y120 := output_LED;
      W100 := integer_to_slave;


Registering a slave profile (in CSP+ format)
--------------------------------------------
#. Make sure no project is open.

#. Use the menu :menuselection:`Tool --> "Profile Management" --> Register`.

#. Select the file (in ``.cspp`` or ``.zip`` format) and click :guilabel:`Register`
   to load the file.

#. To view or delete installed profiles, use the menu
   :menuselection:`Tool --> "Profile Management" --> Delete`.


Adjusting CC-Link IE Field Basic settings
-----------------------------------------
#. Use the left side menu :menuselection:`Parameter --> FX5CPU --> "Module parameters" --> "Ethernet Port"`.
   Use these values:

   * IP Address: ``192.168.3.250``
   * Subnet Mask: ``255.255.255.0``
   * Default Gateway: ``192.168.3.254``
   * Communication Data Code: Binary
   * CC-Link IEF Basic: Use

#. Click :guilabel:`Network Configuration Settings`.

#. In the new window, drag the relevant module from the right-side module
   list to image of the network in the lower part of the window.

#. In the table adjust the slave settings to "number of occupied stations" = 1,
   IP address ``192.168.3.1`` and the subnet mask ``255.255.255.0``.

#. Click :guilabel:`Close with Reflecting the Setting`.

#. On the Module Configuration page, click :guilabel:`Refresh Settings`.
   It sets the mapping of the remote inputs/outputs to the names used in the
   PLC program. Use these values:

   * RX: Device Name X, start 100
   * RY: Device Name Y, start 100
   * RWr: Device Name W, start 0
   * RWw: Device Name W, start 100

#. Click the :guilabel:`Check` button, and then click :guilabel:`Apply`.


Running node scan
-----------------
To be able to perform a node search, the PC running GX Works3 must be connected
to the PLC.

#. To trigger a node scan, use the left side menu
   :menuselection:`Parameter --> FX5CPU --> "Module parameters" --> "Ethernet Port"`.

#. In the :guilabel:`Basic Settings` area, use the :guilabel:`CC-Link IEF Basic Settings` item.

#. Click the :guilabel:`Network configuration settings`, and in the new window click
   the button :guilabel:`Detect Now`. Accept in the popup window.

   For GX Works3 to find the correct profile for a slave, the vendor code,
   model code and equipment version values in the CSP+ must all be the same as
   in the node search response.


Setting slave IP address
------------------------
In order to remotely set the IP address of a slave, it must first have been
detected and its CSP+ file must be loaded. The CSP+ file must also allow
remote setting of IP-address.

#. Do a node search by clicking the :guilabel:`Detect Now` button. The slave will appear
   in the network diagram and in the main table on the side.

#. Adjust the network settings in the main table.

#. Right-click on the icon of the slave station and select
   :menuselection:`Online --> "Communication Setting Reflection of Slave Station"`.


Adjusting PLC digital input response time
-----------------------------------------
To limit the effect of button bouncing, use a long response time for
any digital input (directly on the PLC) connected to a physical button.

#. Use the left side menu :menuselection:`Parameter --> FX5UCPU --> "Module parameter" -->"Input response
   time"`.

#. For the relevant input channel, select the time ``70ms``.

#. Click :guilabel:`Check` and then :guilabel:`Apply`.
