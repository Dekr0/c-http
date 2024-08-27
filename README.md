## HTTP Server in C

- This a bare bone HTTP server built during the free month in https://app.codecrafters.io/
- This HTTP server has 
    - a fully functional HTTP Request Parser that runs in linear time and 
    require zero memory allocation (operate on existing buffer),
    - the ability to handle concurrent request,
    - support `gzip` compression.
- Here are the current pending taksed of this HTTP server
    - provide an implementation of thread pool and an implementation of event loop for concurrent handling
    - provide a set of helper function for extracting different parts of a HTTP request instead of manually
    navigating the buffer
    - provide simple dynamic endpoint routing
    - optimiz HTTP server performance under extreme high load (target: ??? request / seconds)
