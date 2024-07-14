#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ip.h"
#include "http_parser.h"

#define BACKLOG 10
#define PORT "4221"

#define READ_SIZE 3


int http_request_reader(int, struct http_request **);

void router(int, struct http_request *);

void safe_free(void *);

int main(void)
{
    int rcode;

    /** Server Configuration */
    int server_fd;
    int reuse_port = 1;
    struct addrinfo hints;
    struct addrinfo *server_opts, *server_info;
    

    /** Client Configuration */
    int client_fd;
    char address[INET6_ADDRSTRLEN];
    socklen_t addr_size;
    struct sockaddr_storage client_addr;


    /** Server Setup */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rcode = getaddrinfo(NULL, PORT, &hints, &server_opts)) != 0)
    {
        printf("getaddrinfo error: %s\n", gai_strerror(rcode));
        return 1;
    }
    for (server_info = server_opts; server_info; server_info = server_info->ai_next)
    {
        if ((server_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol)) == -1)
        {
            printf("socket error: %s\n", strerror(errno));
            continue;
        }
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_port, sizeof(int)) == -1)
        {
            printf("setsockopt error %s:\n", strerror(errno));
            return 1;
        }
        if (bind(server_fd, server_info->ai_addr, server_info->ai_addrlen) == -1)
        {
            close(server_fd);
            printf("bind error %s:\n", strerror(errno));
            continue;
        }
        break;
    }
    freeaddrinfo(server_opts);
    if (!server_info)
    {
        printf("Failed to bind\n");
        return 1;
    }
    if (listen(server_fd, BACKLOG) == -1)
    {
        printf("listen error: %s\n", strerror(errno));
        return 1;
    }


    /** Accepting incoming TCP connections */
    while (1) {
        addr_size = sizeof client_addr;
        client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &addr_size);
        if (client_fd == -1) {
            printf("accept error: %s", strerror(errno));
            continue;
        }

        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *) &client_addr), address, sizeof address);
        printf("New TCP connection from %s\n", address);

        if (!fork()) {
            close(server_fd);
            /** Handling */
            struct http_request *r;
            rcode = http_request_reader(client_fd, &r);
            if (rcode) {
                router(client_fd, r);
            }
            free_http_request(r);
            close(client_fd);
            exit(0);
        }
        close(client_fd);
    }

    /** Server Shutdown */
    close(server_fd);

    return 0;
}

int http_request_reader(int fd, struct http_request **r) {
    char *buffer = NULL;
    int rcode = 0, nrecv = 0, trecv = 0;

    /** Malloc */
    buffer = malloc(READ_SIZE * sizeof(char));
    if (buffer == NULL) {
        printf("buffer malloc error\n");
        if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
            printf("response send error: %s\n", strerror(errno));
        }
        return 0;
    }
    memset(buffer, 0, READ_SIZE * sizeof(char));

    *r = malloc(sizeof(struct http_request));
    if (r == NULL) {
        printf("http request malloc error\n");
        if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
            printf("response send error: %s\n", strerror(errno));
        }
        safe_free(buffer);
        return 0;
    }
    memset(*r, 0, sizeof(struct http_request));

    (*r)->__buf_cap = HTTP_REQUEST_HEADER_SIZE;
    (*r)->__buf = malloc(HTTP_REQUEST_HEADER_SIZE * sizeof(char));
    if ((*r)->__buf == NULL) {
        printf("http request buffer malloc error\n");
        if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
            printf("response send error: %s\n", strerror(errno));
        }
        safe_free(buffer);
        return 0;
    }
    memset((*r)->__buf, 0, HTTP_REQUEST_HEADER_SIZE);

    /* Receiver HTTP request header */
    while (1) {
        memset(buffer, 0, READ_SIZE * sizeof(char));
        nrecv = recv(fd, buffer, READ_SIZE, 0);
        if (nrecv == -1) {
            if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                printf("response send error: %s\n", strerror(errno));
            }
            safe_free(buffer);
            return 0;
        }

        if ((trecv += nrecv) > HTTP_REQUEST_HEADER_SIZE) {
            printf("HTTP request header exceed maximum size\n");
            if ((send(fd, HTTP_413_R, strlen(HTTP_413_R), 0)) == -1) {
                printf("response send error: %s\n", strerror(errno));
            }
            safe_free(buffer);
            return 0;
        }

        rcode = parse_http_request(*r, buffer, nrecv, trecv);
        if (rcode == -1) {
            if ((send(fd, HTTP_400_R, strlen(HTTP_400_R), 0)) == -1) {
                printf("response send error: %s\n", strerror(errno));
            }
            safe_free(buffer);
            return 0;
        }
        if (rcode > 0) break;
    }

    free(buffer);

    return 1;
}

void router(int fd, struct http_request *r) {
    if (match_uri(r, "/")) {
        if (send(fd, HTTP_200_R, strlen(HTTP_200_R), 0) == -1) {
            printf("response send error: %s\n", strerror(errno));
        }
    } else if (match_uri_prefix(r, "/echo/")) {
        int rsrc_len;
        char * rsrc;
        if ((rsrc_len = get_rsrc(r, &rsrc)) == -1) {
            printf("rsrc malloc error\n");
            if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                printf("response send error: %s\n", strerror(errno));
            }
            return;
        }

        struct http_response res = { 0 };
        if (!init_http_response(r, &res)) {
            printf("http response malloc error: %s\n", strerror(errno));
            if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                printf("response send error: %s\n", strerror(errno));
            }
            safe_free(rsrc);
            return;
        }

        write_response_status(&res, "200", "OK");
        write_response_header(&res, "Content-Type", "text/plain",
                strlen("text/plain"));
        write_content_length(&res, rsrc_len);
        write_response_body(&res, rsrc, rsrc_len);
        if (write_response_end(fd, &res) == -1) {
            printf("http response error: %s\n", strerror(errno));
        }

        free_http_response(&res);
        safe_free(rsrc);
    } else if (match_uri(r, "/user-agent")) {
        int user_agent_len;
        char *user_agent;
        if ((user_agent_len = get_header(r, "User-Agent", &user_agent)) == -1) {
            printf("user_agent malloc error\n");
            if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                printf("response send error: %s\n", strerror(errno));
            }
            return;
        }

        struct http_response res = { 0 };
        if (!init_http_response(r, &res)) {
            printf("http response malloc error: %s\n", strerror(errno));
            if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                printf("response send error: %s\n", strerror(errno));
            }
            safe_free(user_agent);
            return;
        }

        write_response_status(&res, "200", "OK");
        write_response_header(&res, "Content-Type", "text/plain",
                strlen("text/plain"));
        write_content_length(&res, user_agent_len);
        write_response_body(&res, user_agent, user_agent_len);
        if (write_response_end(fd, &res) == -1) {
            printf("http response error: %s\n", strerror(errno));
        }

        free_http_response(&res);
        safe_free(user_agent);
    } else {
        printf("miss match\n");
        if (send(fd, HTTP_404_R, strlen(HTTP_404_R), 0) == -1) {
            printf("response send error: %s\n", strerror(errno));
        }
    }
}

void safe_free(void *p) {
    if (p != NULL) {
        free(p);
    }
}
