#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <errno.h>
#include <linux/tcp.h>


#include "globals.h"


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

struct sockaddr_in client;
struct sockaddr_in server;
struct sockaddr_in troll;
struct sockaddr_in final;

typedef struct TrollHeader{
	struct sockaddr_in header;
	//char body[1000];
}TrollHeader;

typedef struct Packet{
	struct TrollHeader trollhdr;
	struct tcphdr tcpHdr;
	char payload[MSS];
}Packet;

unsigned short checksum(const char *buf, unsigned size);

int main(int argc, char argv[]){
	
	int sockIn;
	int sockOut;
	int bytes =0;
	int bytesIn =0;
	int bytesToServ = 0;
	int bytesToTroll = 0;
	char buffer[MSS];
	char bufferOut[1036];
	char buffS[MSS];
	uint32_t seq_num = 0;
	fd_set portUp;
	TrollHeader head;
	Packet pckt;
	Packet pcktS;
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
	pcktS.tcpHdr.window = 0;
	pcktS.tcpHdr.check = 0;
	pcktS.tcpHdr.urg_ptr = 0;
	
	FD_ZERO(&portUp);
	FD_SET(sockIn, &portUp);
	FD_SET(sockOut, &portUp);
	
	while(1){
		if(select(FD_SETSIZE,&portUp,NULL,NULL,NULL)){
			//perror("select");
		}
		if(FD_ISSET(sockIn,&portUp)){
			bzero(&buffer,sizeof(buffer));
			int addr_len =sizeof(client);
			bytesIn = recvfrom(sockIn,buffer,sizeof(buffer),0,(struct sockaddr *)&client,&addr_len);
			seq_num++;	//increment seq# each time 
			pckt.tcpHdr.seq = seq_num; //set seq num;
			pckt.tcpHdr.ack_seq = 0;
			printf("Bytes from client :%d\n",bytesIn);
			memcpy(&pckt.payload,&buffer,MSS);	//copies bytes from client to payload of packet
			pckt.tcpHdr.check = checksum((char *)&pckt+16,sizeof(struct tcphdr)+bytesIn);	//hopefully does the checksum
			printf("The checksum for packet %d being sent is: %u\n",pckt.tcpHdr.seq,pckt.tcpHdr.check);
			bytesToTroll = sendto(sockIn,(char *)&pckt,(sizeof(pckt.trollhdr)+sizeof(pckt.tcpHdr)+bytesIn),0,(struct sockaddr*)&troll,sizeof(troll));
			printf("Sent to the Troll: %d\n",bytesToTroll);
			
		}
		if(FD_ISSET(sockOut,&portUp)){
			int addr_len = sizeof(server);
			bytes =recvfrom(sockOut,bufferOut,sizeof(bufferOut),0,(struct sockaddr*)&server,&addr_len);
			memcpy(&pcktS.trollhdr,bufferOut,16);
			memcpy(&pcktS.tcpHdr,bufferOut+16,20);
			memcpy(&pcktS.payload,bufferOut+36,bytes-36);
			printf("Bytes recv from troll %d\n",bytes);
			short check =checksum((char *)&pcktS+16,sizeof(struct tcphdr)+bytes);
			if(check == ntohs(pcktS.tcpHdr.check)){
				printf("Checksum is same for packet %d\n",pcktS.tcpHdr.seq);
				printf("Checksum rcvd is: %u\n",pcktS.tcpHdr.check);
				printf("Checksum calculated is %u\n",check);
			}else{
				printf("Checksum is different for packet %u\n",pcktS.tcpHdr.seq);
				printf("Checksum rcvd is: %u\n",pcktS.tcpHdr.check);
			}
			bytesToServ = sendto(sockOut,bufferOut+36,bytes-36,0,(struct sockaddr*)&final,sizeof(final));
			printf("Bytes sent to server:%d\n",bytesToServ);
		}
		FD_ZERO(&portUp);
		FD_SET(sockIn, &portUp);
		FD_SET(sockOut, &portUp);		
	}	
	
	return 0;
}

/*
used from site http://locklessinc.com/articles/tcp_checksum/
This version of the algorthim gave a better time performance
 */
unsigned short checksum(const char *buf, unsigned size)
{
	unsigned long long sum = 0;
	const unsigned long long *b = (unsigned long long *) buf;

	unsigned t1, t2;
	unsigned short t3, t4;

	/* Main loop - 8 bytes at a time */
	while (size >= sizeof(unsigned long long))
	{
		unsigned long long s = *b++;
		sum += s;
		if (sum < s) sum++;
		size -= 8;
	}

	/* Handle tail less than 8-bytes long */
	buf = (const char *) b;
	if (size & 4)
	{
		unsigned s = *(unsigned *)buf;
		sum += s;
		if (sum < s) sum++;
		buf += 4;
	}

	if (size & 2)
	{
		unsigned short s = *(unsigned short *) buf;
		sum += s;
		if (sum < s) sum++;
		buf += 2;
	}

	if (size)
	{
		unsigned char s = *(unsigned char *) buf;
		sum += s;
		if (sum < s) sum++;
	}

	/* Fold down to 16 bits */
	t1 = sum;
	t2 = sum >> 32;
	t1 += t2;
	if (t1 < t2) t1++;
	t3 = t1;
	t4 = t1 >> 16;
	t3 += t4;
	if (t3 < t4) t3++;

	return ~t3;
}
