#ifndef EMUCXL_IOCTL_H
#define EMUCXL_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

// typedef struct
// {
// 	int status, dignity, ego;
// } query_arg_t;

typedef struct
{
	int size;
	void* address;
	int ret;
	int numa_node;
} emucxl_arg_t;

typedef struct
{
	int* address;
	int index;
	int value;
} emucxl_memory_t;

#define EMUCXL_ALLOCATE_MEMORY _IOWR('e', 4, emucxl_arg_t *)
#define EMUCXL_FREE_MEMORY _IOW('e', 5, emucxl_arg_t *)
#define EMUCXL_RESIZE_MEMORY _IOWR('e', 6, emucxl_arg_t *)
#define EMUCXL_MIGRATE_MEMORY _IOWR('e', 7, emucxl_arg_t *)
#define EMUCXL_LOAD _IOWR('e', 8, emucxl_memory_t *)
#define EMUCXL_STORE _IOWR('e', 9, emucxl_memory_t *)

#define LOCAL_MEMORY 0
#define REMOTE_MEMORY 1

#endif
