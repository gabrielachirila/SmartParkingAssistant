# Makefile

all: 
	gcc server.c server_functions.c database_functions.c -o server -lsqlite3 -pthread
	gcc client.c client_functions.c -o client
clean:
	rm -f server client
