#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>
#include<signal.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct
{
    int socket_fd;
    char username[256];
} client_info;

int server_fd;
client_info clients[MAX_CLIENTS];
fd_set read_fds, master_fds;

void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\nReceived SIGINT signal\n");
        close(server_fd);
        exit(0);
    }
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void remove_client(int index)
{
    close(clients[index].socket_fd);
    FD_CLR(clients[index].socket_fd, &master_fds);
    clients[index].socket_fd = -1;
    clients[index].username[0] = '\0';
}

void handle_client_message(int index)
{
    int socket_fd = clients[index].socket_fd;
    char buffer[BUFFER_SIZE];
    int n = read(socket_fd, buffer, BUFFER_SIZE);
    int i;
    if (n < 0)
        {
        error("ERROR reading from socket");
    }
        else if (n == 0)
        {
        printf("Client %s has disconnected\n", clients[index].username);
        remove_client(index);
    }
        else
        {
        buffer[n] = '\0';

        if (strncmp(buffer, "login ", 6) == 0)
                {
                                                        sscanf(buffer, "login %s", clients[index].username);
                                                        printf("Client %s has logged in\n", clients[index].username);
        }
                else if (strcmp(buffer, "logout") == 0)
                {
            printf("Client %s has logged out\n", clients[index].username);
                        clients[index].username[0] = '\0';
        }
                else if (strncmp(buffer, "chat ", 5) == 0)
                {
            char recipient[256], message[BUFFER_SIZE], check[BUFFER_SIZE];
            sscanf(buffer, "chat %[^\n]", check);
                        if(clients[index].username[0]!='\0')
                        {
                                if (check[0] == '@')
                                {
                                        sscanf(buffer, "chat %s %[^\n]", recipient, message);
                                        for (i = 0; i < MAX_CLIENTS; i++)
                                        {
                                                if (i != index && clients[i].socket_fd != -1 && strcmp(clients[i].username, recipient+1) == 0)
                                                {
                                                        char formatted_message[BUFFER_SIZE];
                                                        snprintf(formatted_message, BUFFER_SIZE, "%s >> %s", clients[index].username, message);
                                                        write(clients[i].socket_fd, formatted_message, strlen(formatted_message));
                                                        break;
                                                }
                                        }
                                }
                                else
                                {
                                        sscanf(buffer, "chat %[^\n]", message);
                                        for (i = 0; i < MAX_CLIENTS; i++)
                                        {
                                                if (i != index && clients[i].socket_fd != -1 && clients[i].username[0]!='\0')
                                                {
                                                        char formatted_message[BUFFER_SIZE];
                                                        snprintf(formatted_message, BUFFER_SIZE, "%s >> %s", clients[index].username, message);
                                                        write(clients[i].socket_fd, formatted_message, strlen(formatted_message));
                                                }
                                        }
                                }
                        }
						else
						{
							char formatted_message[BUFFER_SIZE];
							snprintf(formatted_message, BUFFER_SIZE, "Please Login to chat");
							write(clients[index].socket_fd,formatted_message,strlen(formatted_message));
						}
        }
               
                else
                {
            write(socket_fd, "Invalid command\n", 16);
        }
    }
}

int main(int argc, char *argv[])
{
    int port;
    int i,j;
        signal(SIGINT, signal_handler);
        FILE *fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
        error("Error opening configuration file");
    }

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, fp) != NULL)
    {
        char* key = strtok(buffer, ":");
        char* value = strtok(NULL, "\n");
        if (strcmp(key, "port") == 0)
        {
            port = atoi(value);
        }
    }
    fclose(fp);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        {
        error("ERROR opening socket");
    }

    int optval = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        {
        error("ERROR setting socket options");
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        {
        error("ERROR on binding");
    }

        if (port == 0) {
    socklen_t len = sizeof(server_addr);
    if (getsockname(server_fd, (struct sockaddr *)&server_addr, &len) < 0) {
        perror("getsockname");
        exit(EXIT_FAILURE);
    }
}

    if (listen(server_fd, MAX_CLIENTS) < 0)
        {
        error("ERROR on listen");
    }

    printf("Server listening on %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].socket_fd = -1;
        clients[i].username[0] = '\0';
    }
    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);

    while (1)
        {
        read_fds = master_fds;
        if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0)
                {
            error("ERROR on select");
        }

        for (i = 0; i < FD_SETSIZE; i++)
                {
            if (FD_ISSET(i, &read_fds))
                        {
                if (i == server_fd)
                                {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_len);

                    if (client_fd < 0)
                                        {
                        error("ERROR on accept");
                    }
                                        else
                                        {
                        int index = -1;
                        for (j = 0; j < MAX_CLIENTS; j++)
                                                {
                            if (clients[j].socket_fd == -1)
                                                        {
                                index = j;
                                break;
                            }
                        }
                        if (index < 0)
                                                {
                            error("ERROR: too many clients");
                                                }
                                                else
                                                {
                            clients[index].socket_fd = client_fd;
                            FD_SET(client_fd, &master_fds);
                            printf("New client connected\n");
                        }
                    }
                }
                                else
                                {
                    int index = -1;
                    for (j = 0; j < MAX_CLIENTS; j++)
                                        {
                        if (clients[j].socket_fd == i)
                                                {
                            index = j;
                            break;
                        }
                    }
                    if (index < 0)
                                        {
                        error("ERROR: client not found");
                    }
                                        else
                                        {
                        handle_client_message(index);
                    }
                }
            }
        }
    }
    return 0;
}

