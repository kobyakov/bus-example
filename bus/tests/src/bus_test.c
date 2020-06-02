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

// Пример использования интерфейса декоратора
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
	printf("Correct call test\n");
	BUS_CHECK(correct_call_client(), "Correct call client test failed");
	printf("Correct call test completed!\n\n");

	return STATUS_OK;

error:
	return STATUS_ERR;
}