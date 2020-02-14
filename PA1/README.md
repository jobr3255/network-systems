**By:** Josh/Chloe Brown

# **Project 1**

Simple file transfer system between a client and server.

## Compile and run

#### Client

Run the makefile inside the `client/` directory.
```
PA1/client$ make
```
Now run the program, giving it the address of the server and what port.
```
PA1/client$ ./uftp_client [address] [port]
```

#### Server

Run the makefile inside the `server/` directory.
```
PA1/server$ make
```
Now run the program, giving it the port to listen on.
```
PA1/server$ ./uftp_server [port]
```

## Commands

##### get [filename]

Retrieves a file from the server and saves it locally

##### put [filename]

Sends a file to the server to be saved on the server

##### delete [filename]

Deletes a file from the server

##### ls

Lists all the files on the server

##### exit

Shuts down the server gracefully
