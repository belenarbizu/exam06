#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define MAX_CONNECTIONS 200
#define MAX_CLIENTS 100
#define BUFFER_SIZE 12000

typedef struct client
{
    int fd;
    int id;
} Client;

void print_error(char *str)
{
    write(2, str, strlen(str));
}

int main(int argc, char **argv)
{
    int server_socket;
    struct sockaddr_in server_addr;

    if (argc != 3)
    {
        print_error("Wrong number of arguments\n");
        exit(1);
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket == -1)
    {
        print_error("Fatal error\n");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        print_error("Fatal error\n");
        exit(1);
    }

    if (listen(server_socket, MAX_CONNECTIONS) == -1)
    {
        print_error("Fatal error\n");
        exit(1);
    }

    Client clients[MAX_CLIENTS];
    fd_set readfds;
    char buffer[BUFFER_SIZE];
    int max_fd = server_socket;
    int next_id = 0;

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].fd > 0)
            {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd >= max_fd)
                    max_fd = clients[i].fd;
            }
        }

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0)
        {
            print_error("Fatal error\n");
            exit(1);
        }

        if (FD_ISSET(server_socket, &readfds))
        {
            int client_fd = accept(server_socket, NULL, NULL);
            if (client_fd == -1)
            {
                print_error("Fatal error\n");
                exit(1);
            }
            clients[next_id].fd = client_fd;
            clients[next_id].id = next_id++;

            bzero(buffer, BUFFER_SIZE);
            sprintf(buffer, "server: client %d just arrived\n", next_id - 1);
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].fd > 0)
                    send(clients[i].fd, buffer, strlen(buffer), 0);
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].fd > 0 && FD_ISSET(clients[i].fd, &readfds))
            {
                bzero(buffer, BUFFER_SIZE);
                int bytes_recv = recv(clients[i].fd, buffer, strlen(buffer), 0);
                if (bytes_recv < 0)
                {
                    bzero(buffer, BUFFER_SIZE);
                    sprintf(buffer, "server: client %d just left\n", clients[i].id);
                    for (int j = 0; j < MAX_CLIENTS; j++)
                    {
                        if (clients[j].fd > 0 && j != i)
                            send(clients[j].fd, buffer, strlen(buffer), 0);
                    }
                    close(clients[i].fd);
                    clients[i].fd = 0;
                }
                else
                {
                    sprintf(buffer, "client %d: %s", clients[i].id, buffer);
                    for (int j = 0; j < MAX_CLIENTS; j++)
                    {
                        if (clients[j].fd > 0 && j != i)
                            send(clients[j].fd, buffer, strlen(buffer), 0);
                    }
                }
            }
        }
    }

    return 0;
}
