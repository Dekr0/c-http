## My Attempt Of Building C HTTP Server From Square Zero

- The state of the source code for this HTTP Server is for exploration and 
learning purpose since I wrote it for one of the CodeCrafter Project.

- The only part that is written with good care is HTTP Request Parser.

- If I have any spare time, I will rewrite the logic for constructing router, 
and handling the request after parsing, and construct the HTTP response.

- Memory safety is also another largest concern I have despite Valgrind does not 
detect any memory leak when it's running in `leak-check=full`.
    - I will need to rewrite how string allocation and memory allocation for 
    network I/O in more clean and verbose manner.
    - Memory allocation and recycle should be done in higher up of call stack 
    instead of deeper call stack because passing pointer around can become tricky
    , and allocation might be forgotten to recycle.
    
