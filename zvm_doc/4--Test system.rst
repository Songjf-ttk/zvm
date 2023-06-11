ZVM Command
===========

创建虚拟机
--------------

.. code:: shell

   ------------------------------
   zvm new -t linux    # new a linux vm with vmid that allocated by system. 

   zvm new -t zephyr   # new a zephyr vm with vmid that allocated by system. 
   ------------------------------

运行虚拟机
--------------

.. code:: shell

   ------------------------------
   zvm run -n 0        # run the vm with vm\'s vmid equal to 0 

   zvm run -n 1        # run the vm with vm\'s vmid equal to 1 
   ------------------------------

取消虚拟机
----------------

.. code:: shell

   ------------------------------
   zvm pause -n 0      # pause the vm with vm\'s vmid equal to 0 

   zvm pause -n 1      # pause the vm with vm\'s vmid equal to 1 
   ------------------------------

打印虚拟机
---------------

.. code:: shell

   ------------------------------
   zvm info            # list vms. 
   ------------------------------

删除虚拟机
-----------------

.. code:: shell
   
   ------------------------------
   zvm delete -n 0     # delete the vm with vm\'s vmid equal to 0 

   zvm delete -n 1     # delete the vm with vm\'s vmid equal to 1 
   ------------------------------

