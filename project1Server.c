#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <stdint.h>
#include "project1.h"

#define RECVBUFSIZE 1000

int run;
int totalBytesRead;

void CtrlCTrap(int signal)
{
	run = 0;
}

void sendString(int socket, char *buf, int cmd)
{
	char *returnString;
	//write buffer to log file
	uint16_t size = (uint16_t)strlen(buf); //buf contents
	size += (uint16_t)strlen(commandNames[cmd]); //command name
	size += (uint16_t)2; //colon and space
	printf("Size of send: %d\n", size);
	strcpy(returnString, (char *)&size);

	strcat(returnString, commandNames[cmd]);
	strcat(returnString, ": ");
	strcat(returnString, buf);
	printf("Send string: %s\n", returnString);
	if(send(socket, returnString, strlen(returnString), 0) != strlen(returnString))
	{
		//printf("Failed to send");
	}
}

void sendNumRecvs(int socket, int numRecvs, int cmd)
{
	char *returnString;
	//write buffer to log file
	uint16_t size = (uint16_t)numRecvs; //numRecvs
	size += (uint16_t)strlen(commandNames[cmd]); //command name
	size += (uint16_t)2; //colon and space
	printf("Size of send: %d\n", size);
	strcpy(returnString, (char *)&size);

	strcat(returnString, commandNames[cmd]);
	strcat(returnString, ": ");
	strcat(returnString, (char *)&numRecvs);
	printf("Send string: %s\n", returnString);
	if(send(socket, returnString, strlen(returnString), 0) != strlen(returnString))
	{
		//printf("Failed to send");
	}
}

int nullTerminated(int socket, char *buf, int bytesRead) //'DONE'
{
	int recvMsgSize = 0;
	int offset = bytesRead;
	int i;
	printf("Offset: %d\n", offset);
	for(i = 1; i < bytesRead; i++) //look through bytes received in the buffer
	{
		if(buf[i] == '\0') //found null terminated byte
		{
			printf("Null terminator found!\n");
			sendString(socket, buf+1, 1); //exclude the cmd byte
			return;
		}
	}
	printf("test\n");
	while(1)
	{
		if((recvMsgSize = recv(socket, buf + bytesRead, RECVBUFSIZE - bytesRead, 0)) < 1) //expecting at least 1 byte
		{
			//failed to receive or connection severed
		}
		totalBytesRead += recvMsgSize; //increment total bytes read
		for(i = 0; i < recvMsgSize; i++)
		{
			if(buf[i+offset] == '\0') //found null terminated byte
			{
				printf("Test!\n");
				sendString(socket, buf+1, 1);  //exclude the cmd byte
				return;
			}
		}
		offset += recvMsgSize; //increment offset
	}
}

int givenLength(int socket, char *buf, int bytesRead) //'DONE'
{
	uint16_t length;
	char *strLen;
	int recvMsgSize = 0;
	length = (uint16_t)atoi(buf+1);
	printf("Length provided: %d\n", length);
	if(length == bytesRead - 3) //buf[0] is the cmd, buf[1] and buf[2] are the 16 bit length, rest is the string
	{
		sendString(socket, buf+3, 2);
		return; //am done
	}
	if((recvMsgSize = recv(socket, buf+bytesRead, length-bytesRead, 0)) < 0)
	{
		//print error
	}
	totalBytesRead += recvMsgSize; //increment total bytes read
	sendString(socket, buf+3, 2);
	return; //am done
}

int goodOrBadInt(int socket, char *buf, int bytesRead, int cmd)
{
	int recvMsgSize = 0;
	int bytes;
	unsigned short byteOrder;
	char *outputBuffer;
	if(bytesRead < 5) //should be 5, 1 for the cmd, 4 for the integer
	{
		if((recvMsgSize = recv(socket, buf+bytesRead, 5-bytesRead, 0)) < 0) //should receive rest of the bytes
		{
			//printf("Failed to recieve...\n");
		}
		totalBytesRead += recvMsgSize; //increment total bytes read
	}
	bytes = (int)atoi(buf+1);
	byteOrder = htonl(bytes);
	sprintf(outputBuffer, "%d", byteOrder);
	sendString(socket, outputBuffer, cmd);
}

