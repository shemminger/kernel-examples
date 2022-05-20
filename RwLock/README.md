# Reader Writer Lock
This is a very simplistic module creates a character device /dev/demo_rwlock.
The device implements a counter that gets incremented by a work queue
once per second.

## Building
To build and install this module use make and insmod.

## Sample
Simple interaction with the device.[^1]

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
[^1] since device is created with root only permission need sudo here.
