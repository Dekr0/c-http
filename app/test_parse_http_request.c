#include <stdio.h>

#include "http_request.h"


int main()
{
    char *request = "GET /index.html";
    struct http_request req = { 0 };

    int r = parse_http_request(&req, request, 16, 16);

    printf("parse return code     : %d\n", r);
    printf("parse lead and tail   : %d and %d\n", req.lead, req.tail);
    printf("state                 : %d\n", req.http_state);
    printf("method (start and end): %d and %d\n", req.method.start, req.method.end);
    printf("uri    (start and end): %d and %d\n", req.uri.start, req.uri.end);
    
    return 0;
}

