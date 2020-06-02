#include <zmq.h>
#include <stdlib.h>

#include "private/bus_zmq.h"
#include "bus_common.h"

static BUS_STATUS i_zmq_connect(void *self, const char *addr); 
static BUS_STATUS i_zmq_close(void* self);
static BUS_STATUS i_zmq_setsockopt(void *self, int option_name, const void* option_value, size_t option_len);
static BUS_STATUS i_zmq_msg_send(void *self, const void *message, size_t message_size, int send_more);
static BUS_STATUS i_zmq_msg_recv(void *self, void **message, size_t *size);
static BUS_STATUS i_zmq_pollinit(void *self);
static BUS_STATUS i_zmq_poll(void *self);
static BUS_STATUS i_zmq_get_event_fd(void *self, int *fd);
static int i_zmq_check_polling_in(void *self);
static int i_zmq_check_sock_polling_in(void *self);

struct bus_options_t zmq_opts = {
	.sendmore  = ZMQ_SNDMORE,
	.subscribe = ZMQ_SUBSCRIBE,
	.pollin    = ZMQ_POLLIN,
	.pollout   = ZMQ_POLLOUT
};

struct bus_socket_interface_t zmq_interface = {
	.done               = zmq_object_destroy,
	.connect            = i_zmq_connect,
	.close              = i_zmq_close,
	.setsockopt         = i_zmq_setsockopt,
	.send               = i_zmq_msg_send,
	.recv               = i_zmq_msg_recv,
	.pollinit           = i_zmq_pollinit,
	.poll               = i_zmq_poll,
	.get_event_fd       = i_zmq_get_event_fd,
	.check_poll_in      = i_zmq_check_polling_in,
	.check_sock_poll_in = i_zmq_check_sock_polling_in
};

static BUS_STATUS zmq_socket_get(struct zmq_object_t *zmq_obj, void **zmq_sock)
{
  	if ( !zmq_obj ) { BUS_FAIL("Cannot find zmq socket"); }

	*zmq_sock = zmq_obj->socket;
	return STATUS_OK;

error:
  *zmq_sock = NULL;
	return STATUS_ERR;
}

static BUS_STATUS bus_zmq_message_create(const void *wlr_msg, size_t size_wlr_msg, zmq_msg_t **zmq_msg)
{
	zmq_msg_t *zmq_msg_tmp      = NULL;
	void      *data_zmq_msg_tmp = NULL;

	/*if ( !wlr_msg  ) { return STATUS_ERR; }*/ /* ( NULL == wlr_msg ) is valid case */
	if ( !zmq_msg  ) { return STATUS_ERR; }

	zmq_msg_tmp = (zmq_msg_t*) malloc(sizeof(zmq_msg_t));
	if ( !zmq_msg_tmp ) { BUS_FAIL("Cannot allocate memory for zmq message"); }

  if ( wlr_msg ) {
    if ( -1 == zmq_msg_init_size(zmq_msg_tmp, size_wlr_msg) ) { BUS_FAIL("Cannot init zmq_message"); }
		data_zmq_msg_tmp = zmq_msg_data(zmq_msg_tmp);
		if ( !data_zmq_msg_tmp ) { BUS_FAIL("Cannot get data section of zmq message"); }
		memcpy(data_zmq_msg_tmp, wlr_msg, size_wlr_msg);
	} else {
		if ( -1 == zmq_msg_init(zmq_msg_tmp) ) { BUS_FAIL("Cannot init zmq_message"); }
	}

  *zmq_msg = zmq_msg_tmp;
	return STATUS_OK;

error:
  if ( zmq_msg_tmp ) { free(zmq_msg_tmp); }
	*zmq_msg = NULL;
	return STATUS_ERR;
}

static BUS_STATUS bus_zmq_message_close(zmq_msg_t *zmq_msg)
{
	if ( !zmq_msg  ) { return STATUS_ERR; }

	if ( -1 == zmq_msg_close(zmq_msg) ) { BUS_FAIL("Cannot close zmq message"); }
	
	free(zmq_msg);
	return STATUS_OK;

error:
	free(zmq_msg);
	return STATUS_ERR;
}

