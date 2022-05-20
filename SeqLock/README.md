# Reader Writer Lock
This module creates a character device /dev/demo_seqlock.
The device implements a 128 bit counter that can be read and written.

## Building
To build and install this module use make and insmod.

## Sample
Simple interaction with the device.[^1]

```
```
[^1] since device is created with root only permission need sudo here.
