#ifndef BUS_CLIENT_H
#define BUS_CLIENT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bus_common.h"

struct bus_client_t;

BUS_STATUS bus_client_init(struct bus_client_t** bus);
BUS_STATUS bus_client_done(struct bus_client_t* bus);
BUS_STATUS register_client(struct bus_client_t* bus, const char* address);
BUS_STATUS request(const struct bus_client_t* bus, const char* address,
                   const void* request, size_t req_size,
                   void** reply, size_t* rep_size);



#endif