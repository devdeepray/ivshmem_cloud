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
int cvid = 123;
int to_transfer = 10000;

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

  ret = shm_create_cv(cvid, uid);
  if (ret) {
    cerr << "CV create Error" << endl;
  }

  void* data = malloc(size - sizeof(bool));

  int i = 0;

  while(i < to_transfer) {
    // Check shared variable by taking lock
    ret = shm_acquire_cv(cvid, uid);
    if (ret) {
      cerr << "CV acquire Error" << endl;
    }
    if (!(*((bool*)offset))) {
      // Data not available, write here
      // scanf("%s", (char*)((bool*)offset + 1));

      memcpy((void*)((bool*)offset + 1), data, size - sizeof(bool));
      ++i;
      *((bool*)offset) = true;

      // Notify the reader
      ret = shm_notify_cv(cvid, uid);
      if (ret) {
        cerr << "CV notify Error" << endl;
      }
    } else {
      // Wait for the reader to complete
      ret = shm_wait_cv(cvid, uid);
      if (ret) {
        cerr << "CV wait Error" << endl;
      }
    }
    ret = shm_release_cv(cvid, uid);
    if (ret) {
      cerr << "CV release Error" << endl;
    }
  }

  ret = shm_delete_cv(cvid, uid);
  if (ret) {
    cerr << "CV delete Error" << endl;
  }

  ret = shm_dealloc(shmid, uid);
  if (ret) {
    cerr << "Dealloc Error" << endl;
  }

  ret = shm_cleanup();
  if (ret) {
    cerr << "Cleanup Error" << endl;
  }
}
