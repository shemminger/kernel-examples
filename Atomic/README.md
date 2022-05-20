# Atomic demo
This is a creates a character device /dev/demo_atomic.
The device implements a counter that gets incremented by a work queue
once per second.

The counter can be read by reading the device 
and set by writing to the device.

## Building
To build and install this module use make and insmod.

## Sample
Simple interaction with the device.
Since device is created with root only permission need sudo here.

```
# cat /dev/demo_rwlock
27
# echo 33 >/dev/demo_rwlock
# cat /dev/demo_rwlock
39
# cat /dev/demo_rwlock
41
# rmmod rwdemo

```
