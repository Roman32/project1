#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <errno.h>
#include <stdlib.h>

#include "buffer_window.h"
#include "globals.h"

#define MAX_BUFF 64000
char servBuffer[MAX_BUFF];
char cliBuffer[MAX_BUFF];

char *servStart = &servBuffer[0];
char *servEnd = &servBuffer[0];
char *cliStart = &cliBuffer[0];
char *cliEnd = &cliBuffer[0];
int isBuffFull = 0; // Set to 1 if  full, 0 not full
int bytesInBuff = 0; // used to see if buffer should be set to full, incremented by # of bytes added to buffer

typedef struct Pkt_Info{
	int seq_num;
	int ack_flag;
	char* pktStart;
	int sizeOfPkt;
}Pkt_Info;

Pkt_Info serWindow[20];
Pkt_Info cliWindow[20];

int writeToBufferC(int bytesToWrite, char pktBuffer[]){
	int bytesWritten = 0;
	isBuffFilled();
	if(isBuffFull == 1){
		printf("The buffer is full!\n");
	}else{
		if(isBuffFull == 0 && (cliEnd + bytesToWrite) >= cliStart){
			if((cliEnd+bytesToWrite) < (cliBuffer+MAX_BUFF)){
				//printf("The buffer is %s\n",pktBuffer);
				printf("Value of cliEnd is %p\n",cliEnd);
				bytesInBuff += bytesToWrite;
				memcpy(&cliBuffer+*cliEnd,&pktBuffer,bytesToWrite);
				cliEnd = cliEnd+bytesToWrite;
				bytesWritten = bytesToWrite;
				printf("Bytes in Buffer is %d\n",bytesInBuff);
				printf("Value of cliEnd is %d\n",(int)*cliEnd);
			}else if((cliEnd+bytesToWrite) == (cliBuffer+MAX_BUFF)){
				bytesInBuff += bytesToWrite;
				memcpy(&cliBuffer+*cliEnd,&pktBuffer,bytesToWrite);
				cliEnd = &cliBuffer[0];
				bytesWritten = bytesToWrite;
			}else{
				int remainder = (int)(cliBuffer+MAX_BUFF) - *cliEnd;
				memcpy(&cliBuffer,&pktBuffer,(int)(cliBuffer+MAX_BUFF)-*cliEnd);
				memcpy(&cliBuffer,&pktBuffer,remainder);
				cliEnd = &cliBuffer[0]+remainder;
				bytesWritten = bytesToWrite;
				
			}
		}
	}
	return bytesWritten;
}

int readFromBufferC(char pktBuffer[]){
	int bytesRead = 0;
	if(isBuffFull == 0 && bytesInBuff == 0){
		printf("The Buffer is empty!\n");
	}else{
		if(cliStart < cliEnd && cliStart <= cliBuffer+MAX_BUFF){
			//memcpy(&pktBuffer,&cliBuffer+*cliStart,MSS);
			printf("Value of cliStart is %p\n",cliStart);
			cliStart = &cliBuffer[0]+MSS;
			printf("Value of cliStart is %p\n",cliStart);
		}
	}
	
}

int isBuffFilled(){
	if(bytesInBuff >= MAX_BUFF){
		isBuffFull = 1;
		return 1;
	}
	return 0;
}

