#include "guestlib.hh"
#include <unistd.h>

int main() {
  int off, size;
  int ops = 100000;
  shm_init("localhost", "8888");

  shm_create_cv(10, 1);

  for (int i = 0; i < ops; ++i) {
    shm_acquire_cv(10, 1);
    shm_release_cv(10, 1);
  }

  shm_delete_cv(10, 1);
  shm_cleanup();
}
