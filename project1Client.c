#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include "project1.h"
//#include "clientCommands.c"

#define BUFSIZE 512

int DoNullTerminated(int sock)
{
  char receiveBuffer[BUFSIZE];
  char *message = commands[1].arg; //message first contains the command arg

  strcpy(message, strcat(message, "\0")); //Add null terminator
  strcpy(message, strcat("1", message) ); //append the command to the front
  printf("Sending to Server: %s\n", message);

  if(SendToServer(message, sock)) //Send server the null terminated string
  {
    return -1;
  }

  //Recieve
  if(ReceiveAndOutput(sock) == -1) //Get back servers response and do output
  {
    return -1;
  }
  return 0;
}

int DoGivenLength(int sock)
{
  uint16_t len;
  char *message;
  
  len = htonl( (uint16_t)strlen(message) ); //Convert the string length ot network byte order
  
  message = (char *) &len;  //turn network byte order length into a string
  strcpy(message, strcat(message, commands[2].arg)); //add the command arg string
  strcpy(message, strcat("2", message) ); //append the command to the front
  printf("Sending to Server: %s\n", message);

  if(SendToServer(message, sock) == -1) //Send to server
  {
    return -1;
  }
  if(ReceiveAndOutput(sock) == -1) //Get back servers response and do output
  {
    return -1;
  }
  return 0;
}

int DoBadInt(int sock)
{
  char *message;
  int unconverted = atoi(commands[3].arg); //convert to int

  message = (char *) &unconverted; //we will send WITHOUT applying htonl()
  strcpy(message, strcat("3", message) ); //append the command to the front
  printf("Sending to Server: %s\n", message);

  if(SendToServer(message, sock) == -1) //Send to server
  {
    return -1;
  }
  if(ReceiveAndOutput(sock) == -1) //Get back servers response and do output
  {
    return -1;
  }
  return 0;
}

int DoGoodInt(int sock)
{
  char *message;
  int converted = htonl( (int) atoi(commands[4].arg) ); //Convert to int and apply htonl()

  message = (char *) &converted; //Add the GoodInt to the message
  strcpy(message, strcat("4", message) ); //append the command to the front
  printf("Sending to Server: %s\n", message);

  if(SendToServer(message, sock) == -1) //Send to server
  {
    return -1;
  }
  if(ReceiveAndOutput(sock) == -1) //Get back servers response and do output
  {
    return -1;
  }
  return 0;
}

int DoByte(int sock)
{
  //printf("%s: ", commandNames[5]);
  //SendToServer("5", sock);
  return 0;
}

int DoKByte(int sock)
{
  //printf("%s: ", commandNames[6]);
  //SendToServer("6", sock);
  return 0;
}

int SendToServer(char* message, int sock) 
{
  int numBytes; //Number of bytes sent in each send
  unsigned int totalBytesSent = 0; // Count of total bytes sent to server
  int messageLen = strlen(message); // Determine input length

  while(totalBytesSent < messageLen) //Keep sending until we have sent the whole message
  { 
    numBytes = send(sock, message+totalBytesSent, messageLen - totalBytesSent, 0); // Send the string to the server
    if (numBytes < 0)
    {
      printf("Was not able to send\n");
      return -1;
    } 
    totalBytesSent += numBytes; //Add what we've read to the total
    if (totalBytesSent > messageLen)
    { 
      printf("We sent an unexpectedly larger number of bytes. Terminate\n");
      return -1;
    }
  }
  return 0;
}

int ReceiveAndOutput(int sock, char buffer[], int maxLength, int minReq)
{
  int messageLen;
  int numBytes = 0;  //How much we got from receive
  unsigned int totalBytesRcvd = 0; // Count of total bytes received
  uint16_t messageSize;
  
  printf("Entering first While loop of Receive\n");
  while(totalBytesRcvd < minReq) //Receive until we get the minimum amount needed for the length
  {
    numBytes = recv(sock, buffer, maxLength, 0);
  
    if (numBytes < 0){
      printf("Failed to recieve from server\n");
      continue;
    }
    else if (numBytes == 0){
      printf("Connection ot server closed unexpectedly\n");
      return -1;
    }
    totalBytesRcvd += numBytes;
  }
  
  totalBytesRcvd += numBytes;
  messageSize = (uint16_t) atoi(buffer);
  printf("IN RECEIVE: messageSize converted to: %d", messageSize); 

  while(totalBytesRcvd < messageSize) //Loop through and keep receiving until we've received the entire message
  {
    numBytes = recv(sock, buffer, maxLength, 0);
  
    if (numBytes < 0){
      printf("Failed to recieve from server\n");
      continue;
    }
    else if (numBytes == 0){
      printf("Connection ot server closed unexpectedly\n");
      return -1;
    }
    totalBytesRcvd += numBytes;
  }
  
  buffer[messageSize+2] = '\0';    // Terminate the string!
  fputs(buffer+2, stdout);      // Print the buffer without the 2byte length at the front
  fputc('\n', stdout); //Print out a new line for the next output that happens

  return 0;
}

int main(int argc, char *argv[]) {
  int i = 0;
  int sock, rtnVal, connectValue;
  in_port_t servPort;
  char *servIP, *port, *test;
  
  
  char buffer[BUFSIZE]; // I/O buffer
  int timeToEnd = 0;

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
  //printf("htons(servPort) returns: %d\n", htons(servPort));
  servAddr.sin_port = htons(servPort);    // Server port

  printf("Attempting Connection...");
  // Establish the connection to the echo server
  connectValue = connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr));
  if (connectValue < 0){
    printf("Failed to connect...\n");
    return 0;
  }
  printf("Now Connected to server\n");

  for(i = 0; i < sizeof(commands); i++) //Loop through all commands
  {
    if(timeToEnd){
      
      break;
    }
    switch(i){
      case nullTerminatedCmd:
        DoNullTerminated(sock);
        break;
      case givenLengthCmd:
        DoGivenLength(sock);
        break;
      case badIntCmd:
        DoBadInt(sock);
        break;
      case goodIntCmd:
        DoGoodInt(sock);
        break;
      case byteAtATimeCmd:
        DoByte(sock);
        break;
      case kByteAtATimeCmd:
        DoKByte(sock);
        break;
      case noMoreCommands:
        timeToEnd = 1;
        break;
      default:
        timeToEnd = 1;
    }
  }

  close(sock); //close the connection
  exit(0); //End Program









//THIS WILL BE USEFUL LATER  
/*
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
    
    // Receive up to the buffer size (minus 1 to leave space for a null terminator) bytes from the sender
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
  */
}
