TARGET = test

CC = gcc

CFLAGS = -Wall -Wextra -g -Iinclude -I../ks

SRCS_LIB = $(wildcard src/*.c)

SRCS_APP = main.c

SRCS = $(SRCS_LIB) $(SRCS_APP)

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean