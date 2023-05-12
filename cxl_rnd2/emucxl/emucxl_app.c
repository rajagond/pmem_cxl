#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "emucxl_ioctl.h"

void* allocate_memory(int fd, int size, int numa_node)
{
	emucxl_arg_t q;
	q.size = size;
	q.numa_node = numa_node;
	if (ioctl(fd, EMUCXL_ALLOCATE_MEMORY, &q) == -1)
	{
		perror("emucxl ioctl allocate");
	}
	else
	{
		printf("Address: %p\n", q.address);
	}
	return q.address;
}

void free_memory(int fd, int* address)
{
	emucxl_arg_t q;
	q.address = (void*) address;
	if (ioctl(fd, EMUCXL_FREE_MEMORY, &q) == -1)
	{
		perror("emucxl ioctl free");
	}
	return ;
}

int main(int argc, char *argv[])
{
	char *file_name = "/dev/emucxl";
	int fd;
	int ARRAY_SZ = 10;
	int size = ARRAY_SZ * sizeof(int);

	fd = open(file_name, O_RDWR);
	if (fd == -1)
	{
		perror("emucxl open");
		return 2;
	}

	int* address = (int *)allocate_memory(fd, size, LOCAL_MEMORY);
	printf("pid = %d\n", getpid());
	printf("HI 1\n");
	if (address)
	{
		printf("HI 2\n");
		
		emucxl_memory_t m;
		m.address = address;
		m.index = 0;
		m.value = 5;
		if (ioctl(fd, EMUCXL_STORE, &m) == -1)
		{
			perror("emucxl ioctl load");
		}
		
		m.value = 10;
		if (ioctl(fd, EMUCXL_LOAD, &m) == -1)
		{
			perror("emucxl ioctl load");
		}
		else
		{
			printf("Value: %d\n", m.value);
		}
	}

	


	close (fd);

	return 0;
}