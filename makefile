CC = gcc
CFLAGS = -Wall -Wextra

all: chat_server.x chat_client.x

chat_server.x: chat_server.c
	$(CC) $(CFLAGS) -o chat_server.x chat_server.c

chat_client.x: chat_client.c
	$(CC) $(CFLAGS) -o chat_client.x chat_client.c -pthread

clean:
	rm -f chat_server.x chat_client.x
