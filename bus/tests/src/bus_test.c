#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "bus_common.h"
#include "bus_impl.h"
#include "bus_handlers.h"
#include "bus_test_interfaces.h"

/*void xdebug(int UNUSED(level), const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char buf[512];
	vsnprintf(buf, sizeof(buf), format, args);
	printf("%s\n", buf);
	va_end(args);
}*/

BUS_STATUS hello_handler(void* request, size_t req_size, void** reply, size_t* rep_size)
{
	xdebug(LOG_DEBUG, "testing_handler_1");
	xdebug_buf(LOG_DEBUG, request, req_size);
	*reply = strdup("Hello, client!");
	*rep_size = strlen(*reply);
	return STATUS_OK;
}

BUS_STATUS goodbye_handler(void* request, size_t req_size, void** reply, size_t* rep_size)
{
	xdebug(LOG_DEBUG, "testing_handler_2");
	xdebug_buf(LOG_DEBUG, request, req_size);
	*reply = strdup("Goodbye, client!");
	*rep_size = strlen(*reply);
	return STATUS_OK;
}

BUS_STATUS whatsup_handler(void* request, size_t req_size, void** reply, size_t* rep_size)
{
	xdebug(LOG_DEBUG, "testing_handler_3");
	xdebug_buf(LOG_DEBUG, request, req_size);
	*reply = strdup("What's up, client?");
	*rep_size = strlen(*reply);
	return STATUS_OK;
}

BUS_STATUS dummy_bus_init(enum bus_type type, struct bus_t** bus)
{
	struct bus_socket_interface_t interface = dummy_interface;
	struct bus_options_t opts = zmq_opts;
	interface.This = NULL;

	struct bus_t* _bus;
	BUS_CHECK(_bus_init(type, &interface, &opts, &_bus), "Cannot init bus");

	*bus = _bus;
	return STATUS_OK;

error:
	if (bus)
		free(bus);
	return STATUS_ERR;
}

BUS_STATUS handlers_test()
{
	struct bus_t* dummy_bus = NULL;
	BUS_CHECK(dummy_bus_init(BUS_CLIENT, &dummy_bus), "Cannot init bus");

	struct address_handler_t* a;

	add_handler(dummy_bus, "hello", hello_handler, NULL, BUS_SERVER);
	assert(get_handlers_list_size(dummy_bus) == 1);

	add_handler(dummy_bus, "goodbye", goodbye_handler, NULL, BUS_SERVER);
	assert(get_handlers_list_size(dummy_bus) == 2);

	add_handler(dummy_bus, "whatsup", whatsup_handler, NULL, BUS_SERVER);
	assert(get_handlers_list_size(dummy_bus) == 3);

	get_address_handler_item(dummy_bus, "hello", &a);
	assert(a->address_handler == hello_handler);

	get_address_handler_item(dummy_bus, "goodbye", &a);
	assert(a->address_handler == goodbye_handler);

	get_address_handler_item(dummy_bus, "whatsup", &a);
	assert(a->address_handler == whatsup_handler);

	remove_handler(dummy_bus, "hello");
	assert(get_handlers_list_size(dummy_bus) == 2);

	remove_handler(dummy_bus, "goodbye");
	assert(get_handlers_list_size(dummy_bus) == 1);

	remove_handler(dummy_bus, "whatsup");
	assert(get_handlers_list_size(dummy_bus) == 0);

	BUS_CHECK(bus_done(dummy_bus), "Cannot stop bus");

	return STATUS_OK;

error:
	return STATUS_ERR;
}

BUS_STATUS decorator_bus_init(enum bus_type type, struct bus_t** bus)
{
	struct zmq_object_t* zmq_object = NULL;
	struct bus_t* _bus = NULL;

	struct bus_options_t opts = zmq_opts;
	BUS_CHECK(zmq_object_init(&zmq_object), "Cannot init zmq object");
	struct bus_socket_interface_t interface = zmq_interface;
	interface.This = zmq_object;

	struct bus_socket_interface_t decorators_interface[11];
	struct log_zmq_object_t* log_zmq_object[11];

	BUS_CHECK(log_zmq_object_init(&interface, zmq_object, 0, &log_zmq_object[0]), "Cannot init log zmq object");

	decorators_interface[0] = log_zmq_interface;
	decorators_interface[0].This = log_zmq_object[0];

	int i;
	for (i = 1; i < 11; i++) {
		BUS_CHECK(log_zmq_object_init(&decorators_interface[i - 1], log_zmq_object[i - 1], i, &log_zmq_object[i]), "Cannot init log zmq object");
		decorators_interface[i] = log_zmq_interface;
		decorators_interface[i].This = log_zmq_object[i];
	}

	BUS_CHECK(_bus_init(type, &decorators_interface[10], &opts, &_bus), "Cannot init bus");

	*bus = _bus;
	return STATUS_OK;

error:
	if (zmq_object)
		free(zmq_object);
	if (bus)
		free(bus);
	return STATUS_ERR;

}


