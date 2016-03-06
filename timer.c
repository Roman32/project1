#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "wrapper_funcs.h"
#include "globals.h"
#include <time.h>
#define max_timer_msg_len 21 //1 byte for type, 4 bytes for seq num, how many for time?

/*One way to implement the timer is as follows. The program wakes up periodically, determines
the time elapsed since the last wake up and decrements the time value in the first node.
If it reduces to 0 or less, the timer has expired for the first node. The timer process must
also receive further requests for timers while it is repeatedly waking up and reducing the
timer value in the first node. The select function call is useful for such purposes.*/
typedef struct packet{
  uint8_t packet_type;
  uint32_t seq_num;
  struct timeval t;
}packet;

struct time_node{
	struct timeval *delta_time;
	//float delta_time;
  uint32_t seq_num;
  struct time_node *next;
  struct time_node *prev;
}time_node;
/*
The time structures involved are defined in <sys/time.h> and look
       like

           struct timeval {
               long    tv_sec;         // seconds
               long    tv_usec;        // microseconds
           };*/


struct sockaddr_in timer_sockaddr;
struct sockaddr_in tcpd;
int tcpd_out_sock;



struct time_node *head = NULL;
struct time_node *cursor = NULL;
int tcpd_sock;


/* Subtract the ‘struct timeval’ values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0. */
int
timeval_subtract (result, x, y)
struct timeval *result, *x, *y;
{
	if(x == NULL){printf("sub x was null\n");}
	if(y == NULL){printf("sub y was null\n");}
	if(result == NULL){printf("sub res was null\n");}
	//printf("in substract\n");
	long x_sec2usec = x -> tv_sec * 1000000;
	long y_sec2usec = y -> tv_sec * 1000000;
	//printf("in substract 2\n");
	long x_t_usec = x_sec2usec + x -> tv_usec;
	long y_t_usec = y_sec2usec + y -> tv_usec;
	long res_t = x_t_usec - y_t_usec;
	result -> tv_sec = res_t / 1000000;
	result -> tv_usec = res_t % 1000000;



  /* Return 1 if result is negative. */
  return res_t <= 0; //changed from < to <= to check if 0 seconds are left
}
/*** Based on the implemenation above but for adding ***/
int
timeval_add (result, x, y)
struct timeval *result, *x, *y;
{
    //printf("in add, x -> tv_sec: %ld x -> tv_usec: %ld\n",x -> tv_sec, x -> tv_usec);
	//printf("in add, y -> tv_sec: %ld y -> tv_usec: %ld\n",y -> tv_sec, y -> tv_usec);
  	if(x == NULL){printf("add x was null\n");}
	if(y == NULL){printf("add y was null\n");}
	if(result == NULL){printf("add res was null\n");}
	//printf("in add\n");
	long x_sec2usec = x -> tv_sec * 1000000;
	long y_sec2usec = y -> tv_sec * 1000000;
	//printf("in add 2\n");
	long x_t_usec = x_sec2usec + x -> tv_usec;
	long y_t_usec = y_sec2usec + y -> tv_usec;
	long res_t = x_t_usec + y_t_usec;
	result -> tv_sec = res_t / 1000000;
	result -> tv_usec = res_t % 1000000;
    //printf("in add, res -> tv_sec: %ld res -> tv_usec: %ld\n",result -> tv_sec, result-> tv_usec);
  return x->tv_sec < y->tv_sec;
}

int
timeval_compare (x, y)
struct timeval *x, *y;
{

  if(x -> tv_sec > y -> tv_sec){
   //printf("here 1 in cmp\n");
   return 1;
 }
 if(x -> tv_sec == y -> tv_sec){
  if(x -> tv_usec > y -> tv_usec){
    //printf("here 2 in cmp\n");
    return 1;
  }else if(x->tv_usec == y -> tv_usec){
    //printf("here 3 in cmp\n");
    return 0;
  }
}
  //x is less than y
//printf("here 4 in cmp\n");
return -1;
}

//add time node to list
int add_to_list(struct timeval *d_time, uint32_t s_num);
//remove time node from list
int remove_from_list(uint32_t s_num);

int print_list();
//update head node in timeing list
void update_timer(struct timeval *elapsed_time);

//recieve info from timer_sockaddr
int recv_from_tcpd();

//let timer_sockaddr know that something timed out
int alert_tcpd();

//returns 1 if seq_num is present in list, -1 otherwise
int seq_is_present(uint32_t s_num);

int send_expr_notice(uint32_t s_num);


