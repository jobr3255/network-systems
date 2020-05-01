if (connections[0] == 1) {
  if (hash == 0) {
    // Send 1 & 2 (primer)
    sprintf(primers[0], "0,%d,1,%d\n", size1, size234);
    // printf("%s", primers[0]);
    write(sockets[0], primers[0], strlen(primers[0]));

    // Wait for response from server
    read(sockets[0], server_ack, sizeof(server_ack));
    if (strncmp(server_ack, "OK\n", 2) == 0) {
      // Send pieces to servers
      write(sockets[0], filePieces[0], strlen(filePieces[0]));
      write(sockets[0], filePieces[1], strlen(filePieces[1]));
    } else {
      printf("Failed comparison\n");
    }
  } else if (hash == 1) {
    // Send 3 & 0
    sprintf(primers[0], "3,%d,0,%d\n", size234, size1);
    write(sockets[0], primers[0], strlen(primers[0]));

    read(sockets[0], server_ack, sizeof(server_ack));
    if (strcmp(server_ack, "OK\n") == 0) {
      write(sockets[0], filePieces[3], strlen(filePieces[3]));
      write(sockets[0], filePieces[0], strlen(filePieces[0]));
    }
  } else if (hash == 2) {
    // Send 2 & 3
    sprintf(primers[0], "2,%d,3,%d\n", size234, size234);
    write(sockets[0], primers[0], strlen(primers[0]));

    read(sockets[0], server_ack, sizeof(server_ack));
    if (strcmp(server_ack, "OK\n") == 0) {
      write(sockets[0], filePieces[2], strlen(filePieces[2]));
      write(sockets[0], filePieces[3], strlen(filePieces[3]));
    }
  } else if (hash == 3) {
    // Send 1 & 2
    sprintf(primers[0], "1,%d,2,%d\n", size234, size234);
    write(sockets[0], primers[0], strlen(primers[0]));

    read(sockets[0], server_ack, sizeof(server_ack));
    if (strcmp(server_ack, "OK\n") == 0) {
      write(sockets[0], filePieces[1], strlen(filePieces[1]));
      write(sockets[0], filePieces[2], strlen(filePieces[2]));
    }
  }
}
