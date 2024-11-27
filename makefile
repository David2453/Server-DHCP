all: server_connection client_connection clean

server: server_connection.o
	gcc server_connection.o -o server_connection
server_connection.o:
	gcc -c server_connection.c -o server_connection.o

client: client_connection.o
	gcc client_connection.o -o client_connection
client.o:
	gcc -c client_connection.c -o client_connection.o

clean:
	rm *.o