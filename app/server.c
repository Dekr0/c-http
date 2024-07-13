#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http.h"
#include "ip.h"
#include "http_parser.h"

#define BACKLOG 3
#define PORT "4221"

#define READ_SIZE 1024


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


    /** Logging Server Information */
    inet_ntop(server_info->ai_family, get_in_addr(server_info->ai_addr), address, sizeof address);
    printf("Listening at %s:%s\n", address, PORT);


    /** TCP I/O */
    char *recv_buffer = malloc(READ_SIZE * sizeof(char));
    if (recv_buffer == NULL)
    {
        printf("recv_buffer malloc() error\n");
        return -1;
    }
    char *raw_request = malloc(HTTP_REQUEST_HEADER_SIZE * sizeof(char));
    if (raw_request == NULL)
    {
        printf("raw_request malloc() error\n");
        return -1;
    }
    char *raw_response = malloc(HTTP_RESPONSE_SIZE * sizeof(char));
    if (raw_response == NULL)
    {
        printf("raw_response malloc() error\n");
        return -1;
    }


    /** HTTP Request State and Metadata */
    struct http_request req = { 0 };
    int bytes_recv = 0;
    size_t bytes_recv_total = 0;
    size_t raw_request_buffer_size = HTTP_REQUEST_HEADER_SIZE * sizeof(char);


    /** HTTP Request */
    char method[16] = { 0 };
    char uri[16] = { 0 };


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

        memset(recv_buffer, 0, READ_SIZE);
        memset(raw_request, 0, raw_request_buffer_size);
        memset(raw_response, 0, HTTP_RESPONSE_SIZE * sizeof(char));
        memset(&req, 0, sizeof(req));
        bytes_recv_total = 0;

        while(1) {
           bytes_recv = recv(client_fd, recv_buffer, READ_SIZE, 0);
           if (bytes_recv == -1)
           {
               printf("recv error: %s\n", strerror(errno));
               break;
           }
           memcpy(raw_request + bytes_recv_total, recv_buffer, bytes_recv);
           bytes_recv_total += bytes_recv;
           if (bytes_recv_total > HTTP_REQUEST_HEADER_SIZE)
           {
               printf("Exceed maximum size of HTTP request header\n");
               // ... Send error code
               break;
           }

#ifdef DEBUG
           printf("\nnumber of bytes received in total: %d\n", bytes_recv_total);
           for (size_t i = 0; i < bytes_recv_total; i++)
           {
               printf("http request byte at pos %d: %c\n", i, buffer[i]);
           }
#endif
           parse_http_request(&req, raw_request, bytes_recv_total, raw_request_buffer_size);
#ifdef DEBUG
           print_http_request_state(&req, buffer);
#endif
           if (req.status == -1)
           {
               printf("Parse failure\n");
               break;
           }
           if (req.status > 0)
           {
               assert(!get_request_method(&req, raw_request, method, sizeof method));
               assert(!get_request_uri(&req, raw_request, uri, sizeof uri));

               if (!strcmp(uri, "/"))
               {
                   if (send(client_fd, HTTP_200, sizeof HTTP_200, 0) == -1)
                   {
                       printf("send HTTP_200 error %s\n", strerror(errno));
                   }
               } else if (!strncmp(uri, "/echo/", 5)) {
                   size_t echo_length = req.uri.end - req.uri.start - 5;
                   size_t write_cursor = 0;

                   char content_type_header[128];
                   sprintf(content_type_header, "Content-Length: %zu\r\n\r\n", echo_length);

                   sprintf(raw_response, "%s", HTTP_200);
                   write_cursor += strlen(HTTP_200);

                   sprintf(raw_response + write_cursor, "%s", CONTENT_TYPE_PLAIN_TEXT);
                   write_cursor += strlen(CONTENT_TYPE_PLAIN_TEXT);

                   sprintf(raw_response + write_cursor, "%s", content_type_header);
                   write_cursor += strlen(content_type_header);

                   sprintf(raw_response + write_cursor, "%s", uri + 6);

                   if (send(client_fd, raw_response, strlen(raw_response), 0))
                   {
                       printf("send response error %s\n", strerror(errno));
                   }
               } else {
                   if (send(client_fd, HTTP_404, sizeof HTTP_404, 0))
                   {
                       printf("send HTTP_404 error %s\n", strerror(errno));
                   }
               }
               break;
           }
        }

        close(client_fd);
        break;
    }


    /** Server Shutdown */
    close(server_fd);

    return 0;
}
