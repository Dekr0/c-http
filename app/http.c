#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "http.h"


int get_request_method(
        struct http_request *r, 
        const char *raw_request_buffer, 
        char *method,
        size_t size)
{
    assert(r->method.end - r->method.start >= 0);

    if (size < r->method.end - r->method.start)
    {
        printf("Given method buffer is smaller than the occupied buffer");
        return 1;
    }

    for (unsigned short i = 0; i <= r->method.end - r->method.start; i++)
    {
        method[i] = raw_request_buffer[r->method.start + i];
    }

    return 0;
}

int get_request_uri(
        struct http_request *r,
        const char *raw_request_buffer,
        char *uri,
        size_t size
        )
{
    assert(r->uri.end - r->uri.start >= 0);
    
    if (size < r->uri.end - r->uri.start)
    {
        printf("Given uri buffer is smaller than the occupied buffer");
        return 1;
    }
    
    for (unsigned short i = 0; i <= r->uri.end - r->uri.start; i++)
    {
        uri[i] = raw_request_buffer[r->uri.start + i];
    }
    
    return 0;
}

int get_header(
        struct http_request *r,
        const char *raw_request_buffer,
        const char *header_name,
        char *header_value,
        const size_t size
        )
{
    struct http_request_header_slice *h;
    unsigned short header_name_len;
    unsigned short header_value_len;
    int matched;
    unsigned char i, j;
    for (i = 0; i < r->num_headers; i++)
    {
        matched = 1;
        h = &r->headers[i];
        header_name_len= h->header_name.end - h->header_name.start + 1;

        if (strlen(header_name) != header_name_len) continue;

        for (j = 0; j < strlen(header_name); j++)
        {
            if (header_name[j] != raw_request_buffer[h->header_name.start + j])
            {
                matched = 0;
                break;
            }
        }

        if (!matched) continue;

        header_value_len = h->header_value.end - h->header_value.start;

        if (size < header_value_len)
        {
            printf("Given header value buffer is smaller than the occupied buffer");
            return -1;
        }

        for (j = 0; j <= header_value_len; j++)
        {
            header_value[j] = raw_request_buffer[h->header_value.start + j];
        }
        return 1;
    }

    printf("%s header not found\n", header_name);

    return -1;
}

void print_http_request_state(struct http_request *r, const char *b)
{
    printf("status: %d\n", r->status);
    printf("state: %d\n", r->__state);
    printf("lead: %d = %d, %c\n", r->lead, b[r->lead], b[r->lead]);
    printf("tail: %d = %d, %c\n", r->tail, b[r->tail], b[r->tail]);
    printf("method.start: %d = %d, %c\n", r->method.start, b[r->method.start], b[r->method.start]);
    printf("method.end  : %d = %d, %c\n", r->method.end  , b[r->method.end  ], b[r->method.end]);
    printf("uri.start   : %d = %d, %c\n", r->uri.start   , b[r->uri.start   ], b[r->uri.start]);
    printf("uri.end     : %d = %d, %c\n", r->uri.end     , b[r->uri.end     ], b[r->uri.end]);
    printf("http version major: %d\n", r->version_major);
    printf("http version minor: %d\n", r->version_minor);
    printf("number of http headers: %d\n", r->num_headers);
}

void print_http_request(struct http_request *r, const char *b)
{
    printf("\nHTTP Version:%d.%d\n", r->version_major, r->version_minor);

    printf("HTTP Method:");
    for (size_t i = r->method.start; i <= r->method.end; i++)
    {
        printf("%c", b[i]);
    }
    printf("\n");

    printf("HTTP URI:");
    for (size_t i = r->uri.start; i <= r->uri.end; i++)
    {
        printf("%c", b[i]);
    }
    printf("\n");

    printf("HTTP Headers:\n");
    for (size_t i = 0; i < r->num_headers; i++)
    {
        for (size_t j = r->headers[i].header_name.start; j <= r->headers[i].header_name.end; j++)
        {
            printf("%c", b[j]);
        }
        printf(":");

        for (size_t j = r->headers[i].header_value.start; j <= r->headers[i].header_value.end; j++)
        {
            printf("%c", b[j]);
        }
        printf("\n");
    }
}
