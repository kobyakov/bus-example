#ifndef BUS_IMPL_H
#define BUS_IMPL_H

#include <stdlib.h>
#include <sys/queue.h>
#include <zmq.h>

#include "bus_common.h"
#include "bus_client.h"
#include "bus_server.h"


enum bus_type {
	BUS_CLIENT,
	BUS_SERVER
};

struct bus_options_t;

struct bus_socket_interface_t {
	void *This;
	BUS_STATUS (*done)(void*);
	BUS_STATUS (*connect)(void*, const char*);
	BUS_STATUS (*close)(void*);
	BUS_STATUS (*setsockopt)(void*, int, const void*, size_t);
	BUS_STATUS (*send)(void*, const void*, size_t, int);
	BUS_STATUS (*recv)(void*, void**, size_t*);
	BUS_STATUS (*pollinit)(void*);
	BUS_STATUS (*poll)(void*);
	BUS_STATUS (*get_event_fd)(void*, int*);
	int (*check_poll_in)(void*);
	int (*check_sock_poll_in)(void*);
};



struct bus_options_t {
	int sendmore;
	int subscribe;
	int pollin;
	int pollout;
};

struct bus_t {
	struct bus_socket_interface_t interface;
	struct bus_options_t options;
	enum bus_type type;
	const char* address;
};

struct bus_client_t {
	struct bus_t* base_bus;
};



struct bus_server_t {
	struct bus_t* base_bus;
	bus_handler* handler;
};

void bus_log(int UNUSED(level), const char* format, ...);

BUS_STATUS send_register_messages(struct bus_t *bus, const char* id);
BUS_STATUS send_reply(struct bus_t* bus, void* envelope_message, size_t envelope_size, void* response_message, size_t response_size);

BUS_STATUS send_message(struct bus_t* bus, const void* data, size_t data_size, int send_more);
BUS_STATUS recv_message(struct bus_t* bus, void** data, size_t* data_size);
BUS_STATUS recv_status(struct bus_t* bus, enum bus_status_message_type* status_type);

BUS_STATUS bus_pollinit(struct bus_t *bus);
BUS_STATUS bus_poll(struct bus_t* bus);
int bus_check_poll_in(struct bus_t* bus);
int bus_check_sock_poll_in(struct bus_t* bus);

BUS_STATUS socket_connect(struct bus_t *bus, const char* url);
BUS_STATUS socket_close(struct bus_t* bus);

BUS_STATUS bus_get_event_fd(struct bus_t* bus, int *fd);

BUS_STATUS bus_init(enum bus_type type, struct bus_t** bus);
BUS_STATUS bus_done(struct bus_t* bus);



#endif // BUS_IMPL_H