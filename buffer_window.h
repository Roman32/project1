#ifndef BUFFER_WINDOW_HEADER
#define BUFFER_WINDOW_HEADER
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

int writeToBufferC(int bytesToWrite, char pktBuffer[], int seq_num);
int isBuffFilled();
int readFromBufferC(char pktBuffer[],int seq_num);
int insertIntoCWindow(int seq_num, int ack_flag, int pktStart, int sizeOfPkt);
int removeFromCWindow(int seq_num);
int updAckFlagInSWindow(int seq_num);
int isCWindowFull();
#endif
