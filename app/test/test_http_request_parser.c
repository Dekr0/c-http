#include <stdio.h>
#include <string.h>

#include "../http_parser.h"


int main() {
    struct http_request r = { 0 };
    r.__buf_cap = HTTP_REQUEST_HEADER_SIZE;
    r.__buf = malloc(HTTP_REQUEST_HEADER_SIZE * sizeof(char));

    const char *buf = "GET /user-agent HTTP/1.1\r\nHost: localhost:4221\r\nUser-Agent: foobar/1.2.3\r\nAccept: */*\r\n\r\n";

    int rcode = parse_http_request(&r, buf, strlen(buf));

    printf("Parser status: %d\n", rcode);

    int method_len;
    char *method;
    if ((method_len = get_method(&r, &method)) == -1) {
        printf("method malloc error\n");
        return 1;
    }

    int uri_len;
    char *uri;
    if ((uri_len = get_uri(&r, &uri)) == -1) {
        printf("uri malloc error\n");
        return 1;
    }

    int host_len;
    char *host;
    if ((host_len = get_header(&r, "Host", &host)) == -1) {
        printf("host malloc error\n");
        return 1;
    } else if (!host_len) {
        printf("Header field 'Host' is an empty header\n");
        host = "";
    }

    int user_agent_len;
    char *user_agent;
    if ((user_agent_len = get_header(&r, "User-Agent", &user_agent)) == -1) {
        printf("user_agent malloc error\n");
        return 1;
    } else if (!user_agent_len) {
        printf("Header field 'User-Agent' is an empty header\n");
        user_agent = "";
    }

    int accpet_len;
    char *accept;
    if ((accpet_len = get_header(&r, "Accept", &accept)) == -1) {
        printf("accpet malloc error\n");
        return 1;
    } else if (!accpet_len) {
        printf("Header field 'Accept' is an empty header\n");
        accept = "";
    }

    int rsrc_len;
    char *rsrc;
    if ((rsrc_len = get_rsrc(&r, &rsrc)) == -1) {
        printf("rsrc malloc error\n");
    }

    printf("method    :%s\n", method);
    printf("URI       :%s\n", uri);
    printf("Host      :%s\n", host);
    printf("User-Agent:%s\n", user_agent);
    printf("Accept    :%s\n", accept);
    printf("Resource  :%s\n", rsrc);
    
    free(method);
    free(uri);
    free(host);
    free(user_agent);
    free(accept);
    free(rsrc);

    return 0;
}
