SRCDIR = src
LIBSRC = ../src
INCLUDE = -Iinclude -I../include
CFLAGS = -Wall -Werror -g
LIBS = -lzmq -lprotobuf-c

.PHONY: all clean buildtest-TESTS 

all: buildtest-TESTS

buildtest-TESTS:
	$(CC) $(CFLAGS) $(INCLUDE) $(LDFLAGS) -o bus_test $(SRCDIR)/*.c $(LIBSRC)/*.c $(LIBS)

clean:
	rm -rf *.o bus_test
	