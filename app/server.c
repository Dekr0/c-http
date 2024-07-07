#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT "4221"
#define OK "HTTP/1.1 200 OK\r\n\r\n"
#define BACKLOG 5


void * get_in_addr(struct sockaddr *sa)
{
     if (sa->sa_family == AF_INET) 
     {
         return &(((struct sockaddr_in*)sa)->sin_addr);
     }

     return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    /** Server Configuration */
    int server_fd;
    int reuse_port = 1;
    struct addrinfo hints;
    struct addrinfo *suggested_server_info, *server_info;
    
    /** Client Configuration */
    int client_fd;
    char address[INET6_ADDRSTRLEN];
    socklen_t client_addr_size;
    struct sockaddr_storage client_addr;

    int rcode;

    /** Server Setup */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rcode = getaddrinfo(NULL, PORT, &hints, &suggested_server_info)) != 0)
    {
        printf("getaddrinfo error: %s\n", gai_strerror(rcode));
        return 1;
    }
    for (server_info = suggested_server_info; server_info; server_info = server_info->ai_next)
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
    freeaddrinfo(suggested_server_info);
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

    /** Accepting incoming TCP connections */
    while (1) {
        client_addr_size = sizeof client_addr;
        client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_size);
        if (client_fd == -1) {
            printf("accept error: %s", strerror(errno));
            continue;
        }

        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *) &client_addr), address, sizeof address);
        printf("New TCP connection from %s\n", address);

        if (send(client_fd, OK, strlen(OK), 0) == -1)
        {
            printf("send error %s\n", strerror(errno));
        }

        close(client_fd);
        break;
    }

    /** Server Shutdown */
    close(server_fd);

    return 0;
}
