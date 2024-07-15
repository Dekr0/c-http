## HTTP Server in C

- This a bare bone HTTP server built during the free month in 
https://app.codecrafters.io/
- This HTTP server has 
    - a fully functional HTTP Request Parser that runs in linear time and 
    require zero memory allocation (operate on existing buffer),
    - the ability to handle concurrent request,
    - support `gzip` compression.
- This HTTP server was written in a rush. There are a lot places I want to 
improve if I have the time to do so.
    - process pool / thread pool
    - async. I/O 
    - refactor 
        - reading request body
        - extracting request fields
        - construct route and handler
    ...
