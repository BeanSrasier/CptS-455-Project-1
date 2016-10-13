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

	memcpy(returnString, &size, sizeof(size));

	if(send(socket, returnString, lengthOfString+2, 0) != lengthOfString+2)
	{
		printf("Failed to send");
	}
}

/*void sendNumRecvs(int socket, int numRecvs, int cmd)
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
}*/

int nullTerminated(int socket, char *buf, int bytesRead) //'DONE'
{
	int recvMsgSize = 0;
	while(1)
	{
		if(buf[bytesRead - 1] == '\0')
		{
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
	while(1)
	{
		if(stringLength == bytesRead - 3) //buf[0] is the cmd, buf[1] and buf[2] are the 16 bit length, rest is the string
		{
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
	char outputBuffer[BUFSIZE];
	int recvMsgSize = 0;
	int bytes;
	uint32_t byteOrder;
	while(bytesRead < 5) //should be 5, 1 for the cmd, 4 for the integer
	{
		if((recvMsgSize = recv(socket, buf+bytesRead, 5-bytesRead, 0)) < 0) //should receive rest of the bytes
		{
			//printf("Failed to recieve...\n");
		}
		totalBytesRead += recvMsgSize; //increment total bytes read
	}
	memcpy(&byteOrder, buf+1, 4);
	bytes = (int)ntohl(byteOrder);
	sprintf(outputBuffer, "%d", bytes);
	fputs(buf, outfile);
	sendString(socket, outputBuffer, cmd);
	return 0;
}

int xBytesAtATime(int socket, char *buf, int xBytes, int bytesRead, int cmd)
{
	printf("Command: %d\n", cmd);
	int bytesReceived = bytesRead;
	int numRecvs = 1; //first receive
	uint32_t bytes;
	int numBytes;
	char recvBuffer[BUFSIZE];
	memcpy(&bytes, buf+1, 4); //Amount of bytes to receive
	numBytes = (int)ntohl(bytes);
	fputs(buf, outfile);

	while(bytesReceived != numBytes)
	{
		if((bytesReceived += recv(socket, buf, xBytes, 0)) < 0)
		{
			printf("Receive failed\n");
			return -1;
		}
		numRecvs++;
		fputs(buf, outfile);
	}
	totalBytesRead += bytesReceived;
	printf("xByte receives: %d\n", numRecvs);
	sprintf(recvBuffer, "%d", numRecvs);
	sendString(socket, recvBuffer, cmd);
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
			totalBytesRead += recvMsgSize;
			cmd = (int)recvBuffer[0];
			switch(cmd) //first byte is associated command
			{
				case nullTerminatedCmd: 
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
					xBytesAtATime(clntSock, recvBuffer, 1, recvMsgSize, byteAtATimeCmd);
					break;
				case kByteAtATimeCmd: 
					printf("test\n");
					xBytesAtATime(clntSock, recvBuffer, 1000, recvMsgSize, kByteAtATimeCmd);
					break;
			}
		}
		printf("Server read %d bytes\n", totalBytesRead); //print #bytes received
		totalBytesRead = 0;
		fclose(outfile);
		close(clntSock);
	}
}
