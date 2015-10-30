#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>

struct request {
  int mode;
  int id;
  int uid;
  int rw_perm;
};

struct alloc_response {
  int err;
  int offset;
  int size;
};

struct error_response {
  int err;
};

// Some globals
int REQUEST_LEN = sizeof(request);
int socketfd;
void* baseptr;
int shared_dev_fd;
int total_size;

// Opens a TCP connection with the host daemon
int shm_open_conn(char* daemon_ip, char* daemon_port) {
  struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.
  struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.

  // The MAN page of getaddrinfo() states "All  the other fields in the structure pointed
  // to by hints must contain either 0 or a null pointer, as appropriate." When a struct
  // is created in C++, it will be given a block of memory. This memory is not necessary
  // empty. Therefor we use the memset function to make sure all fields are NULL.
  memset(&host_info, 0, sizeof host_info);

  host_info.ai_family = AF_INET;       // IP version not specified. Can be both.
  host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.

  // Now fill up the linked list of host_info structs with google's address information.

  if (getaddrinfo(daemon_ip, daemon_port, &host_info, &host_info_list)) {
    perror("SHEET");
    fflush(stderr);
    return -1;
  }

  socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
  if (socketfd < 0) {
    perror("SHEEET");
    fflush(stderr);
    return -1;
  }

  if (connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen)) {
    perror("SHEEET");
    fflush(stderr);
    return -1;
  }
  return 0;
}

// Inits the library
//  Opens a conn to the host daemon
//  mmaps the entire shared memory region
//  makes this memory unaccessible by mprotecting it
int shm_init(char* daemon_ip, char* daemon_port) {
  shm_open_conn(daemon_ip, daemon_port);
  request r;
  r.mode = 0;
  int sent_bytes = 0;
  while(sent_bytes < REQUEST_LEN) {
    sent_bytes += send(socketfd, (&r), REQUEST_LEN, 0);
  }

  alloc_response ars;
  recv(socketfd, (&ars), sizeof(alloc_response), 0);

  shared_dev_fd = open("/dev/ivshmem", O_RDWR);
  if (shared_dev_fd < 0) {
    printf("Failed to open the ivshmem /dev node!\n");
    return (-1);
  } else {
    printf("Opened the ivshmem /dev node!\n");
  }

  baseptr = mmap(0, ars.size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_dev_fd, 0);
  if (baseptr == MAP_FAILED) {
    printf("Failed to mmap /dev/ivshmem!\n");
    return (-2);
  } else {
    printf("MMap on /dev/ivshmem successfull!\n");
  }

  if (mprotect(baseptr, ars.size, PROT_NONE)) {
    printf("Failed to mmap /dev/ivshmem!\n");
    return (-2);
  } else {
    printf("MMap on /dev/ivshmem successfull!\n");
  }

  total_size = ars.size;
}

// Closes the socket conn to the host daemon
int shm_close_conn() {
  int errno;
  return close(socketfd);
}

// Contacts the daemon for allocation
// mprotects the allocated area for access
int shm_alloc(int shmid, int uid, char rw_perms, int write_exclusive, void** ptr, int *size) {
  request r;
  r.mode = 1;
  r.id = shmid;
  r.uid = uid;
  if (rw_perms == 'w' && write_exclusive)
    r.rw_perm = 1;
  else if (rw_perms == 'w')
    r.rw_perm = 2;
  else r.rw_perm = 3;

  int sent_bytes = 0;
  while(sent_bytes < REQUEST_LEN) {
    sent_bytes += send(socketfd, (&r), REQUEST_LEN, 0);
  }
  alloc_response ars;
  recv(socketfd, (&ars), sizeof(alloc_response), 0);

  *ptr = (void*)ars.offset;
  if(mprotect(ptr, ars.size, PROT_READ | PROT_WRITE | PROT_EXEC)) {
    perror("");
    return -1;
  }
  *size = ars.size;
  return ars.err;
}

// Contacts the daemon for deallocation
// mprotects the deallocated area for no access
int shm_dealloc(int shmid, int uid) {
  request r;
  r.mode = 2;
  r.id = shmid;
  r.uid = uid;

  int sent_bytes = 0;
  while(sent_bytes < REQUEST_LEN)
    sent_bytes += send(socketfd, (&r), REQUEST_LEN, 0);

  alloc_response ars;
  recv(socketfd, (&ars), sizeof(alloc_response), 0);

  if(mprotect((void*)ars.offset, ars.size, PROT_NONE)) {
    perror("");
    return -1;
  }

  return ars.err;
}

// Conditional variable methods
int shm_create_cv(int id, int uid) {
  request r;
  r.mode = 3;
  r.id = id;
  r.uid = uid;

  int sent_bytes = 0;
  while(sent_bytes < REQUEST_LEN)
    sent_bytes += send(socketfd, (&r), REQUEST_LEN, 0);

  error_response ers;
  recv(socketfd, (&ers), sizeof(alloc_response), 0);

  return ers.err;
}

int shm_delete_cv(int id, int uid) {
  request r;
  r.mode = 8;
  r.id = id;
  r.uid = uid;

  int sent_bytes = 0;
  while(sent_bytes < REQUEST_LEN)
    sent_bytes += send(socketfd, (&r), REQUEST_LEN, 0);

  error_response ers;
  recv(socketfd, (&ers), sizeof(alloc_response), 0);

  return ers.err;
}

int shm_acquire_cv(int id, int uid) {
  request r;
  r.mode = 4;
  r.id = id;
  r.uid = uid;

  int sent_bytes = 0;
  while(sent_bytes < REQUEST_LEN)
    sent_bytes += send(socketfd, (&r), REQUEST_LEN, 0);

  error_response ers;
  recv(socketfd, (&ers), sizeof(alloc_response), 0);

  return ers.err;
}

int shm_release_cv(int id, int uid) {
  request r;
  r.mode = 5;
  r.id = id;
  r.uid = uid;

  int sent_bytes = 0;
  while(sent_bytes < REQUEST_LEN)
    sent_bytes += send(socketfd, (&r), REQUEST_LEN, 0);

  error_response ers;
  recv(socketfd, (&ers), sizeof(alloc_response), 0);

  return ers.err;
}

int shm_notify_cv(int id, int uid) {
  request r;
  r.mode = 7;
  r.id = id;
  r.uid = uid;

  int sent_bytes = 0;
  while(sent_bytes < REQUEST_LEN)
    sent_bytes += send(socketfd, (&r), REQUEST_LEN, 0);

  error_response ers;
  recv(socketfd, (&ers), sizeof(alloc_response), 0);

  return ers.err;
}

int shm_wait_cv(int id, int uid) {
  request r;
  r.mode = 6;
  r.id = id;
  r.uid = uid;

  int sent_bytes = 0;
  while(sent_bytes < REQUEST_LEN)
    sent_bytes += send(socketfd, (&r), REQUEST_LEN, 0);

  error_response ers;
  recv(socketfd, (&ers), sizeof(alloc_response), 0);

  return ers.err;
}

// Closes daemon connection
// munmaps the sghared memory region
int shm_cleanup() {
  shm_close_conn();
  munmap(baseptr, total_size);
}
