#ifndef _GUESTLIB_H_
#define _GUESTLIB_H_

int shm_alloc(int shmid, int uid, char rw_perms, int write_exclusive, void **offset, int *size);

int shm_dealloc(int shmid, int uid);

int shm_create_cv(int id, int uid);

int shm_delete_cv(int id, int uid);

int shm_acquire_cv(int id, int uid);

int shm_release_cv(int id, int uid);

int shm_notify_cv(int id, int uid);

int shm_wait_cv(int id, int uid);

int shm_init(char* daemon_ip, char* daemon_port);

int shm_cleanup();

#endif // _GUESTLIB_H_
