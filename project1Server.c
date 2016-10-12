#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include "project1.h"

int totalBytesRead;
FILE *outfile;

void sendString(int socket, char *buf, int cmd)
{
	char returnString[BUFSIZE];
	int lengthOfString;
	uint16_t size;

	strcpy(returnString+2, commandNames[cmd]);
	strcat(returnString+2, ": ");
	strcat(returnString+2, buf);

	lengthOfString = strlen(returnString+2);
	size = htons(lengthOfString);

	printf("Size of send(uint16): %x\n", size);

	memcpy(returnString, &size, sizeof(size));

	printf("Send string: %s\n", returnString+2);
	printf("Size of send string: %d\n", strlen(returnString+2));
	if(send(socket, returnString, lengthOfString+2, 0) != lengthOfString+2)
	{
		printf("Failed to send");
	}
}

void sendNumRecvs(int socket, int numRecvs, int cmd)
{
	char *returnString;
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
		printf("Failed to send");
	}
}

int nullTerminated(int socket, char *buf, int bytesRead) //'DONE'
{
	int recvMsgSize = 0;
	while(1)
	{
		if(buf[bytesRead - 1] == '\0')
		{
			printf("Null terminator found!\n");
			fputs(buf, outfile);
			sendString(socket, buf+1, nullTerminatedCmd); //exclude the cmd byte
			return 0;
		}
		if((recvMsgSize = recv(socket, buf + bytesRead, BUFSIZE - bytesRead, 0)) < 1)
		{
			//failed to receive or connection severed
			return -1;
		}
		totalBytesRead += recvMsgSize; //increment total bytes read
		bytesRead += recvMsgSize;
	}
}

int givenLength(int socket, char *buf, int bytesRead) //'DONE'
{
	uint16_t length;
	int stringLength;
	int recvMsgSize = 0;
	while(bytesRead < 3)
	{
		if((recvMsgSize = recv(socket, buf+bytesRead, BUFSIZE - bytesRead, 0)) < 0)
		{
			printf("Receive error\n");
		}
		bytesRead += recvMsgSize;
	}
	memcpy(&length, buf+1, 2); //length of buffer
	stringLength = (int)ntohs(length);
	printf("Length provided: %d\n", stringLength);
	while(1)
	{
		if(stringLength == bytesRead - 3) //buf[0] is the cmd, buf[1] and buf[2] are the 16 bit length, rest is the string
		{
			printf("testing\n");
			fputs(buf, outfile);
			sendString(socket, buf+3, givenLengthCmd);
			return 0; //am done
		}
		if((recvMsgSize = recv(socket, buf+bytesRead, length-bytesRead, 0)) < 0)
		{
			printf("Receive error\n");
		}
		bytesRead += recvMsgSize;
		totalBytesRead += recvMsgSize;
	}
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
	fputs(buf, outfile);
	sendString(socket, outputBuffer, cmd);
}

int byteAtATime(int socket, char *buf)
{
	int recvMsgSize = 0;
	int numRecvs = 1; //cmd byte
	fputs(buf, outfile);
	while((recvMsgSize = recv(socket, buf, 1, 0)) != 0) //still receiving bytes
	{
		numRecvs++;
		fputs(buf, outfile);
		totalBytesRead += recvMsgSize; //increment total bytes read
	}
	printf("Byte at a time receives: %d\n", numRecvs);
	sendNumRecvs(socket, numRecvs, 5);
}

int kByteAtATime(int socket, char *buf)
{
	int recvMsgSize = 0;
	int numRecvs = 1; //cmd byte
	fputs(buf, outfile);
	while((recvMsgSize = recv(socket, buf, 1000, 0)) != 0) //still receiving bytes
	{
		numRecvs++;
		fputs(buf, outfile);
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

	if(argc != 2) //incorrect format
	{
		printf("Incorrect number of arguments passed\n");
		return 0;
	}
	servPort = atoi(argv[1]); //get port from argv[1]
        printf("servPort = %d\n", servPort);

        if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
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
		printf("Failed to bind...error %d\n", errsv);
		return 0;
	}
	if(listen(servSock, 5) < 0)
	{
		printf("Listen() has failed...\n");
		return 0;
	}

        printf("Entering while(run) loop\n");
	while(1) //change to if ctrl-c is hit
	{
		clntAddrLen = sizeof(clntAddr);
		if((clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen)) < 0)
		{
			//printf("Failed to Accept...\n");
		}
		outfile = fopen("outfile.txt", "w");
		while(1)
		{
			if((recvMsgSize = recv(clntSock, recvBuffer, BUFSIZE, 0)) < 0) //receive 1000 bytes (first read)
			{
				//printf("Failed to recieve...\n");
			}
			if(recvMsgSize == 0)
			{
				break;
			}
			printf("Message Receive size in main: %d\n", recvMsgSize);
			totalBytesRead += recvMsgSize;
			printf("Buffer: %s\n", recvBuffer);
			cmd = (int)(recvBuffer[0]-'0');
			printf("Command number: %d\n", cmd);
			switch(cmd) //first byte is associated command
			{
				case nullTerminatedCmd: 
					printf("In Null terminated command\n");
					nullTerminated(clntSock, recvBuffer, recvMsgSize);
					break;
				case givenLengthCmd: 
					givenLength(clntSock, recvBuffer, recvMsgSize);
					break;
				case badIntCmd: 
					goodOrBadInt(clntSock, recvBuffer, recvMsgSize, 3);
					break;
				case goodIntCmd: 
					goodOrBadInt(clntSock, recvBuffer, recvMsgSize, 4);
					break;
				case byteAtATimeCmd: 
					byteAtATime(clntSock, recvBuffer);
					break;
				case kByteAtATimeCmd: 
					kByteAtATime(clntSock, recvBuffer);
					break;
			}
		}
		printf("Server read %d bytes\n", totalBytesRead); //print #bytes received
		fclose(outfile);
		close(clntSock);
	}
}
