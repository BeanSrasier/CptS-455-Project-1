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


int DoNullTerminated(int socket, char *arg)
{
	char message[1000];
	int bytesRead = 0;
	strcpy(message, "1");
  	strcat(message, arg); //cat the message onto the cmd
	strcat(message, "\0"); //Add null terminator
	printf("Sending to Server: %s\n", message);

	if((bytesRead = send(socket, message, strlen(message) + 1, 0)) < 0) //Send server the null terminated string
	{
		//error sending
	}
	printf("Sent %d bytes to the server\n", bytesRead);

	//Recieve
	if(ReceiveAndOutput(socket) == -1) //Get back servers response and do output
	{
		return -1;
	}
	return;
}

int DoGivenLength(int sock, char *arg)
{
	unsigned short i;
	int tempLength;

	uint16_t len;
	char message[1024], temp[1024];
	char lo, hi;

	tempLength = strlen(commands[1].arg);
	i = htons(tempLength);
	*((unsigned short *) temp) = i;
	printf("temp is: %s \n\n", temp);

	//printf("TRYING DoGivenLength()\n");
	printf("Length to send BEFORE htonl: %d\n", strlen(commands[1].arg));
	len = htons((uint16_t)strlen(commands[1].arg)); //Convert the string length to network byte order
	printf("Length to send AFTER htonl: %d\n", len);
	strcpy(message, "2");

	memcpy(message+1, (void *)&len, sizeof(len));
	//printf("temp AFTER memcpy:%s\n", temp);

	/*lo = len & 0xFF;
	hi = len >> 8;
	message[1] = lo;
	message[2] = hi;*/
	//strcat(message, itoa(len));  //turn network byte order length into a string

	strcat(message, temp);
	strcat(message, commands[1].arg); //add the command arg string
	printf("Sending to Server: %s\n", message);

	/*if(SendToServer(sock, message, strlen(message)) == -1) //Send to server
	{
		return -1;
	}*/
	if(ReceiveAndOutput(sock) == -1) //Get back servers response and do output
	{
		return -1;
	}
	return;
}

int DoBadInt(int sock, char *arg)
{
	char message[1000];
	int unconverted = atoi(commands[2].arg); //convert to int

	//message = (char *) &unconverted; //we will send WITHOUT applying htonl()
	strcpy(message, "1");
	memset(message, unconverted, sizeof(unconverted));
	printf("Sending to Server: %s\n", message);

	/*if(SendToServer(sock, message, strlen(message)) == -1) //Send to server
	{
		return -1;
	}*/
	if(ReceiveAndOutput(sock) == -1) //Get back servers response and do output
	{
		return -1;
	}
	return;
}

int DoGoodInt(int sock, char *arg)
{
	char message[1024];
	int converted = htonl( (int) atoi(commands[3].arg) ); //Convert to int and apply htonl()

	//message = (char *) &converted; //Add the GoodInt to the message
	strcpy(message, strcat("4", message) ); //append the command to the front
	printf("Sending to Server: %s\n", message);

	/*if(SendToServer(sock, message, strlen(message)) == -1) //Send to server
	{
		return -1;
	}*/
	if(ReceiveAndOutput(sock) == -1) //Get back servers response and do output
	{
		return -1;
	}
	return;
}

int DoByte(int sock, char *arg)
{
	//printf("%s: ", commandNames[5]);
	//SendToServer("5", sock, strlen(message));
	return 0;
}

int DoKByte(int sock, char *arg)
{
	//printf("%s: ", commandNames[6]);
	//SendToServer("6", sock, strlen(message));
	return 0;
}

int ReceiveAndOutput(int socket)
{
	char buffer[BUFSIZE];
	int messageLen;
	int numBytes = 0;  //How much we got from receive
	unsigned int totalBytesRcvd = 0; // Count of total bytes received
	uint16_t messageSize;

	printf("Entering first While loop of Receive\n");
	while(totalBytesRcvd < 2) //Receive until we get 2 bytes (length of buffer)
	{
		if((numBytes = recv(socket, buffer, BUFSIZE, 0)) < 0)
		{
			printf("Receive failed\n");
		}
		else if (numBytes == 0)
		{
			printf("Connection to server closed unexpectedly\n");
			return -1;
		}
		totalBytesRcvd += numBytes;
		printf("We have now recieved: %d bytes\n", totalBytesRcvd);
	}
	printf("Out of the first loop!\n");
	memcpy(&messageSize, buffer, 2);
	messageLen = (int)ntohs(messageSize);
	printf("Message Len: %d\n", messageLen);

	while(totalBytesRcvd < messageLen+2) //Loop through and keep receiving until we've received the entire message
	{
		if((numBytes = recv(socket, buffer+totalBytesRcvd, BUFSIZE, 0)) < 0)
		{
			printf("Receive failed\n");
		}
		else if (numBytes == 0)
		{
			printf("Connection to server closed unexpectedly\n");
			return -1;
		}
		totalBytesRcvd += numBytes;
	}

	buffer[messageSize+2] = '\0';    // Terminate the string!
	printf("%s\n", buffer+2);
	//fputs(buffer+2, stdout);      // Print the buffer without the 2byte length at the front
	//fputc('\n', stdout); //Print out a new line for the next output that happens
	return 0;
}

int main(int argc, char *argv[])
{
	int i = 0;
	int sock, rtnVal, connectValue;
	in_port_t servPort;
	char *servIP, *port, *test;


	char buffer[BUFSIZE]; // I/O buffer
	int timeToEnd = 0;

	if (argc != 3) // Test for correct number of arguments
	{
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
	if (sock < 0)
	{
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
	if (rtnVal <= 0)
	{
		printf("Changing of Address to binary has failed\n");
		return 0;
	}
	printf("Address Converted\n");
	//printf("htons(servPort) returns: %d\n", htons(servPort));
	servAddr.sin_port = htons(servPort);    // Server port

	printf("Attempting Connection...");
	// Establish the connection to the echo server
	connectValue = connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr));
	if (connectValue < 0)
	{
		printf("Failed to connect...\n");
		return 0;
	}
	printf("Now Connected to server\n");

	for(i = 0;; i++) //Loop through all commands (will exit upon hitting no more commands)
	{
		if(timeToEnd)
		{
			break;
		}
		switch(commands[i].cmd)
		{
			case nullTerminatedCmd:
				printf("PROCESS NULL COMMAND\n");
				DoNullTerminated(sock, commands[i].arg);
				break;
			/*case givenLengthCmd:
				DoGivenLength(sock, commands[i].arg);
				break;
			case badIntCmd:
				printf("PROCESS BadINT COMMAND\n");
				DoBadInt(sock, commands[i].arg);
				break;
			case goodIntCmd:
				printf("PROCESS GoodINT COMMAND\n");
				DoGoodInt(sock, commands[i].arg);
				break;
			case byteAtATimeCmd:
				printf("PROCESS BYTEATATIME COMMAND\n");
				DoByte(sock, commands[i].arg);
				break;
			case kByteAtATimeCmd:
				printf("PROCESS KBYTESATEATIME COMMAND\n");
				DoKByte(sock, commands[i].arg);
				break;*/
			case noMoreCommands:
				printf("NO MORE COMMANDS\n");
				timeToEnd = 1;
				break;
			default:
				printf("Entering default case\n");
				timeToEnd = 1;
				break;
		}
	}
	close(sock); //close the connection
	exit(0); //End Program
}