int main(){
  tcpd_out_sock = socket(AF_INET, SOCK_DGRAM,0);
 if(tcpd_out_sock< 0) {
	perror("opening datagram socket timer_ssock");
	exit(2);
    }
	tcpd.sin_family = AF_INET;
	tcpd.sin_port = htons(TIMERLPORT);
	tcpd.sin_addr.s_addr = 0;
 
  fd_set portUp;
	//for sleeping the program

	//for determining time at which sleep started
  struct timeval start_time;
	//when sleep ends, the time at which this occurs is stored here
  struct timeval curr_time;
	//for storing the result of curr_time - start_time
  struct timeval result_time;
/*Set up for timer_sockaddr Socket*/

  tcpd_sock = socket(AF_INET,SOCK_DGRAM,0);
  if(tcpd_sock < 0) {
   perror("error: opening datagram socket");
   exit(1);
 }

 timer_sockaddr.sin_family = AF_INET;
 timer_sockaddr.sin_port = htons(TIMERPORT);
 timer_sockaddr.sin_addr.s_addr = INADDR_ANY;

 if(bind(tcpd_sock, (struct sockaddr *)&timer_sockaddr, sizeof(timer_sockaddr)) < 0) {
   perror("getting socket name");
   exit(2);
 }

 printf("Timer waiting on port # %d\n", ntohs(timer_sockaddr.sin_port));



 int tv_sub_ret_val;
 int select_ret_val;

//for testing only, watch stdin (fd 0) to see when it has input

 FD_ZERO(&portUp);
 FD_SET(tcpd_sock,&portUp);
//recv_from_tcpd();

 while(1){
 struct timeval timer;
  /*
  if(head == NULL){
    timer.tv_sec = 5;
    timer.tv_usec = 0;
  }else{
    timer.tv_sec = head -> delta_time -> tv_sec;
    timer.tv_usec = head -> delta_time -> tv_usec;
  }*/
    if(head != NULL){

     long x = 1;
     long y = 0;
     //we are going to set the timeout the time of the head node
     x = head -> delta_time -> tv_sec;
     y = head -> delta_time -> tv_usec;
     //if in some cases, the usec is negative as a result from a previous subtract
	 //select will throw an error so do some checks before setting the timeout
	 if(y < 0){y = 0;}
     if(x < 0){x = 0;}
     if(x == 0 && y == 0){x = 1;}
     timer.tv_sec = x;
     timer.tv_usec = y;
	 printf("waiting for %ld sec %ld usec\n",timer.tv_sec,timer.tv_usec);

	}else{
		//if there is nothing in the list
		timer.tv_sec = 1;
		timer.tv_usec = 0;
	}
    //get the start time before select
    gettimeofday(&start_time,NULL);
    select_ret_val = select(FD_SETSIZE,&portUp,NULL,NULL,&timer);
    gettimeofday(&curr_time,NULL); //now get the time after select returns

    if(select_ret_val == -1){ //select has an error
    	perror("select()");
    }else if(select_ret_val){
    	printf("Packet incoming.\n");
		    //we want to update timer values before inserting/deleting if list != empty
	    if(head != NULL){
		    struct timeval *test_el_time;
		    test_el_time = malloc(sizeof(struct timeval));
		    test_el_time -> tv_usec = 0;
			test_el_time -> tv_sec = 0;
		    /* note that this will include the time that it takes to loop
				to iterate. So when viewing output, there will be a few microseconds between
				inserting into the list */
		    timeval_subtract(test_el_time,&curr_time,&start_time);

		    //printf("waiting for %ld sec %ld usec\n",test_el_time->tv_sec,test_el_time->tv_usec);
		    update_timer(test_el_time);
        }
    	recv_from_tcpd();
    	print_list();
    }else{
      //there was no packet received during before timeout period, no we update the list
      if(head != NULL){
        struct timeval *test_el_time;
        test_el_time = malloc(sizeof(struct timeval));
        test_el_time -> tv_usec = 0;
		test_el_time -> tv_sec = 0;

        timeval_subtract(test_el_time,&curr_time,&start_time);


        update_timer(test_el_time);
        print_list();
      }

    }
    FD_ZERO(&portUp);
    FD_SET(tcpd_sock,&portUp);

  }
close(tcpd_sock);
free(head);
free(cursor);
return;
}


