#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <errno.h>
#include <stdlib.h>

#define MAX_BUFF 64000
char servBuffer[MAX_BUFF];
char cliBuffer[MAX_BUFF];

char *servStart = &servBuffer[0];
char *servEnd = servStart;
char *cliStart = &cliBuffer[0];
char *cliEnd = cliStart;
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
	if(isBuffFull == 1){
		printf("The buffer is full!\n");
	}else{
		if(isBuffFull == 0 && (cliEnd + bytesToWrite) >= cliStart){
			bytesInBuff += bytesToWrite;
			memcpy(&cliBuffer,&pktBuffer,bytesToWrite);
			cliEnd += bytesInBuff;
		}
	}

void readFromBufferC();

int isBuffFilled(){
	if(bytesInBuff == MAX_BUFF){
		isBuffFull = 1;
		return 1;
	}
	return 0;
}

