#!/bin/sh
#
# DON'T EDIT THIS!
#
# CodeCrafters uses this file to test your code. Don't make any changes here!
#
# DON'T EDIT THIS!
set -e
tmpFile=$(mktemp)
gcc -lcurl -lz app/*.c -o $tmpFile
exec "$tmpFile" "$@"
(sleep 3 && printf "GET / HTTP/1.1\r\n\r\n") | nc localhost 4221 &
(sleep 3 && printf "GET / HTTP/1.1\r\n\r\n") | nc localhost 4221 &
(sleep 3 && printf "GET / HTTP/1.1\r\n\r\n") | nc localhost 4221 &
