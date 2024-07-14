#include <stdio.h>
#include <string.h>

#include "../http.h"


int main() {
    struct http_request req = { 0 };
    req.ver_maj = 1;
    req.ver_min = 1;

    struct http_response *res = init_http_response(&req);
    if (res == NULL) {
        return 1;
    }

    write_response_status(res, "200", "OK");

    printf("len = %zu; cursor = %d\n", strlen(res->__buf), res->__cursor);
    printf("status code: start at %d and end at %d\n", res->status_code.beg,
            res->status_code.end);
    printf("status text: start at %d and end at %d\n", res->status_text.beg,
            res->status_text.end);
    printf("%s\n", res->__buf);
    
    free(res->__buf);
    free(res);
}