static BUS_STATUS bus_zmq_message_get_data(zmq_msg_t *zmq_msg, void **msg, size_t *size_msg)
{
  void   *data_zmq_msg = NULL;
	size_t  size_zmq_msg = 0;
	void   *wlr_msg      = NULL;

	if ( !zmq_msg  ) { return STATUS_ERR; }
	if ( !msg      ) { return STATUS_ERR; }
	if ( !size_msg ) { return STATUS_ERR; }
		
	data_zmq_msg = zmq_msg_data(zmq_msg);
	if ( !data_zmq_msg ) { BUS_FAIL("Cannot get zmq data"); }

	size_zmq_msg = zmq_msg_size(zmq_msg);
	if ( !size_zmq_msg ) { BUS_FAIL("Size of zmq data is zero"); }

	wlr_msg = malloc(size_zmq_msg);
	if ( !wlr_msg ) { BUS_FAIL("Cannot allocate memory for message"); }

	memcpy(wlr_msg, data_zmq_msg, size_zmq_msg);

	*msg      = wlr_msg;
	*size_msg = size_zmq_msg;
	return STATUS_OK;

error:
	if ( wlr_msg ) { free(wlr_msg); }
	*msg      = NULL;
	*size_msg = 0;
	return STATUS_ERR;	
}

static BUS_STATUS zmq_connect_raw(void *zmq_ctx, const char *addr, void** socket)
{
  	void   *zmq_sock         = NULL;
  	int     zmq_fd_sock      = 0;
	size_t  size_zmq_fd_sock = 0;

	if ( !zmq_ctx       ) { return STATUS_ERR; }
	if ( !addr          ) { return STATUS_ERR; }

	size_zmq_fd_sock = sizeof(zmq_fd_sock);

  	zmq_sock = zmq_socket(zmq_ctx, ZMQ_REQ);
	if ( !zmq_sock ) { 
		BUS_FAIL("Cannot create socket"); 
	}

	if ( -1 == zmq_getsockopt(zmq_sock, ZMQ_FD, &zmq_fd_sock, &size_zmq_fd_sock) ) { 
		BUS_FAIL("Cannot get zmq socket fd"); 
	}

	if ( -1 == zmq_connect(zmq_sock, addr) ) { 
		BUS_FAIL("Cannot connect to broker"); 
	}

	*socket = zmq_sock;


	return STATUS_OK;

error:
  if ( zmq_sock ) { zmq_close(zmq_sock); }
  return STATUS_ERR;
}

static BUS_STATUS i_zmq_connect(void *self, const char *addr) 
{
	struct zmq_object_t *zmq_obj = NULL;
	void** zmq_socket= NULL;

	if ( !self ) { return STATUS_ERR; }
	if ( !addr ) { return STATUS_ERR; }

	zmq_obj = (struct zmq_object_t*) self;

	zmq_socket = &zmq_obj->socket;

	BUS_CHECK(zmq_connect_raw(zmq_obj->context, addr, zmq_socket), "Cannot connect");

	return STATUS_OK;

error:
  return STATUS_ERR;
}

static BUS_STATUS i_zmq_close(void *self) 
{
	struct zmq_object_t *zmq_obj  = NULL;
	void                *zmq_sock = NULL;

	if ( !self ) { return STATUS_ERR; }

	zmq_obj = (struct zmq_object_t*) self;

	BUS_CHECK(zmq_socket_get(zmq_obj, &zmq_sock), "Cannot get zmq socket");

  if ( -1 == zmq_close(zmq_sock) ) { 
		BUS_FAIL("Cannot close zmq socket"); 
	}

	return STATUS_OK;

error:
	return STATUS_ERR;
}

static BUS_STATUS i_zmq_setsockopt(void *self, int opt_name, const void *opt_val, size_t opt_val_len) 
{
	struct zmq_object_t *zmq_obj  = NULL;
	void                *zmq_sock = NULL;

	if ( !self    ) { return STATUS_ERR; }
	if ( !opt_val ) { return STATUS_ERR; }

	zmq_obj = (struct zmq_object_t*) self;

	BUS_CHECK(zmq_socket_get(zmq_obj, &zmq_sock), "Cannot get zmq socket");

	if ( -1 == zmq_setsockopt(zmq_sock, opt_name, opt_val, opt_val_len) ) { 
		BUS_FAIL("Cannot set zmq socket option"); 
	}

	return STATUS_OK;

error:
	return STATUS_ERR;
}

