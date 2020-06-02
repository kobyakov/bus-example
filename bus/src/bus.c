#include <zmq.h>
#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "bus_common.h"
#include "bus_client.h"
#include "bus_server.h"
#include "private/bus_impl.h"
#include "private/bus_zmq.h"

static BUS_STATUS create_unique_name(const char* address, char** result)
{
	const size_t maxname_size = 1024;
	char* name = NULL;
	name = (char*)malloc(maxname_size * sizeof(char));
	if (!name) BUS_FAIL("create_unique_name: cannot allocate buffer");
	const unsigned int pid = (unsigned int)getpid();
	const unsigned int sid = (unsigned int)getsid(0);
	const int written = snprintf(name, maxname_size, "%s_%u_%u"
	                             , address
	                             , sid
	                             , pid
	                            );
	if ((written < 0) || (written >= (int)maxname_size))
		BUS_FAIL("create_unique_name: cannot format name");

	*result = name;
	return STATUS_OK;

error:
	if (name) free(name);
	*result = NULL;
	return STATUS_ERR;
}


BUS_STATUS bus_client_init(struct bus_client_t** bus)
{
	struct bus_client_t* bus_ = (struct bus_client_t*)malloc(sizeof(struct bus_client_t));
	if (!bus_) BUS_FAIL("Cannot allocate memory for bus");
	BUS_CHECK(bus_init(BUS_CLIENT, &bus_->base_bus), "Cannot init bus");
	*bus = bus_;
	
	return STATUS_OK;

error:
	if (bus_) free(bus_);
	bus_ = NULL;
	return STATUS_ERR;
}