int recv_from_tcpd(){

  int timer_sockaddr_len = sizeof(timer_sockaddr);
  int buflen = max_timer_msg_len;
  char recv_buff[buflen];


 // printf("waiting to receive\n");
  int res = recvfrom(tcpd_sock, recv_buff, buflen, 0, (struct sockaddr *)&timer_sockaddr, &buflen);

//printf("res is %d\n",res);
  if(res < 0) {

    perror("error receiving in timer");
    exit(4);

  }

  uint8_t inc_ptype;
  uint32_t inc_seq_num;
  uint64_t inc_sec;
  uint64_t inc_usec;

  memcpy(&inc_ptype,recv_buff,1);
  memcpy(&inc_seq_num,recv_buff+1,4);
  memcpy(&inc_sec,recv_buff+5,8);
  memcpy(&inc_usec,recv_buff+13,8);



  if(inc_ptype == 6){

   if(seq_is_present(ntohl(inc_seq_num)) == 1){printf("error: seq num is present already\n"); return -1;}

   printf("Packet type 6 received, adding packet seq_num %d to delta list\n",ntohl(inc_seq_num));
   //printf("usec was %ld\n",be64toh(inc_usec));

   struct timeval *t;
   t = malloc(sizeof(struct timeval));

   t -> tv_sec = be64toh(inc_sec);
   t -> tv_usec = be64toh(inc_usec);

   add_to_list(t,ntohl(inc_seq_num));

  }else if(inc_ptype == 7){
   if(seq_is_present(ntohl(inc_seq_num)) != 1){printf("error: seq num not present\n"); return -1;}
   printf("Packet type 7 received, removing seq num %d from  delta list.\n",ntohl(inc_seq_num));
   remove_from_list(ntohl(inc_seq_num));
   }

return 1;
}

int print_list(){
  printf("\n******* NODE LIST *******\n");
  if(head == NULL){
    printf("List is empty\n");
	printf("*******    END    *******\n");
    return -1;
  }
  cursor = head;
  while(cursor != NULL){
    printf("Node seq_num: %d   time: sec: %ld usec: %ld\n",cursor->seq_num,cursor->delta_time->tv_sec,cursor->delta_time->tv_usec);
    cursor = cursor -> next;
  }
  printf("*******    END    *******\n");
  return 0;
}


int add_to_list(struct timeval *d_time, uint32_t s_num){
  struct time_node *new_t_node_ptr = (struct time_node*)malloc(sizeof(struct time_node));
  struct time_node *prev_p  = head;

  struct timeval *cur_node_time;
  struct timeval *upd_node_time;

  cur_node_time = malloc(sizeof(struct timeval));
  upd_node_time = malloc(sizeof(struct timeval));

  new_t_node_ptr -> seq_num = s_num;
  cursor = head;

  if(head != NULL){
    if( timeval_compare(d_time,head->delta_time) < 1 ){

      new_t_node_ptr -> next = head;
      new_t_node_ptr -> prev = NULL;

      //get the updated timval struct using function (x - y)
      timeval_subtract (head -> delta_time, head -> delta_time, d_time);

      head -> prev = new_t_node_ptr;

      new_t_node_ptr -> delta_time = d_time;
      head = new_t_node_ptr;


    }else{


     timeval_subtract (d_time, d_time, cursor -> delta_time);
     cursor = cursor -> next;
     prev_p = cursor;

    if(cursor != NULL){

      while(cursor != NULL && timeval_compare(d_time,cursor->delta_time) == 1 ){

         timeval_subtract (d_time, d_time, cursor -> delta_time);
         prev_p = cursor;
         cursor = cursor -> next;
        }


       if(cursor == NULL){
          prev_p -> next = new_t_node_ptr;
          new_t_node_ptr -> next = NULL;
          new_t_node_ptr -> prev = prev_p;
          new_t_node_ptr -> delta_time = d_time;
		//here we are at the end of the list, no need
		//to update the remaining list (there is nothing remaining)

        }else{
          cursor -> prev -> next = new_t_node_ptr;
          new_t_node_ptr -> prev  = cursor -> prev;
          cursor -> prev = new_t_node_ptr;
          new_t_node_ptr -> next = cursor;
          new_t_node_ptr -> delta_time = d_time;
			//now we need to update the next item
			//since we insterted into the middle of the list
          timeval_subtract (upd_node_time,new_t_node_ptr -> next -> delta_time,new_t_node_ptr -> delta_time);
          new_t_node_ptr -> next -> delta_time = upd_node_time;
        }

    }else{
            head -> next = new_t_node_ptr;
            new_t_node_ptr -> next = NULL;
            new_t_node_ptr -> delta_time = d_time;
            new_t_node_ptr -> prev = head;
			//new node is second in the list and nothing comes after
			//so no need to update
			//none
          }



      }
  }else{

    new_t_node_ptr -> delta_time = d_time;
    new_t_node_ptr -> next = NULL;
    new_t_node_ptr -> prev = NULL;
    head = new_t_node_ptr;
    //no need to update times since only one element in list
  }
  return -1;
  }



  int remove_from_list(uint32_t s_num){
   struct timeval *cur_node_time;
   struct timeval *upd_node_time;
   struct time_node *rem_ptr;
   cur_node_time = malloc(sizeof(struct timeval));
   upd_node_time = malloc(sizeof(struct timeval));


   int removed = -1;
   if(head == NULL){
    printf("Timer List is empty!");
    return -1;
  }

  cursor = head;
  if(head -> seq_num == s_num){
        rem_ptr = head;
        removed = head -> seq_num;
        cur_node_time = head -> delta_time;

    if(head -> next != NULL){
          cursor = head -> next;
          timeval_add(upd_node_time,cur_node_time,cursor -> delta_time);
          cursor -> delta_time = upd_node_time;
          head = cursor;
          head -> prev = NULL;

    }else{
         head = NULL;
         }


    free(rem_ptr);
    return removed;
  }else{
      cursor = cursor -> next;
      while(cursor != NULL  && cursor -> seq_num != s_num){
       cursor = cursor->next;
     }

     if(cursor != NULL){
      rem_ptr = cursor;
      removed = cursor -> seq_num;
      cur_node_time = cursor -> delta_time;
        if(cursor -> next != NULL){
          timeval_add(upd_node_time,cursor -> next -> delta_time, cur_node_time);
          cursor -> next -> delta_time = upd_node_time;
          cursor -> next -> prev = cursor ->prev;
        }
      cursor -> prev -> next = cursor -> next;
    }

  }
  free(rem_ptr);
  return removed;
}

    void update_timer(struct timeval *elapsed_time){
     struct timeval *head_time;
     struct timeval *upd_head_time;
     upd_head_time = malloc(sizeof(struct timeval));
     head_time = malloc(sizeof(struct timeval));

     head_time = head -> delta_time;
     //printf("head time is %d\n",head_time -> tv_sec);
     //printf("elapsed_time is %d\n",elapsed_time -> tv_sec );
     int res = timeval_subtract(upd_head_time,head_time,elapsed_time);
     //printf("res is %d\n",res);
     head -> delta_time = upd_head_time;

     if(res > 0){
       send_expr_notice(head -> seq_num);
       remove_from_list(head -> seq_num);
       printf("Head node expired. Time to send a packet to TCPD\n");
       
     }else{
       head -> delta_time = upd_head_time;
     }
   }



