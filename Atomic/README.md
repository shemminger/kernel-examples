# Atomic demo
This is a creates a character device /dev/demo_atomic.
The device implements a counter that gets incremented by a work queue
once per second as long as the device is open.

The counter can be read by reading the device 
and set by writing to the device.

## Building
To build and install this module use make and insmod.

## Sample
Simple interaction with the device.


```
# insmod atomic_demo.ko
# sleep 30 < /dev/demo_atomic
# cat /dev/demo_atomic
29
# echo 11 >/dev/demo_atomic
# sleep 4 < /dev/demo_atomic
# cat /dev/demo_atomic
14
# rmmod atomic_demo
```
