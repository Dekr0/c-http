#ifndef HTTP_MESSAGE_PARSER_H 
#define HTTP_MESSAGE_PARSER_H

#include <stdlib.h>

#include "http.h"


/**
 * The robustness of this HTTP Request Parser is not reliable. Thus, there are 
 * cases it cannot handle situation where an incoming HTTP request is malformed 
 * in terms of structure and format
 * 
 * The correctness of each HTTP request token is handled by handler. The parser 
 * solely take the responsibility of splitting out token.
 */
void parse_http_request(
        struct http_request *,
        const char *, 
        size_t,
        size_t);


#endif
