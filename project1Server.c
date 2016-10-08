#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
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
	int servPort;
	int recvMsgSize;
	int clntAddrLen;
	char recvBuffer[32];
	struct sockaddr_in servAddr;
	struct sockaddr_in clntAddr;

	run = 1;
	signal(SIGINT, &CtrlCTrap);
	if(argc != 2) //incorrect format
	{
		//print error message, inccorect number of arguments
	}
	servPort = atoi(argv[1]); //get port for argv[1]
	memset(&servAddr, 0, sizeof(servAddr)); //zero out structure
	servAddr.sin_family = AF_INET; //IPv4
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY); //any incoming interface
	servAddr.sin_port = htons(servPort); //local port
	if(bind(servSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
	{
		//print error message (bind failed)
	}
	if(listen(servSock, 5) < 0)
	{
		//print error message (listen failed)
	}
	while(run) //change to if ctrl-c is hit
	{
		clntAddrLen = sizeof(clntAddr);
		if((clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen)) < 0)
		{
			//print error message (accept failed)
		}
		if((recvMsgSize = recv(clntSock, recvBuffer, 32, 0)) < 0)
		{
			//print error message (receive failed)
		}
		while(recvMsgSize > 0)
		{
			if(send(clntSock, recvBuffer, recvMsgSize, 0) != recvMsgSize)
			{
				//print error message (send failed)
			}
			if((recvMsgSize = recv(clntSock, recvBuffer, 32, 0)) < 0)
			{
				//print error message (receive failed)
			}
		}
		close(clntSock);
	}
	//exit with ctrl-c
}