int seq_is_present(uint32_t s_num){
	int found_flag = -1;
  if(head == NULL){
    printf("head is null\n");
    return -1;
  }
  cursor = head;
  while(cursor != NULL){
    if(cursor -> seq_num == s_num){found_flag = 1;}
    cursor = cursor -> next;
  }

  return found_flag;
}

int send_expr_notice(uint32_t s_num){
   int buffSize = sizeof(uint32_t);
   char *tcpd_buf = malloc(buffSize);
   bzero(tcpd_buf,buffSize);
   s_num = htonl(s_num);
   memcpy(tcpd_buf,&s_num,4);
   int res = sendto(tcpd_out_sock, tcpd_buf,buffSize, 0, (struct sockaddr *)&tcpd, sizeof(tcpd));
      printf("Sent packet to tpcd res is %d\n",res);
      if(res < 0) {
        perror("sending datagram message");
        exit(4);
      }
}
/**** Unit testing section ****/
/*
 struct timeval *inc_dtime;
 inc_dtime = malloc(sizeof(struct timeval));

 struct timeval *inc_dtime1;
 inc_dtime1 = malloc(sizeof(struct timeval));

 struct timeval *inc_dtime2;
 inc_dtime2 = malloc(sizeof(struct timeval));

 struct timeval *inc_dtime3;
 inc_dtime3 = malloc(sizeof(struct timeval));

 struct timeval *inc_dtime4;
 inc_dtime4 = malloc(sizeof(struct timeval));

 inc_dtime -> tv_sec = 30;
 inc_dtime -> tv_usec = 0;
 add_to_list(inc_dtime,1);

 inc_dtime1 -> tv_sec = 34;
 inc_dtime1 -> tv_usec = 0;
 add_to_list(inc_dtime1,2);

 inc_dtime2 -> tv_sec = 35;
 inc_dtime2 -> tv_usec = 0;
 add_to_list(inc_dtime2,3);

 inc_dtime3 -> tv_sec = 36;
 inc_dtime3 -> tv_usec = 0;
 add_to_list(inc_dtime3,4);

 inc_dtime4 -> tv_sec = 38;
 inc_dtime4 -> tv_usec = 0;
 add_to_list(inc_dtime4,5);

 print_list();
*/
//printf("\n\n/**** Remove testing ****/\n\n");
/*
int removed = remove_from_list(3);
printf("Seq_num removed is  : %d\n",removed);
print_list();
removed = remove_from_list(5);
printf("\nSeq_num removed is  : %d\n",removed);
print_list();
removed = remove_from_list(1);
printf("\nSeq_num removed is  : %d\n",removed);
print_list();

struct timeval *test_el_time;
test_el_time = malloc(sizeof(struct timeval));
test_el_time -> tv_sec = 5;
test_el_time -> tv_usec = 0;
update_timer(test_el_time);
printf("5 seconds elapsed\n");
print_list();
//update_timer(26); //should make head be less than zero
//printf("26 seconds elapsed\n");
//print_list();*/
/**** End unit testing section ****/
