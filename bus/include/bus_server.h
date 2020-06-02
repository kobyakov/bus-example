#ifndef BUS_SERVER_H
#define BUS_SERVER_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bus_common.h"

struct bus_server_t;
typedef BUS_STATUS bus_handler(void *request, size_t req_size, void **reply, size_t *rep_size);

BUS_STATUS bus_server_init(struct bus_server_t** bus);
BUS_STATUS bus_server_done(struct bus_server_t* bus);
BUS_STATUS register_server(struct bus_server_t* bus, const char* address, bus_handler* handler);
BUS_STATUS loop(const struct bus_server_t* bus);


#endif