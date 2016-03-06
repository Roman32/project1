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
		printf("Bytes in Buffer is %d\n",bytesInBuff);
		sleep(3);
	}else{
		if(isBuffFull == 0){
			if((cliEnd+bytesToWrite) < MAX_BUFF){
				//printf("The buffer is %s\n",pktBuffer);
				bytesInBuff += bytesToWrite;
				memcpy(cliBuffer+cliEnd,pktBuffer,bytesToWrite);
				cliEnd += bytesToWrite;
				bytesWritten = bytesToWrite;
				printf("1Bytes in Buffer is %d\n",bytesInBuff);
				printf("Value of cliEnd is %d\n",cliEnd);
			}else if((cliEnd+bytesToWrite) == (MAX_BUFF)){
				bytesInBuff += bytesToWrite;
				memcpy(cliBuffer+cliEnd,pktBuffer,bytesToWrite);
				cliEnd = 0;
				bytesWritten = bytesToWrite;
				printf("2Bytes in Buffer is %d\n",bytesInBuff);
				printf("Value of cliEnd is %d\n",cliEnd);
			}else if((cliEnd+bytesToWrite > MAX_BUFF) && cliStart != 0){
				int remainder = (MAX_BUFF - cliEnd);
				memcpy(cliBuffer+cliEnd,pktBuffer,(MAX_BUFF)-cliEnd);

                char t[(MAX_BUFF)-cliEnd];
				memcpy(t,pktBuffer,(MAX_BUFF)-cliEnd);
                printf("buff_window first split %s\n",t);

				memcpy(cliBuffer,pktBuffer + (MAX_BUFF)-cliEnd,remainder);
                
     			 char t2[remainder];
				memcpy(t2,pktBuffer + (MAX_BUFF)-cliEnd,remainder);
                printf("buff_window second split %s\n",t2);

				cliEnd = bytesToWrite - remainder;
				bytesWritten = bytesToWrite;	
				bytesInBuff += bytesToWrite;	
				printf("3Bytes in Buffer is %d\n",bytesInBuff);
				printf("Value of cliEnd is %d\n",cliEnd);
			}else if((cliEnd+bytesToWrite > MAX_BUFF && cliStart == 0)){
				int remainder = (MAX_BUFF - cliEnd);
				memcpy(cliBuffer+cliEnd,pktBuffer,(MAX_BUFF)-cliEnd);
				cliEnd = MAX_BUFF;
				bytesInBuff += remainder;
				bytesWritten = remainder;
				isBuffFull =1;
				printf("4Bytes in Buffer is %d\n",bytesInBuff);
				printf("Value of cliEnd is %d\n",cliEnd);
				printf("Value of isBuffFull %d\n",isBuffFull);
			}
		}
	}
    
	return bytesWritten;
}

int readFromBufferC(char pktBuffer[],int bytesOut){
	
	int bytesRead = 0;
	isBuffFull = isBuffFilled();
	if(isBuffFull == 0 && bytesInBuff == 0){
		printf("The Buffer is empty!\n");
	}else{
		if(cliStart+bytesOut < MAX_BUFF){
			memcpy(pktBuffer,cliBuffer+cliStart,bytesOut);
			printf("1Data Starts at %d\n",cliStart);
			cliStart += bytesOut;
			bytesInBuff -= bytesOut;
			bytesRead = bytesOut;
			printf("Bytes remaining %d\n",bytesInBuff);
		}else if(cliStart+bytesOut > MAX_BUFF && cliEnd != 0){
			printf("2Data Starts at %d\n",cliStart);
			int remainder = (MAX_BUFF - cliStart);			
			memcpy(pktBuffer,cliBuffer+cliStart,remainder);
			memcpy(pktBuffer+remainder,cliBuffer,bytesOut-remainder);
			bytesInBuff -= bytesOut;
			cliStart = bytesOut-remainder;
			bytesRead = bytesOut;
			printf("Bytes remaining in Buffer %d\n",bytesInBuff);
		}else if(cliStart+bytesOut == MAX_BUFF && cliEnd != 0){
			printf("3Data Starts at %d\n",cliStart);
			memcpy(pktBuffer,cliBuffer+cliStart,bytesOut);
			cliStart = 0;
			bytesRead = bytesOut;
			bytesInBuff -= bytesOut;
			printf("Bytes remaining in Buffer %d\n",bytesInBuff);
		}
	}
   
	return bytesRead;
}

int isBuffFilled(){
	if(bytesInBuff >= MAX_BUFF){
		isBuffFull = 1;
		return 1;
	}
	return 0;
}

