#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus_common.h"
#include "bus_client.h"

void print_buf(char* b, size_t s)
{
	int i;
	for (i = 0; i < s; i++) {
		printf("%c", b[i]);
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	struct bus_client_t* bus = NULL;
	const char* id = "TEST_CLIENT";
	
	BUS_CHECK(bus_client_init(&bus), "Cannot init bus");
	BUS_CHECK(register_client(bus, id), "Cannot register client");

	const char *server_id = "TEST_SERVER";	
	const char *message = "Hello, world!";
	const size_t message_size = strlen(message);

	void* reply = NULL;
	size_t rep_size = 0;

	BUS_CHECK(request(bus, server_id, message, message_size, &reply, &rep_size), "Request error");	
	printf("Reply: "); print_buf(reply, rep_size);
	free(reply);

	BUS_CHECK(bus_client_done(bus), "Cannot stop bus");

	return STATUS_OK;

error:
	return STATUS_ERR;
}