Using the reference stack
=========================
A reference CCIEFB stack is available, and it is useful for learning the protocol.

See:

* https://software-dl.ti.com/processor-sdk-linux/esd/docs/latest/linux/Industrial_Protocols_CCLINK.html
* https://software-dl.ti.com/processor-sdk-rtos/esd/docs/latest/rtos/index_Foundational_Components.html#cclink
* https://git.ti.com/cgit/processor-sdk/cclink/tree/


Test running the slave reference implementation
-----------------------------------------------
#. Download the source code::

      git clone https://git.ti.com/git/processor-sdk/cclink.git

#. Compile the code (ignore any warnings)::

      cd cclink/CCIEF-BASIC_Slave/build/linux
      make

#. Use these values in the :file:`SlaveParameter.csv` settings file::

      ,,
      CCIEF-BASIC Slave Sample Parameter,,
      ,,
      ID,DATA,COMMENT
      1,0,IP Address
      2,0,Subnet Mask
      3,0,Default Gateway IP Address
      4,1,Occupied Station Number
      5,0,Cyclic Response Wait Time

#. Adjust IP settings and start the slave::

      sudo ifconfig eth0 192.168.0.201 netmask 255.255.255.0 up
      sudo ./Slave_sample ../../sample/SlaveParameter.csv

#. Select the network interface when prompted.

#. Start your PLC with the correct slave IP address setting. Cyclic data
   transmission will appear.


Test running the master reference implementation
------------------------------------------------

#. Compile the code (ignore any warnings)::

      cd CCIEF-BASIC_Master/build/linux
      make

#. Modify the slave IP address setting by using::

      nano ../../sample/MasterParameter.csv

#. Use for example these settings in the file::

      ,,
      CCIEF-BASIC Master Sample Parameter,,
      ,,
      Group,,
      ID,DATA,COMMENT
      1,1,Total number of group
      2,1,Number of group
      3,100,Group1 Cyclic transmission timeout
      4,3,Group1 Count of cyclic transmission timeout
      5,0,Group1 Constant link scan time
      ,,
      Slave,,
      ID,DATA,COMMENT
      1,1,Total number of slave
      2,192.168.0.201,Slave1 IP address
      3,1,Slave1 Number of occupied stations
      4,1,Slave1 Number of group

#. To increase the link scan time (to for example 50 ms), change the corresponding
   line to::

      5,50,Group1 Constant link scan time

#. Start some CCIEFB slave at the correct IP address.

#. Adjust the IP setting for the master device and start the communication::

      sudo ifconfig eth0 192.168.0.200 netmask 255.255.255.0 up
      ./Master_sample ../../sample/MasterParameter.csv

#. Select the network interface when prompted. The cyclic data transmission will
   start immediately.

   The command "Stop the application" will set all transmitted data to ``0``.

#. Use the command "Stop the cyclic" to stop transmission.
