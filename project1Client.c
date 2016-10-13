//Sean Brasier and Erik Lystad

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

	message[0] = 1; //associated cmd byte
  	strcpy(message+1, arg); //copy the arg string into the message
	strcat(message+1, "\0"); //Add null terminator
	if((bytesRead = send(socket, message, strlen(message) + 1, 0)) < 0) //Send server the null terminated string
	{
		return -1;
	}
	if(ReceiveAndOutput(socket) == -1) //Get back servers response and do output
	{
		return -1;
	}
	return 0;
}

int DoGivenLength(int sock, char *arg)
{
	uint16_t length;
	char message[BUFSIZE];

	message[0] = 2; //associated cmd byte
	length = htons(strlen(arg)); //convert to network byte short
	memcpy(message+1, &length, sizeof(length)); //copy 2 byte length into the message
	strcpy(message+3, arg); //add the command arg to the string
	if(send(sock, message, strlen(arg)+3, 0) < 0) //Send to server
	{
		return -1;
	}
	if(ReceiveAndOutput(sock) == -1) //Get back servers response and do output
	{
		return -1;
	}
	return 0;
}

int DoBadInt(int socket, char *arg)
{
	char message[BUFSIZE];
	int badInt;

	badInt = atoi(arg); //convert command arg to an integer
	message[0] = 3; //associated cmd byte
	memcpy(message+1, &badInt, sizeof(badInt)); //mem copy integer into message
	if(send(socket, message, sizeof(badInt)+1, 0) < 0) //Send to server
	{
		return -1;
	}
	if(ReceiveAndOutput(socket) == -1) //Get back servers response and do output
	{
		return -1;
	}
	return 0;
}

int DoGoodInt(int socket, char *arg)
{
	char message[BUFSIZE];
	int number;
	uint32_t goodInt;

	number = atoi(arg); //convert command arg into an integer
	goodInt = htonl(number); //convert integer to network byte long
	message[0] = 4; //associated cmd byte
	memcpy(message+1, &goodInt, sizeof(goodInt)); //mem copy 4 byte integer into the message
	if(send(socket, message, sizeof(goodInt)+1, 0) < 0) //Send to server
	{
		return -1;
	}
	if(ReceiveAndOutput(socket) == -1) //Get back servers response and do output
	{
		return -1;
	}
	return 0;
}

int sendXBytes(int socket, char *arg, int xBytes, int cmd)
{
	char message[BUFSIZE];
	int number, bytesSent = 0;
	int recvMsgSize = 0;
	uint32_t numBytes;

	number = atoi(arg); //convert command arg to an integer
	numBytes = htonl(number); //convert integer to network byte long
	message[0] = cmd; //associated cmd byte
	memcpy(message+1, &numBytes, sizeof(numBytes)); //mem copy 4 byte integer into the message
	if(send(socket, message, sizeof(numBytes)+1, 0) < 0) //Send to server
	{
		return -1;
	}
	while(bytesSent != number) //need to send numBytes amount of bytes
	{
		if(bytesSent % 1000 == 0) //alternate every 1000 bytes between 1's and 0's
		{
			if((bytesSent / 1000) % 2 == 0) //even
			{
				memset(message, 0, BUFSIZE); //fill up buffer with 0 bytes
			}
			else //odd
			{
				memset(message, 1, BUFSIZE); //fill up buffer with 1 bytes
			}
		}
		if((recvMsgSize = send(socket, message, xBytes, 0)) < 0) //send x amount of bytes to the server
		{
			return -1;
		}
		bytesSent += recvMsgSize;
	}
	if(ReceiveAndOutput(socket) == -1) //Get back servers response and do output
	{
		return -1;
	}
	return 0;
}

int ReceiveAndOutput(int socket)
{
	char buffer[BUFSIZE];
	int messageLen;
	int numBytes = 0;  //How much we got from receive
	unsigned int totalBytesRcvd = 0; // Count of total bytes received
	uint16_t messageSize;

	while(totalBytesRcvd < 2) //Receive until we get 2 bytes (length of buffer)
	{
		if((numBytes = recv(socket, buffer, BUFSIZE - totalBytesRcvd, 0)) < 0) //receive from server
		{
			printf("Receive failed\n");
			return -1;
		}
		else if (numBytes == 0) //connection closed, close program
		{
			printf("Connection to server closed unexpectedly\n");
			return -1;
		}
		totalBytesRcvd += numBytes;
	}
	memcpy(&messageSize, buffer, 2); //mem copy the length from the buffer into messageSize
	messageLen = (int)ntohs(messageSize); //convert memory size to host byte short and cast as an integer
	while(totalBytesRcvd < messageLen+2) //Loop through and keep receiving until we've received messageLength amount of bytes
	{
		if((numBytes = recv(socket, buffer+totalBytesRcvd, BUFSIZE, 0)) < 0) //receive from server
		{
			printf("Receive failed\n");
			return -1;
		}
		else if (numBytes == 0) //connection closed, close program
		{
			printf("Connection to server closed unexpectedly\n");
			return -1;
		}
		totalBytesRcvd += numBytes;
	}
	buffer[messageLen+2] = '\0'; //terminate the string
	printf("%s\n", buffer+2); //print string to stdout
	return 0;
}

//Used the code from Donahoo/Calvert as a basis for our main
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
	//set the port
	servPort = atoi(port);
	// Create a reliable, stream socket using TCP
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
	{
		printf("Failed to get socket\n");
		return 0;
	}
	// Construct the server address structure
	struct sockaddr_in servAddr;            // Server address
	memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
	servAddr.sin_family = AF_INET;          // IPv4 address family
	// Convert address
	rtnVal = inet_pton(AF_INET, servIP, &servAddr.sin_addr.s_addr);
	if (rtnVal <= 0)
	{
		printf("Changing of Address to binary has failed\n");
		return 0;
	}
	servAddr.sin_port = htons(servPort);    // Server port
	// Establish the connection to the echo server
	connectValue = connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr));
	if (connectValue < 0)
	{
		printf("Failed to connect...\n");
		return 0;
	}
	for(i = 0;; i++) //Loop through all commands (will exit upon hitting no more commands)
	{
		if(timeToEnd) //no more commands, exit
		{
			break;
		}
		switch(commands[i].cmd)
		{
			case nullTerminatedCmd:
				DoNullTerminated(sock, commands[i].arg);
				break;
			case givenLengthCmd:
				DoGivenLength(sock, commands[i].arg);
				break;
			case badIntCmd:
				DoBadInt(sock, commands[i].arg);
				break;
			case goodIntCmd:
				DoGoodInt(sock, commands[i].arg);
				break;
			case byteAtATimeCmd:
				sendXBytes(sock, commands[i].arg, 1, commands[i].cmd);
				break;
			case kByteAtATimeCmd:
				sendXBytes(sock, commands[i].arg, 1000, commands[i].cmd);
				break;
			case noMoreCommands:
				timeToEnd = 1;
				break;
			default:
				timeToEnd = 1;
				break;
		}
	}
	close(sock); //close the connection
	exit(0); //End Program
}
