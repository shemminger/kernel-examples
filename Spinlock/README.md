# Spinlock
This is a very simplistic module which prints messages the system log.
It spawns two kernel threads and each one outputs a line into the log
once per second.

## Building
To build and install the useless module:
```
$ cd Spinlock/
$ make
make -C /lib/modules/5.13.0-1023-azure/build  M=/home/azureuser/kernel-examples/Spinlock modules
make[1]: Entering directory '/usr/src/linux-headers-5.13.0-1023-azure'
  CC [M]  /home/azureuser/kernel-examples/Spinlock/spindemo.o
  MODPOST /home/azureuser/kernel-examples/Spinlock/Module.symvers
  CC [M]  /home/azureuser/kernel-examples/Spinlock/spindemo.mod.o
  LD [M]  /home/azureuser/kernel-examples/Spinlock/spindemo.ko
  BTF [M] /home/azureuser/kernel-examples/Spinlock/spindemo.ko

$ sudo insmod spindemo.ko
```

After several seconds remove the module to avoid overfilling disk and logs!
```
$ sudo rmmod spindemo
```

## Expected output
You should see output like this at the end of dmesg:
```
[  561.385570] spindemo: loading out-of-tree module taints kernel.
[  561.385605] spindemo: module verification failed: signature and/or required key missing - tainting kernel
[  561.385843] Major = 236 Minor = 0
[  561.385988] Kthread1 Created Successfully...
[  561.385993] In thread_function1 1
[  561.386043] Kthread2 Created Successfully...
[  561.386045] Inserted spin_demo
[  561.386048] spin_demo thread2 2
[  562.402501] spin_demo thread2 3
[  562.402548] spin_demo thread1 4
[  563.426537] spin_demo thread2 5
[  563.426582] spin_demo thread1 6
[  564.450452] spin_demo thread1 7
[  564.450514] spin_demo thread2 8
[  565.474509] spin_demo thread2 9
[  565.474519] spin_demo thread1 10
[  566.498496] spin_demo thread1 11
```
