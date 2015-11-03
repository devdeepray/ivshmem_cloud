#include <guestlib.hh>
#include <iostream>

using namespace std;

int main() {
	int ret = shm_init("localhost", "8888");
	if (ret) {
		cout << "So raha hai daemon" << endl;
	}
	void* addr;
	int size;
	ret = shm_alloc(3, 1, 'r', 0, &addr, &size);
	if (ret) {
		cout << "Fart machaya boss" << endl;
	}
	char *x = (char*) addr;
	cout << "Legit write" << endl;
	x[10] = 'a';
	cout << "Done" << endl;
	cout << "Illegit write" << endl;
	x[size] = 'b';
	cout << "Done" << endl;
}
