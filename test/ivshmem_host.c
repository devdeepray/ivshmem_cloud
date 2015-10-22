#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
	int fd;
	int input = 0;
	int result = 0;
	void* data = NULL;

	fd = shm_open("ivshmem", O_RDWR, (mode_t) 0666);
	if (result < 0) {
		printf("Failed to open the ivshmem object\n");
		return (-1);
	}

	data = mmap(0, 0x100000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		printf("Failed to mmap shm object!\n");
		return (-2);
	}

	strcpy((char *)data, "Hello!");
	scanf("%d", &input);

	munmap(data, 0x100000);

	close(fd);

	return (0);
}
