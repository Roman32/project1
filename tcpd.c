#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <errno.h>
#include <linux/tcp.h>
#include <math.h>
#include <inttypes.h>
#include "globals.h"
#include "buffer_window.h"
#include <time.h>
/* For Reference 20 bytes long
struct tcphdr {
         __u16   source;
         __u16   dest;
         __u32   seq;
         __u32   ack_seq;
 #if defined(__LITTLE_ENDIAN_BITFIELD)
         __u16   res1:4,
                 doff:4,
                 fin:1,
                 syn:1,
                 rst:1,
                 psh:1,
                 ack:1,
                 urg:1,
                 ece:1,
                 cwr:1;
 #elif defined(__BIG_ENDIAN_BITFIELD)
         __u16   doff:4,
                 res1:4,
                 cwr:1,
                 ece:1,
                 urg:1,
                 ack:1,
                 psh:1,
                 rst:1,
                 syn:1,
                 fin:1;
 #else
 #error  "Adjust your <asm/byteorder.h> defines"
 #endif
         __u16   window;
         __u16   check;
         __u16   urg_ptr;
};
*/
#define POLY 0x8408
struct sockaddr_in client;
struct sockaddr_in server;
struct sockaddr_in troll;
struct sockaddr_in final;
struct sockaddr_in timer_listen;
struct sockaddr_in timer_send;
struct sockaddr_in ackFromServer;
struct sockaddr_in ackToServerTroll;
struct sockaddr_in cplt_send;
struct sockaddr_in cptl_recv;

typedef struct pktTimeInfo{
	uint32_t seq_num;
	time_t start_time;
}pktTimeInfo;

pktTimeInfo pktTimes[20];
int pktTimeIndex = 0;
uint32_t expSeq = 3;
typedef struct TrollHeader{
	struct sockaddr_in header;
	//char body[1000];
}TrollHeader;

typedef struct Packet{
	struct TrollHeader trollhdr;
	struct tcphdr tcpHdr;
	char payload[MSS];
}Packet;

struct timeval start_time;
//when sleep ends, the time at which this occurs is stored here
struct timeval curr_time;
//for storing the result of curr_time - start_time
struct timeval result_time;

#define MAX_BUFF 64000
extern char servBuffer[MAX_BUFF];
char cliBuffer[MAX_BUFF];

extern int servStart;
extern int servEnd;
extern int cliStart;
extern int cliEnd;
extern int isBuffFull;
extern int numberOfPacketsInWindow;
int bytesInBuff;
int lastSerIndexSent = 1;
uint64_t rto = 10000;
float dev = 0;
float est_rtt = 10000;
int first_rto_calc = 1;
float alpha = 7.0 / 8.0;
float small_delta = 1.0 / 8.0;
float mu = 1.0;
float phi = 4.0;
float diff = 0;
int receivedFileSize = 0;
int receivedFileName = 0;
TrollHeader head;
Packet pckt;
Packet pcktS;
int last_seq_sent = 0;
void send_to_timer(uint8_t packet_type,
	uint32_t seq_num,
	uint64_t tv_sec,
	uint64_t tv_usec,
	int sock,
	struct sockaddr_in name);

void send_ack(uint32_t seq_num, int sock, struct sockaddr_in ackToServerTroll);

void send_to_SEND(int sock, struct sockaddr_in name);
int resend_packet(uint32_t s_num, int sockIn, int timer_ssock);
unsigned short checksum(char *data_p, int length);

