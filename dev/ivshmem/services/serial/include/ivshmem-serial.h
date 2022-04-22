
#ifndef __IVSHMEM_SERIAL_H
#define __IVSHMEM_SERIAL_H
#include <stddef.h>
#include <compiler.h>
#include <err.h>

__BEGIN_CDECLS

#define IVSHM_SERIAL_RX_CB_MAX_LEN (64)

typedef void (*ivshm_serial_rx_cb_t)(unsigned ept_id, char *data, uint16_t len);

struct ivshm_info;

int ivshm_init_serial(struct ivshm_info *);
void ivshm_exit_serial(struct ivshm_info *);

/*!
 * \brief Register a handler to be called on receipt of data.
 * By default, the serial service will start in buffered mode, wehre
 * received data is stored in the service's internal ring buffer and
 * must be retreived by calls to ivshm_serial_getchars() or
 * ivshm_serial_getchars_noblock().
 * 
 * By registering a callback, buffered mode is disabled and the
 * callback is responsible for handling incoming data.
 * 
 * If data is already in the internal buffer when the callback is
 * registered, this function will drain the internal buffer to the
 * callback before proceeding.
 * 
 * \param id The ivshm serial instance id, equal to "id" parameter
 * in device tree
 * \param cb Function to call on receipt of data
 * \return int NO_ERROR if succesful, otherwise error code
 */
int ivshm_serial_register_rx_cb(unsigned id, ivshm_serial_rx_cb_t cb);

/*!
 * \brief Un-register whatever callback is registered for this id
 * If no callback is registered, this has no effect.
 * 
 * \param id the ivshmem instance to do this action on.  Must be the same as
 * the "id" field in the device tree.
 * \return int number of callbacks removed, or ERR_NOT_FOUND if id not found
 */
int ivshm_serial_unregister_rx_cb(unsigned id);

/*!
 * \brief Get the service name (if any) of the given endpoint id
 * 
 * \param id endpoint id
 * \return char const* name, or NULL if service not found
 */
char const *ivshm_serial_get_name(unsigned id);

/*!
 * \brief Put one char into an ivshmem serial stream
 * 
 * \param id Endpoint id of the ivshm serial path
 * (same as "id" field in device tree entry)
 * \param c The char to sent
 * \return int The actual number of bytes sent
 */
int ivshm_serial_putchar(unsigned id, char c);

/*!
 * \brief Get chars from the ivshmem internal circular buffer.
 * If there is no data available will return immediately.
 * 
 * \param id The ivshm serial instance id, equal to "id" parameter
 * in device tree
 * \param buf target buffer where data will be written
 * \param buflen size of the target buffer
 * \return int number of bytes actually written, or error code
 */
int ivshm_serial_getchars_noblock(unsigned id, char *buf, uint16_t buflen);

/*!
 * \brief Send data over the ivshmem connection
 * DO NOT CALL FROM INTERRUPT
 * 
 * \param id endpoint id to send to
 * \param buf buffer holding data to send
 * \param len size of data to send
 * \return int bytes send (positive) or error code (negative)
 */
int ivshm_serial_send(unsigned id, char *buf, unsigned len);

/*!
 * \brief Find out how much data is waiting in the ivshmem
 * ring buffer to be read out
 * 
 * \param id endpoint id
 * \return int num of bytes ready (positive) or error (negative)
 */
int ivshm_serial_get_n_pending_rx (unsigned id);


void ivshm_acquire_lock(void);
void ivshm_release_lock(void);

__END_CDECLS

#endif //__IVSHMEM_SERIAL_H


