Building documentation
======================
It is recommended to use a Python virtual environment to build the documentation.

Run these commands (adapt to your needs)::

  python3 -m venv myvenv
  source myvenv/bin/activate
  pip3 install -r docs/requirements.txt

Build Sphinx HTML output using::

  cmake --preset docs
  cmake --build --preset docs

The generated docs will be available at :file:`build.docs/docs/sphinx/index.html`.

To generate the documentation in PDF format::

  cmake --build --preset docs --target sphinx-pdf

Check spelling and links using::

  cmake --build --preset docs --target sphinx-spelling
  cmake --build --preset docs --target codespell
  cmake --build --preset docs --target sphinx-linkcheck

Create requirements reports
---------------------------
The commands above will automatically create the requirements reports (in .rst format)
that are included in the HTML and PDF output.

It is also possible to trigger the creation of those reports manually:

.. command-output:: python3 parse_requirements.py -h


Troubleshooting
---------------

Before running Sphinx, help texts for the sample apps should be available in the files
:file:`docs/_generated/helptext_sampleslave.txt` and :file:`docs/_generated/helptext_samplemaster.txt`,
which can be generated with the commands::

  build/cl_sample_slave -h > docs/_generated/helptext_sampleslave.txt
  build/cl_sample_master -h > docs/_generated/helptext_samplemaster.txt

These commands are executed automatically when starting the documentation generation via cmake.
