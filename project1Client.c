#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "project1.h"

#define BUFSIZE 512

int main(int argc, char *argv[]) {
  int sock, rtnVal;
  in_port_t servPort;
  char *servIP, *port, *test;
  ssize_t numBytes;
  size_t messageLen;
  char buffer[BUFSIZE]; // I/O buffer
  
  test = "thing";

  if (argc != 3){ // Test for correct number of arguments
    printf("We got an incorrect number of inputs\n");
    return 0;
  }

  servIP = argv[1];     // First arg: server IP address (dotted quad)
  port = argv[2]; // Second arg: string to echo
  
  printf("We were passed IPAddress: %s and Port %s\n", servIP, port);

  //set the port
  servPort = atoi(port);
  printf("servPort = %d\n", servPort);
  // Create a reliable, stream socket using TCP
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0){
    printf("Failed to get socket\n");
    return 0;
  }
  printf("Socket Successfully Created\n");

  // Construct the server address structure
  struct sockaddr_in servAddr;            // Server address
  memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
  servAddr.sin_family = AF_INET;          // IPv4 address family
  
  printf("Server Address Structure Susccessfully Constructed\n");
  
  // Convert address
  rtnVal = inet_pton(AF_INET, servIP, &servAddr.sin_addr.s_addr);
  if (rtnVal <= 0){
    printf("Changing of Address to binary has failed\n");
    return 0;
  }
  printf("Address Converted\n");
  printf("htons(servPort) returns: %d\n", htons(servPort));
  servAddr.sin_port = htons(servPort);    // Server port

  printf("Attempting Connection...");
  // Establish the connection to the echo server
  if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
    printf("Failed to connect...\n");
    return 0;
  }
  printf("Now Connected to server\n");

  messageLen = strlen(test); // Determine input length

  // Send the string to the server
  numBytes = send(sock, test, messageLen, 0);
  if (numBytes < 0){
    printf("Was not able to send\n");
    return 0;
  } 
  else if (numBytes != messageLen){
    printf("We sent an unexpected number of bytes. Terminate\n");
    return 0;
  }

  // Receive the same string back from the server
  unsigned int totalBytesRcvd = 0; // Count of total bytes received
  fputs("Received: ", stdout);     // Setup to print the echoed string

  while (totalBytesRcvd < messageLen) {
    
    /* Receive up to the buffer size (minus 1 to leave space for a null terminator) bytes from the sender */
    numBytes = recv(sock, buffer, BUFSIZE - 1, 0);
    if (numBytes < 0){
      printf("Failed to recieve from server\n");
      return 0;
    }
    else if (numBytes == 0){
      printf("Connection ot server closed unexpectedly\n");
      return 0;
    }

    totalBytesRcvd += numBytes; // Keep tally of total bytes
    buffer[numBytes] = '\0';    // Terminate the string!
    fputs(buffer, stdout);      // Print the echo buffer
  }

  fputc('\n', stdout); // Print a final linefeed

  close(sock);
  exit(0);
}