BUS_STATUS bus_client_done(struct bus_client_t* bus)
{
	BUS_NULL_POINTER_CHECK(bus);
	BUS_CHECK(bus_done(bus->base_bus), "Cannot done base bus");
	free(bus);
	
	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS register_client(struct bus_client_t* bus, const char* address)
{
	BUS_NULL_POINTER_CHECK(bus);
	char* unique_address = NULL;
	BUS_CHECK(create_unique_name(address, &unique_address), "Cannot get unique name");
	BUS_CHECK(send_register_messages(bus->base_bus, unique_address), "Cannot send register client message");
	bus->base_bus->address = address;

	free(unique_address);
	return STATUS_OK;

error:
	if (unique_address) free(unique_address);
	return STATUS_ERR;
}


BUS_STATUS request(const struct bus_client_t* bus, const char* address, 
	               const void* request, size_t req_size, 
	               void** reply, size_t* rep_size) 
{
	BUS_NULL_POINTER_CHECK(bus);
	void* reply_message = NULL;
	size_t reply_size = 0;

	void* error_message = NULL;
	size_t error_size = 0;

	char m = MSG_BUS_QUERY;
	BUS_CHECK(send_message(bus->base_bus, &m, 1, bus->base_bus->options.sendmore), "Cannot send type message");
	BUS_CHECK(send_message(bus->base_bus, address, strlen(address), bus->base_bus->options.sendmore), "Cannot send address message");
	BUS_CHECK(send_message(bus->base_bus, request, req_size, 0), "Cannot send request message");

	enum bus_status_message_type status_message_type;
	BUS_CHECK(recv_status(bus->base_bus, &status_message_type), "Cannot recv status message");
	if (status_message_type == UNDEF_MESSAGE)
		BUS_FAIL("Wrong status message");

	switch (status_message_type) {
		case REPLY_MESSAGE:
			BUS_CHECK(recv_message(bus->base_bus, &reply_message, &reply_size), "Cannot receive reply message");
			// Если пришло пустое сообщение - ошибка обработчика
			if (!reply_message)
				BUS_FAIL("Error handling request");
			break;
		case ERROR_MESSAGE:
			BUS_CHECK(recv_message(bus->base_bus, &error_message, &error_size), "Cannot get error message");
			bus_log(LOG_ERR, "%.*s", error_size, (const char*)error_message);
			BUS_FAIL("Send request error");
			break;
		default:
			BUS_FAIL("send_request: Wrong status message type");
			break;
	}
	if (error_message)
		free(error_message);
	
	*reply = reply_message;
	*rep_size = reply_size;
	return STATUS_OK;

error:
	if (reply_message)
		free(reply_message);
	if (error_message)
		free(error_message);
	*reply = NULL;
	*rep_size = 0;
	return STATUS_ERR;
}


BUS_STATUS bus_server_init(struct bus_server_t** bus)
{
	struct bus_server_t* bus_ = (struct bus_server_t*)malloc(sizeof(struct bus_server_t));
	if (!bus_) BUS_FAIL("Cannot allocate memory for bus");
	BUS_CHECK(bus_init(BUS_SERVER, &bus_->base_bus), "Cannot init bus");
	*bus = bus_;
	
	return STATUS_OK;

error:
	if (bus_) free(bus_);
	bus_ = NULL;
	return STATUS_ERR;
}


BUS_STATUS bus_server_done(struct bus_server_t* bus)
{
	BUS_NULL_POINTER_CHECK(bus);
	BUS_CHECK(bus != NULL, "bus NULL");
	BUS_CHECK(bus->base_bus != NULL, "base bus NULL");
	BUS_CHECK(bus_done(bus->base_bus), "Cannot done base bus");
	free(bus);
	
	return STATUS_OK;

error:
	return STATUS_ERR;
}


BUS_STATUS register_server(struct bus_server_t* bus, const char* address, bus_handler* handler)
{
	BUS_NULL_POINTER_CHECK(bus);
	BUS_CHECK(send_register_messages(bus->base_bus, address), "Cannot send register client message");
	bus->base_bus->address = address;
	bus->handler = handler;
	
	return STATUS_OK;

error:
	return STATUS_ERR;
}


static BUS_STATUS bus_message_handle(const struct bus_server_t* bus, int *scheduled_exit)
{
	BUS_NULL_POINTER_CHECK(bus);
	void   *envelope_data = NULL;
	size_t  envelope_size = 0;
	void   *request_data  = NULL;
	size_t  request_size  = 0;
	void   *response_data = NULL;
	size_t  response_size = 0;
	char   *topic         = NULL;

	BUS_STATUS handler_result = STATUS_ERR;
   	BUS_CHECK(recv_message(bus->base_bus, &envelope_data, &envelope_size), "Cannot receive envelope data");
	BUS_CHECK(recv_message(bus->base_bus, &request_data, &request_size), "Cannot receive request data");
	handler_result = bus->handler(request_data, request_size, &response_data, &response_size);
	BUS_CHECK(send_reply(bus->base_bus, envelope_data, envelope_size, response_data, response_size), "Cannot send reply");
    BUS_CHECK(handler_result, "Cannot handle request");

	if ( topic         ) { free(topic);         }
	if ( envelope_data ) { free(envelope_data); }
	if ( request_data  ) { free(request_data);  }
	if ( response_data ) { free(response_data); }

  	*scheduled_exit = ( STATUS_FALSE == handler_result ) ? TRUE : FALSE;
	return STATUS_OK;

error:
	if ( topic         ) { free(topic);         }
	if ( envelope_data ) { free(envelope_data); }
	if ( request_data  ) { free(request_data);  }
	if ( response_data ) { free(response_data); }

  	*scheduled_exit  = FALSE; 
	return STATUS_ERR;
}


BUS_STATUS loop(const struct bus_server_t* bus)
{
	BUS_NULL_POINTER_CHECK(bus);
  	int scheduled_exit = FALSE;

	BUS_CHECK(bus_pollinit(bus->base_bus), "Cannot initialization bus polling");

	while ( 1 ) {
		BUS_CHECK(bus_poll(bus->base_bus), "Cannot bus polling");

		if ( bus_check_poll_in(bus->base_bus) ) {
		  BUS_CHECK(bus_message_handle(bus, &scheduled_exit), "Cannot handle receive message");
      if ( scheduled_exit ) {	break; }
		}
	}

	return STATUS_OK;

error:
	return STATUS_ERR;
}