int byteAtATime(int socket, char *buf)
{
	int recvMsgSize = 0;
	int numRecvs = 1; //cmd byte
	while((recvMsgSize = recv(socket, buf, 1, 0)) != 0) //still receiving bytes
	{
		numRecvs++;
		totalBytesRead += recvMsgSize; //increment total bytes read
	}
	printf("Byte at a time receives: %d\n", numRecvs);
	sendNumRecvs(socket, numRecvs, 5);
}

int kByteAtATime(int socket, char *buf)
{
	int recvMsgSize = 0;
	int numRecvs = 1; //cmd byte
	while((recvMsgSize = recv(socket, buf, 1000, 0)) != 0) //still receiving bytes
	{
		numRecvs++;
		totalBytesRead += recvMsgSize; //increment total bytes read
	}
	printf("KByte at a time receives: %d\n", numRecvs);
	sendNumRecvs(socket, numRecvs, 6);
}


int main(int argc, char *argv[]) //argv[1] is the port to listen to
{
	int servSock; //Socket descriptor for server
	int clntSock; //Socket descriptor for client
	in_port_t servPort;
	int cmd;
	int recvMsgSize;
	int clntAddrLen;
	char recvBuffer[32];
	struct sockaddr_in servAddr;
	struct sockaddr_in clntAddr;
        int errsv;

	run = 1;
	signal(SIGINT, &CtrlCTrap);
	if(argc != 2) //incorrect format
	{
          //print error message, inccorect number of arguments
          printf("Incorrect number of arguments passed\n");
          return 0;
	}
	servPort = atoi(argv[1]); //get port for argv[1]
        printf("servPort = %d\n", servPort);

        if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
          printf("Socket() Failed\n");
          return 0;
        }

	memset(&servAddr, 0, sizeof(servAddr)); //zero out structure
	servAddr.sin_family = AF_INET; //IPv4
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY); //any incoming interface
	servAddr.sin_port = htons(servPort); //local port

	if(bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
	{
          errsv = errno;
	  //print error message (bind failed)
          printf("Failed to bind...error %d\n", errsv);
          return 0;
	}
	if(listen(servSock, 5) < 0)
	{
	  printf("Listen() has failed...\n");
          return 0;
	}

        printf("Entering while(run) loop\n");
	while(run) //change to if ctrl-c is hit
	{
		clntAddrLen = sizeof(clntAddr);
		if((clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen)) < 0)
		{
			//printf("Failed to Accept...\n");
		}
		while(run)
		{
			if((recvMsgSize = recv(clntSock, recvBuffer, RECVBUFSIZE, 0)) < 0) //receive 1000 bytes (first read)
			{
				//printf("Failed to recieve...\n");
			}
			if(recvMsgSize == 0)
			{
				break;
			}
			printf("Message Receive size in main: %d\n", recvMsgSize);
			totalBytesRead += recvMsgSize;
			//read into log file
			printf("Buffer: %s\n", recvBuffer);
			cmd = (int)(recvBuffer[0]-'0');
			printf("Command number: %d\n", cmd);
			switch(cmd) //first byte is associated command
			{
				case 1: 
					printf("In Null terminated command\n");
					nullTerminated(clntSock, recvBuffer, recvMsgSize);
					break;
				case 2: 
					givenLength(clntSock, recvBuffer, recvMsgSize);
					break;
				case 3: 
					goodOrBadInt(clntSock, recvBuffer, recvMsgSize, 3);
					break;
				case 4: 
					goodOrBadInt(clntSock, recvBuffer, recvMsgSize, 4);
					break;
				case 5: 
					byteAtATime(clntSock, recvBuffer);
					break;
				case 6: 
					kByteAtATime(clntSock, recvBuffer);
					break;
			}
		}
		printf("Server read %d bytes\n", totalBytesRead); //print #bytes received
		close(clntSock);
	}
}
