#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus_common.h"
#include "bus_server.h"

void print_buf(const char *b, size_t s)
{
	int i;
	for (i = 0; i < s; i++)
	{
		printf("%c", b[i]);
	}
}

BUS_STATUS handler(void *request, size_t req_size, void **reply, size_t *rep_size) {

	print_buf(request, req_size);
	memcpy(*reply, request, req_size);
	rep_size = req_size;
	return STATUS_OK;
}

int main()
{
	const char* id = "TEST_SERVER";
	struct bus_server_t *bus = NULL;

	BUS_CHECK(bus_server_init(&bus), "Cannot init bus");
	BUS_CHECK(register_server(bus, id, handler), "Cannot register server");

	while(loop(bus)) {};

	BUS_CHECK(bus_done(bus), "Cannot stop bus");
	
	return STATUS_OK;

error:
	return STATUS_ERR;
}