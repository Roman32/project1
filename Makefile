#Makefile for Lab2

CC = gcc
OBJCLI = ftpc.c wrapper_funcs.c 
OBJSRV = ftps.c wrapper_funcs.c
OBJD = tcpd.c wrapper_funcs.c
OBJTIME = timer.c
OBJTTIME = timer_test_file.c
all: ftpc ftps tcpd timer testT

ftpc:	$(OBJCLI)
	$(CC) -o $@ $(OBJCLI)
	
ftps:	$(OBJSRV)
	$(CC) -o $@ $(OBJSRV)
	
tcpd:	$(OBJD)
	$(CC) -o $@ $(OBJD)
	
timer:	$(OBJTIME)
	$(CC) -o $@ $(OBJTIME)

testT:	$(OBJTTIME)
	$(CC) -o $@ $(OBJTTIME)
		
clean:
	rm ftpc ftps ftpd timer testT