typedef struct SerWinInfo{
	int serBuffIndex;
	int seq_num;
	int size;
}SerWinInfo;
SerWinInfo serWindow[64];
int serWinStatus[64];
int main(int argc, char argv[]){
	
	int sockIn;
	int sockOut;
	int ackFserverSock;
	int sendAckSock;
	int timer_lsock;
	int timer_ssock;
	int recv_sock;
	int serverTrollSock;
	int timer_buff_len;
	int bytes =0;
	int bytesIn =0;
	int bytesToServ = 0;
	int bytesToTroll = 0;
	char buffer[MSS];
	char timerBuffer[sizeof(uint32_t)];
	char bufferOut[1036];
	char ackBuffer[36]; //36 = tcp header + troll header
	uint32_t seq_num = 0;
    	//for determining time at which sleep started
	int l;
	for(l = 0; l < 64; l++){
		serWinStatus[l] = -1;
	}
	fd_set portUp;
	//TrollHeader head;
	//Packet pckt;
	//Packet pcktS;
	if(argc != 1){
		printf("Error! Proper use is ./tcpd");
		exit(1);
	}

	/*Set up for Client Socket*/
	sockIn = socket(AF_INET,SOCK_DGRAM,0);
	client.sin_family = AF_INET;
	client.sin_port = htons(TCPDIN);
	client.sin_addr.s_addr = inet_addr("0");
	if(bind(sockIn,(struct sockaddr*)&client,sizeof(client)) < 0){
		perror("Failed to bind socket for client comm.\n");
	}


	/*Set up for sending to Server*/
	sockOut = socket(AF_INET,SOCK_DGRAM,0);
	server.sin_family = AF_INET;
	server.sin_port = htons(TCPDOUT);
	server.sin_addr.s_addr = 0;
	if(bind(sockOut,(struct sockaddr*)&server,sizeof(server)) < 0){
		perror("Failed to bind socket for server comm.\n");
	}




	/* Setup for tcpd sending to timer */
	timer_ssock = socket(AF_INET, SOCK_DGRAM,0);
    if(timer_ssock< 0) {
	perror("opening datagram socket timer_ssock");
	exit(2);
    }
	timer_send.sin_family = AF_INET;
	timer_send.sin_port = htons(TIMERPORT);
	timer_send.sin_addr.s_addr = 0;
    //only need to bind port if we are listening
	/*if(bind(timer_ssock,(struct sockaddr*)&timer_send,sizeof(timer_send)) < 0){
		perror("Failed to bind socket for timer comm.poo1\n");
	}*/

  /* setup for tcpd listeing for packet from timer */
	timer_lsock = socket(AF_INET, SOCK_DGRAM,0);
	timer_listen.sin_family = AF_INET;
	timer_listen.sin_port = htons(TIMERLPORT);
	timer_listen.sin_addr.s_addr = 0;
	if(bind(timer_lsock,(struct sockaddr*)&timer_listen,sizeof(timer_listen)) < 0){
		perror("Failed to bind socket for timer comm.poo2\n");
	}
    /* set up sock to receive acks from server troll */
    	ackFserverSock = socket(AF_INET, SOCK_DGRAM,0);
	ackFromServer.sin_family = AF_INET;
	ackFromServer.sin_port = htons(ACKRPORT);
	ackFromServer.sin_addr.s_addr = 0;
	if(bind(ackFserverSock,(struct sockaddr*)&ackFromServer,sizeof(ackFromServer)) < 0){
		perror("Failed to bind socket for incoming ackspoo3\n");
	}

	/* set up sock to send ack l */


	//serverTrollSock = socket(AF_INET,SOCK_DGRAM,0);
	ackToServerTroll.sin_family = AF_INET;
	ackToServerTroll.sin_port = htons(STROLLPORT);
	ackToServerTroll.sin_addr.s_addr = 0;
	/*if(bind(serverTrollSock,(struct sockaddr*)&ackToServerTroll,sizeof(ackToServerTroll)) < 0){
		perror("Failed to bind socket for ackToServerTroll comm.\n");
	}*/
	
	/*Socket for sending to RECV()*/
	recv_sock = socket(AF_INET,SOCK_DGRAM,0);
	cptl_recv.sin_family = AF_INET;
	cptl_recv.sin_port = htons(CPTLRECVSENDPORT);
	cptl_recv.sin_addr.s_addr = 0;
	if(bind(recv_sock,(struct sockaddr*)&cptl_recv,sizeof(cptl_recv)) < 0){
		perror("Failed to bind socket for recv\n");
	}

    	cplt_send.sin_family = AF_INET;
	cplt_send.sin_port = htons(CPTLSENDRECVPORT);
    	cplt_send.sin_addr.s_addr = 0;


	head.header.sin_family = htons(AF_INET);
	head.header.sin_addr.s_addr = inet_addr(serverIP);
	head.header.sin_port = htons(TCPDOUT);

	final.sin_family = AF_INET;
	final.sin_port = htons(SERVERPORT);
	final.sin_addr.s_addr = inet_addr(serverIP);

	troll.sin_family = AF_INET;
	troll.sin_addr.s_addr = inet_addr(clientIP);
	troll.sin_port = htons(TROLLPORT);

	/*Packet to Troll*/
	memcpy(&pckt.trollhdr,&head,sizeof(struct TrollHeader));
	memcpy(&pckt.tcpHdr.source,&client.sin_port,sizeof(client.sin_port));
	memcpy(&pckt.tcpHdr.dest,&final.sin_port,sizeof(client.sin_port));
	pckt.tcpHdr.res1 = 0;
	pckt.tcpHdr.doff = 5;
	pckt.tcpHdr.fin = 0;
        pckt.tcpHdr.syn = 0;
        pckt.tcpHdr.rst = 0;
        pckt.tcpHdr.psh = 0;
        pckt.tcpHdr.ack = 0;
        pckt.tcpHdr.urg = 0;
        pckt.tcpHdr.ece = 0;
        pckt.tcpHdr.cwr = 0;
	pckt.tcpHdr.window = htons(MSS);
	pckt.tcpHdr.check = 0;
	pckt.tcpHdr.urg_ptr = 0;

	/*Packet from Troll*/
	//memcpy(&pcktS.trollhdr,&head,sizeof(struct TrollHeader));
	//memcpy(&pcktS.tcpHdr.source,&client.sin_port,sizeof(client.sin_port));
	//memcpy(&pcktS.tcpHdr.dest,&final.sin_port,sizeof(client.sin_port));
	pcktS.tcpHdr.res1 = 0;
	pcktS.tcpHdr.doff = 5;
	pcktS.tcpHdr.fin = 0;
        pcktS.tcpHdr.syn = 0;
        pcktS.tcpHdr.rst = 0;
        pcktS.tcpHdr.psh = 0;
        pcktS.tcpHdr.ack = 0;
        pcktS.tcpHdr.urg = 0;
        pcktS.tcpHdr.ece = 0;
        pcktS.tcpHdr.cwr = 0;
	pcktS.tcpHdr.window = htons(MSS);
	pcktS.tcpHdr.check = 0;
	pcktS.tcpHdr.urg_ptr = 0;

	FD_ZERO(&portUp);
	FD_SET(sockIn, &portUp);
	FD_SET(sockOut, &portUp);
	FD_SET(timer_lsock,&portUp);
	FD_SET(ackFserverSock,&portUp);
	FD_SET(recv_sock,&portUp);
	struct timeval start_time;
		//when sleep ends, the time at which this occurs is stored here
	struct timeval curr_time;
		//for storing the result of curr_time - start_time
	struct timeval result_time;
	while(1){
        fflush(stdout);
		if(select(FD_SETSIZE,&portUp,NULL,NULL,NULL)){
			//perror("select");
		}
		//receiving from client
		if(FD_ISSET(sockIn,&portUp)){
			printf("\n\n\n*****RECEIVED PACKET FROM CLIENT *****\n\n");
			pckt.tcpHdr.check = 0;
			bzero(&buffer,sizeof(buffer));
			int addr_len =sizeof(client);
			bytesIn = recvfrom(sockIn,buffer,sizeof(buffer),0,(struct sockaddr *)&client,&addr_len);
			seq_num++;	//increment seq# each time
			pckt.tcpHdr.seq = seq_num; //set seq num;
			pckt.tcpHdr.ack_seq = 0;
		
            		//printf("Decoded bytes going in \n\n%s\n",buffer);
			int numberOfBytesWrittenToBuffer = writeToBufferC(bytesIn,buffer,seq_num);
			printf("Bytes written to buffer %d\n",numberOfBytesWrittenToBuffer);
			//let send know that we wrote all byte sto buffer
			if(numberOfBytesWrittenToBuffer == bytesIn){
				send_to_SEND(sockIn,client);
			}else{
				perror("buffer was full when trying to insert\n");
			}

			//printf("Bytes from client :%d\n",bytesIn);
			if(numberOfPacketsInWindow < 20){
				printf("here\n");
				int old_cliStart = cliStart;
				bzero(&buffer,sizeof(buffer));
				readFromBufferC(buffer,bytesIn);
				//printf("Bytes read from buffer %d\n",);

				printf("Decoded bytes coming out SN: %d \n\n%s\n",seq_num,buffer);

				//check that bytes were written fully then send message to SEND() letting
				//it know that it is ok to send another packet

				memcpy(&pckt.payload,&buffer,MSS);	//copies bytes from client to payload of packet
				pckt.tcpHdr.check = checksum((char *)&pckt+16,sizeof(struct tcphdr)+bytesIn);	//hopefully does the checksum
				printf("The checksum for packet %d being sent is: %hu\n",pckt.tcpHdr.seq,pckt.tcpHdr.check);

				//rto = 1000000;
				send_to_timer(6,seq_num,rto / 1000000,rto % 1000000,timer_ssock,timer_send);
		  			//call send timer here
				pktTimes[pktTimeIndex].seq_num = seq_num;
			
				time_t s_time;
				time(&s_time);
				pktTimes[pktTimeIndex].start_time = s_time;
	  			pktTimeIndex++;
				if(pktTimeIndex == 20){pktTimeIndex = 0;}
				
				if(insertIntoCWindow(seq_num,0,old_cliStart,bytesIn,start_time) == 1){
		    		printf("inserted seq_num %d into window\n\n",seq_num);
					//send_to_SEND(sockIn,cplt_send);
					//printf("here\n");
					bytesToTroll = sendto(sockIn,(char *)&pckt,(sizeof(pckt.trollhdr)+sizeof(pckt.tcpHdr)+bytesIn),0,(struct sockaddr*)&troll,sizeof(troll));
					//printf("Sent to the Troll: %d\n",bytesToTroll);
					//sleep(1);
			        //send at the end

		        }
		    }else{
					printf("not sending window is full\n");
			}
			printf("\n*****END RECEIVED PACKET FROM CLIENT *****\n\n");

		}
		//receiving from timer
		if(FD_ISSET(timer_lsock,&portUp)){
			printf("\n\n\n*****RECEIVED PACKET FROM TIMER *****\n\n");
			bzero(&timerBuffer,sizeof(timerBuffer));
			int addr_len = sizeof(timer_listen);
			int timer_bytes_in = recvfrom(timer_lsock,timerBuffer,sizeof(uint32_t),0,(struct sockaddr*)&timer_listen,&addr_len);
			uint32_t inc_seq_num;
			memcpy(&inc_seq_num,&timerBuffer,4);
            		inc_seq_num = ntohl(inc_seq_num);
			printf("Received a packet from timer. seq num %d\n",inc_seq_num);
			//timer for ntohl(inc_seq_num) timed out resend packet

			if(getGetPktLocation(inc_seq_num) >= 0){
            			resend_packet(inc_seq_num,sockIn,timer_ssock);
			}else{
				printf("packet not in window: not resending");
			}

			printf("\n*****END: RECEIVED PACKET FROM TIMER *****\n\n");
		}
		//receiving from tcpd server side acks
		if(FD_ISSET(ackFserverSock,&portUp)){
			 printf("\n\n\n*****RECEIVED ACK FROM SERVER TCPD*****\n\n");
       	     		bzero(&ackBuffer,sizeof(ackBuffer));
			 int addr_len = sizeof(ackFromServer);
			 int ack_bytes_in = recvfrom(ackFserverSock,ackBuffer,36,0,(struct sockaddr*)&ackFromServer,&addr_len);
			 printf("received ack bytes %d\n",ack_bytes_in);
			 Packet p;
			 memcpy(&p.trollhdr,ackBuffer,sizeof(struct TrollHeader)); //Copy troll header to recieved packet troll header, not really needed.
			 memcpy(&p.tcpHdr,ackBuffer+16,sizeof(struct tcphdr)); //copy tcpHdr

			 uint32_t ack_num = ntohl(p.tcpHdr.ack_seq);
     		 printf("ack num is %d\n",ack_num);



			 //we only want to update rtt when the oldest packet in window is acked
			// printWindow();
			 //printf("oldestPacketInWindow() is %d\n",getOldestPacketInWindow());



			 int r;
			 for(r = 0; r < 20; r++){
				if(pktTimes[r].seq_num == ack_num){
				  printf("r was %d\n",r);
				}
			 }

             //call function to remove packet from buffer
			 //printWindow();
			 //getPktStartTime(ack_num);

		     time_t cur_time;
			 time(&cur_time);
			 time_t s = pktTimes[r].start_time;
			 double diffs = difftime(cur_time,s);
			 printf("diffs is %f\n",diffs);
			 diffs = diffs * 1000000;
			 
			 update_rtt(diffs);
			 printf("new rto is %"PRIu64" %"PRIu64" \n\n",rto/1000000,rto%1000000);
			 //call function to send a packet to timer to cancel timer for packet seq
     		 	 send_to_timer(7,ack_num,rto / 1000000,rto % 1000000,timer_ssock,timer_send);
			 removeFromCWindow(ack_num);

				//send_to_SEND(sockIn,cplt_send);
             

			 printf("\n*****END: RECEIVED ACK FROM SERVER TCPD *****\n\n");
		}
		//receiving from troll on server side
		if(FD_ISSET(sockOut,&portUp)){
			fflush(stdout);
			printf("\n\n\n***************RECEIVING FROM CLIENT TROLL ********************\n\n");
			bzero(&bufferOut,sizeof(bufferOut));
			int addr_len = sizeof(server);
			bytes =recvfrom(sockOut,bufferOut,sizeof(bufferOut),0,(struct sockaddr*)&server,&addr_len);
			memcpy(&pcktS.trollhdr,bufferOut,sizeof(struct TrollHeader)); //Copy troll header to recieved packet troll header, not really needed.
			memcpy(&pcktS.tcpHdr,bufferOut+16,sizeof(struct tcphdr)); //copy tcpHdr

			memcpy(&pcktS.payload,bufferOut+36,bytes-36); //copy of the payload
			printf("Bytes recv from troll %d\n",bytes);
		    
			uint16_t checkRecv = pcktS.tcpHdr.check; //set recved checksum value to a temp value
			//printf("Check recvd %hu\n",checkRecv);
			pcktS.tcpHdr.check = 0; //zero out checksum
			uint16_t check = checksum((char *)&pcktS+16,sizeof(struct tcphdr)+bytes-36); //recompute checksum to see if packet was garbled.
			if(check == checkRecv){
				printf("Checksum is *****SAME***** for packet %d\n",pcktS.tcpHdr.seq);
				printf("Checksum rcvd is: %hu\n",checkRecv);
				printf("Checksum calculated is %hu\n",check);
				
				/*if(FD_ISSET(recv_sock,&portUp)){
					char sizeBuff[sizeof(uint32_t)];
					int len = sizeof(cptl_recv);
					int bytesWanted = recvfrom(recv_sock,sizeBuff,sizeof(uint32_t),0,(struct sockaddr *)&cptl_recv,&len);
					int requested;
					memcpy(&requested,sizeBuff,4);
					requested = ntohl(requested);
					printf("Value of requested is %d: \n",requested);*/
								

				if(receivedFileSize == 0 && bytes == 40){
					last_seq_sent = 1;
				   receivedFileSize = 1;
					uint32_t n;
					memcpy(&n,&pcktS.payload,4);
					printf("first data is %d\n",ntohl(n));
					printf("----WRITE TO BUFFER FILE SIZE----\n");
					writeToBufferS(pcktS);
					//bzero(&bufferOut,sizeof(bufferOut));
					char toServer[MSS];
					bzero(&toServer,MSS);
					printf("----READ TO BUFFER----\n");
					readFromBufferS(toServer,MSS,0);
					printf("FILESIZE SENDING TO SERVER is: %d\n",atoi(toServer));
					bytesToServ = sendto(sockOut,toServer,bytes-36,0,(struct sockaddr*)&final,sizeof(final));
					//put received info into buffer
					//send ack to troll on server side
					send_ack(pcktS.tcpHdr.seq,sockOut,ackToServerTroll);
					serWinStatus[0] = 1;
				}
				if(receivedFileSize == 1 && bytes == 56 && receivedFileName == 0){
					last_seq_sent = 2;
					receivedFileName = 1;
					printf("----WRITE TO BUFFER FILE NAME----\n");
					writeToBufferS(pcktS);
					//bzero(&bufferOut,sizeof(bufferOut));
					char toServer[MSS];
					bzero(&toServer,MSS);
					printf("----READ TO BUFFER----\n");
					readFromBufferS(toServer,bytes-36,1);
					printf("FILENAME SENDING TO SERVER is: %s\n",toServer);
					bytesToServ = sendto(sockOut,toServer,bytes-36,0,(struct sockaddr*)&final,sizeof(final));
					//put received info into buffer
					//send ack to troll on server side
					send_ack(pcktS.tcpHdr.seq,sockOut,ackToServerTroll);
					serWinStatus[1] = 1;
				}
				uint32_t seq;
			    memcpy(&seq,&pcktS.tcpHdr.seq,sizeof(uint32_t));
				
				printf("Packet seq is %d  expected: %d\n",seq,expSeq);
				if(receivedFileName == 1 && receivedFileSize == 1 && bytes != 56 /*last_seq_sent < seq*/){
					serWindow[(seq-1) % 64].serBuffIndex = ((seq-1) % 64);
					serWindow[(seq-1) % 64].size = bytes -36;
					
					printf("----WRITE TO BUFFER----\n");
					writeToBufferS(pcktS);
					bzero(&bufferOut,sizeof(bufferOut));
					char toServer[MSS];
					bzero(&toServer,MSS);
					printf("----READ TO BUFFER----\n");
					//readFromBufferS(toServer,bytes-36,(seq-1) % 64);
					//bytesToServ = sendto(sockOut,toServer,bytes-36,0,(struct sockaddr*)&final,sizeof(final));
					
					int lastSerIndexSentplusone = lastSerIndexSent+1;
					printf("lastSerIndexSentplusone = %d\n",lastSerIndexSentplusone);
					if(lastSerIndexSentplusone == 64){
						int yy;
						for(yy = 0; yy < 64; yy++){serWinStatus[yy] = -1;}

						lastSerIndexSentplusone = 0;
					}
					lastSerIndexSent = lastSerIndexSentplusone;
					serWinStatus[(seq-1) % 64] = 1;
					printf("\n-------------serverWinStatus-----------\n");
					int zz;
					for(zz = 0; zz < 64; zz++){
						printf("I: %d val %d\n",zz,serWinStatus[zz]);
						
					}
					int goodtosend = 1;
					for(zz = 0; zz < (seq-1) % 64; zz++){
						//printf("I: %d val %d\n",zz,serWinStatus[zz]);
						if(serWinStatus[zz] == -1){goodtosend = -1;break;}
					}
					printf("---------end serverWindStatus--------\n");
					printf("G2S: %d Sm64 %d\n",goodtosend,(seq-1) % 64);
					if( goodtosend == 1/*lastSerIndexSentplusone == (seq-1) % 64*/){
						int v;
						for(v = (seq-1) % 64; v < 64; v++){
							if(serWinStatus[v] == 1){
								printf("Seq: %d V: %d servWindow[%d].serBuffIndex = %d\n",seq,v,v,serWindow[v].serBuffIndex);
								int bytesR = readFromBufferS(toServer,serWindow[v].size,serWindow[v].serBuffIndex);
								printf("V: %d numB: %d FILEDATA SENDING TO SERVER is: %s\n",v,bytesR,toServer);
								bytesToServ = sendto(sockOut,toServer,serWindow[v].size,0,(struct sockaddr*)&final,sizeof(final));
								last_seq_sent = seq;
							}else{
								break;
							}
							
							
						}
					}
					//put received info into buffer
					//send ack to troll on server side
					send_ack(pcktS.tcpHdr.seq,sockOut,ackToServerTroll);
					expSeq++;
				}
					/*writeToBufferS(pcktS);
					bzero(&bufferOut,sizeof(bufferOut));
					char toServer[MSS];
					bzero(&toServer,MSS);
					readFromBufferS(toServer,bytes-36);
					printf("ToServer is: %s\n",toServer);
					bytesToServ = sendto(sockOut,toServer,bytes-36,0,(struct sockaddr*)&final,sizeof(final));
					//put received info into buffer
					//send ack to troll on server side
					send_ack(pcktS.tcpHdr.seq,sockOut,ackToServerTroll);*/

			}else{
				printf("Checksum is **********DIFFERENT********** for packet %u\n",pcktS.tcpHdr.seq);
				printf("Checksum rcvd is: %hu\n",checkRecv);
				printf("Checksum calculated is %hu\n",check);
			}

			//send bytes to server
			//bytesToServ = sendto(sockOut,bufferOut+36,bytes-36,0,(struct sockaddr*)&final,sizeof(final));
			printf("Bytes sent to server:%d\n",bytesToServ);
		}
		FD_ZERO(&portUp);
		FD_SET(sockIn, &portUp);
		FD_SET(sockOut, &portUp);
		FD_SET(timer_lsock,&portUp);
        	FD_SET(ackFserverSock,&portUp);
        	FD_SET(recv_sock,&portUp);
		
	}

	return 0;
}

