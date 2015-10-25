# ivshmem_cloud

This is a wrapper around QEMU-KVM's IVSHMEM, an implementation for shared memory between guests running on QEMU-KVM.

This wrapper aims to provide a library so that it is convenient for processes in guests to use for inter-processes communication over TCP/UDP sockets. It also will provide methods for synchronisation needed in the use of this shared memory

