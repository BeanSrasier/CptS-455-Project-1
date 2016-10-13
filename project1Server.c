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

int sendString(int socket, char *buf, int cmd)
{
	char returnString[BUFSIZE];
	int lengthOfString;
	uint16_t size;

	//Copy into the return string "commandName[cmd]: (contents of buffer)"
	strcpy(returnString+2, commandNames[cmd]);
	strcat(returnString+2, ": ");
	strcat(returnString+2, buf);
	lengthOfString = strlen(returnString+2);
	size = htons(lengthOfString); //convert length to network byte short
	memcpy(returnString, &size, sizeof(size)); //mem copy message length as 2 byte short into the beginning of the message
	if(send(socket, returnString, lengthOfString+2, 0) != lengthOfString+2) //send message to the client
	{
		printf("Failed to send");
		return -1;
	}
}

int nullTerminated(int socket, char *buf, int bytesRead)
{
	int recvMsgSize = 0;
	while(1)
	{
		if(buf[bytesRead - 1] == '\0') //search for null terminator
		{
			fwrite(buf, 1, bytesRead, outfile);
			sendString(socket, buf+1, nullTerminatedCmd); //exclude the cmd byte
			return 0;
		}
		if((recvMsgSize = recv(socket, buf + bytesRead, BUFSIZE - bytesRead, 0)) < 1) //must receive at least 1 byte
		{
			printf("Receive in nullTerminated failed\n");
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
	while(bytesRead < 3) //must read first three bytes (cmd byte and 2 byte length)
	{
		if((recvMsgSize = recv(socket, buf+bytesRead, BUFSIZE - bytesRead, 0)) < 0)
		{
			printf("Receive in givenLength failed\n");
			return -1;
		}
		bytesRead += recvMsgSize;
	}
	memcpy(&length, buf+1, 2); //length of buffer
	stringLength = (int)ntohs(length); //convert to host byte short
	while(1)
	{
		if(stringLength == bytesRead - 3) //buf[0] is the cmd, buf[1] and buf[2] are the 16 bit length, rest is the string
		{
			fwrite(buf, 1, bytesRead, outfile);
			sendString(socket, buf+3, givenLengthCmd);
			return 0; //am done
		}
		if((recvMsgSize = recv(socket, buf+bytesRead, length-bytesRead, 0)) < 0) //keep receiving bytes
		{
			printf("Receive in givenLength failed\n");
			return -1;
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
	while(bytesRead < 5) //should receive 5 bytes, 1 for the cmd, 4 for the integer
	{
		if((recvMsgSize = recv(socket, buf+bytesRead, 5-bytesRead, 0)) < 0) //should receive rest of the bytes
		{
			printf("Receive in goodOrBadInt failed\n");
			return -1;
		}
		bytesRead += recvMsgSize;
		totalBytesRead += recvMsgSize; //increment total bytes read
	}
	memcpy(&byteOrder, buf+1, 4); //mem copy integer from buffer into uint32_t variable
	bytes = (int)ntohl(byteOrder); //convert to host byte long
	sprintf(outputBuffer, "%d", bytes); //copy into outputBuffer
	fwrite(buf, 1, bytesRead, outfile);
	sendString(socket, outputBuffer, cmd);
	return 0;
}

int xBytesAtATime(int socket, char *buf, int xBytes, int bytesRead, int cmd)
{
	int bytesReceived = bytesRead;
	int recvMsgSize = 0;
	int numRecvs = 0;
	uint32_t bytes;
	int numBytes;
	char recvBuffer[BUFSIZE];

	while(bytesReceived < 5) //make sure to receive first 5 bytes (cmd byte and 4 byte integer)
	{
		if((recvMsgSize = recv(socket, buf+bytesReceived, BUFSIZE - bytesReceived, 0)) < 0) //keep receiving bytes
		{
			printf("Receive in xBytesAtATime failed\n");
			return -1;
		}
		bytesReceived += recvMsgSize;
		numRecvs++; //increment num recvs
	}
	memcpy(&bytes, buf+1, 4); //Amount of bytes to receive
	numBytes = (int)ntohl(bytes); //convert bytes into host byte long
	fwrite(buf, 1, bytesRead, outfile);
	while(bytesReceived < numBytes+5) //#bytes + 5 header bytes
	{
		if((recvMsgSize = recv(socket, buf, xBytes, 0)) < 1) //receive x bytes at a time (1 or 1000 depending on function call)
		{
			printf("Receive in xBytesAtATime failed\n");
			return -1;
		}
		bytesReceived += recvMsgSize;
		numRecvs++; //increment num recvs
		fwrite(buf, 1, bytesRead, outfile);
	}
	totalBytesRead += bytesReceived;
	sprintf(recvBuffer, "%d", numRecvs); //copy numRecvs integer into recvBuffer string
	sendString(socket, recvBuffer, cmd); //send string to client
}

//Used the code from Donahoo/Calvert as a basis for our main
int main(int argc, char *argv[]) //argv[1] is the port to listen to
{
	int servSock; //Socket descriptor for server
	int clntSock; //Socket descriptor for client
	in_port_t servPort;
	int cmd;
	int recvMsgSize;
	int clntAddrLen;
	char recvBuffer[BUFSIZE];
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
	while(1)
	{
		clntAddrLen = sizeof(clntAddr);
		if((clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen)) < 0)
		{
			//printf("Failed to Accept...\n");
		}
		outfile = fopen("outfile.txt", "w"); //open "outfile.txt" for write
		while(1)
		{
			if((recvMsgSize = recv(clntSock, recvBuffer, BUFSIZE, 0)) < 0) //receive 1000 bytes (first read)
			{
				printf("Receive in main failed\n");
				break;
			}
			if(recvMsgSize == 0)
			{
				printf("Client closed connection\n");
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
					xBytesAtATime(clntSock, recvBuffer, 1000, recvMsgSize, kByteAtATimeCmd);
					break;
			}
		}
		printf("Server read %d bytes\n", totalBytesRead); //print #bytes received
		totalBytesRead = 0; //reset bytes read in case of new client connection
		fclose(outfile); //close outfile
		close(clntSock); //close client socket
	}
}
