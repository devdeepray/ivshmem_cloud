#include <guestlib.hh>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

char* ip = "localhost";
char* port = "8888";
int shmid = 1234;
int uid = 1;
int to_transfer = 20000;

int main() {
  int ret = shm_init(ip, port);
  if (ret) {
    cerr << "Init Error" << endl;
  }

  char* offset;
  int size;

  ret = shm_alloc(shmid, uid, 'w', 0, (void**)&offset, &size);
  if (ret) {
    cerr << "Alloc Error" << endl;
  }

  void* data = malloc(size - sizeof(bool));

  int i = 0;

  while(i < to_transfer) {
    memcpy((void*)((bool*)offset + 1), data, size - sizeof(bool));
    ++i;
  }

  cout << "Read " << 1.0 * size * to_transfer << " bytes of data." << endl;

  ret = shm_dealloc(shmid, uid);
  if (ret) {
    cerr << "Dealloc Error" << endl;
  }

  ret = shm_cleanup();
  if (ret) {
    cerr << "Cleanup Error" << endl;
  }
}
