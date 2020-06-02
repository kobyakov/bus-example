#include <zmq.h>
#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "bus_common.h"
#include "bus_impl.h"
#include "bus_test_interfaces.h"
#include "bus_zmq.h"

enum call_type call_type_enum;

struct bus_socket_interface_t log_zmq_interface = {
	.done = log_zmq_object_destroy,
	.connect = i_log_zmq_connect,
	.close = i_log_zmq_close,
	.setsockopt = i_log_zmq_setsockopt,
	.send = i_log_zmq_msg_send,
	.recv = i_log_zmq_msg_recv,
	.pollinit = i_log_zmq_pollinit,
	.poll = i_log_zmq_poll,
	.check_poll_in = i_log_zmq_check_polling_in,
	.check_sock_poll_in = i_log_zmq_check_sock_polling_in,
	.get_event_fd = i_log_zmq_check_get_fd
};

struct bus_socket_interface_t dummy_interface = {
	.done = dummy_destroy,
	.connect = dummy_connect,
	.close = dummy_close,
	.setsockopt = dummy_setsockopt,
	.send = dummy_msg_send,
	.recv = dummy_msg_recv,
	.pollinit = dummy_pollinit,
	.poll = dummy_poll,
	.check_poll_in = dummy_check_polling_in,
	.check_sock_poll_in = dummy_check_sock_polling_in,
	.get_event_fd = dummy_get_fd
};

struct bus_socket_interface_t check_correct_interface = {
	.done = check_destroy,
	.connect = check_connect,
	.close = check_close,
	.setsockopt = check_setsockopt,
	.send = check_msg_send,
	.recv = check_msg_recv,
	.pollinit = check_pollinit,
	.poll = check_poll,
	.check_poll_in = check_check_polling_in,
	.check_sock_poll_in = check_check_sock_polling_in,
	.get_event_fd = check_get_fd
};

BUS_STATUS log_zmq_object_init(struct bus_socket_interface_t* decoration_interface, void* decoration_This, int level, struct log_zmq_object_t** log_zmq_object)
{

	struct log_zmq_object_t* t = (struct log_zmq_object_t*)malloc(sizeof(struct log_zmq_object_t));
	if (!t)
		RM_FAIL("Cannot allocate memory for log_zmq_object");

	memset(t, 0, sizeof(struct log_zmq_object_t));

	printf("ZMQ object init\n");

	t->level = level;
	t->This = decoration_This;
	t->interface = *decoration_interface;

	*log_zmq_object = t;
	return STATUS_OK;

error:
	return STATUS_ERR;
}

BUS_STATUS log_zmq_object_destroy(void* _log_zmq_object)
{
	struct log_zmq_object_t* log_zmq_object = (struct log_zmq_object_t*)_log_zmq_object;
	printf("ZMQ object destroy [%d]\n", log_zmq_object->level);
	log_zmq_object->interface.done(log_zmq_object->This);
	free(log_zmq_object);
	return STATUS_OK;
}

