# RCU counter demo
This module creates a character device /dev/demo_rcu.
The device implements a simple string storage facility

When the device is written, the string is saved.
When the device is read, it returns the saved value.

## Building
To build and install this module use make and insmod.

## Sample
Simple interaction with the device.

```
# insmod rcu_demo.ko
# echo 'may the force be with you' >/dev/rcu_demo
# cat /dev/rcu_demo
may the force be with you
# echo 'all people are created equal' >/dev/rcu_demo
# cat /dev/rcu_demo
all people are created equal
# rmmod rcu_demo
```
