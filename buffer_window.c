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

int servStart = 0;
int servEnd = 0;
int cliStart = 0;
int cliEnd = 0;
int isBuffFull = 0; // Set to 1 if  full, 0 not full
int bytesInBuff = 0; // used to see if buffer should be set to full, incremented by # of bytes added to buffer

typedef struct Pkt_Info{
	int seq_num;
	int ack_flag;
	int pktStart;
	int sizeOfPkt;
}Pkt_Info;

Pkt_Info serWindow[20];
Pkt_Info cliWindow[20];
int windIndex;

int writeToBufferC(int bytesToWrite, char pktBuffer[],int seq_num){
	int bytesWritten = 0;
	isBuffFull = isBuffFilled();
	if(isBuffFull == 1){
		printf("The buffer is full!\n");
	}else{
		if(isBuffFull == 0 && (cliEnd + bytesToWrite) >= cliStart){
			if((cliEnd+bytesToWrite) < MAX_BUFF){
				//printf("The buffer is %s\n",pktBuffer);
				printf("Value of cliEnd is %d\n",cliEnd);
				bytesInBuff += bytesToWrite;
				memcpy(cliBuffer+cliEnd,&pktBuffer,bytesToWrite);
				cliEnd += bytesToWrite;
				bytesWritten = bytesToWrite;
				printf("Bytes in Buffer is %d\n",bytesInBuff);
				printf("Value of cliEnd is %d\n",cliEnd);
			}else if((cliEnd+bytesToWrite) == (MAX_BUFF) && cliStart != 0 ){
				bytesInBuff += bytesToWrite;
				memcpy(cliBuffer+cliEnd,&pktBuffer,bytesToWrite);
				cliEnd = 0;
				bytesWritten = bytesToWrite;
			}else if((cliEnd+bytesToWrite > MAX_BUFF) && cliStart != 0){
				int remainder = (MAX_BUFF - cliEnd);
				memcpy(cliBuffer+cliEnd,&pktBuffer,(MAX_BUFF)-cliEnd);
				memcpy(cliBuffer,&pktBuffer,remainder);
				cliEnd = remainder;
				bytesWritten = bytesToWrite;	
				bytesInBuff += bytesToWrite;		
			}else{
				printf("shit is full!\n");
				isBuffFull =1;
			}
		}
	}
	return bytesWritten;
}

int readFromBufferC(char pktBuffer[],int seq_num){
	int bytesRead = 0;
	isBuffFull = isBuffFilled();
	if(isBuffFull == 0 && bytesInBuff == 0){
		printf("The Buffer is empty!\n");
	}else{
		printf("here2\n");
		if(cliStart < MAX_BUFF){
			memcpy(&pktBuffer,cliBuffer+cliStart,MSS);
			printf("here3\n");
			bytesInBuff -= MSS;
			printf("%d",bytesInBuff);
		}else if(cliStart+MSS > MAX_BUFF){
			printf("here4\n");
			int remainder = (MAX_BUFF - cliStart);			
			memcpy(&pktBuffer,cliBuffer+cliStart,remainder);
			memcpy(&pktBuffer+remainder,cliBuffer,MSS-remainder);
			bytesInBuff -= remainder;
			bytesInBuff -= MSS-remainder;
			cliStart = remainder;
		}
	}
	return 0;
}

int isBuffFilled(){
	if(bytesInBuff >= MAX_BUFF){
		isBuffFull = 1;
		return 1;
	}
	return 0;
}

