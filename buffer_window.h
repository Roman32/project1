#ifndef BUFFER_WINDOW_HEADER
#define BUFFER_WINDOW_HEADER
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

int writeToBufferC(int bytesToWrite, char pktBuffer[]);
int isBuffFilled();
int readFromBufferC(char pktBuffer[]);

#endif
