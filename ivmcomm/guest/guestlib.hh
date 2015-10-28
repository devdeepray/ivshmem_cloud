#ifndef _GUESTLIB_H_
#define _GUESTLIB_H_

#include <iostream>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

using namespace std;

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

extern int socketfd;

static int REQUEST_LEN = sizeof(request);

int shm_alloc(int shmid, int uid, char rw_perms, int write_exclusive, int *offset, int *size);

int shm_dealloc(int shmid, int uid);

int shm_create_cv(int id, int uid);

int shm_delele_cv(int id, int uid);

int shm_acquire_cv(int id, int uid);

int shm_release_cv(int id, int uid);

int shm_notify_cv(int id, int uid);

int shm_wait_cv(int id, int uid);

int shm_open_conn(char* daemon_ip, char* daemon_port);

int shm_close_conn();

#endif // _GUESTLIB_H_
