# Blocking reader channel
This module creates a character device /dev/demo_wait.
The device implements a counter that can be read and written.

When the counter is read, it blocks until the value is changed
by a writer. The value set by the writer is then returned.

## Building
To build and install this module use make and insmod.

## Sample
Simple interaction with the device.
