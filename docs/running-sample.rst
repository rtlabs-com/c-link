Running the sample applications
===============================

In this tutorial you will learn how to run and use the sample applications.

.. tip:: A suitable set-up for trying out the sample applications is to run
   the master application on a Windows PC and the slave application on a
   Raspberry Pi, or vice versa. Or any other platform combination, actually.

.. note:: The master and the slave must be on the same network. Check that the
   slave is accessible from the master, e.g. by using ping. It may be necessary
   to change IP addresses to make connection possible.

A simple slave application for use with a master (e.g. a PLC) is found in the
:file:`sample_apps` directory. The state of a physical button is sent from
the sample application to the master. The master controls a physical LED via the
sample application. When pressing the button the master will stop or restart the
flashing of the LED.

The sample application will print "forty-two" when the RWw00 changes to 0x002A.

The PLC example program listing is found in :ref:`using-gxworks`.

Another physical button controls sending error messages from the sample
application.

It is possible to use regular files to simulate the physical buttons when
running the sample application on Linux or Windows.

Sample applications on Windows
------------------------------
The sample applications, when built as described in :ref:`Building for Windows`,
will end up in the :file:`build.x64\\Release` directory.
Note that the path will depend on what type of build you do::

    cd build.x64\Release

Running the master sample application on Windows
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To show the help information for the master, including the available interfaces, use::

       .\cl_sample_master.exe --help

#. Run the master application with the relevant network interface name and
   slave IP address, for example::

       .\cl_sample_master.exe -s 192.168.0.202

Press :kbd:`Control-c` to quit the application.

Running the slave sample application on Windows
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To show the help information for the slave, use::

    .\cl_sample_slave.exe --help

#. Run the slave application with the relevant number of occupied slave stations,
   for example::

       .\cl_sample_slave.exe -n 2

Press :kbd:`Control-c` to quit the application.

If no incoming Ethernet frames are received by the sample application (but
appears in Wireshark on the same machine),
the frames might have been dropped by the Windows firewall.
Try this by connecting your device to a safe network, and temporarily disable
the Windows firewall. If this solves the issue,
you need to add an inbound rule allowing UDP traffic to ports 61450 and 61451.
You might have to restart your Windows machine for this to have an effect.

.. tip:: It is recommended to run the conformance test tool on another PC
   when testing the Windows c-link sample slave application.

Sample applications on Linux (for example Raspberry Pi)
-------------------------------------------------------
The sample applications, when built as described in :ref:`Building for Linux`,
will end up in the :file:`build` directory::

    cd build

Running the master sample application on Linux (for example Raspberry Pi)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To show the help information for the master, use::

    ./cl_sample_master --help

The output will be:

.. literalinclude:: _generated/helptext_samplemaster.txt

#. Run the master application with the relevant slave IP address, for example::

       ./cl_sample_master -s 192.168.0.202

Press :kbd:`Control-c` to quit the application.

Run the slave sample application on Linux (for example Raspberry Pi)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To show the help information for the slave, use::

    ./cl_sample_slave --help

The output will be:

.. literalinclude:: _generated/helptext_sampleslave.txt

#. Run the slave application::

       ./cl_sample_slave

Press :kbd:`Control-c` to quit the application.
