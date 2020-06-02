#ifndef BUS_TEST_IF_H
#define BUS_TEST_IF_H

#include "bus_common.h"
#include "bus_zmq.h"
#include "bus_impl.h"

#define MAX_LOG_POS			64
#define MAX_MSG_SEQ_SIZE	64

struct log_zmq_object_t {
	void* This;
	int level;
	struct bus_socket_interface_t interface;
};

enum correct_object_state {
	INIT_STATE,
	HELLO_STATE,
	QUERY_STATE
};

enum call_type {
	CONNECT,
	CLOSE,
	SEND,
	RECV,
	POLLINIT,
	POLL,
	CHECK_POLL_IN,
	CHECK_SOCK_POLL_IN,
	SETSOCKOPT,
	GET_FD
};

struct msg_seq_item_t {
	const void *msg;
	size_t msg_size;
};

struct check_correct_object_t {
	enum call_type* call_log;
	int log_pos;
	int log_size;
	struct msg_seq_item_t *msg_seq;
	int msg_pos;
	int msg_seq_size;
};


extern struct bus_socket_interface_t log_zmq_interface;
extern struct bus_socket_interface_t dummy_interface;
extern struct bus_socket_interface_t check_correct_interface;

BUS_STATUS log_zmq_object_init(struct bus_socket_interface_t* decoration_interface, void* decoration_This, int level, struct log_zmq_object_t** log_zmq_object);
BUS_STATUS log_zmq_object_destroy(void* _zmq_object);

BUS_STATUS i_log_zmq_connect(void* _zmq_object, const char* url);
BUS_STATUS i_log_zmq_close(void* _zmq_object);
BUS_STATUS i_log_zmq_setsockopt(void* _zmq_object, int option_name, const void* option_value, size_t option_len);
BUS_STATUS i_log_zmq_msg_send(void* zmq_object, void* message, size_t message_size, int send_more);
BUS_STATUS i_log_zmq_msg_recv(void* zmq_object, void** message, size_t* size);
BUS_STATUS i_log_zmq_pollinit(void* _zmq_object);
BUS_STATUS i_log_zmq_poll(void* _zmq_object);
int i_log_zmq_check_get_fd(void* _log_zmq_object, int* _fd);
int i_log_zmq_check_polling_in(void* _zmq_object);
int i_log_zmq_check_sock_polling_in(void* _log_zmq_object);

BUS_STATUS dummy_destroy(void *object);
BUS_STATUS dummy_connect(void* object, const char* url);
BUS_STATUS dummy_close(void* object);
BUS_STATUS dummy_setsockopt(void* object, int option_name, const void* option_value, size_t option_len);
BUS_STATUS dummy_msg_send(void* object, void* message, size_t message_size, int send_more);
BUS_STATUS dummy_msg_recv(void* object, void** message, size_t* size);
BUS_STATUS dummy_pollinit(void* object);
BUS_STATUS dummy_poll(void* object);
BUS_STATUS dummy_get_fd(void* object, int* fd);
int dummy_check_polling_in(void* object);
int dummy_check_sock_polling_in(void* object);

BUS_STATUS check_object_init(struct msg_seq_item_t *msg_seq, int msg_seq_size, int log_size, struct check_correct_object_t** check_object);
void check_object_done(struct check_correct_object_t* check_object);
BUS_STATUS check_destroy(void *object);
BUS_STATUS check_connect(void *object, const char *url);
BUS_STATUS check_close(void* object);
BUS_STATUS check_setsockopt(void* object, int option_name, const void* option_value, size_t option_len);
BUS_STATUS check_msg_send(void* object, void *message, size_t message_size, int send_more);
BUS_STATUS check_msg_recv(void* object, void **message, size_t *size);
BUS_STATUS check_pollinit(void *object);
BUS_STATUS check_poll(void *object);
BUS_STATUS check_get_fd(void* _object, int* fd_);
int check_check_polling_in(void *object);
int check_check_sock_polling_in(void* _object);


#endif