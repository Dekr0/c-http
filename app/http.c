#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http.h"
#include "type.h"


int __get_slice(const struct __http_slice *s, const char *b, char *dest) {
    const u16 len = s->end - s->beg + 2; 

    memcpy(dest, b + s->beg, len - 1);

    return len - 1;
}

u8 __match_slice(const struct __http_slice *s, const char *b, 
        const char *m) {
    const u8 len_a = s->end - s->beg + 1;
    const u8 len_b = strlen(m);
    if (len_a != len_b) return 0;
    for (u8 i = 0; i < len_b; i++) {
        if (b[s->beg + i] != m[i]) return 0;
    }
    return 1;
}

u8 __match_slice_prefix(const struct __http_slice *s, const char *b, 
        const char *p) {
    const u8 len_a = s->end - s->beg + 1;
    const u8 len_b = strlen(p);
    if (len_b > len_a) {
        return 0;
    }
    u8 i;
    for (i = 0; i < len_b; i++) {
        if (b[s->beg + i] != p[i]) return 0;
    }
    return s->beg + i;
}

int flush(const struct http_request *r, int fd) {
    const int nflush = write(fd, r->__buf + r->__lead + 1, 
            strlen(r->__buf + r->__lead + 1));
    return nflush;
}

int get_header(const struct http_request *r, const char *name, char *val) {
    const int i = has_header(r, name);
    if (i == -1) return 0;

    return __get_slice(&r->headers[i].val, r->__buf, val);
}

int get_method(const struct http_request *r, char *method) {
    return __get_slice(&r->method, r->__buf, method);
}

int get_uri(const struct http_request *r, char *uri) {
    return __get_slice(&r->uri, r->__buf, uri);
}

int get_rsrc_prefix(const struct http_request *r, char *rsc, const char *p) {
    struct __http_slice s = { 0 };
    if ((s.beg = !match_uri_prefix(r, p)) == 0) {
        rsc = "";
        return 0;
    }
    s.end = r->uri.end;
    return __get_slice(&s, r->__buf, rsc);
}

int get_rsrc(const struct http_request *r, char *rsrc) {
    struct __http_slice s = { 0 };

    if (r->uri.beg == r->uri.end && r->__buf[r->uri.beg] == '/') {
        rsrc = "";
        return 0; 
    };

    u8 i = r->uri.end;
    while (r->__buf[i] != '/' && i > r->uri.beg) {
        i--;
    }

    s.beg = i + 1;
    s.end = r->uri.end;

    return __get_slice(&s, r->__buf, rsrc);
}

int has_header(const struct http_request *r, const char *name) {
    for (u8 i = 0; i < r->header_count; i++) {
        if (__match_slice(&r->headers[i].name, r->__buf, name)) return i;
    }
    return -1;
}

u8 match_method(const struct http_request *r, const char *method) {
    return __match_slice(&r->method, r->__buf, method);
}

u8 match_uri(const struct http_request *r, const char *uri) {
    return __match_slice(&r->uri, r->__buf, uri);
}

u8 match_uri_prefix(const struct http_request *r, const char *prefix) {
    return __match_slice_prefix(&r->uri, r->__buf, prefix);
}

void free_http_request(struct http_request *r) { }

int init_http_request(struct http_request *r) {
    memset(r, 0, sizeof(struct http_request));
    memset(r->__buf, 0, HTTP_REQUEST_HEADER_SIZE * sizeof(char));
    r->__buf_cap = HTTP_REQUEST_HEADER_SIZE;
    return 1;
}

int init_http_response(struct http_request *req, struct http_response *res) {
    memset(res, 0, sizeof(struct http_response));
    memset(res->__buf, 0, HTTP_RESPONSE_SIZE * sizeof(char));

    res->__cursor = 0;

    char src[16];
    memset(src, 0, 16 * sizeof(char));

    sprintf(src, "HTTP/%d.%d ", req->ver_maj, req->ver_min);
    memcpy(res->__buf + res->__cursor, src, strlen(src));
    res->__cursor += strlen(src);

    return 1;
}

void write_response_status(struct http_response *r, const char *code, 
        const char *text) {
    memcpy(r->__buf + r->__cursor, code, strlen(code));
    r->__cursor += strlen(code);

    memcpy(r->__buf + r->__cursor, SP, strlen(SP));
    r->__cursor += strlen(SP);

    memcpy(r->__buf + r->__cursor, text, strlen(text));
    r->__cursor += strlen(text);

    memcpy(r->__buf + r->__cursor, CRLF, strlen(CRLF));
    r->__cursor += strlen(CRLF);
}

void write_response_header(struct http_response *r, const char * name, 
        const char *val, const size_t vsize) {
    const char *sep = ": ";
    memcpy(r->__buf + r->__cursor, name, strlen(name));
    r->__cursor += strlen(name);

    memcpy(r->__buf + r->__cursor, sep, strlen(sep));
    r->__cursor += strlen(sep);

    memcpy(r->__buf + r->__cursor, val, vsize);
    r->__cursor += vsize;
    
    memcpy(r->__buf + r->__cursor, CRLF, strlen(CRLF));
    r->__cursor += strlen(CRLF);
}

void write_content_length(struct http_response *r, const int len) {
    char content_length[16];
    memset(content_length, 0, 16 * sizeof(char));

    sprintf(content_length, "%d", len);
    write_response_header(r, "Content-Length", content_length, 
            strlen(content_length));
}

void write_response_end_header(struct http_response *r) {
    memcpy(r->__buf + r->__cursor, CRLF, strlen(CRLF));
    r->__cursor += strlen(CRLF);
}

void write_response_body(struct http_response *r, const char *body, 
        const size_t size) {
    memcpy(r->__buf + r->__cursor, body, size);
    r->__cursor += size;
}

int write_response_end(int fd, struct http_response *r) {
    return send(fd, r->__buf, r->__cursor, 0);
}
