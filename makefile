CC = gcc
CFLAGS = -Wall -g

SRCS = server_connection.c config_reader.c ip_pool_manager.c

OBJS = server_connection.o config_reader.o ip_pool_manager.o

# executabil
EXEC = server

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC)

server_connection.o: server_connection.c
	$(CC) $(CFLAGS) -c server_connection.c -o server_connection.o

config_reader.o: config_reader.c
	$(CC) $(CFLAGS) -c config_reader.c -o config_reader.o

ip_pool_manager.o: ip_pool_manager.c
	$(CC) $(CFLAGS) -c ip_pool_manager.c -o ip_pool_manager.o

clean:
	rm -f $(OBJS) $(EXEC)

