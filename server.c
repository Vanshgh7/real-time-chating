#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "helper.h"

#define BUFFER_SIZE 1000

pthread_mutex_t mutex_lock; // Mutex for thread-safe access

/* Client node structure */
struct client_node {
    char *username;
    int socket_fd;
    struct client_node *next;
};

struct client_node *head = NULL;

/* Add a new client to the head of the list (O(1)) */
void add_client(struct client_node *new_client) {
    new_client->next = head;
    head = new_client;
}

/* Remove a client from the list (O(n)) */
void remove_client(int socket_fd) {
    struct client_node *curr = head, *prev = NULL;

    while (curr && curr->socket_fd != socket_fd) {
        prev = curr;
        curr = curr->next;
    }

    if (!curr) return;

    if (!prev)
        head = curr->next;
    else
        prev->next = curr->next;

    free(curr->username);
    free(curr);
}

/* Create a listening socket on the given port */
int setup_server_socket(const char *port) {
    struct addrinfo hints, *res, *p;
    int listen_fd, rc, optval = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG | AI_PASSIVE | AI_NUMERICSERV;

    if ((rc = getaddrinfo(NULL, port, &hints, &res)) != 0) {
        fprintf(stderr, "Invalid port: %s\n", port);
        return -1;
    }

    for (p = res; p; p = p->ai_next) {
        listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listen_fd < 0) continue;

        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

        if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == 0) break;

        close(listen_fd);
    }

    freeaddrinfo(res);

    if (!p) return -1;

    if (listen(listen_fd, 1024) < 0) {
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

/* Send message to all or specific clients */
void broadcast_message(int sender_fd, const char *message, const char *receiver, const char *sender) {
    char buffer[BUFFER_SIZE];
    struct client_node *curr = head;

    if (!receiver) {
        while (curr) {
            if (curr->socket_fd == sender_fd) {
                strcpy(buffer, "msg sent\n\r\n");
                rio_writen(curr->socket_fd, buffer, strlen(buffer));
            } else {
                sprintf(buffer, "start\n%s:%s\n\r\n", sender, message);
                rio_writen(curr->socket_fd, buffer, strlen(buffer));
            }
            curr = curr->next;
        }
    } else {
        while (curr) {
            if (strcmp(curr->username, receiver) == 0) {
                sprintf(buffer, "start\n%s:%s\n\r\n", sender, message);
                rio_writen(curr->socket_fd, buffer, strlen(buffer));
                strcpy(buffer, "msg sent\n\r\n");
                rio_writen(sender_fd, buffer, strlen(buffer));
                return;
            }
            curr = curr->next;
        }
        strcpy(buffer, "user not found\n\r\n");
        rio_writen(sender_fd, buffer, strlen(buffer));
    }
}

/* Evaluate commands from clients */
void evaluate_command(char *buf, int client_fd, const char *username) {
    char buffer[BUFFER_SIZE], msg[BUFFER_SIZE], receiver[BUFFER_SIZE], keyword[BUFFER_SIZE];
    struct client_node *curr = head;

    buffer[0] = msg[0] = receiver[0] = keyword[0] = '\0';

    if (strcmp(buf, "help") == 0) {
        sprintf(buffer, "msg \"text\" : send message to all clients\n");
        sprintf(buffer + strlen(buffer), "msg \"text\" user : send message to a specific client\n");
        sprintf(buffer + strlen(buffer), "online : list all online users\n");
        sprintf(buffer + strlen(buffer), "quit : exit chatroom\n\r\n");
        rio_writen(client_fd, buffer, strlen(buffer));
        return;
    }

    if (strcmp(buf, "online") == 0) {
        buffer[0] = '\0';
        pthread_mutex_lock(&mutex_lock);
        while (curr) {
            sprintf(buffer + strlen(buffer), "%s\n", curr->username);
            curr = curr->next;
        }
        pthread_mutex_unlock(&mutex_lock);
        sprintf(buffer + strlen(buffer), "\r\n");
        rio_writen(client_fd, buffer, strlen(buffer));
        return;
    }

    if (strcmp(buf, "quit") == 0) {
        pthread_mutex_lock(&mutex_lock);
        remove_client(client_fd);
        pthread_mutex_unlock(&mutex_lock);
        strcpy(buffer, "exit");
        rio_writen(client_fd, buffer, strlen(buffer));
        close(client_fd);
        return;
    }

    sscanf(buf, "%s \" %[^\"] \"%s", keyword, msg, receiver);

    if (strcmp(keyword, "msg") == 0) {
        pthread_mutex_lock(&mutex_lock);
        broadcast_message(client_fd, msg, receiver[0] ? receiver : NULL, username);
        pthread_mutex_unlock(&mutex_lock);
    } else {
        strcpy(buffer, "Invalid command\n\r\n");
        rio_writen(client_fd, buffer, strlen(buffer));
    }
}

/* Thread to handle each client */
void* handle_client(void *arg) {
    int client_fd = *((int *)arg);
    free(arg);
    char username[BUFFER_SIZE], buf[BUFFER_SIZE];
    rio_t rio;

    pthread_detach(pthread_self());
    rio_readinitb(&rio, client_fd);

    long n = rio_readlineb(&rio, username, BUFFER_SIZE);
    if (n == -1) {
        close(client_fd);
        return NULL;
    }
    username[n - 1] = '\0'; // Remove newline

    struct client_node *client = malloc(sizeof(struct client_node));
    if (!client) {
        perror("Memory allocation failed");
        close(client_fd);
        return NULL;
    }

    client->username = strdup(username);
    client->socket_fd = client_fd;

    pthread_mutex_lock(&mutex_lock);
    add_client(client);
    pthread_mutex_unlock(&mutex_lock);

    while ((n = rio_readlineb(&rio, buf, BUFFER_SIZE)) > 0) {
        buf[n - 1] = '\0';
        evaluate_command(buf, client_fd, username);
    }

    return NULL;
}

int main(int argc, char **argv) {
    char *port = (argc > 1) ? argv[1] : "80";
    int listen_fd = setup_server_socket(port);
    struct sockaddr_storage client_addr;
    socklen_t client_len = sizeof(client_addr);

    if (listen_fd == -1) {
        printf("Failed to set up server on port %s\n", port);
        exit(1);
    }

    printf("Server listening on port %s...\n", port);

    while (1) {
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        printf("A new client connected.\n");
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_fd);
    }

    return 0;
}
