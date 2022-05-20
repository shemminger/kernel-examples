# Per CPU Counter
This module creates a character device /dev/totaler.
The device implements a counter that can be read and written.

When the counter is written, it adds the increment value to the total.
When the counter is read, it returns the current total.

## Building
To build and install this module use make and insmod.

## Sample
Simple interaction with the device.
