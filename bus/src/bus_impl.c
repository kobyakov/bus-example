#include <zmq.h>
#include <assert.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "private/bus_zmq.h"
#include "private/bus_impl.h"
#include "bus_common.h"

const char broker_router_url[] = "tcp://127.0.0.1:5555";


BUS_STATUS send_register_messages(struct bus_t* bus, const char* id)
{
	BUS_NULL_POINTER_CHECK(bus);
	char m = MSG_BUS_HELLO;
	BUS_CHECK(send_message(bus, &m, 1, bus->options.sendmore), "Cannot send type message");
	BUS_CHECK(send_message(bus, id, strlen(id), 0), "Cannot send id");
	enum bus_status_message_type status_message_type;
	BUS_CHECK(recv_status(bus, &status_message_type), "send_register_messages: Cannot recv register message");

	void *error_message = NULL;
	size_t error_size = 0;
	switch (status_message_type) {
		case OK_MESSAGE:
			break;
		case ERROR_MESSAGE:
			BUS_CHECK(recv_message(bus, &error_message, &error_size), "send_register_messages: Cannot get error message");
			bus_log(LOG_ERR, "%.*s", error_size, (const char*)error_message);
			BUS_FAIL("Cannot register to bus");
			break;
		default:
			BUS_FAIL("send_register_messages: Wrong status message type");
			break;
	}

	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS send_reply(struct bus_t* bus, void* envelope_message, size_t envelope_size, void* response_message, size_t response_size)
{
	BUS_NULL_POINTER_CHECK(bus);
	void* error_message = NULL;
	size_t error_size = 0;

	char m = MSG_BUS_REPLY;
	BUS_CHECK(send_message(bus, &m, 1, bus->options.sendmore), "Cannot send type message");
	BUS_CHECK(send_message(bus, envelope_message, envelope_size, bus->options.sendmore), "Cannot send envelope message");
	BUS_CHECK(send_message(bus, response_message, response_size, 0), "Cannot send response message");

	enum bus_status_message_type status_message_type;
	BUS_CHECK(recv_status(bus, &status_message_type), "Cannot receive status message");

	switch (status_message_type) {
		case OK_MESSAGE:
			break;
		case ERROR_MESSAGE:
			BUS_CHECK(recv_message(bus, &error_message, &error_size), "Cannot get error message");
			bus_log(LOG_ERR, "%.*s", error_size, (const char*) error_message);
			break;
		default:
			BUS_FAIL("Wrong status message");
	}
	if (error_message)
		free(error_message);
	return STATUS_OK;

error:
	if (error_message)
		free(error_message);
	return STATUS_ERR;
}


enum bus_status_message_type get_status_message_type(char* status)
{
	switch (*status) {
		case MSG_BUS_OK:
			return OK_MESSAGE;	
		case MSG_BUS_ERROR:
			return ERROR_MESSAGE;
		case MSG_BUS_REPLY:
			return REPLY_MESSAGE;
		default:
			return UNDEF_MESSAGE;
	}
}


BUS_STATUS recv_status(struct bus_t* bus, enum bus_status_message_type* status_type)
{
	BUS_NULL_POINTER_CHECK(bus);
	void* status_message;
	size_t status_size;
	enum bus_status_message_type _status_type;

	BUS_CHECK(recv_message(bus, &status_message, &status_size), "Cannot recv error message");

	if (status_size != 1)
		BUS_FAIL("recv_status: Wrong status message");

	char *status = (char*)status_message;
	_status_type = get_status_message_type(status);

	free(status_message);
	*status_type = _status_type;
	return STATUS_OK;

error:
	if (status_message)
		free(status_message);
	return STATUS_ERR;
}


BUS_STATUS socket_connect(struct bus_t* bus, const char* url)
{
	BUS_NULL_POINTER_CHECK(bus);
	BUS_CHECK(bus->interface.connect(bus->interface.This, url), "Cannot connect socket");

	return STATUS_OK;

error:

	return STATUS_ERR;
}


BUS_STATUS socket_close(struct bus_t *bus)
{
	BUS_CHECK(bus->interface.close(bus->interface.This), "Cannot close interface");
	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS send_message(struct bus_t* bus, const void* data, size_t data_size, int send_more)
{
	BUS_NULL_POINTER_CHECK(bus);
	if (!data)
		return STATUS_ERR;
	if (!data_size)
		return STATUS_ERR;

	BUS_CHECK(bus->interface.send(bus->interface.This, data, data_size, send_more), "Cannot send message");

	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS recv_message(struct bus_t* bus, void** data, size_t* data_size)
{
	BUS_NULL_POINTER_CHECK(bus);
	void* buffer = NULL;
	size_t size;

	BUS_CHECK(bus->interface.recv(bus->interface.This, &buffer, &size), "Cannot recv message");

	*data = buffer;
	*data_size = size;
	
	return STATUS_OK;

error:
	*data = NULL;
	*data_size = 0;
	return STATUS_ERR;
}


BUS_STATUS bus_pollinit(struct bus_t* bus) {
	BUS_NULL_POINTER_CHECK(bus);
	BUS_CHECK(bus->interface.pollinit(bus->interface.This), "Cannot pollinit");
	
	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS bus_poll(struct bus_t* bus) {
	BUS_NULL_POINTER_CHECK(bus);
	BUS_CHECK(bus->interface.poll(bus->interface.This), "Cannot poll");
	
	return STATUS_OK;

error:
	return STATUS_ERR;
}


int bus_check_poll_in(struct bus_t* bus) {
	int state = bus->interface.check_poll_in(bus->interface.This);

	return state;
}


int bus_check_sock_poll_in(struct bus_t* bus) {

	int state = bus->interface.check_sock_poll_in(bus->interface.This);

	return state;
}


BUS_STATUS bus_get_event_fd(struct bus_t* bus, int *fd) {
	BUS_NULL_POINTER_CHECK(bus);
	BUS_CHECK(bus->interface.get_event_fd(bus->interface.This, fd), "Cannot get fd");
	
	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS bus_init(enum bus_type type, struct bus_t** bus)
{

	struct zmq_object_t* zmq_object = NULL;
	BUS_CHECK(zmq_object_init(&zmq_object), "Cannot init zmq object");
	struct bus_options_t opts = zmq_opts;
	zmq_interface.This = zmq_object;

	struct bus_t* bus_ = (struct bus_t*)malloc(sizeof(struct bus_t));
	if (!bus_) BUS_FAIL("Cannot allocate memory for bus");
	memset(bus_, 0, sizeof(struct bus_t));
	bus_->interface = zmq_interface;
	bus_->options = opts;
	bus_->type = type;
	switch (type) {
		case BUS_CLIENT:
			BUS_CHECK(socket_connect(bus_, broker_router_url), "Cannot connect REQ");
			break;

		case BUS_SERVER:
			BUS_CHECK(socket_connect(bus_, broker_router_url), "Cannot connect REQ");
			break;

		default:
			BUS_FAIL("Unknown bus type");
	}
	*bus = bus_;
	return STATUS_OK;

error:
	if (zmq_object)
		zmq_object_destroy(zmq_object);
	if (bus_)
		bus_done(bus_);
	zmq_object = NULL;
	*bus = NULL;
	return STATUS_ERR;
}

BUS_STATUS bus_done(struct bus_t* bus)
{
	if (!bus)
		return STATUS_OK;
	switch (bus->type) {
		case BUS_CLIENT:
			BUS_CHECK(socket_close(bus), "Cannot close REQ");
			break;
		case BUS_SERVER:
			BUS_CHECK(socket_close(bus), "Cannot close REQ");
			BUS_CHECK(socket_close(bus), "Cannot close SUB");
			break;
		default:
			BUS_FAIL("Unknown bus type");
	}

	if (bus->interface.This)
		bus->interface.done(bus->interface.This);
	free(bus);

	return STATUS_OK;

error:
	return STATUS_ERR;
}


