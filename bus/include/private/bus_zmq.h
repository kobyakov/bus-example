#ifndef BUS_ZMQ_H
#define BUS_ZMQ_H
#include "bus_common.h"
#include "bus_impl.h"

struct zmq_socket_decr_t {
	void* socket;
	int   fd;
};

struct zmq_object_t {
	void* context;
	void* socket;;
	zmq_pollitem_t* pollitems;
	int fd;
	int n_items;
};

extern struct bus_options_t zmq_opts;
extern struct bus_socket_interface_t zmq_interface;

BUS_STATUS zmq_object_init(struct zmq_object_t** zmq_object);
BUS_STATUS zmq_object_destroy(void *_zmq_object);

#endif