/*
// Taken from stjarnhimlen.se/snippets/ Look at similiar CRC16
// implementations, but this one was cleaner.
//                                      16   12   5
// this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
// This works out to be 0x1021, but the way the algorithm works
// lets us use 0x8408 (the reverse of the bit pattern).  The high
// bit is always assumed to be set, thus we only use 16 bits to
// represent the 17 bit value.
*/

unsigned short checksum(char *data_p, int length)
{
      unsigned char i;
      unsigned int data;
      unsigned int crc = 0xffff; //16 bits, all 1s

      if (length == 0)
            return (~crc);

      do
      {
            for (i=0, data=(unsigned int)0xff & *data_p++;
                 i < 8;
                 i++, data >>= 1) //Iterting over the bits of the data to be checksummed.
            {
                  if ((crc & 0x0001) ^ (data & 0x0001)) //checks if the bits differ
                        crc = (crc >> 1) ^ POLY; //Shifting bits left by 1, and XORing with 0x8408.
                  else  crc >>= 1;
            }
      } while (--length);

      crc = ~crc;//flips bits
      data = crc;
      crc = (crc << 8) | (data >> 8 & 0xff);//crc shifted 8 bits to the left OR'd with data shifted 8 to right AND'd with 255 or 11111111

      return (crc);
}


