#ifndef HTTP_H
#define HTTP_H


#include <stdlib.h>

#include "type.h"

#define HTTP_REQUEST_HEADER_SIZE  4096
#define HTTP_RESPONSE_SIZE        4096

#define HTTP_STATE_START        0
#define HTTP_STATE_METHOD       1
#define HTTP_STATE_URI          2
#define HTTP_STATE_VERSION      3
#define HTTP_STATE_HEADER_NAME  4
#define HTTP_STATE_HEADER_COLON 5
#define HTTP_STATE_HEADER_VALUE 6
#define HTTP_STATE_CR           100
#define HTTP_STATE_LF1          101
#define HTTP_STATE_LF2          102

#define HTTP_MAX_HEADERS 29 

#define HTTP_200   "HTTP/1.1 200 OK\r\n"
#define HTTP_200_R "HTTP/1.1 200 OK\r\n\r\n"
#define HTTP_201   "HTTP/1.1 201 Created\r\n"
#define HTTP_201_R "HTTP/1.1 201 Created\r\n\r\n"
#define HTTP_400   "HTTP/1.1 400 Bad Request\r\n"
#define HTTP_400_R "HTTP/1.1 400 Bad Request\r\n\r\n"
#define HTTP_404   "HTTP/1.1 404 Not Found\r\n"
#define HTTP_404_R "HTTP/1.1 404 Not Found\r\n\r\n"
#define HTTP_413   "HTTP/1.1 413 Content Too Large\r\n"
#define HTTP_413_R "HTTP/1.1 413 Content Too Large\r\n\r\n"
#define HTTP_500   "HTTP/1.1 500 Internal Server Error\r\n"
#define HTTP_500_R "HTTP/1.1 500 Internal Server Error\r\n\r\n"

#define HTTP   "HTTP"
#define CR     "\r"
#define LF     "\r"
#define SP     " "
#define CRLF   "\r\n"


struct __http_slice {
    u16 beg; // 16
    u16 end; // 16
};

struct __http_header {
    struct __http_slice name; // 32
    struct __http_slice val;  // 32
};

struct http_request {
    u32    __lead;             // 32
    u32    __tail;             // 32
    u8     __state;            // 8
    u8     ver_maj;            // 8
    u8     ver_min;            // 8
    u8     header_count;       // 8
    char   __buf[HTTP_REQUEST_HEADER_SIZE]; // 32
    size_t __buf_cap;          // 64
    struct __http_slice  method; // 32
    struct __http_slice  uri;    // 32
    struct __http_slice  body;   // 32
    struct __http_header headers[HTTP_MAX_HEADERS]; 
};

struct http_header {
    char *name;
    char *val ;
};

struct http_response {
    u32    __cursor;
    char   __buf[HTTP_RESPONSE_SIZE];
};

int get_header(const struct http_request *, const char *, char *) ;

int get_method(const struct http_request *, char *);

int get_rsrc(const struct http_request *, char *);

int get_rsrc_prefix(const struct http_request *, char *, const char *);

int get_uri(const struct http_request *, char *);

int has_header(const struct http_request *, const char *);

int flush(const struct http_request *r, int fd);

u8 match_method(const struct http_request *, const char *);

u8 match_uri(const struct http_request *, const char *);

u8 match_uri_prefix(const struct http_request *, const char *);

int init_http_request(struct http_request *);

void free_http_request(struct http_request *);

/** This four functions need to perform in sequence and effect is finalized. The
 * write cursor only move forward.
 *
 * The main reason is to save on memory space of keep the state of response 
 * since it is rather odd to construct and set fields for a response and 
 * change it later before finalizing all the necessary pieces for a response */
int init_http_response(struct http_request *, struct http_response *);

void write_response_status(struct http_response *, const char *, const char *);

/** This function can be called multiple times before writing the body */
void write_response_header(struct http_response *, const char *, const char *, const size_t);

void write_content_length(struct http_response *, const int);

void write_response_end_header(struct http_response *);

void write_response_body(struct http_response *, const char *, const size_t);

int write_response_end(int fd, struct http_response *);

void free_http_response(struct http_response *);


#endif // !HTTP_H
