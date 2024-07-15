#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "http_parser.h"
#include "http.h"
#include "type.h"


const char http_version_prefix[5] = "HTTP/";

/** Valid HTTP token */
const char http_token[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x00
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x10
    0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1,  // 0x20
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  // 0x30
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x40
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1,  // 0x50
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x60
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0,  // 0x70
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x80
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x90
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xa0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xb0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xc0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xd0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xe0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xf0
};


/**
 * @return bytes on success; -1 on failure; 0 if incomplete (It needs more data)
 */
int parse_http_request(struct http_request *r, const char *buffer, 
        size_t nrecv, size_t trecv) {
    char c = 0;
    u32 len = 0;

    memcpy(r->__buf + r->__lead, buffer, nrecv);

    while (r->__lead < trecv && r->__lead < r->__buf_cap) {
        c = r->__buf[r->__lead];
#ifdef DEBUG
        printf("At state %d: tokenizing pos %d (v = %d)\n", r->__state, r->__lead,
                buffer[r->__lead]);
#endif /* ifdef DEBUG */
        switch (r->__state) {
            case HTTP_STATE_START:
                if (c == '\r' || c == '\n') break;
                
                if (!http_token[c]) {
                    printf("At state %d: bad token at pos %d (v = %c [%d])\n",
                            r->__state, r->__lead, c, c);
                    return -1;
                }

                r->__tail = r->__lead;
#ifdef DEBUG
                printf("At state %d: change to state %d\n", r->__state, 
                        HTTP_STATE_METHOD);
#endif /* ifdef DEBUG */
                r->__state = HTTP_STATE_METHOD;
                break;
            case HTTP_STATE_METHOD:
                if (c == ' ') {
                    r->method.beg = r->__tail;
                    r->method.end = r->__lead - 1;
                    r->__tail = r->__lead + 1;
#ifdef DEBUG
                    printf("At state %d: change to state %d\n", r->__state, 
                            HTTP_STATE_URI);
#endif /* ifdef DEBUG */
                    r->__state = HTTP_STATE_URI;
                    break;
                }

                if (!http_token[c]) {
                    printf("At state %d: bad token at pos %d (v = %c [%d])\n",
                            r->__state, r->__lead, c, c);
                    return -1;
                }
                break;
            case HTTP_STATE_URI:
                if (c == ' ') {
                    if (r->__lead == r->__tail) {
                        printf("At state %d: missing HTTP URI\n", r->__state);
                        return -1;
                    }

                    r->uri.beg = r->__tail;
                    r->uri.end = r->__lead - 1;
                    r->__tail = r->__lead + 1;
#ifdef DEBUG
                    printf("At state %d: change to state %d\n", r->__state,
                            HTTP_STATE_VERSION);
#endif
                    r->__state = HTTP_STATE_VERSION;
                    break;
                }

                if (!http_token[c]) {
                    printf("At state %d: bad token at pos %d (v = %c [%d])\n",
                            r->__state, r->__lead, c, c);
                    return -1;
                }

                break;
            case HTTP_STATE_VERSION:
                len = r->__lead - r->__tail;
                if (len == 8 && (c == ' ' || c == '\r' || c == '\n')) {
                    size_t i = r->__tail;

                    int isHTTP = strncmp(r->__buf + i, HTTP, strlen(HTTP)); 
                    if (isHTTP) {
                        printf("At state %d: bad HTTP Version token\n", 
                                r->__state);
                        return -1;
                    }
                    i += strlen(HTTP);

                    if (r->__buf[i++] != '/') {
                        printf("At state %d: expect / after 'HTTP'\n", 
                                r->__state);
                        return -1;
                    }

                    if (!isdigit(r->__buf[i])) {
                        printf("At state %d: expect digit after 'HTTP/'\n", 
                                r->__state);
                        return -1;
                    }
                    r->ver_maj = r->__buf[i++] - '0';

                    if (r->__buf[i++] != '.') {
                        printf("At state %d: expect '.' after HTTP/x",
                                r->__state);
                        return -1;
                    }

                    if (!isdigit(r->__buf[i])) {
                        printf("At state %d: expect digit after HTTP/x.\n", 
                                r->__state);
                        return -1;
                    }
                    r->ver_min = r->__buf[i++] - '0';

                    if (c == '\r') {
#ifdef DEBUG
                        printf("At state %d: change to state %d\n", r->__state,
                                HTTP_STATE_CR);
#endif
                        r->__state = HTTP_STATE_CR;
                    } else {
#ifdef DEBUG
                        printf("At state %d: change to state %d\n", r->__state,
                                HTTP_STATE_LF1);
#endif /* ifdef DEBUG */
                        r->__state = HTTP_STATE_LF1;
                    }
                    break;
                }
                break;
            case HTTP_STATE_CR:
                if (c != '\n') {
                    printf("At state %d: expected LF after CR at post %d\n",
                            r->__state, r->__lead);
                    return -1;
                }
#ifdef DEBUG
                printf("At state %d: change to state %d\n", r->__state,
                        HTTP_STATE_LF1);
#endif /* if 0 */
                r->__state = HTTP_STATE_LF1;
                break;
            case HTTP_STATE_LF1:
                if (c == '\r') {
#ifdef DEBUG
                    printf("At state %d: change to state %d\n", r->__state,
                            HTTP_STATE_LF2);
#endif
                    r->__state = HTTP_STATE_LF2;
                    break;
                }

                if (c == '\n') {
                    return r->__lead;
                }

                if (!http_token[c]) {
                    printf("At state %d: bad token at pos %d (v = %c, [%d])\n",
                            r->__state, r->__lead, c, c);
                    printf("Forbid empty header name or line folding\n");
                    return -1;
                }

                r->__tail = r->__lead;
                r->headers[r->header_count].name.beg = r->__tail;
#ifdef DEBUG
                printf("At state %d: change to state %d\n",
                        r->__state, HTTP_STATE_HEADER_NAME);
#endif /* ifdef DEBUG */
                r->__state = HTTP_STATE_HEADER_NAME;
                break;
            case HTTP_STATE_HEADER_NAME:
                if (c == ':') {
                    r->headers[r->header_count].name.end = r->__lead - 1;
#ifdef DEBUG
                    printf("At state %d: change to state %d\n",
                            r->__state, HTTP_STATE_HEADER_COLON);
#endif /* ifdef DEBUG */
                    r->__state = HTTP_STATE_HEADER_COLON;
                    break;
                }
                if (!http_token[c]) {
                    printf("At state %d: bad token at pos %d (v = %c, [%d])\n",
                            r->__state, r->__lead, c, c);
                    return -1;
                }
                break;
            case HTTP_STATE_HEADER_COLON:
                if (c == ' ' || c == '\t') break;
                r->__tail = r->__lead;
                r->headers[r->header_count].val.beg = r->__tail;
#ifdef DEBUG
                printf("At state %d: change to state %d\n",
                        r->__state, HTTP_STATE_HEADER_VALUE);
#endif /* ifdef DEBUG */
                r->__state = HTTP_STATE_HEADER_VALUE;
            case HTTP_STATE_HEADER_VALUE:
                if (c == '\r' || c == '\n') {
                    size_t i = r->__lead - 1;
                    while (i >= r->__tail && (r->__buf[i] == ' ' ||
                                r->__buf[i] == '\t'))
                        i--;

                    r->headers[r->header_count].val.end = i;
                    r->header_count++;

                    if (c == '\r') {
#ifdef DEBUG
                        printf("At state %d: change to state %d\n",
                                r->__state, HTTP_STATE_CR);
#endif /* ifdef DEBUG */
                        r->__state = HTTP_STATE_CR;
                        break;
                    }
                    r->__state = HTTP_STATE_LF1;
#ifdef DEBUG
                    printf("At state %d: change to state %d\n",
                            r->__state, HTTP_STATE_LF1);
#endif /* ifdef DEBUG */
                    break;
                }

                if ((c < 0x20 && c != '\t') || (0x7F <= c)) {
                    printf("At state %d: bad token for HTTP field value at \
                            pos %d (v = %c, [%d])",
                            r->__state, r->__lead, c, c);
                    return -1;
                }
                break;
            case HTTP_STATE_LF2:
                if (c != '\n') {
                    printf("At state %d: bad token at pos %d (v = %c, [%d])\n",
                            r->__state, r->__lead, c, c);
                    return -1;
                }
                r->__tail = r->__lead;
                return r->__lead;
        }
        r->__lead += 1;
    }

    if (r->__lead < r->__buf_cap) {
#ifdef DEBUG
        printf("At state %d: require further recv\n", r->__state);
#endif /* ifdef DEBUG */
        return 0;
    }

    return -1;
}
