#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <errno.h>
#include <stdlib.h>

char servBuffer[64000];
char cliBuffer[64000];

char *servStart = 0;
char *servEnd = 0;
char *cliStart = 0;
char *cliEnd = 0;
int isBuffFull = 0;

typedef struct Pkt_Info{
	int seq_num;
	int ack_flag;
	char* pktStart;
	int sizeOfPkt;
}Pkt_Info;

Pkt_Info serWindow[20];
Pkt_Info cliWindow[20];

void writeToBuffer(){
	if(isBuffFull == 1){
		printf("The buffer is full!\n");
	}else{
	
	}
}
void readFromBuffer();

