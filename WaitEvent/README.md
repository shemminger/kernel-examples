# Blocking reader channel
This module creates a character device /dev/demo_wait.
The device implements a counter that can be read and written.

When the counter is read, it blocks until the value is changed
by a writer. The value set by the writer is then returned.

## Building
To build and install this module use make and insmod.

## Sample
Simple interaction with the device.
Start a cat in background that reads the device, and it blocks.
Put some data into device and the cat wakes up and prints output
and then it goes back to sleep.

```
# insmod wait_demo.ko
# cat /dev/demo_wait &
[1] 16616
# echo 33 >/dev/demo_wait
33
```
