#ifndef BUFFER_WINDOW_HEADER
#define BUFFER_WINDOW_HEADER
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

int writeToBufferC(int bytesToWrite, char pktBuffer[], int seq_num);
int isBuffFilled();
int readFromBufferC(char pktBuffer[],int seq_num);
int insertIntoCWindow(int seq_num, int ack_flag, int pktStart, int sizeOfPkt,struct timeval s_time);
int readFromBufferToResend(char pktBuffer[],int bytesOut,int dataStart);
int removeFromCWindow(int seq_num);
struct timeval getPktStartTime(int seq_num);
int writeToBufferS();
int readFromBufferS(char pktBuffer[],int seq_num);
int updAckFlagInSWindow(int seq_num);
int isCWindowFull();
int getGetPktLocation(int seq_num);
int getPacketSize(int seq_num);
#endif
