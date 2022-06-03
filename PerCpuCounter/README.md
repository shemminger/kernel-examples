# Per CPU Counter
This module creates a character device /dev/totaler.
The device implements a counter that can be read and written.

When the counter is written, it adds the increment value to the total.
When the counter is read, it returns the current total.

## Building
To build and install this module use make and insmod.

```
$ cd PerCpuCounter/
$ make
make -C /lib/modules/5.13.0-1023-azure/build  M=/home/azureuser/kernel-examples/PerCpuCounter modules
make[1]: Entering directory '/usr/src/linux-headers-5.13.0-1023-azure'
  CC [M]  /home/azureuser/kernel-examples/Spinlock/totaler.o
  MODPOST /home/azureuser/kernel-examples/Spinlock/Module.symvers
  CC [M]  /home/azureuser/kernel-examples/Spinlock/totaler.mod.o
  LD [M]  /home/azureuser/kernel-examples/Spinlock/totaler.ko
  BTF [M] /home/azureuser/kernel-examples/Spinlock/totaler.ko

$ sudo insmod totaler.ko
```
## Sample
Since device is created with root only permission need sudo here.
Simple interaction with the device; set the counter twice (on different CPU)
and see the total.


```
# cat /dev/totaler
0
# taskset 1 echo 2 >/dev/totaler
# taskset 2 echo 2 >/dev/totaler
# cat /dev/totaler
4
# rmmod totaler
```

