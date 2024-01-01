#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define BUFFER_SIZE 1024

int socket_fd;
char username[256];


void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void handle_server_message()
{
    char buffer[BUFFER_SIZE];

        while(1)
        {
                memset(buffer, 0, BUFFER_SIZE);
                int n = read(socket_fd, buffer, BUFFER_SIZE);
                if (n < 0)
                {
                        error("ERROR reading from socket");
                }
                else if (n == 0)
                {
                        printf("Server has disconnected\n");
                        exit(0);
                }
                else
                {
                        buffer[n] = '\0';
                        printf("%s\n", buffer);
                        fflush(stdout);
                }
        }
}

void send_message(char* message)
{
    int n = write(socket_fd, message, strlen(message));

    if (n < 0)
        {
        error("ERROR writing to socket");
    }
}

void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\nReceived SIGINT signal\n");
        close(socket_fd);
        exit(0);
    }
}

void handle_user_input()
{
    char buffer[BUFFER_SIZE];
	int flag=0;

    while (1)
    {
        fgets(buffer, BUFFER_SIZE, stdin);

        buffer[strcspn(buffer, "\n")] = '\0';

        if (strncmp(buffer, "login ", 6) == 0 && flag==0)
        {
            sprintf(username, "%s", buffer+6);
            char login_message[BUFFER_SIZE];
			flag=1;
            sprintf(login_message, "login %s", username);
            send_message(login_message);
        }
        else if (strcmp(buffer, "logout") == 0 && flag==1)
        {
			flag=0;
            send_message("logout");
        }
        else if (strncmp(buffer, "chat ", 5) == 0 && flag==1)
        {
            char chat_message[BUFFER_SIZE];
            sprintf(chat_message, "chat %s", buffer+5);
            send_message(chat_message);
        }
        else if (strcmp(buffer,"exit")==0 && flag==0)
        {
                        close(socket_fd);
                        exit(0);
        }
        else
        {
            printf("Invalid command\n");
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        {
        fprintf(stderr,"Usage: %s <configuration_file>\n", argv[0]);
        exit(1);
    }

        signal(SIGINT, signal_handler);

    char servhost[256];
    int servport = -1;

    FILE *config_file = fopen(argv[1], "r");
    if (config_file == NULL)
    {
        error("ERROR opening configuration file");
    }

    char line[BUFFER_SIZE];
    char *token;

    while (fgets(line, BUFFER_SIZE, config_file) != NULL)
    {
        token = strtok(line, ":\n");
        if (token == NULL)
        {
            continue;
        }

        if (strcmp(token, "servhost") == 0)
        {
            token = strtok(NULL, ":\n");
            if (token == NULL)
            {
                continue;
            }
            sprintf(servhost, "%s", token);
        }
        else if (strcmp(token, "servport") == 0)
        {
            token = strtok(NULL, ":\n");
            if (token == NULL)
            {
                continue;
            }
            servport = atoi(token);
        }
        else
        {
            continue;
        }
    }

    fclose(config_file);

    if (servport == -1)
    {
        error("ERROR: Invalid configuration file - servport is missing");
    }

    if (strlen(servhost) == 0)
    {
        error("ERROR: Invalid configuration file - servhost is missing");
    }

    struct sockaddr_in server_addr;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
        {
        error("ERROR opening socket");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(servhost);
    server_addr.sin_port = htons(servport);

    if (server_addr.sin_addr.s_addr == INADDR_NONE)
{
    error("ERROR: invalid server address");
        exit(1);
}

    if (server_addr.sin_port == 0)
        {
                error("ERROR: Invalid port number");
                exit(1);
        }
    if (connect(socket_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
        {
        error("ERROR connecting to server");
    }

    pthread_t input_thread, message_thread;
    pthread_create(&input_thread, NULL, (void*) &handle_user_input, NULL);
    pthread_create(&message_thread, NULL, (void*) &handle_server_message, NULL);

    while (1)
        {
    }

    close(socket_fd);

    return 0;
}
