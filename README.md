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

- Memory efficiency is also another concern I have. There are char array (string)
being allocated across different places for convenience. There are a mix of 
heap allocation and stack allocation.

- For the current scope of this HTTP server, it should utilize stack memory more 
instead constantly allocate it from heap and recycle it. Heap is slow and system 
call for heap is slow as well.

- Units test is a must have since there are so many places will go wrong.

- Ensure each module is unit test thoroughly before hook them up together.
