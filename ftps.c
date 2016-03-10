#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>


#include "globals.h"
#include "wrapper_funcs.h"
/*
Author: Roman Claprood
Project: Lab2 
Due Date: 1/28/16

This server will receive a file from the client using a
TCP connection. 
*/
long fileSize = 0;
int totalRecv = 0;
void receiveFile(int sock);

int main(int argc,char *argv[]){
	int sock,newsock;
	int portNum,pid;
	socklen_t clientLen;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	char buffer[MSS];

	if(argc != 2){
		printf("ERROR, wrong number of arguments!\n");
		printf("./ftps portNumber");
		exit(0);
	}
	sock = SOCKET(AF_INET,SOCK_STREAM,0);
	if(sock < 0){
		printf("ERROR OPENING SOCKET!");
		exit(0);
	}
	/*Sets up connection for the server */
	//portNum = atoi(argv[1]);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = 0;
	server_addr.sin_port = htons(SERVERPORT);
	
	if(BIND(sock,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0){
		perror("ERROR binding to socket!");
		exit(0);
	}
	
	receiveFile(sock);
		
	return 0;	
}

/*Function is used to receive all data from the 
client. It also creates or overwrites the file
in lab2/output directory.*/
void receiveFile(int sock){

	//int bytesIn = 0;
	long fileSize =0;
	//char buffer[MSS];
	bzero(buffer,MSS);
	int totalRecv = 0;
	char fileIn[50] = "output/";
	
	/*Recieves the size of the file in 1st 4 bytes*/
	bytesIn = RECV(sock,buffer,4,MSG_WAITALL);
	printf("bytes in: %d %s\n",bytesIn,buffer);	
	if(bytesIn < 0){
		perror("Error reading from socket!1st");
		exit(1);
	}
	memcpy(&fileSize,buffer,4);
	fileSize = ntohl(fileSize);
	printf("The size of the file is: %ld bytes.\n",fileSize);
	bzero(buffer,MSS);
	/*Recieves the file name in 20 bytes*/
	bytesIn = RECV(sock,buffer,20,MSG_WAITALL);
	printf("bytes in: %d\n",bytesIn);	
	if(bytesIn < 0){
		printf("Error reading from socket! Here");
		exit(1);
	}
	strcat(fileIn,buffer);
	printf("The name of the outputfile will be %s.\n",fileIn);
	bzero(buffer,MSS);
	/*Open the file and start receiving bytes as long as the 
	total received is less than the total size of the file
	*/
	FILE *fname = fopen(fileIn,"wb");
	if(fname == NULL){
		printf("Could not write to file %s\n",fileIn);
		exit(1);
	}
	/* The while condition could probably be written more elegantly,I was getting
	1000 more bytes than what was really in the file*/	
	while((fileSize-totalRecv) > 0){
		bytesIn = RECV(sock,buffer,MSS,0);
		if(bytesIn < 0){
			printf("Error reading from socket!");
			exit(1);
		}
		fwrite(buffer,1,bytesIn,fname);
		bzero(buffer,MSS);	
		totalRecv += bytesIn;
		printf("Total rcv'd this far %d\n",totalRecv);
	}
	printf("Total bytes recieved: %d\n",totalRecv);
	close(fname);
	close(sock);
}