static BUS_STATUS i_zmq_msg_send(void *self, const void *wlr_msg, size_t size_wlr_msg, int send_more)
{
	struct zmq_object_t *zmq_obj  = NULL;
	void                *zmq_sock = NULL;
	zmq_msg_t           *zmq_msg  = NULL;

	if ( !self    ) { return STATUS_ERR; }
	if ( !wlr_msg ) { return STATUS_ERR; }

	zmq_obj = (struct zmq_object_t*) self;

	BUS_CHECK(zmq_socket_get(zmq_obj, &zmq_sock), "Cannot get zmq socket");
	BUS_CHECK(bus_zmq_message_create(wlr_msg, size_wlr_msg, &zmq_msg), "Cannot create zmq message");
	if ( -1 == zmq_msg_send(zmq_msg, zmq_sock, send_more) ) { BUS_FAIL("Cannot send message"); }
	BUS_CHECK(bus_zmq_message_close(zmq_msg), "Cannot destroy zmq message");

	return STATUS_OK;

error:
	if ( zmq_msg ) { bus_zmq_message_close(zmq_msg); }
	return STATUS_ERR;
}

static BUS_STATUS i_zmq_msg_recv(void *self, void **msg, size_t *size_msg)
{
	struct zmq_object_t *zmq_obj      = NULL;
	zmq_msg_t           *zmq_msg      = NULL;
	void                *wlr_msg      = NULL;
	size_t               size_wlr_msg = 0;
	void                *zmq_sock     = NULL;

	if ( !self     ) { return STATUS_ERR; }
	if ( !msg      ) { return STATUS_ERR; }
	if ( !size_msg ) { return STATUS_ERR; }

	zmq_obj = (struct zmq_object_t*) self;

	BUS_CHECK(zmq_socket_get(zmq_obj, &zmq_sock), "Cannot get zmq socket");
	BUS_CHECK(bus_zmq_message_create(NULL, 0, &zmq_msg), "Cannot create zmq message");
  if ( -1 == zmq_msg_recv(zmq_msg, zmq_sock, 0) ) { BUS_FAIL("Cannot receive message"); }
	BUS_CHECK(bus_zmq_message_get_data(zmq_msg, &wlr_msg, &size_wlr_msg), "Cannot get zmq message data");
  BUS_CHECK(bus_zmq_message_close(zmq_msg), "Cannot destroy zmq message");

	*msg      = wlr_msg;
	*size_msg = size_wlr_msg;

	return STATUS_OK;

error:
	if ( zmq_msg ) { bus_zmq_message_close(zmq_msg); }
	if ( wlr_msg ) { free(wlr_msg); }
	*msg      = NULL;
	*size_msg = 0;
	return STATUS_ERR;
}

static BUS_STATUS i_zmq_pollinit(void *self) 
{
	struct zmq_object_t *zmq_obj       = NULL;
  zmq_pollitem_t      *zmq_pollitems = NULL;
	const size_t         count_items   = 1;
	
	if ( !self ) { return STATUS_ERR; }

	zmq_obj = (struct zmq_object_t*) self;
	
	zmq_pollitems = (zmq_pollitem_t*) malloc(sizeof(zmq_pollitem_t) * count_items);
	if ( !zmq_pollitems ) {	BUS_FAIL("Cannot allocate memory for zmq_pollitems"); }

	memset(zmq_pollitems, 0, sizeof(zmq_pollitem_t) * count_items);
	zmq_pollitems[0].socket  = zmq_obj->socket;
	zmq_pollitems[0].fd      = 0;
	zmq_pollitems[0].events  = ZMQ_POLLIN | ZMQ_POLLOUT;
	zmq_pollitems[0].revents = 0;
	
	zmq_obj->pollitems = zmq_pollitems;
	zmq_obj->n_items   = count_items;

	return STATUS_OK;

error:
	if ( zmq_pollitems ) { free(zmq_pollitems); }
	return STATUS_ERR;
}

