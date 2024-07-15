## Pointer

- If you want to make memory allocation inside of a function call, you need to 
pass in the reference of that point (**) instead of passing the pointer (*). If 
you pass the pointer but not the reference of that pointer, it will be treated 
as pass by value. The reference return from the memory allocation will not get 
assign to the pointer that lives in the caller stack.
- Preferably, make memory allocation and free happen high up in the call stack 
instead of lower up in the call stack. This avoid possibility of not freeing 
the memory or double freeing the memory.

## Memory

- Utilize stack memory as much as possible before heap memory.
    - Heap memory is slow.
    - Sys call relative to heap memory can be slow.
- Allocate resources (variables, pointer, structure, ...) in the stack memory 
in a such way that there is no gap in between and cache friendly.

## Buffer and I/O

- Things can become very tricky when you receive bytes partially in different 
sizes from different I/Os. It make it more difficult when you decide to parse 
bytes one by one under this context since there are situations can happen, such as 
you might need to read more bytes or more bytes are received when the parsing is 
finished but the cursor of your parser is not pointing to the next receiving 
position in the buffer.

## Unit Test

## Tooling

- Valgrind and its `leak-check=full -s`