BUS_STATUS i_log_zmq_connect(void* _log_zmq_object, const char* url)
{
	struct log_zmq_object_t* log_zmq_object = (struct log_zmq_object_t*)_log_zmq_object;
	printf("ZMQ connect [%d]\n", log_zmq_object->level);
	BUS_CHECK(log_zmq_object->interface.connect(log_zmq_object->This, url), "Cannot <function>");

	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS i_log_zmq_close(void* _log_zmq_object)
{
	struct log_zmq_object_t* log_zmq_object = (struct log_zmq_object_t*)_log_zmq_object;
	printf("ZMQ close [%d]\n", log_zmq_object->level);
	BUS_CHECK(log_zmq_object->interface.close(log_zmq_object->This), "Cannot close");

	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS i_log_zmq_setsockopt(void* _log_zmq_object, int option_name, const void* option_value, size_t option_len)
{
	struct log_zmq_object_t* log_zmq_object = (struct log_zmq_object_t*)_log_zmq_object;
	printf("ZMQ setsockopt [%d]\n", log_zmq_object->level);
	BUS_CHECK(log_zmq_object->interface.setsockopt(log_zmq_object->This, option_name, option_value, option_len), "Cannot <function>");

	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS i_log_zmq_msg_send(void* _log_zmq_object, void* message, size_t message_size, int send_more)
{
	struct log_zmq_object_t* log_zmq_object = (struct log_zmq_object_t*)_log_zmq_object;
	printf("ZMQ msg send [%d]\n", log_zmq_object->level);
	BUS_CHECK(log_zmq_object->interface.send(log_zmq_object->This, message, message_size, send_more), "Cannot zmq msg send");

	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS i_log_zmq_msg_recv(void* _log_zmq_object, void** message, size_t* size)
{
	struct log_zmq_object_t* log_zmq_object = (struct log_zmq_object_t*)_log_zmq_object;
	printf("ZMQ msg recv [%d]\n", log_zmq_object->level);
	BUS_CHECK(log_zmq_object->interface.recv(log_zmq_object->This, message, size), "Cannot zmq msg recv");

	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS i_log_zmq_pollinit(void* _log_zmq_object)
{
	struct log_zmq_object_t* log_zmq_object = (struct log_zmq_object_t*)_log_zmq_object;
	printf("ZMQ pollinit [%d]\n", log_zmq_object->level);
	BUS_CHECK(log_zmq_object->interface.pollinit(log_zmq_object->This), "Cannot pollinit");

	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS i_log_zmq_poll(void* _log_zmq_object)
{
	struct log_zmq_object_t* log_zmq_object = (struct log_zmq_object_t*)_log_zmq_object;
	printf("ZMQ poll [%d]\n", log_zmq_object->level);
	BUS_CHECK(log_zmq_object->interface.poll(log_zmq_object->This), "Cannot poll");

	return STATUS_OK;

error:
	return STATUS_ERR;
}


int i_log_zmq_check_polling_in(void* _log_zmq_object)
{
	struct log_zmq_object_t* log_zmq_object = (struct log_zmq_object_t*)_log_zmq_object;
	printf("ZMQ check polling in [%d]\n", log_zmq_object->level);
	int state = log_zmq_object->interface.check_poll_in(log_zmq_object->This);
	return state;
}


int i_log_zmq_check_sock_polling_in(void* _log_zmq_object)
{
	struct log_zmq_object_t* log_zmq_object = (struct log_zmq_object_t*)_log_zmq_object;
	printf("ZMQ check socket polling in [%d]\n", log_zmq_object->level);
	int state = log_zmq_object->interface.check_sock_poll_in(log_zmq_object->This);
	return state;
}


int i_log_zmq_check_get_fd(void* _log_zmq_object, int* _fd)
{
	struct log_zmq_object_t* log_zmq_object = (struct log_zmq_object_t*)_log_zmq_object;
	printf("ZMQ check get fd in [%d]\n", log_zmq_object->level);
	BUS_CHECK(log_zmq_object->interface.get_event_fd(log_zmq_object->This, _fd), "Cannot get fd");

	return STATUS_OK;

error:
	return STATUS_ERR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BUS_STATUS dummy_destroy(void* object)
{
	printf("dummy destroy\n");
	return STATUS_OK;
}

BUS_STATUS dummy_connect(void* object, const char* url)
{
	printf("dummy connect\n");
	return STATUS_OK;
}


BUS_STATUS dummy_close(void* object)
{
	printf("dummy close\n");
	return STATUS_OK;
}


BUS_STATUS dummy_setsockopt(void* object, int option_name, const void* option_value, size_t option_len)
{
	printf("dummy setsockopt\n");
	return STATUS_OK;
}


BUS_STATUS dummy_msg_send(void* object, void* message, size_t message_size, int send_more)
{
	printf("dummy msg send\n");
	return STATUS_OK;
}


BUS_STATUS dummy_msg_recv(void* object, void** message, size_t* size)
{
	printf("dummy msg recv\n");

	*message = NULL;
	*size = 0;
	return STATUS_OK;
}


BUS_STATUS dummy_pollinit(void* object)
{
	printf("dummy pollinit\n");

	return STATUS_OK;
}


BUS_STATUS dummy_poll(void* object)
{
	printf("dummy poll\n");
	return STATUS_OK;
}


BUS_STATUS dummy_get_fd(void* object, int* fd)
{
	printf("dummy get_event_fd\n");
	return STATUS_OK;
}


int dummy_check_polling_in(void* object)
{
	printf("dummy check polling\n");
	return 1;
}


int dummy_check_sock_polling_in(void* object)
{
	printf("dummy check sock polling\n");
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////

BUS_STATUS check_object_init(struct msg_seq_item_t *msg_seq, int msg_seq_size, int log_size, struct check_correct_object_t** check_object)
{

	struct check_correct_object_t* t = (struct check_correct_object_t*)malloc(sizeof(struct check_correct_object_t));
	if (!t)
		RM_FAIL("Cannot allocate memory for check_object");
	memset(t, 0, sizeof(struct check_correct_object_t));

	printf("Check object init\n");

	enum call_type* call_log = (enum call_type*)malloc(log_size * sizeof(enum call_type));
	memset(call_log, 0, log_size * sizeof(enum call_type));

	struct msg_seq_item_t* seq = (struct msg_seq_item_t*)malloc(msg_seq_size * sizeof(struct msg_seq_item_t));
	int i;
	for (i = 0; i < msg_seq_size; i++) {
		int s = msg_seq[i].msg_size;
		void *m = malloc(s);
		memcpy(m, msg_seq[i].msg, s);

		seq[i].msg = m;
		seq[i].msg_size = s;
	}

	t->call_log = call_log;
	t->log_pos = 0;
	t->log_size = log_size;
	t->msg_seq = seq;
	t->msg_pos = 0;
	t->msg_seq_size = msg_seq_size;

	*check_object = t;
	return STATUS_OK;

error:
	return STATUS_ERR;
}

void check_object_done(struct check_correct_object_t* check_object) {
	int i;
	for (i = 0; i < check_object->msg_seq_size; i++)
		free((void *)check_object->msg_seq[i].msg);
	free(check_object->call_log);
	free(check_object);
}

BUS_STATUS check_destroy(void* _object)
{
	printf("object destroy\n");
	return STATUS_OK;
}

BUS_STATUS check_connect(void* _object, const char* url)
{
	struct check_correct_object_t* object = (struct check_correct_object_t*)_object;
	printf("check connect\n");

	if (object->log_pos >= object->log_size)
		RM_FAIL("check_connect: Log size limit");

	object->call_log[object->log_pos++] = CONNECT;

	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS check_close(void* _object)
{
	struct check_correct_object_t* object = (struct check_correct_object_t*)_object;
	printf("check close\n");

	if (object->log_pos >= object->log_size)
		RM_FAIL("check_close: Log size limit");
	object->call_log[object->log_pos++] = CLOSE;

	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS check_setsockopt(void* _object, int option_name, const void* option_value, size_t option_len)
{
	struct check_correct_object_t* object = (struct check_correct_object_t*)_object;
	printf("check setsockopt\n");

	if (object->log_pos >= object->log_size)
		RM_FAIL("check_setsockopt: Log size limit");
	object->call_log[object->log_pos++] = SETSOCKOPT;

	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS check_msg_send(void* _object, void* message, size_t message_size, int send_more)
{
	struct check_correct_object_t* object = (struct check_correct_object_t*)_object;

	printf("check msg send\n");

	if (object->log_pos >= object->log_size)
		RM_FAIL("check_msg_send: Log size limit");
	if (object->msg_pos >= object->msg_seq_size)
		RM_FAIL("check_msg_send: msg seq size limit");

	object->call_log[object->log_pos++] = SEND;

	if (object->msg_seq[object->msg_pos].msg_size != message_size) {
		printf("ethalon message=");
		print_char_buf((const char*)object->msg_seq[object->msg_pos].msg, object->msg_seq[object->msg_pos].msg_size);
		printf("sended message=");
		print_char_buf(message, message_size);
		printf("ethalon size=%zu; sended size=%zu\n", object->msg_seq[object->msg_pos].msg_size, message_size);
		RM_FAIL("check_msg_send: Wrong message size");
	}
	if (memcmp(object->msg_seq[object->msg_pos].msg, message, message_size)) {
		printf("ethalon message=");
		print_char_buf((const char*)object->msg_seq[object->msg_pos].msg, object->msg_seq[object->msg_pos].msg_size);
		printf("sended message=");
		print_char_buf(message, message_size);
		RM_FAIL("check_msg_send: Wrong message");
	}
	object->msg_pos++;

	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS check_msg_recv(void* _object, void** message, size_t* size)
{
	struct check_correct_object_t* object = (struct check_correct_object_t*)_object;
	printf("check msg recv\n");

	if (object->log_pos >= object->log_size)
		RM_FAIL("check_msg_recv: Log size limit");
	if (object->msg_pos >= object->msg_seq_size)
		RM_FAIL("check_msg_recv: msg seq size limit");

	object->call_log[object->log_pos++] = RECV;

	size_t _size = object->msg_seq[object->msg_pos].msg_size;
	char *_message = (char *)malloc(_size * sizeof(char));
	memcpy(_message, object->msg_seq[object->msg_pos].msg, _size);

	object->msg_pos++;

	*message = _message;
	*size = _size;
	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS check_pollinit(void* _object)
{
	struct check_correct_object_t* object = (struct check_correct_object_t*)_object;
	printf("check pollinit\n");

	if (object->log_pos >= object->log_size)
		RM_FAIL("check_pollinit: Log size limit");
	object->call_log[object->log_pos++] = POLLINIT;

	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS check_poll(void* _object)
{
	struct check_correct_object_t* object = (struct check_correct_object_t*)_object;
	printf("check poll\n");

	if (object->log_pos >= object->log_size)
		RM_FAIL("check_poll: Log size limit");
	object->call_log[object->log_pos++] = POLL;

	return STATUS_OK;

error:
	return STATUS_ERR;
}


int check_check_polling_in(void* _object)
{
	struct check_correct_object_t* object = (struct check_correct_object_t*)_object;
	printf("check check polling in\n");

	if (object->log_pos >= object->log_size)
		RM_FAIL("check_check_polling_in: Log size limit");
	object->call_log[object->log_pos++] = CHECK_POLL_IN;

	return 1;

error:
	return -1;
}


int check_check_sock_polling_in(void* _object)
{
	struct check_correct_object_t* object = (struct check_correct_object_t*)_object;
	printf("check check sock polling in\n");

	if (object->log_pos >= object->log_size)
		RM_FAIL("check_check_polling_in: Log size limit");
	object->call_log[object->log_pos++] = CHECK_SOCK_POLL_IN;

	return 1;

error:
	return -1;
}


BUS_STATUS check_get_fd(void* _object, int* _fd)
{
	struct check_correct_object_t* object = (struct check_correct_object_t*)_object;
	printf("check getting fd\n");

	if (object->log_pos >= object->log_size)
		RM_FAIL("check_get_fd: Log size limit");
	object->call_log[object->log_pos++] = GET_FD;

	*_fd = 0;
	return STATUS_OK;

error:
	*_fd = -1;
	return STATUS_ERR;
}
