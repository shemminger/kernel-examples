# kernel-examples
Locking and concurrency in Linux kernel examples

This repository has small sample kernel driver examples.
Originally done for an internal class.

# Prerequisite
Building these examples assume you already have all the necessary
packages to build a kernel module and headers for your distribution.

# Building module
Each subdirectory contains a different small example.

- [Spin locks](Spinlock/README.md)
- [Reader Writer locks](RwLock/README.md)
- [Sequence locks](SeqLock/README.md)
- [Wait Event](WaitEvent/README.md)
- [Atomic](Atomic/README.md)
- [Per CPU Counter](PerCpuCounter/README.md)
- [Read Copy Update](RCU/README.md)

