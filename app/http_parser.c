#include <stdio.h>

#include "http_parser.h"


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
void parse_http_request(
        struct http_request *req,
        const char *buffer,
        size_t buffer_size,
        size_t buffer_cap
        )
{
    printf("\n");
    char c;
    while (req->lead < buffer_size)
    {
        c = buffer[req->lead];
#ifdef DEBUG
        printf("At state %d: tokenizing pos %d (v = %d)\n", req->__state, req->lead,
                buffer[req->lead]);
#endif /* ifdef DEBUG */
        switch (req->__state) 
        {
            case HTTP_STATE_START:
                if (c == '\r' || c == '\n') break;
                
                if (!http_token[c])
                {
                    printf("At state %d: bad token at pos %d (v = %d)\n",
                            req->__state, req->lead, c);
                    req->status = -1;
                    return;
                }

                req->tail = req->lead;
#ifdef DEBUG
                printf("At state %d: change to state %d\n", req->__state, 
                        HTTP_STATE_METHOD);
#endif
                req->__state = HTTP_STATE_METHOD;
                break;
            case HTTP_STATE_METHOD:
                if (c == ' ')
                {
                    if (req->lead == req->tail)
                    {
                        printf("At state %d: missing HTTP method\n", req->__state);
                        req->status = -1;
                        return;
                    }

                    req->method.start = req->tail;
                    req->method.end = req->lead - 1;

                    req->tail = req->lead + 1;
#ifdef DEBUG
                    printf("At state %d: change to state %d\n", req->__state, 
                            HTTP_STATE_URI);
#endif
                    req->__state = HTTP_STATE_URI;
                    break;
                }

                if (!http_token[c])
                {
                    printf("At state %d: bad token at pos %d (v = %d)\n",
                            req->__state, req->lead, c);
                    req->status = -1;
                    return;
                }
                break;
            case HTTP_STATE_URI:
                if (c == ' ')
                {
                    if (req->lead == req->tail)
                    {
                        printf("At state %d: missing HTTP URI\n", req->__state);
                        req->status = -1;
                        return;
                    }

                    req->uri.start = req->tail;
                    req->uri.end = req->lead - 1;

                    req->tail = req->lead + 1;
#ifdef DEBUG
                    printf("At state %d: change to state %d\n", req->__state,
                            HTTP_STATE_VERSION);
#endif /* ifdef DEBUG */
                    req->__state = HTTP_STATE_VERSION;
                    break;
                }

                if (!http_token[c])
                {
                    printf("At state %d: bad token at pos %d (v = %d)\n",
                            req->__state, req->lead, c);
                    req->status = -1;
                    return;
                }

                break;
            case HTTP_STATE_VERSION:
                if (req->lead - req->tail == 8 && (c == ' ' || c == '\r' || c == '\n')) // Backtracking
                {
                    size_t i = 0;
                    for ( ; i < sizeof(http_version_prefix); i++)
                    {
                        if (buffer[req->tail + i] != http_version_prefix[i])
                        {
                            printf("At state %d: bad HTTP Version token at pos %d (v = %d)\n", 
                                    req->__state, req->tail + i, buffer[req->tail + i]);
                            req->status = -1;
                            return;
                        }
                    }

                    if (buffer[req->tail + i] < 48 || buffer[req->tail + i] > 57)
                    {
                        printf("At state %d: bad HTTP Version digit token at pos %d (v = %d)\n", 
                                req->__state, req->tail + i, buffer[req->tail + i]);
                        req->status = -1;
                        return;
                    }

                    req->version_major = buffer[req->tail + i] - 48;
                    i++;

                    if (buffer[req->tail + i] != '.')
                    {
                        printf("At state %d: bad HTTP Version token at pos %d (v = %d)\n", 
                                req->__state, req->tail + i, buffer[req->tail + i]);
                        req->status = -1;
                        return;
                    }
                    i++;

                    if (buffer[req->tail + i] < 48 || buffer[req->tail + i] > 57)
                    {
                        printf("At state %d: bad HTTP Version digit token at pos %d (v = %d)\n", 
                                req->__state, req->tail + i, buffer[req->tail + i]);
                        req->status = -1;
                        return;
                    }
                    req->version_minor = buffer[req->tail + i] - 48;
                    i++;

                    if (c == '\r')
                    {
#ifdef DEBUG
                        printf("At state %d: change to state %d\n", req->__state,
                                HTTP_STATE_CR);
#endif /* ifdef DEBUG */
                        req->__state = HTTP_STATE_CR;
                    } else {
#ifdef DEBUG
                        printf("At state %d: change to state %d\n", req->__state,
                                HTTP_STATE_LF1);
#endif /* ifdef DEBUG */
                        req->__state = HTTP_STATE_LF1;
                    }
                    break;
                }
                break;
            case HTTP_STATE_CR:
                if (c != '\n')
                {
                    printf("At state %d: expected LF after CR at post %d but received %d\n",
                            req->__state, req->lead, c);
                    req->status = -1;
                    return;
                }
#ifdef DEBUG
                printf("At state %d: change to state %d\n", req->__state,
                        HTTP_STATE_LF1);
#endif /* ifdef DEBUG */
                req->__state = HTTP_STATE_LF1;
                break;
            case HTTP_STATE_LF1:
                if (c == '\r')
                {
#ifdef DEBUG
                    printf("At state %d: change to state %d\n", req->__state,
                            HTTP_STATE_LF2);
#endif /* ifdef DEBUG */
                    req->__state = HTTP_STATE_LF2;
                    break;
                }

                if (c == '\n')
                {
                    req->status = req->lead;
                    return;
                }

                if (!http_token[c])
                {
                    printf("At state %d: bad token at pos %d (v = %d)\n",
                            req->__state, req->lead, c);
                    printf("Forbid empty header name or line folding\n");
                    req->status = -1;
                    return;
                }

                req->tail = req->lead;
                req->headers[req->num_headers].header_name.start = req->tail;

#ifdef DEBUG
                printf("At state %d: change to state %d\n",
                        req->__state, HTTP_STATE_HEADER_NAME);
#endif /* ifdef DEBUG */
                req->__state = HTTP_STATE_HEADER_NAME;
                break;
            case HTTP_STATE_HEADER_NAME:
                if (c == ':')
                {
                    if (req->tail == req->lead)
                    {
                        printf("At state %d: forbid empty header name\n",
                                req->__state);
                        req->status = -1;
                        return;
                    }
                    req->headers[req->num_headers].header_name.end = req->lead - 1;

#ifdef DEBUG
                    printf("At state %d: change to state %d\n",
                            req->__state, HTTP_STATE_HEADER_COLON);
#endif /* ifdef DEBUG */
                    req->__state = HTTP_STATE_HEADER_COLON;
                    break;
                }
                if (!http_token[c])
                {
                    printf("At state %d: bad token at pos %d (v = %d)\n",
                            req->__state, req->lead, c);
                    req->status = -1;
                    return;
                }
                break;
            case HTTP_STATE_HEADER_COLON:
                if (c == ' ' || c == '\t') break; // Ignoring multiple spaces / tabs

                req->tail = req->lead;
                req->headers[req->num_headers].header_value.start = req->tail;
#ifdef DEBUG
                printf("At state %d: change to state %d\n",
                        req->__state, HTTP_STATE_HEADER_VALUE);
#endif /* ifdef DEBUG */
                req->__state = HTTP_STATE_HEADER_VALUE;
            case HTTP_STATE_HEADER_VALUE:
                if (c == '\r' || c == '\n')
                {
                    size_t i = req->lead - 1;
                    while (i >= req->tail && 
                           (buffer[i] == ' ' || buffer[i] == '\t'))
                    {
                        i--;
                    }
                    req->headers[req->num_headers].header_value.end = i;
                    req->num_headers++;

                    if (c == '\r')
                    {
#ifdef DEBUG
                        printf("At state %d: change to state %d\n",
                                req->__state, HTTP_STATE_CR);
#endif /* ifdef DEBUG */
                        req->__state = HTTP_STATE_CR;
                        break;
                    }
                    req->__state = HTTP_STATE_LF1;
#ifdef DEBUG
                    printf("At state %d: change to state %d\n",
                            req->__state, HTTP_STATE_LF1);
#endif /* ifdef DEBUG */
                    break;
                }

                if ((c < 0x20 && c != '\t') || (0x7F <= c))
                {
                    printf("At state %d: bad token for HTTP field value at pos %d (v = %d)",
                            req->__state, req->lead, c);
                    req->status = -1;
                    return;
                }
                break;
            case HTTP_STATE_LF2:
                if (c != '\n')
                {
                    printf("At state %d: bad token at pos %d (v = %d)\n",
                            req->__state, req->lead, c);
                    req->status = -1;
                    return;
                }
                req->status = req->lead;
                return;
        }
        req->lead += 1;
    }

    if (req->lead < buffer_cap)
    {
#ifdef DEBUG
        printf("At state %d: require further recv\n", req->__state);
        req->status = 0;
#endif /* ifdef DEBUG */
        return;
    }

    req->status = -1;
}
