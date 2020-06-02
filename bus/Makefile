TARGET = libbus

CC = gcc
#CFLAGS = -g -Wall -Werror
CFLAGS = -g

INCDIR = include
OBJDIR = obj
SRCDIR = src

INCLUDE = -I$(INCDIR)
LIBS = -lzmq

SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC))

.PHONY: all $(TARGET)  clean

all: $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $< $(LIBS)

$(TARGET): $(TARGET).a

$(TARGET).a: $(OBJ)
	ar rcs $@ $^


clean:
	rm -rf $(OBJDIR)/[!.]* $(TARGET).a