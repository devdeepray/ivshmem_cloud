#include "guestlib.hh"
#include <cerrno>

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
    perror("");
    return errno;
  }

  socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
  if (socketfd < 0) {
    perror("");
    return errno;
  }
}

int shm_close_conn() {
  int errno;
  return close(socketfd);
}

int shm_alloc(int shmid, int uid, char rw_perms, int write_exclusive, int *offset, int *size) {
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
  while(sent_bytes < REQUEST_LEN)
    sent_bytes += send(socketfd, (&r), REQUEST_LEN, 0);

  alloc_response ars;
  recv(socketfd, (&ars), sizeof(alloc_response), 0);

  *offset = ars.offset;
  *size = ars.size;
  return ars.err;
}

int shm_dealloc(int shmid, int uid) {
  request r;
  r.mode = 2;
  r.id = shmid;
  r.uid = uid;

  int sent_bytes = 0;
  while(sent_bytes < REQUEST_LEN)
    sent_bytes += send(socketfd, (&r), REQUEST_LEN, 0);

  error_response ers;
  recv(socketfd, (&ers), sizeof(alloc_response), 0);

  return ers.err;
}

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

int shm_delele_cv(int id, int uid) {
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
