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
	printf("Size: %d", size);
	strcpy(returnString, (char *)&size);
	strcat(returnString, commandNames[cmd]);
	strcat(returnString, ": ");
	strcat(returnString, buf);
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
	for(i = 1; i < bytesRead; i++) //look through bytes received in the buffer
	{
		if(buf[i] == '\0') //found null terminated byte
		{
			sendString(socket, buf+1, 1); //exclude the cmd byte
			return;
		}
	}
	while(1)
	{
		if((recvMsgSize = recv(socket, buf + bytesRead, RECVBUFSIZE - bytesRead, 0)) < 1) //expecting at least 1 byte
		{
			//failed to receive or connection severed
		}
		totalBytesRead += recvMsgSize;
		for(i = 0; i < recvMsgSize; i++)
		{
			if(buf[i+offset] == '\0') //found null terminated byte
			{
				sendString(socket, buf+1, 1);  //exclude the cmd byte
				return;
			}
		}
		offset += recvMsgSize; //increment offset
	}
}

int givenLength(int socket, char *buf, int bytesRead)
{
	uint16_t length;
	char *strLen;
	int recvMsgSize = 0;
	length = (uint16_t)atoi(buf+1);
	if(length == bytesRead - 3) //buf[0] is the cmd, buf[1] and buf[2] are the 16 bit length, rest is the string
	{
		sendString(socket, buf+3, 2);
		return; //am done
	}
	if((recvMsgSize = recv(socket, buf+bytesRead, length-bytesRead, 0)) < 0)
	{
		//print error
	}
	sendString(socket, buf+3, 2);
	return; //am done
}

void goodOrBadInt(int socket, int cmd)
{
	char buf[RECVBUFSIZE];
	int recvMsgSize = 0;
	int bytes; //uint 32
	unsigned short byteOrder;
	if((recvMsgSize = recv(socket, buf, 4, 0)) < 0) //receive 4 byte int from client (can be good or bad int, behavior is same)
	{
		//printf("Failed to recieve...\n");
	}
	bytes = atoi(buf);
	byteOrder = htonl(bytes);
	sprintf(buf, "%d", byteOrder);
	sendString(socket, buf, cmd);
}

void byteAtATime(int socket)
{
	char buf[RECVBUFSIZE];
	int recvMsgSize = 0;
	if((recvMsgSize = recv(socket, buf, 1, 0)) < 0) //receive 1 byte at a time
	{
		//printf("Failed to recieve...\n");
	}
	
}

void kByteAtATime(int socket)
{
	
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
			totalBytesRead += recvMsgSize;
			//read into log file
			cmd = (int)recvBuffer[0];
			switch(cmd) //first byte is associated command
			{
				case 1: 
					nullTerminated(clntSock, recvBuffer, recvMsgSize);
					break;
				case 2: 
					givenLength(clntSock, recvBuffer, recvMsgSize);
					break;
				case 3: 
					goodOrBadInt(clntSock, 3);
					break;
				case 4: 
					goodOrBadInt(clntSock, 4);
					break;
				case 5: 
					byteAtATime(clntSock);
					break;
				case 6: 
					kByteAtATime(clntSock);
					break;
			}
		}
		/*while(recvMsgSize > 0)
		{
			if(send(clntSock, recvBuffer, recvMsgSize, 0) != recvMsgSize)
			{
				printf("Failed to send...\n");
			}
			if((recvMsgSize = recv(clntSock, recvBuffer, 32, 0)) < 0)
			{
				printf("Failed to Recieve (v2)...\n");
			}
		}*/
		printf("Server read %d bytes\n", totalBytesRead); //print #bytes received
		close(clntSock);
	}
}