// It's example of using decorator interface
int decoration_client()
{
	struct bus_t* bus = NULL;
	char* id = "TEST_CLIENT";

	char* request = NULL;
	size_t req_size = 0;

	void* reply = NULL;
	size_t rep_size = 0;


	BUS_CHECK(decorator_bus_init(BUS_CLIENT, &bus), "Cannot init bus");
	BUS_CHECK(register_client(bus, id), "Cannot register client");

	request = "Hello, server!";
	req_size = strlen(request);

	printf("Request: %s\n", request);
	BUS_CHECK(send_request(bus, "HELLO_SERVICE", request, req_size, &reply, &rep_size), "Request error");
	printf("Reply: ");
	xdebug_buf(LOG_DEBUG, reply, rep_size);

	BUS_CHECK(bus_done(bus), "Cannot stop bus");

	free(reply);
	return STATUS_OK;

error:
	return STATUS_ERR;
}

int correct_call_client()
{
	struct check_correct_object_t* check_object = NULL;

	struct bus_t* bus = NULL;
	struct bus_options_t opts = zmq_opts;

	enum call_type correct_call_sequence[] = {
		CONNECT,
		SEND,
		SEND,
		RECV,
		SEND,
		SEND,
		SEND,
		RECV,
		RECV,
		CLOSE
	};

	char* id = "TEST_CLIENT";
	char* address = "TEST_SERVICE";

	char* request = "TEST REQUEST";
	size_t request_size = strlen(request);

	char* reply = "TEST_REPLY";
	size_t reply_size = strlen(reply);


	void* test_reply = NULL;
	size_t test_reply_size = 0;

	{
		// Создадим объект-имитатор интерфейса сокета и инициализируем его корректной последовательностью сообщений
		char MSG_BUS_OK[] = { MSG_BUS_OK };
		char MSG_BUS_REPLY[] = { MSG_BUS_REPLY };

		char MSG_BUS_HELLO[] = { MSG_BUS_HELLO };
		char MSG_BUS_QUERY[] = { MSG_BUS_QUERY };


		struct msg_seq_item_t correct_message_sequence[] = {
			{ MSG_BUS_HELLO, COUNT_OF(MSG_BUS_HELLO) },
			{ id, strlen(id) },
			{ MSG_BUS_OK, COUNT_OF(MSG_BUS_OK) },
			{ MSG_BUS_QUERY, COUNT_OF(MSG_BUS_QUERY) },
			{ address, strlen(address) },
			{ request, request_size },
			{ MSG_BUS_REPLY, COUNT_OF(MSG_BUS_REPLY) },
			{ reply, reply_size }

		};

		BUS_CHECK(check_object_init(correct_message_sequence, COUNT_OF(correct_message_sequence), COUNT_OF(correct_call_sequence), &check_object), "Cannot init check object");
	}


	{
		// Выполним сценарий
		struct bus_socket_interface_t check_interface = check_correct_interface;
		check_interface.This = check_object;
		_bus_init(BUS_CLIENT, &check_interface, &opts, &bus);
		BUS_CHECK(register_client(bus, id), "Cannot register client");
		BUS_CHECK(send_request(bus, address, request, request_size, &test_reply, &test_reply_size), "Request error");
		BUS_CHECK(bus_done(bus), "Cannot stop bus");
		bus = NULL;
	}

	{
		// Проверим последовательность вызовов
		// Количество актуальных вызовов должно совпадать с эталонным
		assert(COUNT_OF(correct_call_sequence) == check_object->log_pos);

		// Последовательность вызовов должна совпадать с эталонной
		int i;
		for (i = 0; i < check_object->log_pos; i++) {
			assert(check_object->call_log[i] == correct_call_sequence[i]);
		}

		// Ответ должен совпадать с эталонным
		assert(test_reply_size == reply_size);
		assert(strncmp(reply, test_reply, reply_size) == 0);
	}

	free(test_reply);
	check_object_done(check_object);
	return STATUS_OK;

error:
	free(test_reply);
	if (bus)
		bus_done(bus);
	if (check_object)
		check_object_done(check_object);
	return STATUS_ERR;
}

BUS_STATUS main()
{
	printf("Handlers test\n");
	BUS_CHECK(handlers_test(), "Handlers test failed");
	printf("Handlers test completed!\n\n");
	printf("Correct call test\n");
	BUS_CHECK(correct_call_client(), "Correct call client test failed");
	printf("Correct call test completed!\n\n");

	return STATUS_OK;

error:
	return STATUS_ERR;
}