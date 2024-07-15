#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "http.h"
#include "ip.h"
#include "http_parser.h"

#define BACKLOG 10
#define PORT "4221"
#define TMP  "/tmp/data/codecrafters.io/http-server-tester"

#define READ_SIZE 3


int http_request_reader(int, struct http_request *);

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
            struct http_request r = { 0 };
            if (!init_http_request(&r)) {
                if (send(client_fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                    printf("response send error: %s\n", strerror(errno));
                }
                exit(1);
            }

            rcode = http_request_reader(client_fd, &r);
            if (rcode) {
                router(client_fd, &r);
            }
            close(client_fd);
            exit(0);
        }
        close(client_fd);
    }

    /** Server Shutdown */
    close(server_fd);

    return 0;
}

int http_request_reader(int fd, struct http_request *r) {
    char buffer[READ_SIZE] = { 0 };
    memset(buffer, 0, READ_SIZE * sizeof(char));

    int rcode = 0, nrecv = 0, trecv = 0;

    /* Receiver HTTP request header */
    while (1) {
        memset(buffer, 0, READ_SIZE * sizeof(char));
        nrecv = recv(fd, buffer, READ_SIZE, 0);
        if (nrecv == -1) {
            if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                printf("response send error: %s\n", strerror(errno));
            }
            return 0;
        }

        if ((trecv += nrecv) > HTTP_REQUEST_HEADER_SIZE) {
            printf("HTTP request header exceed maximum size\n");
            if ((send(fd, HTTP_413_R, strlen(HTTP_413_R), 0)) == -1) {
                printf("response send error: %s\n", strerror(errno));
            }
            return 0;
        }

        rcode = parse_http_request(r, buffer, nrecv, trecv);
        if (rcode == -1) {
            if ((send(fd, HTTP_400_R, strlen(HTTP_400_R), 0)) == -1) {
                printf("response send error: %s\n", strerror(errno));
            }
            return 0;
        }
        if (rcode > 0) break;
    }

    return 1;
}

