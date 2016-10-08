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
#include "project1.h"

int run;

void CtrlCTrap(int signal)
{
	run = 0;
}

int main(int argc, char *argv[]) //argv[1] is the port to listen to
{
	int servSock; //Socket descriptor for server
	int clntSock; //Socket descriptor for client
	in_port_t servPort;
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
		if((recvMsgSize = recv(clntSock, recvBuffer, 32, 0)) < 0)
		{
			//printf("Failed to recieve...\n");
		}
		while(recvMsgSize > 0)
		{
			if(send(clntSock, recvBuffer, recvMsgSize, 0) != recvMsgSize)
			{
				printf("Failed to send...\n");
			}
			if((recvMsgSize = recv(clntSock, recvBuffer, 32, 0)) < 0)
			{
				printf("Failed to Recieve (v2)...\n");
			}
		}
		close(clntSock);
	}
	//exit with ctrl-c
}
