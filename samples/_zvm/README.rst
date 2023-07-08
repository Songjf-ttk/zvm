.. _hypervisor:

Hypervisor
###########

Overview
********

ver1.0
The first step is to make a shell to get command, such as 
zvisor. This step mainly create by system build-in module,
which is at subsys/shell.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: hypervisor
   :host-os: Linux
   :board: qemu_cortex_max
   :goals: run
   :compact:

To build for another board, change "qemu_cortex_max" above to that board's name.

Sample Output
=============

.. code-block:: console

    [uart]: <shell>

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
