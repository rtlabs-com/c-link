# CC-Link IE Field Basic stack (master and slave)

This repository contains an evaluation version of **c-link**, a
CC-Link IE Field Basic stack. This evaluation version is built for
x86_64 Linux, GLIBC 2.31 and runs on e.g. Ubuntu 20.04 or later.

This version of c-link can be used for evaluation purposes
only. Contact sales@rt-labs.com if you intend to use this stack in a
product or if you need assistance during evaluation. The commercial
version of this stack is supplied with full sources.

Key features:

- Both master and slave are implemented
- Easy to use
  - Extensive documentation and instructions on how to get started.
- Portable
  - Written in C.
  - Linux, RTOS, Windows or bare metal.
  - Sources for supported port layers provided.
- Node search
- Remote setting of slave IP address (configurable to allow or not)

It is easy to use and has a small footprint. It
is especially well suited for embedded systems where resources are
limited and efficiency is crucial.

Limitations or not yet implemented:

- Writing and reading of startup parameters is not supported.

## Slave stack

- Each slave device can support up to 16 occupied stations
- Slave can disable cyclic communication

## Master stack

Master supports:

- Multiple groups (max 64)
- Variable or constant link time
- Up to 64 occupied slave stations (max 16 per group)
- Performing node search
- Setting IP address of slave devices
- Slaves occupying up to 16 stations

## Web resources

- Documentation: [https://rt-labs.com/docs/c-link](https://rt-labs.com/docs/c-link)
- RT-Labs (stack integration, certification services and training): [https://rt-labs.com](https://rt-labs.com)

## Getting started

See the [getting started](docs/tutorial/quickstart.rst) guide. The
complete documentation is [here](https://rt-labs.com/docs/c-link).
