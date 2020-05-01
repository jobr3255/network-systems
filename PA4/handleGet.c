void handleGet(char *file_name, int sockets[4], char *username) {
	int n;
	char buf[BUFSIZE];
	char *file1, *file2, *trash;
	char fileName1[100], fileName2[100];
	int fileSize1, fileSize2;
	char okMsg[20] = "OK\n";
	char errorMsg[20] = "ERROR\n";

	// for (int i=0; i<4; i++) {
	// 	if (connections[i] != 1) {
	// 		continue;
	// 	}
	// }
	if (connections[0] == 1) {
		// Get primer from server (filename1,filename2)
		bzero(buf, sizeof(buf));
		if (read(sockets[0], buf, sizeof(buf)) < 0) {
			error("Error in receiving from server 1");
			write(sockets[0], errorMsg, sizeof(errorMsg));
		} else {
			// printf("Primer: %s", buf);
			trash = strtok(buf, username);
			trash = strtok(NULL, username);
			trash = strtok(NULL, "/");
			file1 = strtok(NULL, ",");
			trash = strtok(NULL, username);
			trash = strtok(NULL, username);
			trash = strtok(NULL, "/");
			file2 = strtok(NULL, "\n");
			// printf("Parsed files: %s, %s\n", file1, file2);
			strcpy(fileName1, file1);
			strcpy(fileName2, file2);

			// Return acknowledgement
			n = write(sockets[0], okMsg, strlen(okMsg));
			printf("Sent %d bytes of ACK: %s\n", n, okMsg);

			// Get content of file from servers (write to file)
			bzero(buf, sizeof(buf));
			n = read(sockets[0], buf, sizeof(buf));
			if (n < 0) {
				printf("Error receiving from server 1\n");
			} else {
				fileSize1 = atoi(buf);
				printf("File size: %d\n", fileSize1);
			}
			bzero(buf, sizeof(buf));
			n = read(sockets[0], buf, fileSize1);
			printf("Buffer: %s\n", buf);
			if (n < 0) {
				printf("Error receiving from server 1\n");
			} else {
				// Write piece to file on client side
				// printf("%s\n", file1);
				FILE *fptr = fopen(fileName1, "wb");
				if (fptr == NULL) {
					printf("Couldn't open file!\n");
				}
				else {
					printf("Server 1: Writing into file: %s\n", fileName1);
					// printf("Buffer1: %s\n", buf);
					fwrite(buf, 1, n, fptr);
					fclose(fptr);
				}
			}

			// Return acknowledgement
			write(sockets[0], okMsg, strlen(okMsg));

			// Second file
			bzero(buf, sizeof(buf));
			n = read(sockets[0], buf, sizeof(buf));
			if (n <= 0) {
				printf("Error receiving from server 1\n");
			} else {
				fileSize2 = atoi(buf);
				// printf("File size: %d\n", fileSize2);
			}
			bzero(buf, sizeof(buf));
			n = read(sockets[0], buf, fileSize2);
			// printf("Buffer: %s\n", buf);
			if (n < fileSize2) {
				printf("Error receiving from server 1: %d\n\n", n);
			} else {
				// Write piece to file on client side
				FILE *fptr = fopen(fileName2, "wb");
				if (fptr == NULL) {
					printf("Couldn't open file!\n");
				}
				else {
					printf("Server 1: Writing into file: %s\n", fileName2);
					fwrite(buf, 1, n, fptr);
					fclose(fptr);
				}
			}
		}
	}
	if (connections[1] == 1) {
		// Get primer from server (filename1,filename2)
		bzero(buf, sizeof(buf));
		if (read(sockets[1], buf, sizeof(buf)) < 0) {
			error("Error in receiving from server 2");
			write(sockets[1], errorMsg, strlen(errorMsg));
		} else {
			// printf("Primer: %s", buf);
			trash = strtok(buf, username);
			trash = strtok(NULL, username);
			trash = strtok(NULL, "/");
			file1 = strtok(NULL, ",");
			trash = strtok(NULL, username);
			trash = strtok(NULL, username);
			trash = strtok(NULL, "/");
			file2 = strtok(NULL, "\n");
			// printf("Parsed files: %s, %s\n", file1, file2);
			strcpy(fileName1, file1);
			strcpy(fileName2, file2);

			// Return acknowledgement
			write(sockets[1], okMsg, strlen(okMsg));

			// Get content of file from servers (write to file)
			bzero(buf, sizeof(buf));
			n = read(sockets[1], buf, sizeof(buf));
			if (n < 0) {
				printf("Error receiving from server 2\n");
			} else {
				fileSize1 = atoi(buf);
				// printf("File size: %d\n", fileSize1);
			}
			bzero(buf, sizeof(buf));
			n = read(sockets[1], buf, fileSize1);
			if (n < 0) {
				printf("Error receiving from server 2\n");
			} else {
				// Write piece to file on client side
				// printf("%s\n", file1);
				FILE *fptr = fopen(fileName1, "wb");
				if (fptr == NULL) {
					printf("Couldn't open file!\n");
				}
				else {
					printf("Server 2: Writing into file: %s\n", fileName1);
					fwrite(buf, 1, n, fptr);
					fclose(fptr);
				}
			}

			// Return acknowledgement
			write(sockets[1], okMsg, strlen(okMsg));

			// Second file
			bzero(buf, sizeof(buf));
			n = read(sockets[1], buf, sizeof(buf));
			if (n < 0) {
				printf("Error receiving from server 2\n");
			} else {
				fileSize2 = atoi(buf);
				// printf("File size: %d\n", fileSize2);
			}
			bzero(buf, sizeof(buf));
			n = read(sockets[1], buf, fileSize2);
			if (n < 0) {
				printf("Error receiving from server 2\n");
			} else {
				// Write piece to file on client side
				FILE *fptr = fopen(fileName2, "wb");
				if (fptr == NULL) {
					printf("Couldn't open file!\n");
				}
				else {
					printf("Server 2: Writing into file: %s\n", fileName2);
					fwrite(buf, 1, n, fptr);
					fclose(fptr);
				}
			}
		}
	}
	if (connections[2] == 1) {
		// Get primer from server (filename1,filename2)
		bzero(buf, sizeof(buf));
		n = read(sockets[2], buf, sizeof(buf));
		// printf("Read %d\n", n);
		if (n <= 0) {
			error("Error in receiving from server 3");
			write(sockets[2], errorMsg, strlen(errorMsg));
		} else {
			// printf("Primer: %s", buf);
			trash = strtok(buf, username);
			trash = strtok(NULL, username);
			trash = strtok(NULL, "/");
			file1 = strtok(NULL, ",");
			trash = strtok(NULL, username);
			trash = strtok(NULL, username);
			trash = strtok(NULL, "/");
			file2 = strtok(NULL, "\n");
			// printf("Parsed files: %s, %s\n", file1, file2);
			strcpy(fileName1, file1);
			strcpy(fileName2, file2);

			// Return acknowledgement
			write(sockets[2], okMsg, strlen(okMsg));

			// Get content of file from servers (write to file)
			bzero(buf, sizeof(buf));
			n = read(sockets[0], buf, sizeof(buf));
			if (n < 0) {
				printf("Error receiving from server 3\n");
			} else {
				fileSize1 = atoi(buf);
			}
			bzero(buf, sizeof(buf));
			n = read(sockets[2], buf, fileSize1);
			if (n < 0) {
				printf("Error receiving from server 3\n");
			} else {
				// Write piece to file on client side
				// printf("%s\n", file1);
				FILE *fptr = fopen(fileName1, "wb");
				if (fptr == NULL) {
					printf("Couldn't open file!\n");
				}
				else {
					printf("Server 3: Writing into file: %s\n", fileName1);
					fwrite(buf, 1, n, fptr);
					fclose(fptr);
				}
			}

			// Return acknowledgement
			write(sockets[2], okMsg, strlen(okMsg));

			// Second file
			bzero(buf, sizeof(buf));
			n = read(sockets[2], buf, sizeof(buf));
			if (n < 0) {
				printf("Error receiving from server 3\n");
			} else {
				fileSize2 = atoi(buf);
			}
			bzero(buf, sizeof(buf));
			n = read(sockets[2], buf, fileSize2);
			if (n < 0) {
				printf("Error receiving from server 3\n");
			} else {
				// Write piece to file on client side
				FILE *fptr = fopen(fileName2, "wb");
				flock(fileno(fptr),2);
				if (fptr == NULL) {
					printf("Couldn't open file!\n");
				}
				else {
					printf("Server 3: Writing into file: %s\n", fileName2);
					// printf("Buffer6: %s\n", buf);
					fwrite(buf, 1, n, fptr);
					flock(fileno(fptr), 8);
					fclose(fptr);
				}
			}
		}
	}
	if (connections[3] == 1) {
		// Get primer from server (filename1,filename2)
		bzero(buf, sizeof(buf));
		if (read(sockets[3], buf, sizeof(buf)) < 0) {
			error("Error in receiving from server 4");
			write(sockets[3], errorMsg, strlen(errorMsg));
		} else {
			// printf("Primer: %s", buf);
			trash = strtok(buf, username);
			trash = strtok(NULL, username);
			trash = strtok(NULL, "/");
			file1 = strtok(NULL, ",");
			trash = strtok(NULL, username);
			trash = strtok(NULL, username);
			trash = strtok(NULL, "/");
			file2 = strtok(NULL, "\n");
			// printf("Parsed files: %s, %s\n", file1, file2);
			strcpy(fileName1, file1);
			strcpy(fileName2, file2);

			// Return acknowledgement
			write(sockets[3], okMsg, strlen(okMsg));

			// Get content of file from servers (write to file)
			bzero(buf, sizeof(buf));
			n = read(sockets[3], buf, sizeof(buf));
			if (n < 0) {
				printf("Error receiving from server 4\n");
			} else {
				fileSize1 = atoi(buf);
			}
			bzero(buf, sizeof(buf));
			n = read(sockets[3], buf, sizeof(buf));
			if (n < 0) {
				printf("Error receiving from server 4\n");
			} else {
				// Write piece to file on client side
				// printf("%s\n", file1);
				FILE *fptr = fopen(fileName1, "wb");
				if (fptr == NULL) {
					printf("Couldn't open file!\n");
				}
				else {
					printf("Server 4: Writing into file: %s\n", fileName1);
					fwrite(buf, 1, n, fptr);
					fclose(fptr);
				}
			}

			// Return acknowledgement
			write(sockets[3], okMsg, strlen(okMsg));

			// Second file
			bzero(buf, sizeof(buf));
			n = read(sockets[3], buf, sizeof(buf));
			if (n < 0) {
				printf("Error receiving from server 4\n");
			} else {
				fileSize2 = atoi(buf);
			}
			bzero(buf, sizeof(buf));
			// printf("%d\n", fileSize2);
			n = read(sockets[3], buf, sizeof(buf));
			if (n < 0) {
				printf("Error receiving from server 4\n");
			} else {
				// Write piece to file on client side
				// printf("%s\n", fileName2);
				FILE *fptr = fopen(fileName2, "wb");
				flock(fileno(fptr), 2);
				if (fptr == NULL) {
					printf("Couldn't open file!\n");
				}
				else {
					printf("Server 4: Writing into file: %s\n", fileName2);
					fwrite(buf, 1, n, fptr);
					flock(fileno(fptr),8);
					fclose(fptr);
				}
			}
		}
	}

	// Check if enough to reconstruct file
	// Build file names...
	char piece1_name[100], piece2_name[100], piece3_name[100], piece4_name[100];
	sprintf(piece1_name, "%s.1", file_name);
	sprintf(piece2_name, "%s.2", file_name);
	sprintf(piece3_name, "%s.3", file_name);
	sprintf(piece4_name, "%s.4", file_name);
	// printf("%s\n", piece1_name);
	// Piece 1
	int file_complete = 1;
	FILE *fptr1 = fopen(piece1_name, "rb");
	if (fptr1 == NULL) {
		printf("Couldn't open file: %s\n", piece3_name);
		file_complete = 0;
	}
	// Piece 2
	FILE *fptr2 = fopen(piece2_name, "rb");
	if (fptr2 == NULL) {
		printf("Couldn't open file: %s\n", piece3_name);
		file_complete = 0;
	}
	// Piece 3
	FILE *fptr3 = fopen(piece3_name, "rb");
	if (fptr3 == NULL) {
		printf("Couldn't open file: %s\n", piece3_name);
		file_complete = 0;
	}
	// Piece 4
	FILE *fptr4 = fopen(piece4_name, "rb");
	if (fptr4 == NULL) {
		printf("Couldn't open file: %s\n", piece4_name);
		file_complete = 0;
	}

	// If all pieces available
	char pieces[4][1024];
	if (file_complete == 1) {
		// Reconstruct file
		char got_file[100];
		sprintf(got_file, "fetched_%s", file_name);
		FILE *fptr_whole = fopen(got_file, "wb");
		if (fptr3 == NULL) {
			error("Couldn't create file on client");
		} else {
			fclose(fptr_whole);
			// Piece 1
			bzero(pieces[0], sizeof(pieces[0]));
			fread(pieces[0], (size_t) fileSize1, 1, fptr1);
			// Piece 2
			bzero(pieces[1], sizeof(pieces[1]));
			fread(pieces[1], (size_t) fileSize1, 1, fptr2);
			// Piece 3
			bzero(pieces[2], sizeof(pieces[2]));
			fread(pieces[2], (size_t) fileSize1, 1, fptr3);
			// Piece 4
			bzero(pieces[3], sizeof(pieces[3]));
			fread(pieces[3], (size_t) fileSize1, 1, fptr4);

			FILE *fptr_whole = fopen(got_file, "a+");
			if (fptr3 == NULL) {
				error("Couldn't create file on client");
			} else {
				fprintf(fptr_whole, "%s%s%s%s", pieces[0], pieces[1], pieces[2], pieces[3]);
			}
			fclose(fptr_whole);
		}

	} else {
		// Throw error message
		printf("File is incomplete.\n");
	}

	// Delete & close partial files
	if (fptr1 != NULL) {
		fclose(fptr1);
		remove(piece1_name);
	}
	if (fptr2 != NULL) {
		fclose(fptr2);
		remove(piece2_name);
	}
	if (fptr3 != NULL) {
		fclose(fptr3);
		remove(piece3_name);
	}
	if (fptr4 != NULL) {
		fclose(fptr4);
		remove(piece4_name);
	}
}
