

#include "ivshmem-pipe.h"
#include "ivshmem-endpoint.h"
#include "ivshmem-msg.h"
#include <debug.h>
#include <assert.h>
#include <stdio.h>
#include <trace.h>
#include <lib/cbuf.h>
#include <lk/init.h>
#include <lib/io.h>
#include <lib/appargs.h>
#include <string.h>
#include <kernel/mutex.h>
#include <lib/console.h>
#include <dev/driver.h>
#include <dev/ivshm.h>
#include <err.h>

#include <string.h>
#include <stdlib.h>


#define IVSHM_MAX_EP_CBUF_SIZE (1024 * 64)
#define LOCAL_TRACE (0)


/******************************************************************************/
// Types
/******************************************************************************/
struct ivshm_msg_service {
	struct list_node node;
	unsigned id;
	struct ivshm_endpoint *ept;
	char const *name;
	bool running;
	ivshm_msg_rx_cb_t rx_cb;
    void * priv_data;
};

/******************************************************************************/
// File-scope Globals
/******************************************************************************/
static struct list_node msg_service_list =
	LIST_INITIAL_VALUE(msg_service_list);



/******************************************************************************/
// Forward Declarations
/******************************************************************************/
static inline struct ivshm_msg_service
*_get_service(unsigned id);

/******************************************************************************/
// Public Functions
/******************************************************************************/

int ivshm_init_msg(struct ivshm_info *info)
{
	return 0;
}

void ivshm_exit_msg(struct ivshm_info *info)
{
	struct ivshm_msg_service *service, *next;
	int ret = 0;

	list_for_every_entry_safe(&msg_service_list, service, next,
							  struct ivshm_msg_service, node) {
		service->running = false;
		ivshm_endpoint_destroy(service->ept);
		list_delete(&service->node);
		free(service);
	}
}



int ivshm_msg_send(unsigned id, const char *buf, unsigned len)
{
	struct ivshm_msg_service *service;
    struct ivshm_ep_buf ep_buf;
	int ret = 0;

	if (!buf)
		return ERR_INVALID_ARGS;

	service = _get_service(id);
	if (!service) {
		return ERR_NOT_FOUND;
	}

    ivshm_ep_buf_init(&ep_buf);
    ivshm_ep_buf_add(&ep_buf, buf, len);
    ivshm_endpoint_write(service->ept, &ep_buf); //FIXME: should check for return but this should not fail
    return ret;
}



int ivshm_msg_register_rx_cb(unsigned id, ivshm_msg_rx_cb_t cb, void * priv_data)
{
	if (!cb) {
		return ERR_INVALID_ARGS;
	}

	struct ivshm_msg_service *service = _get_service(id);
	if ( !service ) {
		return ERR_NOT_FOUND;
	}

	service->priv_data = priv_data;
    service->rx_cb = cb;

	return NO_ERROR;
}

int ivshm_msg_unregister_rx_cb(unsigned id)
{
	struct ivshm_msg_service *service = _get_service(id);
	if ( !service ) {
		return ERR_NOT_FOUND;
	}

	int ret = 0;
	if ( service->rx_cb ) {
		ret = 1;
	}
	service->rx_cb = NULL;
	return ret;
}

char const *ivshm_msg_get_name(unsigned id)
{
	struct ivshm_msg_service *service = _get_service(id);
	if ( !service ) {
		return NULL;
	}

	return service->name;
}


static ssize_t ivshm_msg_consume(struct ivshm_endpoint *ep, struct ivshm_pkt *pkt)
{
	char *payload = (char *) &pkt->payload;

	// apparently, the ivshmem endpoint appends a null char, so we need to drop it
	size_t len = ivshm_pkt_get_payload_length(pkt);
	
	unsigned id = IVSHM_EP_GET_ID(ep->id);

	// get the service struct and either buffer or send it to callback
	struct ivshm_msg_service *service = _get_service(id);
	if ( service->rx_cb ) {
		service->rx_cb(service->priv_data, id, payload, len);
	}
	return 0;
}

static inline struct ivshm_msg_service
*_get_service(unsigned id)
{
	struct ivshm_msg_service *service;

	list_for_every_entry(&msg_service_list, service,
						 struct ivshm_msg_service, node) {
		if (service->id == id)
			return service;
	}

	printlk(LK_ERR, "%s:%d: Could not find msg_%u endpoint\n",
			__PRETTY_FUNCTION__, __LINE__, id);
	return NULL;
}


static inline void print_err_property(char *property_name)
{
	printlk(LK_ERR, "%s:%d: Endpoint %s property cannot be read, aborting!\n",
			__PRETTY_FUNCTION__, __LINE__, property_name);
}

static status_t ivshm_msg_dev_init(struct device *dev)
{
	int ret = NO_ERROR;
	char *property_name = NULL;
	char name[IVSHM_EP_MAX_NAME];
	uint32_t id, ep_size;
	struct ivshm_msg_service *service = NULL;

	struct ivshm_dev_data *ivshm_dev = ivshm_get_device(0);
	if (!ivshm_dev) {
		return ERR_NOT_READY;
	}

	struct ivshm_info *info = ivshm_dev->handler_arg;
	if (!info) {
		return ERR_NOT_READY;
	}

	property_name = (char *)"id";
	ret = of_device_get_int32(dev, property_name, &id);
	if (ret) {
		print_err_property(property_name);
		return ERR_NOT_FOUND;
	}

	property_name = (char *)"size";
	ret = of_device_get_int32(dev, property_name, &ep_size);
	if (ret) {
		print_err_property(property_name);
		return ERR_NOT_FOUND;
	}

	DEBUG_ASSERT(ep_size < IVSHM_MAX_EP_CBUF_SIZE);

	service = malloc(sizeof(struct ivshm_msg_service));
	if (!service) {
		ret = ERR_NO_MEMORY;
		goto cleanup;
	}

	memset(service, 0, sizeof(struct ivshm_msg_service));

	property_name = (char *)"id_str";
	ret = of_device_get_strings(dev, property_name, &service->name, 1);
	if ( 1 != ret ) {
		TRACEF("Failed to get id_str!\n");
		ret = ERR_NOT_FOUND;
		goto cleanup;
	}

	service->id = id;
	snprintf(name, IVSHM_EP_MAX_NAME, "msg_%u_%s", service->id, service->name);
	service->ept = ivshm_endpoint_create(
					   name,
					   service->id,
					   ivshm_msg_consume,
					   info,
					   ep_size,
					   0
				   );

	if (!service->ept) {
		ret = ERR_NO_MEMORY;
		goto cleanup;
	}

	printf("\n%s:%u: new endpoint id = %u\n", __PRETTY_FUNCTION__, __LINE__,
		   IVSHM_EP_GET_ID(service->ept->id) );

	service->running = true;
	list_add_tail(&msg_service_list, &service->node);

	return ret;

cleanup:
	if (service->ept) { ivshm_endpoint_destroy(service->ept); }
	if (service) { free(service); }

	return ret;
}

static struct driver_ops the_ops = {
	.init = ivshm_msg_dev_init
};

DRIVER_EXPORT_WITH_LVL(ivshm_msg, &the_ops, DRIVER_INIT_CORE);
