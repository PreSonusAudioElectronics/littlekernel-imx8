/*!
 * \file ivshmem-serial.h
 * \author D. Anderson
 * \brief A "serial" (in retrospect, poorly named) wrapper for simple ivshmem communication
 * with hypervisor host environment
 * 
 * Copyright (c) 2022 Presonus Audio Electronics
 * 
 */

#ifndef __IVSHMEM_MSG_H
#define __IVSHMEM_MSG_H
#include <stddef.h>
#include <compiler.h>
#include <err.h>

__BEGIN_CDECLS


typedef void (*ivshm_msg_rx_cb_t)(void * priv_data, unsigned ept_id, char *data, uint16_t len);


struct ivshm_info;

int ivshm_init_msg(struct ivshm_info *);
void ivshm_exit_msg(struct ivshm_info *);

/*!
 * \brief Register a handler to be called on receipt of data.
 * This callback will be called for every message received on the
 * endpoint. This is called in the ivshmen_endpoint thread context.
 * 
 * 
 * \param id The ivshm serial instance id, equal to "id" parameter
 * in device tree
 * \param cb Function to call on receipt of a message
 * \param priv_data Private data pointer to be issued with callback.
 * \return int NO_ERROR if succesful, otherwise error code
 */
int ivshm_msg_register_rx_cb(unsigned id, ivshm_msg_rx_cb_t cb, void * priv_data);

/*!
 * \brief Un-register whatever callback is registered for this id
 * If no callback is registered, this has no effect.
 * Received messages will be discarded when there are no callback
 * installed.
 * 
 * \param id the ivshmem instance to do this action on.  Must be the same as
 * the "id" field in the device tree.
 * \return int number of callbacks removed, or ERR_NOT_FOUND if id not found
 */
int ivshm_msg_unregister_rx_cb(unsigned id);

/*!
 * \brief Get the service name (if any) of the given endpoint id
 * 
 * \param id endpoint id
 * \return char const* name, or NULL if service not found
 */
char const *ivshm_msg_get_name(unsigned id);

/*!
 * \brief send a message on the endpoint
 * 
 * \param id Endpoint id
 * (same as "id" field in device tree entry)
 * \param buf buffer containing message to send
 * \param len size of the message in bytes
 * \return NO_ERROR if success
 */
int ivshm_msg_send(unsigned id, const char *buf, unsigned len);


__END_CDECLS

#endif //__IVSHMEM_MSG_H


