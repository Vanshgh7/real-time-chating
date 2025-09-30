#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "helper.h"  // Custom helper functions (rio_readinitb, rio_readlineb, rio_writen)

#define BUFFER_SIZE 1024
char chat_prompt[] = "Chatroom> ";

/* Show usage instructions */
void print_usage() {
    printf("-h  : Show help\n");
    printf("-a  : Server IP address [Required]\n");
    printf("-p  : Server port number [Required]\n");
    printf("-u  : Your username [Required]\n");
}

/* Connect to server and return socket file descriptor */
int connect_to_server(const char* host, const char* port) {
    struct addrinfo hints, *res, *p;
    int sockfd, ret;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV;

    if ((ret = getaddrinfo(host, port, &hints, &res)) != 0) {
        fprintf(stderr, "Invalid host or port\n");
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) != -1) break;

        close(sockfd);
    }

    freeaddrinfo(res);
    if (!p) return -1;
    return sockfd;
}

/* Thread function to continuously read server messages */
void* listen_server(void* arg) {
    int server_fd = (int)(intptr_t)arg;
    char buffer[BUFFER_SIZE];
    rio_t rio;
    rio_readinitb(&rio, server_fd);

    while (1) {
        int n;
        while ((n = rio_readlineb(&rio, buffer, BUFFER_SIZE)) > 0) {
            if (n == -1) exit(1);  // Read error
            if (strcmp(buffer, "\r\n") == 0) break;

            if (strcmp(buffer, "exit") == 0) {
                close(server_fd);
                exit(0);
            }

            if (strcmp(buffer, "start\n") != 0)
                printf("%s", buffer);
        }
        printf("%s", chat_prompt);
        fflush(stdout);
    }
}

int main(int argc, char** argv) {
    char *server_ip = NULL, *server_port = NULL, *user_name = NULL;
    char input[BUFFER_SIZE];
    int sockfd;
    pthread_t thread_id;
    char opt;

    /* Parse command line arguments */
    while ((opt = getopt(argc, argv, "hu:a:p:u:")) != -1) {
        switch (opt) {
            case 'h': print_usage(); exit(0);
            case 'a': server_ip = optarg; break;
            case 'p': server_port = optarg; break;
            case 'u': user_name = optarg; break;
            default: print_usage(); exit(1);
        }
    }

    if (!server_ip || !server_port || !user_name) {
        printf("Invalid usage\n");
        print_usage();
        exit(1);
    }

    sockfd = connect_to_server(server_ip, server_port);
    if (sockfd == -1) {
        printf("Unable to connect to server\n");
        exit(1);
    }

    /* Send username to server */
    char username_line[BUFFER_SIZE];
    snprintf(username_line, sizeof(username_line), "%s\n", user_name);
    if (rio_writen(sockfd, username_line, strlen(username_line)) == -1) {
        perror("Failed to send username");
        close(sockfd);
        exit(1);
    }

    /* Start thread to read server messages */
    pthread_create(&thread_id, NULL, listen_server, (void*)(intptr_t)sockfd);

    /* Main loop to read user input and send to server */
    printf("%s", chat_prompt);
    while (fgets(input, BUFFER_SIZE, stdin) != NULL) {
        if (rio_writen(sockfd, input, strlen(input)) == -1) {
            perror("Failed to send message");
            close(sockfd);
            exit(1);
        }
    }

    close(sockfd);
    return 0;
}