void send_to_SEND(int sock, struct sockaddr_in name){
	printf("send to send called\n");

	int buffSize = sizeof(uint8_t);
	char *cli_buf = malloc(buffSize);
	bzero(cli_buf,buffSize);
	uint8_t packet_type = 13;
	memcpy(cli_buf,&packet_type,1);
   	int res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
	//printf("send to send res is %d\n",res);
	if(res <0) {
		perror("sending datagram message send_to_SEND");
		exit(4);
	}
}

void send_to_timer(uint8_t packet_type,uint32_t seq_num,uint64_t tv_sec,uint64_t tv_usec,int sock,struct sockaddr_in name){
	printf("\n\n*****SENDING TO TIMER****\n\n");
	if(tv_sec == 0 && tv_usec < 10000){tv_usec = 10000;}
	int buffSize = sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint64_t);
	//printf("buffsize is %d\n",buffSize);
	printf("Send Packet T: %d SN: %d TV_SEC: %ld TV_USEC %ld\n",packet_type,seq_num,tv_sec,tv_usec);
	char *cli_buf = malloc(buffSize);
    	bzero(cli_buf,buffSize);

	seq_num = htonl(seq_num);
	tv_sec = htobe64(tv_sec);
	tv_usec = htobe64(tv_usec);

	memcpy(cli_buf,&packet_type,1);
	memcpy(cli_buf+1,&seq_num,4);
	memcpy(cli_buf+5,&tv_sec,8);
	memcpy(cli_buf+13,&tv_usec,8);

	int res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    	
    	if(res <0) {
		perror("sending datagram message");
		exit(4);
    	}
	printf("\n\n*****END: SENDING TO TIMER****\n\n");
}

