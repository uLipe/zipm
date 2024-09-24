.. _zephyr_inter_processor_message:

ZIPM, a Zephyr RTOS friendly inter processor messaging module
#############################################################


Adding ZIPM to your project via ``west``
****************************************

The recommended way is to use ``west`` to initialize this repository directly and
all its dependencies:

.. code-block:: console

   $ west init -m https://github.com/uLipe/zipm 
   $ west update

Alternatively you can add a local copy of this module by adding the following sections
to ``zephyr/west.yml``:

1. In the ``manifest/remotes`` section add:

.. code-block::

   remotes:
     - name: uLipe
       url-base: https://github.com/uLipe

2. In the ``manifest/projects`` section add:

.. code-block::

   - name: zipm
     remote: uLipe
     path: modules/lib/zipm
     revision: main

3. Save the file, and run ``west update`` from the project root to retrieve the
latest version of the library from Github, or whatever ``revision`` was

Getting started with ZIPM
*************************
Please refer the ``samples`` folder inside of this module to see some basic use
cases of ZIPM, they were built for usage without hardware for quick getting started.

Get in touch!
*************
If you have some questions, wants to contribute please contact me at:
ryukokki.felipe@gmail.com