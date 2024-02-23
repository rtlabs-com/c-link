Development environment
=======================

The following tools are useful for development:

 - Clang and utilities (clang-format, clang-tidy)
 - Python

To install on Debian based Linux distributions::

  sudo apt install \
    arp-scan \
    build-essential \
    clang  \
    clang-format \
    clang-format  \
    clang-tidy  \
    cmake \
    cmake-curses-gui \
    doxygen \
    gcovr \
    git \
    graphviz \
    llvm-14 \
    python3 \
    python3-pip

For PDF generation additional packages are required (see the section
about Latexbuilder on https://www.sphinx-doc.org/en/master/usage/builders/index.html)::

  sudo apt install \
    texlive-latex-recommended \
    texlive-fonts-recommended \
    tex-gyre \
    texlive-latex-extra \
    latexmk

If you need to install more recent versions of clang-tidy and clang-format,
follow the instructions on https://apt.llvm.org/

Using clang-format for source code formatting::

  clang-format-14 -n --Werror \
    fuzz/*.c \
    include/*.h \
    sample_apps/*.c \
    sample_apps/*.h \
    src/**/*.c \
    src/**/*.h \
    src/ports/**/*.c \
    src/ports/**/*.h \
    test/*.cpp \
    test/*.h

Create a ZIP archive of the source code (in the project root directory)::

   git archive -o clink_$(git describe --dirty --always --tags).zip master


Environment for building documentation
--------------------------------------

The documentation uses Sphinx, which in turn requires Python. We
recommend using a project-specific `virtual environment
<https://docs.python.org/3/tutorial/venv.html>`_::

  python3 -m venv env
  source env/bin/activate
  pip install -r docs/python_dependencies.txt

Remember to run::

  source env/bin/activate

whenever you want to start the virtual environment.
