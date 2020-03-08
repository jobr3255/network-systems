**By:** Josh/Chloe Brown

# **Project 2**

Simple file transfer system between a client and server.

## Compile and run

Run the makefile to compile the files
```
PA2$ make
```
Now run the program, giving it the port number
```
PA2$ ./webserver [port]
```
The webserver root files are in PA2/www

## Included files

##### webserver.c
The main while loop to accept connections

##### helpers.c
Simple helper functions to extract data, check if a file exists, etc.

##### connections.c
The main file that handles the requests and sends back the appropriate responses.
