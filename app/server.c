#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


#define PORT "4221"
#define BACKLOG 5


void *get_in_addr(struct sockaddr *);

int main(void)
{
    /** Disable output buffering */
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

    int server_fd, client_fd;
    int return_code;
    int reuse_port;
    socklen_t client_addr_size;
    struct addrinfo hints;
    struct addrinfo *kernel_return_servinfo, *p;
    struct sockaddr_storage client_addr;
    char address[INET6_ADDRSTRLEN];

    /** Server setup */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((return_code = getaddrinfo(
                    NULL, 
                    PORT, 
                    &hints, 
                    &kernel_return_servinfo)) 
            != 0)
    {
        printf("getaddrinfo() error: %s\n", gai_strerror(return_code));
    }

    for (p = kernel_return_servinfo; !p; p = p->ai_next)
    {
        if ((server_fd = socket(
                        p->ai_family,
                        p->ai_socktype,
                        p->ai_protocol)
                    ) == -1)
        {
            printf("socket() error: %s\n", strerror(errno));
            continue;
        }
        if ((setsockopt(
                        server_fd,
                        SOL_SOCKET,
                        SO_REUSEADDR,
                        &reuse_port,
                        sizeof(int)))
                == -1)
        {
            printf("setsockopt() error: %s\n", strerror(errno));
            return 1;
        }
        if (bind(server_fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(server_fd);
            printf("bind() error: %s\n", strerror(errno));
            continue;
        }
        break;
    }
    if (!p)
    {
        printf("No bind was successful. Exhaust all possible addrinfo\n");
        return 1;
    }
    if (listen(server_fd, BACKLOG) == -1)
    {
        printf("listen() error %s\n", strerror(errno));
        return 1;
    }

    /** Server Log Info */
    inet_ntop(p->ai_family, get_in_addr(p->ai_addr), address, sizeof address);

    printf("Listening at %s:%s\n", address, PORT);

    freeaddrinfo(kernel_return_servinfo);

    /** Listening for incoming TCP Connection */
    client_addr_size = sizeof client_addr;
    
    /** Accept incoming TCP connection */
    client_fd = accept(
            server_fd,
            (struct sockaddr *) &client_addr,
            &client_addr_size
    );
    if (client_fd == -1)
    {
        printf("accpet() error: %s", strerror(errno));
    } 
    else 
    {
        printf("Accept an incoming TCP connection");
    }

    inet_ntop(
        client_addr.ss_family,
        get_in_addr((struct sockaddr *)&client_addr),
        address,
        sizeof address
    );
    
    printf("Connection from %s\n", address);

    /** Closing Server */
    close(server_fd);

    return 0;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}


/*int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	// server_fd = socket(AF_INET, SOCK_STREAM, 0);
	// if (server_fd == -1) {
	// 	printf("Socket creation failed: %s...\n", strerror(errno));
	// 	return 1;
	// }
	//
	// // Since the tester restarts your program quite often, setting SO_REUSEADDR
	// // ensures that we don't run into 'Address already in use' errors
	// int reuse = 1;
	// if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
	// 	printf("SO_REUSEADDR failed: %s \n", strerror(errno));
	// 	return 1;
	// }
	//
	// struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
	// 								 .sin_port = htons(4221),
	// 								 .sin_addr = { htonl(INADDR_ANY) },
	// 								};
	//
	// if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
	// 	printf("Bind failed: %s \n", strerror(errno));
	// 	return 1;
	// }
	//
	// int connection_backlog = 5;
	// if (listen(server_fd, connection_backlog) != 0) {
	// 	printf("Listen failed: %s \n", strerror(errno));
	// 	return 1;
	// }
	//
	// printf("Waiting for a client to connect...\n");
	// client_addr_len = sizeof(client_addr);
	//
	// accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
	// printf("Client connected\n");
	//
	// close(server_fd);

	return 0;
}*/
