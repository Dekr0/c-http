## Fault Tolerance

### I/O

- `I/O` timeout

### Connectivity

- Handle common abnormal behavior and exception that can happen during a TCP 
session.
    - Connection abort before `accept` returns

### Host

- Handle crashing and rebooting of server **host** 
- Handle normal shutdown of server **host** 

### Corrupted Data

- In one of the test case, the receiving user agent with corrupted bytes.

## I/O

- Handling incomplete request segment

## Refactor

### HTTP Response

- Routing
- Handling requests after parsing are done
- Response construction
