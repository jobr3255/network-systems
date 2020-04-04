**By:** Josh/Chloe Brown

# **Project 3**

Simple file transfer system between a client and server.

## Compile and run

Run the makefile to compile the files
```
PA3$ make
```
Now run the program, giving it the port number
```
PA3$ ./webproxy [port]
```

## Included files

##### webproxy.c
The main while loop to accept connections

##### helpers.c
Simple helper functions to extract data, check if a file exists, etc.

##### connections.c
The main file that handles the requests and sends back the appropriate responses.
