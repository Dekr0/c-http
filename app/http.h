#ifndef HTTP_H
#define HTTP_H


#include <stdlib.h>

#define HTTP_REQUEST_HEADER_SIZE  4096 // Can be extended
#define HTTP_RESPONSE_SIZE 4096

#define HTTP_STATE_START        0
#define HTTP_STATE_METHOD       1
#define HTTP_STATE_URI          2
#define HTTP_STATE_VERSION      3
#define HTTP_STATE_HEADER_NAME  4
#define HTTP_STATE_HEADER_COLON 5
#define HTTP_STATE_HEADER_VALUE 6
#define HTTP_STATE_CR      100
#define HTTP_STATE_LF1     101
#define HTTP_STATE_LF2     102

#define HTTP_MAX_HEADERS   

#define CR "\r"
#define LF "\r"

#define CONTENT_TYPE_PLAIN_TEXT "Content-Type: text/plain\r\n"

#define HTTP_200 "HTTP/1.1 200 OK\r\n\r\n"
#define HTTP_404 "HTTP/1.1 404 Not Found\r\n\r\n"


struct http_request_slice 
{
    unsigned short start;
    unsigned short end;
};

struct http_request_header_slice
{
    struct http_request_slice header_name;
    struct http_request_slice header_value;
};

struct http_request
{
    int status; // 32, 1 => complete, -1 => failure, 0 => incomplete
    unsigned int lead; // 32
    unsigned int tail; // 32
    unsigned char __state; // 8
    unsigned char version_major; // 8
    unsigned char version_minor; // 8
    unsigned char num_headers; // 8
    struct http_request_slice method; // 32
    struct http_request_slice uri; // 32
    struct http_request_header_slice headers[HTTP_MAX_HEADERS]; 
};

struct http_header
{
    char header_name[64];
    char header_value[64];
};

int get_request_method(struct http_request *, const char *, char *, size_t);

int get_request_uri(struct http_request *, const char *, char *, size_t);

void print_http_request(struct http_request *, const char *b);

void print_http_request_state(struct http_request *, const char *b);


#endif // !HTTP_H
