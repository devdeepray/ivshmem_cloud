#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

int main(void)
{
	int fd;
	void *data = NULL;

	fd = open("/dev/ivshmem", O_RDWR);
	if (fd < 0) {
		printf("Failed to open the ivshmem /dev node!\n");
		return (-1);
	} else {
		printf("Opened the ivshmem /dev node!\n");
	}

	data = mmap(0, 0x100000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		printf("Failed to mmap /dev/ivshmem!\n");
		return (-2);
	} else {
		printf("MMap on /dev/ivshmem successfull!\n");
	}

	strcpy((char *)data, "Chinmay");
	//printf("%s\n", (char *)data);

	munmap(data, 0x100000);	
	close(fd);

	return (0);
}