static BUS_STATUS i_zmq_poll(void *self)
{
	struct zmq_object_t *zmq_obj = NULL;

	if ( !self ) { return STATUS_ERR; }
	
	zmq_obj = (struct zmq_object_t*) self;

	if ( -1 == zmq_poll(zmq_obj->pollitems, 1, -1) ) {
		BUS_FAIL("Cannot zmq poll"); 
	}

	return STATUS_OK;

error:
	return STATUS_ERR;
}

static int i_zmq_check_polling_in(void *self)
{
	struct zmq_object_t *zmq_obj = NULL;
	int                  state   = -1;

	if ( !self ) { return 0; }
	
	zmq_obj = (struct zmq_object_t*) self;

	state = zmq_obj->pollitems[0].revents & ZMQ_POLLIN;

	return state;
}

static int i_zmq_check_sock_polling_in(void *self)
{
	struct zmq_object_t *zmq_obj        = NULL;
	void                *zmq_sock       = NULL;
	int                  state          = -1;
  uint32_t             zmq_event      = 0;
	size_t               size_zmq_event = 0;

	if ( !self ) { return 0; }
	
	zmq_obj = (struct zmq_object_t*) self;

	size_zmq_event = sizeof(zmq_event);

	BUS_CHECK(zmq_socket_get(zmq_obj, &zmq_sock), "Cannot get zmq socket");

	if ( -1 == zmq_getsockopt(zmq_sock, ZMQ_EVENTS, &zmq_event, &size_zmq_event) ) {
		BUS_FAIL("Cannot get zmq socket events"); 
	}

	state = zmq_event & ZMQ_POLLIN;

	return state;

error:
  return 0;
}

static BUS_STATUS i_zmq_get_event_fd(void *self, int *fd)
{
  struct zmq_object_t *zmq_obj = NULL;

	if ( !self ) { return 0; }
	if ( !fd   ) { return 0; }

	zmq_obj = (struct zmq_object_t*) self;

  *fd = zmq_obj->fd;

	return STATUS_OK;
}

BUS_STATUS zmq_object_init(struct zmq_object_t **zmq_obj)
{ 
	void                *zmq_ctx     = NULL;
	struct zmq_object_t *zmq_obj_tmp = NULL;

	if ( !zmq_obj ) { return STATUS_ERR; }

	zmq_ctx = zmq_ctx_new();
	if ( !zmq_ctx ) {	BUS_FAIL("Cannot create zmq_context"); }

	zmq_obj_tmp = (struct zmq_object_t*)malloc(sizeof(struct zmq_object_t));
	if ( !zmq_obj_tmp ) { BUS_FAIL("Cannot allocation memory for zmq object"); }
	
	memset(zmq_obj_tmp, 0, sizeof(struct zmq_object_t));
	zmq_obj_tmp->context   = zmq_ctx;
	zmq_obj_tmp->pollitems = NULL;

	*zmq_obj = zmq_obj_tmp;
	return STATUS_OK;

error:
	if ( zmq_obj_tmp ) { free(zmq_obj_tmp);        }
	if ( zmq_ctx     ) { zmq_ctx_destroy(zmq_ctx); }
	*zmq_obj = NULL;
	return STATUS_ERR;
}

BUS_STATUS zmq_object_destroy(void *self)
{
	struct zmq_object_t *zmq_obj = NULL;

	if ( !self ) { return STATUS_ERR; }

	zmq_obj = (struct zmq_object_t*) self;

	if ( -1 == zmq_ctx_destroy(zmq_obj->context) ) { BUS_FAIL("Cannot destroy zmq context"); }
	if ( zmq_obj->pollitems ) { free(zmq_obj->pollitems); }
	free(zmq_obj);

	return STATUS_OK;

error:
	if ( zmq_obj->pollitems ) { free(zmq_obj->pollitems); }
	free(zmq_obj);
  return STATUS_ERR;
}