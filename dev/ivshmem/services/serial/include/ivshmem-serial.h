
#ifndef __IVSHMEM_SERIAL_H
#define __IVSHMEM_SERIAL_H
#include <stddef.h>

struct ivshm_info;

int ivshm_init_serial(struct ivshm_info *);
void ivshm_exit_serial(struct ivshm_info *);

#endif //__IVSHMEM_SERIAL_H


