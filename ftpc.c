#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <errno.h>

#include "wrapper_funcs.h"
#include "globals.h"

/*
Author: Roman Claprood
Project: Lab2 
Due Date: 1/28/16

This client will transmit a file to a server using a 
TCP connection. 
*/

void readAndSendFile(char *fname,int sock,int size);
void findFileSize(int *size,char *fileName);

int main(int argc, char *argv[]){
	
	int sockfd;
	int portNum;
	int size;
	struct sockaddr_in sock_addr;
	struct hostent *server;
	
	if(argc != 2){
		printf("Improper number of arguments!\n");
		printf("Correct format is ./ftpc fileToSend\n");
		exit(0);	
	}

	sockfd = SOCKET(AF_INET,SOCK_STREAM, 0);
	//portNum = atoi(argv[2]);
	
	if(sockfd < 0){
		printf("ERROR OPENING SOCKET!");
		exit(1);
	}
	/*
	server = gethostbyname(argv[1]);
	if(server == 0){
		printf("ERROR! HOST NOT FOUND!");
		exit(1);
	}
	*/
	//bcopy((char *)server->h_addr,(char *)&sock_addr.sin_addr.s_addr,server->h_length);
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(TCPDIN);
	sock_addr.sin_addr.s_addr = inet_addr(clientIP);
	
	if(BIND(sockfd,(struct sockaddr *)&sock_addr,sizeof(struct sockaddr_in)) < 0){

	}

  	sock_addr.sin_port = htons(TCPDIN);
	if(CONNECT(sockfd,(struct sockaddr *)&sock_addr,sizeof(struct sockaddr_in)) < 0){
		close(sockfd);
		printf("ERROR CONNECTION FAILED!");
		exit(1);
	}
	findFileSize(&size,argv[1]);
	readAndSendFile(argv[1],sockfd,size);
	
	return 0;
}
/*Function used to open file,read file, and to 
send the file to the server*/
void readAndSendFile(char *fname,int sock,int size){
	char buffer[MSS];
	int bytesRead = 0;
	int totalSent =0;
	int nSize= htonl(size);
	/*sending the fileSize*/
	memcpy(&buffer,&nSize,4);
	if(SEND(sock,buffer,4,0) < 0){
		printf("Error sending size!");
		close(sock);
		exit(1);
	}
	
	memset(buffer,'\0',MSS);
	/*Sending the file name*/
	memcpy(buffer,fname,20);
	if(SEND(sock,buffer,20,0) < 0){
		printf("Error sending file name!");
		close(sock);
		exit(1);
	}
	/*Opening the file and sending the amount of bytes
	read from the file. Max load is 1000 bytes. Prob should
	declare constant instead of magic numbers. */
	FILE *fileName = fopen(fname,"rb");
	if(fileName == NULL){
		printf("Failure to open file! May not exist!\n");
		exit(1);
	}else{
		while((size-totalSent) > 0){			
			bytesRead=fread(buffer,1,MSS,fileName);
			if(bytesRead < 0){
				printf("Error, read nothing from file");
				exit(1);
			}
			
			if(SEND(sock,buffer,bytesRead,0) < 0){
				printf("Error sending to server!");
				exit(1);
			}
			totalSent += bytesRead;
		}
		close(fileName);
		printf("Total of %d bytes have been sent.\n",totalSent);	
		close(sock);
	}
	
}

/*Function is used to get the length of the file.*/
void findFileSize(int *size,char *fileName){	
	FILE *fname = fopen(fileName,"r");
	if(fname == NULL){
		printf("Failure to open file! May not exist!\n");
		exit(1);
	}else{
		fseek(fname,0l,SEEK_END);
		*size = ftell(fname);
		fseek(fname,0L,SEEK_SET);
	}
	fclose(fname);
}
