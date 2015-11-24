# ivshmem_cloud

This is a wrapper around QEMU-KVM's IVSHMEM, an implementation for shared memory between guests running on QEMU-KVM.

This wrapper aims to provide a library so that it is convenient for processes in guests to use for inter-processes communication over TCP/UDP sockets. It also will provide methods for synchronisation needed in the use of this shared memory

# Usage
Follow the following steps to set up the framework
1. Run hostdaemon.py on the host
2. Start a QEMU guest using the CLI options ```-device ivshmem,size=<size in MB>,[shm=<shm_name>]```
3. Insert the kernel module ```ivshmem_driver.ko``` in the guest


One now can use the framework for communication. Look at the tests folder (```/guest/tests/```) for more exmaples
