#include "guestlib.hh"
#include <unistd.h>

int main() {
  int off, size;

  printf("open");
  fflush(stdout);
  printf("OPEN CODE: %d\n", shm_open_conn("10.0.2.3", "8889"));
  fflush(stdout);

  printf("create");
  fflush(stdout);
  printf("CVCREAT: %d\n", shm_create_cv(10, 2));
  fflush(stdout);

  printf("acq");
  fflush(stdout);
  printf("CVACQUI: %d\n", shm_acquire_cv(10, 2));
  fflush(stdout);

  printf("wait");
  fflush(stdout);
  printf("CVWAIT: %d\n", shm_wait_cv(10, 2));
  fflush(stdout);

  printf("rel");
  fflush(stdout);
  printf("CVRELEA: %d\n", shm_release_cv(10, 2));
  fflush(stdout);

  printf("del");
  fflush(stdout);
  printf("CVDELET: %d\n", shm_delete_cv(10, 2));
  fflush(stdout);

  printf("close");
  fflush(stdout);
  printf("CLOSE CODE: %d\n", shm_close_conn());
  fflush(stdout);
}
