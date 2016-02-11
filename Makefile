#Makefile for Lab2

CC = gcc
OBJCLI = ftpc.c wrapper_funcs.c 
OBJSRV = ftps.c wrapper_funcs.c
OBJD = tcpd.c wrapper_funcs.c

all: ftpc ftps tcpd 

ftpc:	$(OBJCLI)
	$(CC) -o $@ $(OBJCLI)
	
ftps:	$(OBJSRV)
	$(CC) -o $@ $(OBJSRV)
	
tcpd:	$(OBJD)
	$(CC) -o $@ $(OBJD)
	
clean:
	rm ftpc ftps ftpd