void send_ack(uint32_t seq_num, int serverTrollSock, struct sockaddr_in ackToServerTroll){
	//printf("ack for seq_num %d sent\n",seq_num);
	//build tcp ack header
	//send without body (ack seq is in tcp header)

		/*Packet to Troll*/
        head.header.sin_family = htons(AF_INET);
		head.header.sin_addr.s_addr = inet_addr(clientIP);
		head.header.sin_port = htons(ACKRPORT);

		memcpy(&pckt.trollhdr,&head,sizeof(struct TrollHeader));
		memcpy(&pckt.tcpHdr.source,&server.sin_port,sizeof(server.sin_port));
		memcpy(&pckt.tcpHdr.dest,&ackToServerTroll.sin_port,sizeof(ackToServerTroll.sin_port));
        pckt.tcpHdr.ack_seq = htonl(seq_num);
		printf("pcky.tcpHdr.ack_seq in send_ack is %d\n",ntohl(pckt.tcpHdr.ack_seq));
		pckt.tcpHdr.res1 = 0;
		pckt.tcpHdr.doff = 5;
		pckt.tcpHdr.fin = 0;
	        pckt.tcpHdr.syn = 0;
	        pckt.tcpHdr.rst = 0;
	        pckt.tcpHdr.psh = 0;
	        pckt.tcpHdr.ack = 1;
	        pckt.tcpHdr.urg = 0;
	        pckt.tcpHdr.ece = 0;
	        pckt.tcpHdr.cwr = 0;
		pckt.tcpHdr.window = htons(MSS);
		pckt.tcpHdr.check = 0;
		pckt.tcpHdr.urg_ptr = 0;
		int bytesToTroll = sendto(serverTrollSock,(char *)&pckt,(sizeof(pckt.trollhdr)+sizeof(pckt.tcpHdr)),0,(struct sockaddr*)&ackToServerTroll,sizeof(ackToServerTroll));
    		//printf("res in ack send is %d\n",bytesToTroll);
    	if(bytesToTroll < 0) {
		perror("sending datagram message ack");
		exit(4);
    	}
        	head.header.sin_family = htons(AF_INET);
		head.header.sin_addr.s_addr = inet_addr(serverIP);
		head.header.sin_port = htons(TCPDOUT);
        	memcpy(&pckt.trollhdr,&head,sizeof(struct TrollHeader));
		memcpy(&pckt.tcpHdr.source,&client.sin_port,sizeof(client.sin_port));
		memcpy(&pckt.tcpHdr.dest,&final.sin_port,sizeof(client.sin_port));
		pckt.tcpHdr.res1 = 0;
		pckt.tcpHdr.doff = 5;
		pckt.tcpHdr.fin = 0;
		pckt.tcpHdr.syn = 0;
		pckt.tcpHdr.rst = 0;
		pckt.tcpHdr.psh = 0;
		pckt.tcpHdr.ack = 0;
		pckt.tcpHdr.urg = 0;
		pckt.tcpHdr.ece = 0;
		pckt.tcpHdr.cwr = 0;
		pckt.tcpHdr.window = htons(MSS);
		pckt.tcpHdr.check = 0;
		pckt.tcpHdr.urg_ptr = 0;


}
int update_rtt(uint64_t s_rtt){

   diff = s_rtt - est_rtt;
   //printf("diff is %f\n",diff);
   est_rtt = alpha * est_rtt + (1.0 - alpha) * s_rtt;
   //printf("est_rtt is %f\n",est_rtt);
   dev = dev + small_delta * (fabs(diff) - dev);
   //printf("dev is %f\n",dev );
   rto = mu * est_rtt + phi * dev;

}

