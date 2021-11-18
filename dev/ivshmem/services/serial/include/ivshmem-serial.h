
#ifndef __IVSHMEM_SERIAL_H
#define __IVSHMEM_SERIAL_H
#include <stddef.h>

struct ivshm_info;

int ivshm_init_serial(struct ivshm_info *);
void ivshm_exit_serial(struct ivshm_info *);

/*!
 * \brief Put one char into an ivshmem serial stream
 * 
 * \param id Endpoint id of the ivshm serial path
 * \param c The char to sent
 * \return int The actual number of bytes sent
 */
int ivshm_putchar(unsigned id, char c);

int ivshm_getchar_noblock(char *out);
void ivshm_acquire_lock(void);
void ivshm_release_lock(void);

#endif //__IVSHMEM_SERIAL_H