void router(int fd, struct http_request *r) {
    if (match_uri(r, "/")) {
        if (send(fd, HTTP_200_R, strlen(HTTP_200_R), 0) == -1) {
            printf("response send error: %s\n", strerror(errno));
        }
    } else if (match_uri_prefix(r, "/echo/")) {
        int  rsrc_len;
        char rsrc[64];
        memset(rsrc, 0, 64 * sizeof(char));
        rsrc_len = get_rsrc(r, rsrc);

        struct http_response res = { 0 };
        if (!init_http_response(r, &res)) {
            printf("http response init error");
            if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                printf("response send error: %s\n", strerror(errno));
            }
            return;
        }

        write_response_status(&res, "200", "OK");
        write_response_header(&res, "Content-Type", "text/plain",
                strlen("text/plain"));
        write_content_length(&res, rsrc_len);
        write_response_end_header(&res);
        write_response_body(&res, rsrc, rsrc_len);
        if (write_response_end(fd, &res) == -1) {
            printf("response send error: %s\n", strerror(errno));
        }
    } else if (match_uri_prefix(r, "/files/")) {
        if (match_method(r, "POST")) {
            char filename[64];
            memset(filename, 0, 64 * sizeof(char));
            if (get_rsrc(r, filename) == 0) {
                if (send(fd, HTTP_400_R, strlen(HTTP_400_R), 0) == -1) {
                    printf("response send error: %s\n", strerror(errno));
                }
                return;
            }
            char content_length_str[16];
            memset(content_length_str, 0, 16 * sizeof(char));
            if (get_header(r, "Content-Length", content_length_str) == 0) {
                if (send(fd, HTTP_400_R, strlen(HTTP_400_R), 0) == -1) {
                    printf("response send error: %s\n", strerror(errno));
                }
                return;
            }

            u32 content_length = strtol(content_length_str, NULL, 10);
            if (content_length == 0) {
                if (send(fd, HTTP_400_R, strlen(HTTP_400_R), 0) == -1) {
                    printf("response send error: %s\n", strerror(errno));
                }
                return;
            }

            char path[128];
            memset(path, 0, 128 * sizeof(char));
            sprintf(path, "%s/%s", TMP, filename);

            printf("%s\n", path);

            int ffd = open(path, O_WRONLY |O_CREAT| O_TRUNC);
            if (ffd == -1) {
                printf("open error: %s\n", strerror(errno));
                if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                    printf("response send error: %s\n", strerror(errno));
                }
                return;
            }

            const int nflush = flush(r, ffd);
            if (nflush == -1) {
                printf("write error: %s\n", strerror(errno));
                if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                    printf("response send error: %s\n", strerror(errno));
                }
                return;
                close(ffd);
            }
            content_length -= nflush;

            /** Unsafe */
            int nread;
            char buffer[READ_SIZE];
            while (content_length > 0) {
                memset(buffer, 0, READ_SIZE * sizeof(char));
                nread = recv(fd, buffer, content_length, 0);
                printf("bytes read: %d\n", nread);
                if (nread == -1) {
                    printf("recv error: %s\n", strerror(errno));
                    if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                        printf("response send error: %s\n", strerror(errno));
                    }
                    close(ffd);
                    break;
                }
                if (write(ffd, buffer, nread) == -1) {
                    printf("write error: %s\n", strerror(errno));
                    if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                        printf("response send error: %s\n", strerror(errno));
                    }
                    close(ffd);
                    break;
                }
                content_length -= nread;
            }
            close(ffd);
            if (send(fd, HTTP_201_R, strlen(HTTP_201_R), 0) == -1) {
                printf("response send error: %s\n", strerror(errno));
            }
        } else if (match_method(r, "GET")) {
            char filename[64];
            memset(filename, 0, 64 * sizeof(char));
            if (get_rsrc(r, filename) == 0) {
                if (send(fd, HTTP_404_R, strlen(HTTP_404_R), 0) == -1) {
                    printf("response send error: %s\n", strerror(errno));
                }
                return;
            }

            char path[128];
            memset(path, 0, 128 * sizeof(char));
            sprintf(path, "%s/%s", TMP, filename);

            struct stat st;

            if (stat(path, &st) == -1) {
                printf("stat error: %s\n", strerror(errno));
                if (send(fd, HTTP_404_R, strlen(HTTP_404_R), 0) == -1) {
                    printf("response send error: %s\n", strerror(errno));
                }
                return;
            }

            int ffd = open(path, O_RDONLY, 0);
            if (ffd == -1) {
                printf("open error: %s\n", strerror(errno));
                if (send(fd, HTTP_404_R, strlen(HTTP_404_R), 0) == -1) {
                    printf("response send error: %s\n", strerror(errno));
                }
                return;
            }

            struct http_response res = { 0 };
            if (!init_http_response(r, &res)) {
                printf("http response init error");
                if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                    printf("response send error: %s\n", strerror(errno));
                }
                return;
            }

            const char *content_type = "application/octet-stream";
            write_response_status(&res, "200", "OK");
            write_response_header(&res, "Content-Type", content_type,
                    strlen(content_type));
            write_content_length(&res, st.st_size);
            write_response_end_header(&res);

            int nread;
            char buffer[2048];
            while ((nread = read(ffd, buffer, 2048)) != 0) {
                if (nread == -1) {
                    printf("read error: %s\n", strerror(errno));
                    if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                        printf("response send error: %s\n", strerror(errno));
                    }
                    close(ffd);
                    return;
                }
                write_response_body(&res, buffer, nread);
            }
            if (write_response_end(fd, &res) == -1) {
                printf("response send error: %s\n", strerror(errno));
            }
        }
    } else if (match_uri(r, "/user-agent")) {
        int user_agent_len;
        char user_agent[64];
        memset(user_agent, 0, 64 * sizeof(char));
        user_agent_len = get_header(r, "User-Agent", user_agent);

        struct http_response res = { 0 };
        if (!init_http_response(r, &res)) {
            printf("http response init error");
            if (send(fd, HTTP_500_R, strlen(HTTP_500_R), 0) == -1) {
                printf("response send error: %s\n", strerror(errno));
            }
            return;
        }

        write_response_status(&res, "200", "OK");
        write_response_header(&res, "Content-Type", "text/plain",
                strlen("text/plain"));
        write_content_length(&res, user_agent_len);
        write_response_end_header(&res);
        write_response_body(&res, user_agent, user_agent_len);
        if (write_response_end(fd, &res) == -1) {
            printf("response send error: %s\n", strerror(errno));
        }
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