int resend_packet(uint32_t s_num, int sockIn, int timer_ssock){
	
        	head.header.sin_family = htons(AF_INET);
		head.header.sin_addr.s_addr = inet_addr(serverIP);
		head.header.sin_port = htons(TCPDOUT);
        	memcpy(&pckt.trollhdr,&head,sizeof(struct TrollHeader));
		memcpy(&pckt.tcpHdr.source,&client.sin_port,sizeof(client.sin_port));
		memcpy(&pckt.tcpHdr.dest,&final.sin_port,sizeof(client.sin_port));
		pckt.tcpHdr.res1 = 0;
		pckt.tcpHdr.doff = 5;
		pckt.tcpHdr.fin = 0;
		pckt.tcpHdr.syn = 0;
		pckt.tcpHdr.rst = 0;
		pckt.tcpHdr.psh = 0;
		pckt.tcpHdr.ack = 0;
		pckt.tcpHdr.urg = 0;
		pckt.tcpHdr.ece = 0;
		pckt.tcpHdr.cwr = 0;
		pckt.tcpHdr.window = htons(MSS);
		pckt.tcpHdr.check = 0;
		pckt.tcpHdr.urg_ptr = 0;
	pckt.tcpHdr.seq = s_num; //set seq num;
	pckt.tcpHdr.ack_seq = 0;
    int location =  getGetPktLocation(s_num);
	int pktSize = getPacketSize(s_num);
    printf("pktSize is %d\n",pktSize);
	printf("pktLocation is %d\n",location);
     printWindow();
	char resendBuf[pktSize];
	bzero(&resendBuf,sizeof(resendBuf));
	printf("Bytes read from buffer %d\n",readFromBufferToResend(resendBuf,pktSize,location));

	//printf("Decoded bytes coming out \n\n%s\n",buffer);
	printf("Decoded bytes coming out SN: %d \n\n%s\n",s_num,resendBuf);
	//check that bytes were written fully then send message to SEND() letting
	//it know that it is ok to send another packet
	printf("bytes resending : %s\n",resendBuf);
	memcpy(&pckt.payload,&resendBuf,pktSize);	//copies bytes from client to payload of packet
	pckt.tcpHdr.check = checksum((char *)&pckt+16,sizeof(struct tcphdr)+pktSize);	//hopefully does the checksum
	printf("The checksum for packet %d being resent is: %hu\n",pckt.tcpHdr.seq,pckt.tcpHdr.check);


   	send_to_timer(6,s_num,rto / 1000000,rto % 1000000,timer_ssock,timer_send);
	//call send timer here

	int bytesToTroll = sendto(sockIn,(char *)&pckt,(sizeof(pckt.trollhdr)+sizeof(pckt.tcpHdr)+pktSize),0,   (struct sockaddr*)&troll,sizeof(troll));
	printf("Resent packet to the Troll: %d\n",bytesToTroll);
	//sleep(1);ls
    	//send at the end




}
