#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <errno.h>
#include <stdlib.h>

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

int isCWindowFull(){
	printf("numberOfPacketsInWindow is %d\n",numberOfPacketsInWindow);
	if(windowStartOfPacketBlock == windowEndOfPacketBlockPlusOne){
		if(numberOfPacketsInWindow < 20){
			return 0; //window is completly empty
		}else{ 
			return 1;
		}
	}else{
		return 0; //window has room
	}

}
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
   	 	cliWindow[i].pktStart = -1;
   	 	 //prevent some errors i think
   	 	//if this packet was the first packet in our window then we can move the start of packetBlock
   	 	if( i == windowStartOfPacketBlock){
   	 		windowStartOfPacketBlock++; // only want to move the start of window if all packets before start have been acked
   	 		i++;
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




void printWindow(){
	int i;
	for(i = 0; i < 20; i++){
		printf("Window block: %d seq num: %d  ack_flag: %d\n",i,cliWindow[i].seq_num,cliWindow[i].ack_flag);
	}
	printf("windowStartOfPacketBlock %d\n",windowStartOfPacketBlock);
	printf("windowEndOfPacketBlockPlusOne %d\n",windowEndOfPacketBlockPlusOne );
}
int main(){
	int i;
	for(i = 10; i < 30; i++){
		insertIntoCWindow(i,0,10,1000);
	}
	
	printWindow();

	printf("\n\n (expected to fail: addeding pkts 30 31\n");
	insertIntoCWindow(30,0,10,1000);
	insertIntoCWindow(31,0,10,1000);
	
	printWindow();
	
	printf("\n\n removing from window pkts 10 11 12\n");
	removeFromCWindow(10);
	removeFromCWindow(11);
	removeFromCWindow(12);
	printWindow();
	//this should not move windowStartOfPacketBlock 
	printf("\n\n removing from window pkts 14 15\n");
	removeFromCWindow(14);
	removeFromCWindow(15);
	printWindow();
	//this should move windowStartOfPacketBlock
	printf("\n\n removing from window pkts 13\n");
	removeFromCWindow(13);
	printWindow();

	printf("\n\n (expected to success: addeding pkts 30 31\n");
	insertIntoCWindow(30,0,10,1000);
	insertIntoCWindow(31,0,10,1000);
	printWindow();

	printf("\n\n (expected to success: addeding pkts 32 33 34 35\n");
	insertIntoCWindow(32,0,10,1000);
	insertIntoCWindow(33,0,10,1000);
	insertIntoCWindow(34,0,10,1000);
	insertIntoCWindow(35,0,10,1000);
	printWindow();
	


}