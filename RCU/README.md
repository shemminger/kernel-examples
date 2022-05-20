# RCU counter demo
This module creates a character device /dev/demo_rcu.
The device implements a counter that can be read and written.

When the counter is read, it returns the current value.
When the counter is written, it overwrites the last value.

## Building
To build and install this module use make and insmod.

## Sample
Simple interaction with the device.
