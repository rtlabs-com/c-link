Setting loglevels
-----------------

This project can be built with extensive logging support. There are
several levels of logging to choose from (``DEBUG``, ``INFO``,
``WARNING``, ``ERROR`` and ``FATAL``).

Logging messages are usually printed to the standard output, which on
an embedded system is often a relatively slow serial port. Printing
excessive amounts of logging messages can disturb the timing of the
application to the point where normal operation is affected. Therefore
the default loglevel, and the one recommended for production, is
``FATAL``.

The log level is selected by setting the CMake option ``LOG_LEVEL``::

  cmake -B build -DLOG_LEVEL=DEBUG

You can also choose to disable or enable logging for individual
modules::

  cmake -B build -DCL_SLMP_LOG=OFF

.. tip:: Run::

         ccmake build/

   to see a list of all available options.

Logging is enabled for all modules by default.
