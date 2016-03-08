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

int windowStartOfPacketBlock = 0;  //the index of the first unacked packet
int windowEndOfPacketBlockPlusOne = 0; //the index where to insert the next incoming packet
int numberOfPacketsInWindow = 0;

int insertIntoCWindow(int seq_num, int ack_flag, int pktStart, int sizeOfPkt){
	//if window is not full
	if(isCWindowFull() == 0){
		cliWindow[windowEndOfPacketBlockPlusOne].seq_num = seq_num;
		cliWindow[windowEndOfPacketBlockPlusOne].ack_flag = 0;
		cliWindow[windowEndOfPacketBlockPlusOne].pktStart = pktStart;  //byte index of first byte in big buffer
		cliWindow[windowEndOfPacketBlockPlusOne].sizeOfPkt = sizeOfPkt; //use pktStart + sizeOfPkt when obtaining this from buffer
		windowEndOfPacketBlockPlusOne++; //increment window index which is the next "open" spot
		numberOfPacketsInWindow++;
		if(windowEndOfPacketBlockPlusOne == 20){
			windowEndOfPacketBlockPlusOne = 0; //wrap around

		}
		return 1; //return 1 if successsful insert
	}else{
		perror("error: tried to insert when window was full\n");
		return -1; //return -1 if window was full
	}

}

int removeFromCWindow(int seq_num){
	if(isCWindowFull() == -1){
		perror("tried to remvoe from window when window has no packets");
		return -1;
	}
   int i;
   for (i = 0; i < 20; i++){
   	 if(cliWindow[i].seq_num == seq_num){
   	 	numberOfPacketsInWindow--;   
   	 	cliWindow[i].ack_flag = 1;  // set this to one so we know we can override it
   	 	cliWindow[i].pktStart = -1; //prevent some errors i think
   	 	//if this packet was the first packet in our window then we can move the start of packetBlock
   	 	if( i == windowStartOfPacketBlock){
   	 		windowStartOfPacketBlock++; // only want to move the start of window if all packets before start have been acked

   	 		int j;
   	 		for(j = i; j < 20; j++){
   	 			//if packet has not been acked
   	 			if(cliWindow[j].ack_flag == 0){
   	 				break;
   	 			}else{ //continue moving the start of packet block up;
   	 				windowStartOfPacketBlock++;
   	 			}

   	 		}
   	 	}

   	 }
   }
}


int isCWindowFull(){
	if(windowStartOfPacketBlock == windowEndOfPacketBlockPlusOne){
		if(numberOfPacketsInWindow == 0){
			return -1; //window is completly empty
		}
		return 1; //window is full
	}else{
		return 0; //window has room
	}

}

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
				printf("Bytes in Buffer is %d\n",bytesInBuff);
				printf("Value of cliEnd is %d\n",cliEnd);
			}else if((cliEnd+bytesToWrite) == (MAX_BUFF)){
				bytesInBuff += bytesToWrite;
				memcpy(cliBuffer+cliEnd,pktBuffer,bytesToWrite);
				cliEnd = 0;
				bytesWritten = bytesToWrite;
				printf("Bytes in Buffer is %d\n",bytesInBuff);
				printf("Value of cliEnd is %d\n",cliEnd);
			}else if((cliEnd+bytesToWrite > MAX_BUFF) && cliStart != 0){
				int remainder = (MAX_BUFF - cliEnd);
				memcpy(cliBuffer+cliEnd,pktBuffer,(MAX_BUFF)-cliEnd);
				memcpy(cliBuffer,pktBuffer + (MAX_BUFF)-cliEnd,remainder);
				cliEnd = bytesToWrite - remainder;
				bytesWritten = bytesToWrite;	
				bytesInBuff += bytesToWrite;	
				printf("Bytes in Buffer is %d\n",bytesInBuff);
				printf("Value of cliEnd is %d\n",cliEnd);
			}else if((cliEnd+bytesToWrite > MAX_BUFF && cliStart == 0)){
				int remainder = (MAX_BUFF - cliEnd);
				memcpy(cliBuffer+cliEnd,pktBuffer,(MAX_BUFF)-cliEnd);
				cliEnd = 0;
				bytesInBuff += remainder;
				bytesWritten = remainder;
				isBuffFull =1;
				printf("Bytes in Buffer is %d\n",bytesInBuff);
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
			printf("Data Starts at %d\n",cliStart);
			memcpy(pktBuffer,cliBuffer+cliStart,bytesOut);
			cliStart += bytesOut;
			bytesInBuff -= bytesOut;
			bytesRead = bytesOut;
			printf("Bytes remaining %d\n",bytesInBuff);
		}else if(cliStart+bytesOut > MAX_BUFF && cliEnd != 0){
			printf("Data Starts at %d\n",cliStart);
			int remainder = (MAX_BUFF - cliStart);			
			memcpy(pktBuffer,cliBuffer+cliStart,remainder);
			memcpy(pktBuffer+remainder,cliBuffer,bytesOut-remainder);
			bytesInBuff -= bytesOut;
			cliStart = bytesOut-remainder;
			bytesRead = bytesOut;
			printf("Bytes remaining in Buffer %d\n",bytesInBuff);
		}else if(cliStart+bytesOut == MAX_BUFF && cliEnd != 0){
			printf("Data Starts at %d\n",cliStart);
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

