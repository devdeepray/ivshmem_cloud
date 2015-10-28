#include "guestlib.hh"

int main() {
  int off, size;
  printf("OPEN CODE: %d\n", shm_open_conn("10.0.2.3", "8889"));
  fflush(stdout);
  printf("ALLOC CODE: %d\n", shm_alloc(1, 2, 'w', 1, &off, &size));
  fflush(stdout);
  printf("OFF: %d, SIZE: %d \n", off, size);
  fflush(stdout);
  printf("DEALLOC CODE: %d\n", shm_dealloc(1, 2));
  fflush(stdout);
  printf("CLOSE CODE: %d\n", shm_close_conn());
  fflush(stdout);
}
